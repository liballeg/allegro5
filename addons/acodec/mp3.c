/*
 * Allegro5 MP3 reader.
 * Requires MiniMP3 from https://github.com/lieff/minimp3
 * author: Mark Watkin (pmprog) 2019
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "acodec.h"
#include "helper.h"

#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>
#include <minimp3_ex.h>

ALLEGRO_DEBUG_CHANNEL("acodec")

typedef struct MP3FILE
{
   mp3dec_t dec;

   uint8_t* file_buffer;      /* encoded MP3 file */
   int64_t file_size;         /* in bytes */
   int64_t next_frame_offset; /* next frame offset, in bytes */
   int file_pos;              /* position in samples */
   mp3d_sample_t frame_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME]; /* decoded MP3 frame */
   int frame_samples;         /* in samples */
   int frame_pos;             /* position in the frame buffer, in samples */

   int freq;
   ALLEGRO_CHANNEL_CONF chan_conf;
} MP3FILE;

ALLEGRO_SAMPLE *_al_load_mp3(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Could not open file '%s'.\n", filename);
      return NULL;
   }

   spl = _al_load_mp3_f(f);

   al_fclose(f);

   return spl;
}

ALLEGRO_SAMPLE *_al_load_mp3_f(ALLEGRO_FILE *f)
{
   mp3dec_t dec;
   mp3dec_init(&dec);

   mp3dec_file_info_t info;
   ALLEGRO_SAMPLE *spl = NULL;

   // Read our file size
   int64_t filesize = al_fsize(f);
   if (filesize == -1) {
      ALLEGRO_WARN("Could not determine file size.\n");
      return NULL;
   }

   // Allocate buffer and read all the file
   uint8_t* mp3data = (uint8_t*)al_malloc(filesize);
   size_t readbytes = al_fread(f, mp3data, filesize);
   if (readbytes != (size_t)filesize) {
      ALLEGRO_WARN("Failed to read file into memory.\n");
      al_free(mp3data);
      return NULL;
   }

   // Decode the file contents, and copy to a new buffer
   mp3dec_load_buf(&dec, mp3data, filesize, &info, NULL, NULL);
   al_free(mp3data);

   if (info.buffer == NULL) {
      ALLEGRO_WARN("Could not decode MP3.\n");
      return NULL;
   }

   // Create sample from info variable
   spl = al_create_sample(info.buffer, info.samples, info.hz,
      _al_word_size_to_depth_conf(sizeof(mp3d_sample_t)),
      _al_count_to_channel_conf(info.channels), true);

   return spl;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream(const char *filename, size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Could not open file '%s'.\n", filename);
      return NULL;
   }

   stream = _al_load_mp3_audio_stream_f(f, buffer_count, samples);
   if (!stream) {
      al_fclose(f);
   }

   return stream;
}

/* mp3_stream_update:
 *  Updates 'stream' with the next chunk of data.
 *  Returns the actual number of bytes written.
 */
static size_t mp3_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;
   int samples_needed = buf_size / sizeof(mp3d_sample_t);

   int samples_read = 0;
   while (samples_read < samples_needed) {
      int samples_from_this_frame = _ALLEGRO_MIN(
         mp3file->frame_samples - mp3file->frame_pos,
         samples_needed - samples_read
      );
      memcpy(data, mp3file->frame_buffer + mp3file->frame_pos,
         samples_from_this_frame * sizeof(mp3d_sample_t));

      mp3file->frame_pos += samples_from_this_frame;
      mp3file->file_pos += samples_from_this_frame;
      data = (mp3d_sample_t*)(data) + samples_from_this_frame;
      samples_read += samples_from_this_frame;

      if (mp3file->frame_pos >= mp3file->frame_samples) {
         mp3dec_frame_info_t frame_info;
         int frame_samples = mp3dec_decode_frame(&mp3file->dec,
            mp3file->file_buffer + mp3file->next_frame_offset,
            mp3file->file_size - mp3file->next_frame_offset,
            mp3file->frame_buffer, &frame_info);
         if (frame_samples == 0) {
            mp3file->next_frame_offset = 0;
            mp3file->frame_samples = 0; /* so next time we read, it'll immediately decode a new frame */
            mp3file->frame_pos = 0;
            break;
         }

         mp3file->frame_samples = frame_info.channels * frame_samples;
         mp3file->frame_pos = 0;
         mp3file->next_frame_offset += frame_info.frame_bytes;
      }
   }
   return samples_read * sizeof(mp3d_sample_t);
}

static void mp3_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   _al_acodec_stop_feed_thread(stream);

   al_free(mp3file->file_buffer);
   al_free(mp3file);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}

static bool mp3_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   mp3file->next_frame_offset = 0;
   mp3file->frame_samples = 0;

   return true;
}

static double mp3_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   return ((double)mp3file->file_pos) / al_get_channel_count(mp3file->chan_conf) / mp3file->freq;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples)
{
   (void)f;
   (void)buffer_count;
   (void)samples;

   MP3FILE* mp3file = al_calloc(sizeof(MP3FILE), 1);
   mp3dec_init(&mp3file->dec);

   // Read our file size
   mp3file->file_size = al_fsize(f);
   if (mp3file->file_size == -1) {
      ALLEGRO_WARN("Could not determine file size.\n");
      goto failure;
   }

   // Allocate buffer and read all the file
   mp3file->file_buffer = (uint8_t*)al_malloc(mp3file->file_size);
   size_t readbytes = al_fread(f, mp3file->file_buffer, mp3file->file_size);
   if (readbytes != (size_t)mp3file->file_size) {
      ALLEGRO_WARN("Failed to read file into memory.\n");
      goto failure;
   }
   al_fclose(f);

   // decode the first frame, so we can derive the file information from it.
   mp3dec_frame_info_t frame_info;
   int frame_samples = mp3dec_decode_frame(&mp3file->dec, mp3file->file_buffer,
      mp3file->file_size, mp3file->frame_buffer, &frame_info);
   if (frame_samples == 0) {
      ALLEGRO_WARN("Could not decode the first frame.\n");
      goto failure;
   }
   mp3file->frame_samples = frame_info.channels * frame_samples;
   mp3file->chan_conf = _al_count_to_channel_conf(frame_info.channels);
   mp3file->freq = frame_info.hz;
   mp3file->next_frame_offset = frame_info.frame_bytes;

   ALLEGRO_AUDIO_STREAM *stream = al_create_audio_stream(
      buffer_count, samples, mp3file->freq,
      _al_word_size_to_depth_conf(sizeof(mp3d_sample_t)),
      mp3file->chan_conf);
   if (!stream) {
      ALLEGRO_WARN("Failed to create stream.\n");
      goto failure;
   }
   ALLEGRO_DEBUG("Channels %d, frequency %d\n", frame_info.channels, frame_info.hz);

   stream->extra = mp3file;
   stream->feeder = mp3_stream_update;
   stream->unload_feeder = mp3_stream_close;
   stream->rewind_feeder = mp3_stream_rewind;
   stream->seek_feeder = NULL;//mp3_stream_seek;
   stream->get_feeder_position = mp3_stream_get_position;
   stream->get_feeder_length = NULL;//mp3_stream_get_length;
   stream->set_feeder_loop = NULL;//mp3_stream_set_loop;

   _al_acodec_start_feed_thread(stream);

   return stream;
failure:
   al_free(mp3file->file_buffer);
   al_free(mp3file->frame_buffer);
   al_free(mp3file);
   return NULL;
}
