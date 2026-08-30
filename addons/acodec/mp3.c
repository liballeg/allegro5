/*
 * Allegro5 MP3 reader.
 * Requires MiniMP3 from https://github.com/lieff/minimp3
 * author: Mark Watkin (pmprog) 2019
 */


#include <stdint.h>

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
   ALLEGRO_FILE *file;
   mp3dec_io_t io;
   mp3dec_ex_t dec;

   uint64_t file_samples;     /* in samples, including channels */
   double loop_start;
   double loop_end;

   int freq;
   int channels;
   bool error_reported;
   ALLEGRO_CHANNEL_CONF chan_conf;
} MP3FILE;

static size_t mp3_read_callback(void *buffer, size_t size, void *user_data)
{
   return al_fread((ALLEGRO_FILE *)user_data, buffer, size);
}

static int mp3_seek_callback(uint64_t position, void *user_data)
{
   if (position > INT64_MAX)
      return -1;
   return al_fseek((ALLEGRO_FILE *)user_data, (int64_t)position,
      ALLEGRO_SEEK_SET) ? 0 : -1;
}

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
   if (!stream)
      al_fclose(f);

   return stream;
}


static double mp3_stream_get_length(ALLEGRO_AUDIO_STREAM *stream);

static bool mp3_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   MP3FILE *mp3file = (MP3FILE *)stream->extra;
   uint64_t position;

   if (time < 0 || time > mp3_stream_get_length(stream)) {
      ALLEGRO_WARN("Seeking outside the stream bounds: %f\n", time);
      return false;
   }

   position = (uint64_t)(time * mp3file->freq * mp3file->channels);
   if (mp3dec_ex_seek(&mp3file->dec, position) != 0)
      return false;
   mp3file->error_reported = false;
   return true;
}

static bool mp3_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *) stream->extra;

   return mp3_stream_seek(stream, mp3file->loop_start);
}

static double mp3_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *)stream->extra;

   return (double)mp3file->dec.cur_sample
      / mp3file->channels / mp3file->freq;
}

static double mp3_stream_get_length(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *)stream->extra;

   return (double)mp3file->file_samples
      / mp3file->channels / mp3file->freq;
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
   MP3FILE *mp3file = (MP3FILE *)stream->extra;
   size_t samples_needed = buf_size / sizeof(mp3d_sample_t);
   size_t samples_read;
   double ctime = mp3_stream_get_position(stream);
   double btime = (double)samples_needed
      / mp3file->channels / mp3file->freq;

   if (stream->spl.loop != _ALLEGRO_PLAYMODE_STREAM_ONCE) {
      if (ctime >= mp3file->loop_end) {
         if (!mp3_stream_rewind(stream))
            return 0;
         ctime = mp3_stream_get_position(stream);
      }
      if (ctime + btime > mp3file->loop_end) {
         double remaining = mp3file->loop_end - ctime;
         if (remaining <= 0)
            return 0;
         samples_needed = (size_t)(remaining * mp3file->freq
            * mp3file->channels);
      }
   }

   samples_read = mp3dec_ex_read(&mp3file->dec, data, samples_needed);
   if (mp3file->dec.last_error) {
      if (!mp3file->error_reported) {
         ALLEGRO_WARN("MP3 stream decode failed: %d.\n",
            mp3file->dec.last_error);
         mp3file->error_reported = true;
      }
      return samples_read * sizeof(mp3d_sample_t);
   }
   if (samples_read < samples_needed
       && stream->spl.loop != _ALLEGRO_PLAYMODE_STREAM_ONCE) {
      if (!mp3_stream_rewind(stream))
         return samples_read * sizeof(mp3d_sample_t);
      samples_read += mp3dec_ex_read(&mp3file->dec,
         (mp3d_sample_t *)data + samples_read,
         samples_needed - samples_read);
      if (mp3file->dec.last_error) {
         if (!mp3file->error_reported) {
            ALLEGRO_WARN("MP3 stream decode failed: %d.\n",
               mp3file->dec.last_error);
            mp3file->error_reported = true;
         }
      }
   }

   return samples_read * sizeof(mp3d_sample_t);
}

static void mp3_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   MP3FILE *mp3file = (MP3FILE *)stream->extra;

   _al_acodec_stop_feed_thread(stream);

   mp3dec_ex_close(&mp3file->dec);
   al_fclose(mp3file->file);
   al_free(mp3file);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}

ALLEGRO_AUDIO_STREAM *_al_load_mp3_audio_stream_f(ALLEGRO_FILE *f,
   size_t buffer_count, unsigned int samples)
{
   MP3FILE *mp3file = al_calloc(1, sizeof *mp3file);
   ALLEGRO_AUDIO_STREAM *stream;
   int ret;

   mp3file->file = f;
   mp3file->io.read = mp3_read_callback;
   mp3file->io.read_data = f;
   mp3file->io.seek = mp3_seek_callback;
   mp3file->io.seek_data = f;

   ret = mp3dec_ex_open_cb(&mp3file->dec, &mp3file->io,
      MP3D_SEEK_TO_SAMPLE);
   if (ret) {
      ALLEGRO_WARN("Could not decode MP3 stream: %d.\n", ret);
      goto failure;
   }

   mp3file->freq = mp3file->dec.info.hz;
   mp3file->channels = mp3file->dec.info.channels;
   mp3file->chan_conf = _al_count_to_channel_conf(mp3file->channels);
   mp3file->file_samples = mp3file->dec.samples;
   mp3file->loop_end = (double)mp3file->file_samples
      / mp3file->channels / mp3file->freq;

   ALLEGRO_DEBUG("Channels %d, frequency %d\n",
      mp3file->channels, mp3file->freq);

   stream = al_create_audio_stream(buffer_count, samples, mp3file->freq,
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
   mp3dec_ex_close(&mp3file->dec);
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
