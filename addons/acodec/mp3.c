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
   int file_samples;          /* in samples */
   double loop_start;
   double loop_end;

   mp3d_sample_t frame_buffer[MINIMP3_MAX_SAMPLES_PER_FRAME]; /* decoded MP3 frame */
   int frame_pos;             /* position in the frame buffer, in samples */

   int* frame_offsets;        /* in bytes */
   int num_frames;
   int frame_samples;         /* in samples, same across all frames */

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

   /* Read our file size. */
   int64_t filesize = al_fsize(f);
   if (filesize == -1) {
      ALLEGRO_WARN("Could not determine file size.\n");
      return NULL;
   }

   /* Allocate buffer and read the entire file. */
   uint8_t* mp3data = (uint8_t*)al_malloc(filesize);
   size_t readbytes = al_fread(f, mp3data, filesize);
   if (readbytes != (size_t)filesize) {
      ALLEGRO_WARN("Failed to read file into memory.\n");
      al_free(mp3data);
      return NULL;
   }

   /* Decode the file contents, and copy to a new buffer. */
   mp3dec_load_buf(&dec, mp3data, filesize, &info, NULL, NULL);
   al_free(mp3data);

   if (info.buffer == NULL) {
      ALLEGRO_WARN("Could not decode MP3.\n");
      return NULL;
   }

   /* Create sample from info variable. */
   spl = al_create_sample(info.buffer, info.samples / info.channels, info.hz,
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
   /* We load the entire file into memory. */
   al_fclose(f);

   return stream;
}


static bool mp3_stream_seek(ALLEGRO_AUDIO_STREAM * stream, double time)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;
   int file_pos = time * mp3file->freq;
   int frame = file_pos / mp3file->frame_samples;
   /* It is necessary to start decoding a little earlier than where we are
    * seeking to, because frames will reuse decoder state from previous frames.
    * minimp3 assures us that 10 frames is sufficient. */
   int sync_frame = _ALLEGRO_MAX(0, frame - 10);
   int frame_pos = file_pos - frame * mp3file->frame_samples;
   if (frame < 0 || frame > mp3file->num_frames) {
      ALLEGRO_WARN("Seeking outside the stream bounds: %f\n", time);
      return false;
   }
   int frame_offset = mp3file->frame_offsets[frame];
   int sync_frame_offset = mp3file->frame_offsets[sync_frame];

   mp3dec_frame_info_t frame_info;
   do {
      mp3dec_decode_frame(&mp3file->dec,
         mp3file->file_buffer + sync_frame_offset,
         mp3file->file_size - sync_frame_offset,
         mp3file->frame_buffer, &frame_info);
      sync_frame_offset += frame_info.frame_bytes;
   } while (sync_frame_offset <= frame_offset);

   mp3file->next_frame_offset = frame_offset + frame_info.frame_bytes;
   mp3file->file_pos = file_pos;
   mp3file->frame_pos = frame_pos;

   return true;
}

static bool mp3_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   return mp3_stream_seek(stream, mp3file->loop_start);
}

static double mp3_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   return (double)mp3file->file_pos / mp3file->freq;
}

static double mp3_stream_get_length(ALLEGRO_AUDIO_STREAM * stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   return (double)mp3file->file_samples / mp3file->freq;
}

static bool mp3_stream_set_loop(ALLEGRO_AUDIO_STREAM * stream, double start, double end)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;
   mp3file->loop_start = start;
   mp3file->loop_end = end;
   return true;
}

/* mp3_stream_update:
 *  Updates 'stream' with the next chunk of data.
 *  Returns the actual number of bytes written.
 */
static size_t mp3_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;
   int sample_size = sizeof(mp3d_sample_t) * al_get_channel_count(mp3file->chan_conf);
   int samples_needed = buf_size / sample_size;;
   double ctime = mp3_stream_get_position(stream);
   double btime = (double)samples_needed / mp3file->freq;

   if (stream->spl.loop != _ALLEGRO_PLAYMODE_STREAM_ONCE && ctime + btime > mp3file->loop_end) {
      samples_needed = (mp3file->loop_end - ctime) * mp3file->freq;
   }
   if (samples_needed < 0)
      return 0;

   mp3dec_t dec;
   mp3dec_init(&dec);

   int samples_read = 0;
   while (samples_read < samples_needed) {
      int samples_from_this_frame = _ALLEGRO_MIN(
         mp3file->frame_samples - mp3file->frame_pos,
         samples_needed - samples_read
      );
      memcpy(data,
         mp3file->frame_buffer + mp3file->frame_pos * al_get_channel_count(mp3file->chan_conf),
         samples_from_this_frame * sample_size);

      mp3file->frame_pos += samples_from_this_frame;
      mp3file->file_pos += samples_from_this_frame;
      data = (char*)(data) + samples_from_this_frame * sample_size;
      samples_read += samples_from_this_frame;

      if (mp3file->frame_pos >= mp3file->frame_samples) {
         mp3dec_frame_info_t frame_info;
         int frame_samples = mp3dec_decode_frame(&mp3file->dec,
            mp3file->file_buffer + mp3file->next_frame_offset,
            mp3file->file_size - mp3file->next_frame_offset,
            mp3file->frame_buffer, &frame_info);
         if (frame_samples == 0) {
            mp3_stream_rewind(stream);
            break;
         }
         mp3file->frame_pos = 0;
         mp3file->next_frame_offset += frame_info.frame_bytes;
      }
   }
   return samples_read * sample_size;
}

static void mp3_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   _al_acodec_stop_feed_thread(stream);

   al_free(mp3file->frame_offsets);
   al_free(mp3file->file_buffer);
   al_free(mp3file);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples)
{
   MP3FILE* mp3file = al_calloc(sizeof(MP3FILE), 1);
   mp3dec_init(&mp3file->dec);

   /* Read our file size. */
   mp3file->file_size = al_fsize(f);
   if (mp3file->file_size == -1) {
      ALLEGRO_WARN("Could not determine file size.\n");
      goto failure;
   }

   /* Allocate buffer and read all the file. */
   mp3file->file_buffer = (uint8_t*)al_malloc(mp3file->file_size);
   size_t readbytes = al_fread(f, mp3file->file_buffer, mp3file->file_size);
   if (readbytes != (size_t)mp3file->file_size) {
      ALLEGRO_WARN("Failed to read file into memory.\n");
      goto failure;
   }

   /* Go through all the frames, to build the offset table. */
   int frame_offset_capacity = 0;
   int offset_so_far = 0;
   while (true) {
      if (mp3file->num_frames + 1 > frame_offset_capacity) {
         frame_offset_capacity = mp3file->num_frames * 3 / 2  + 1;
         mp3file->frame_offsets = al_realloc(mp3file->frame_offsets,
            sizeof(int) * frame_offset_capacity);
      }

      mp3dec_frame_info_t frame_info;
      int frame_samples = mp3dec_decode_frame(&mp3file->dec,
         mp3file->file_buffer + offset_so_far,
         mp3file->file_size - offset_so_far, NULL, &frame_info);
      if (frame_samples == 0) {
         if (mp3file->num_frames == 0) {
            ALLEGRO_WARN("Could not decode the first frame.\n");
            goto failure;
         }
         else {
            break;
         }
      }
      /* Grab the file information from the first frame. */
      if (offset_so_far == 0) {
         ALLEGRO_DEBUG("Channels %d, frequency %d\n", frame_info.channels, frame_info.hz);
         mp3file->chan_conf = _al_count_to_channel_conf(frame_info.channels);
         mp3file->freq = frame_info.hz;
         mp3file->frame_samples = frame_samples;
      }

      mp3file->frame_offsets[mp3file->num_frames] = offset_so_far;
      mp3file->num_frames += 1;
      offset_so_far += frame_info.frame_bytes;
      mp3file->file_samples += frame_samples;
   }
   mp3file->loop_end = (double)mp3file->file_samples * mp3file->freq;

   ALLEGRO_AUDIO_STREAM *stream = al_create_audio_stream(
      buffer_count, samples, mp3file->freq,
      _al_word_size_to_depth_conf(sizeof(mp3d_sample_t)),
      mp3file->chan_conf);
   if (!stream) {
      ALLEGRO_WARN("Failed to create stream.\n");
      goto failure;
   }

   stream->extra = mp3file;
   stream->feeder = mp3_stream_update;
   stream->unload_feeder = mp3_stream_close;
   stream->rewind_feeder = mp3_stream_rewind;
   stream->seek_feeder = mp3_stream_seek;
   stream->get_feeder_position = mp3_stream_get_position;
   stream->get_feeder_length = mp3_stream_get_length;
   stream->set_feeder_loop = mp3_stream_set_loop;

   mp3_stream_rewind(stream);

   _al_acodec_start_feed_thread(stream);

   return stream;
failure:
   al_free(mp3file->frame_offsets);
   al_free(mp3file->file_buffer);
   al_free(mp3file);
   return NULL;
}


/* --- Start of minimp3 code --- */

/* The following code is copied from minimp3 to avoid depending on internals.
 * `HDR_` and `hdr_` prefixes have been replaced with `IDMP3_` and `idmp3_`.
 */

#define IDMP3_HDR_SIZE           4
#define IDMP3_IS_FREE_FORMAT(h)  ((((h)[2]) & 0xF0) == 0)
#define IDMP3_TEST_PADDING(h)    (((h)[2]) & 0x2)
#define IDMP3_TEST_MPEG1(h)      (((h)[1]) & 0x8)
#define IDMP3_TEST_NOT_MPEG25(h) (((h)[1]) & 0x10)
#define IDMP3_IS_FRAME_576(h)    (((h)[1] & 14) == 2)
#define IDMP3_IS_LAYER_1(h)      (((h)[1] & 6) == 6)
#define IDMP3_GET_LAYER(h)       ((((h)[1]) >> 1) & 3)
#define IDMP3_GET_BITRATE(h)     (((h)[2]) >> 4)
#define IDMP3_GET_SAMPLE_RATE(h) ((((h)[2]) >> 2) & 3)

static int idmp3_frame_samples(const uint8_t* h)
{
   return IDMP3_IS_LAYER_1(h) ? 384 : (1152 >> (int)IDMP3_IS_FRAME_576(h));
}

static int idmp3_bitrate_kbps(const uint8_t* h)
{
   static const uint8_t halfrate[2][3][15] = {
      { { 0,4,8,12,16,20,24,28,32,40,48,56,64,72,80 },
         { 0,4,8,12,16,20,24,28,32,40,48,56,64,72,80 },
         { 0,16,24,28,32,40,48,56,64,72,80,88,96,112,128 } },
      { { 0,16,20,24,28,32,40,48,56,64,80,96,112,128,160 },
         { 0,16,24,28,32,40,48,56,64,80,96,112,128,160,192 },
         { 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224 }
      },
   };
   return 2 * halfrate[!!IDMP3_TEST_MPEG1(h)][IDMP3_GET_LAYER(h) - 1][IDMP3_GET_BITRATE(h)];
}

static int idmp3_sample_rate_hz(const uint8_t* h)
{
   static const int g_hz[3] = { 44100, 48000, 32000 };
   return g_hz[IDMP3_GET_SAMPLE_RATE(h)] >> (int)!IDMP3_TEST_MPEG1(h) >> (int)!IDMP3_TEST_NOT_MPEG25(h);
}

static int idmp3_frame_bytes(const uint8_t* h, int free_format_size)
{
   int frame_bytes = idmp3_frame_samples(h) * idmp3_bitrate_kbps(h) * 125 / idmp3_sample_rate_hz(h);
   if (IDMP3_IS_LAYER_1(h))
      frame_bytes &= ~3; /* slot align */
   return frame_bytes ? frame_bytes : free_format_size;
}

static int idmp3_padding(const uint8_t* h)
{
   return IDMP3_TEST_PADDING(h) ? (IDMP3_IS_LAYER_1(h) ? 4 : 1) : 0;
}

static int idmp3_valid(const uint8_t* h)
{
   return h[0] == 0xff &&
      ((h[1] & 0xF0) == 0xf0 || (h[1] & 0xFE) == 0xe2) &&
      (IDMP3_GET_LAYER(h) != 0) &&
      (IDMP3_GET_BITRATE(h) != 15) &&
      (IDMP3_GET_SAMPLE_RATE(h) != 3);
}

static int idmp3_compare(const uint8_t* h1, const uint8_t* h2)
{
   return idmp3_valid(h2) &&
      ((h1[1] ^ h2[1]) & 0xFE) == 0 &&
      ((h1[2] ^ h2[2]) & 0x0C) == 0 &&
      !(IDMP3_IS_FREE_FORMAT(h1) ^ IDMP3_IS_FREE_FORMAT(h2));
}

/* --- End of minimp3 code --- */

/* Attempts to identify an MP3 file by looking for contiguous MP3 frames.
 * The tested buffer length should be 8KB for the best accuracy.
 * Returns true for a match.
 */
static bool identify_mp3(const uint8_t* buf, size_t len)
{
   const uint8_t* end = buf + len - IDMP3_HDR_SIZE;
   const uint8_t* quarter = buf + (len / 4) - IDMP3_HDR_SIZE;

   /* Too short to possibly contain a valid header. */
   if (len < IDMP3_HDR_SIZE)
      return false;

   /* A file with an ID3v2 tag is assumed to be an MP3 file. */
   if (memcmp(buf, "ID3", 3) == 0)
      return true;

   /* Only scan the first quarter of the file (2KB out of 8KB) for the initial
    * frame. This guarantees at least 2 frames in the worst case, but typically
    * at least 5, can be synced to. */
   for (const uint8_t* p = buf; p < quarter; ++p) {
      if (idmp3_valid(p)) {
         int frame_bytes = idmp3_frame_bytes(p, 0);
         int num_frames = 0;
         const uint8_t* p2 = p;

         /* free-format -- scan for next frame to discover its length. */
         if (frame_bytes == 0) {
            for (p2 += IDMP3_HDR_SIZE; p2 < end; ++p2) {
               if (idmp3_compare(p, p2)) {
                  frame_bytes = (int)(p2 - p);
                  ++num_frames;
                  break;
               }

               /* Excessive distance between frames */
               if ((p2 - p) >= 1152 * 2)
                  goto continue_scan;
            }

            /* Reached end of buffer without finding a matching header */
            if (p2 == end)
               goto continue_scan;
         }

         p2 += frame_bytes + idmp3_padding(p2);

         while (p2 < end) {
            if (idmp3_compare(p, p2)) {
               /* Bitrate can change per-frame (VBR) */
               frame_bytes = idmp3_frame_bytes(p2, frame_bytes);
               p2 += frame_bytes + idmp3_padding(p2);

               /* 10 valid frames is enough to identify this as an MP3 file */
               if (++num_frames >= 9)
                  return true;
            } else {
               goto continue_scan;
            }
         }

         /* Reached the end of the buffer. Everything prior was contiguous
          * frames, so this is likely an MP3 file. */
         return true;
      }

   continue_scan:
      ;
   }

   return false;
}


bool _al_identify_mp3(ALLEGRO_FILE *f)
{
   uint8_t x[8192];
   size_t len = al_fread(f, x, sizeof x);
   return identify_mp3(x, len);
}
