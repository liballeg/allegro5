/*
 * Allegro5 WAV reader
 * author: Matthew Leverton
 */

#include <stdio.h>

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "acodec.h"
#include "helper.h"

ALLEGRO_DEBUG_CHANNEL("wav")


typedef struct WAVFILE
{
   ALLEGRO_FILE *f;
   size_t dpos;     /* the starting position of the data chunk */
   int freq;        /* e.g., 44100 */
   short bits;      /* 8 (unsigned char) or 16 (signed short) */
   short channels;  /* 1 (mono) or 2 (stereo) */
   int sample_size; /* channels * bits/8 */
   int samples;     /* # of samples. size = samples * sample_size */
   double loop_start;
   double loop_end;
} WAVFILE;


/* wav_open:
 *  Opens f and prepares a WAVFILE struct with the WAV format info.
 *  On a successful return, the ALLEGRO_FILE is at the beginning of the sample data.
 *  returns the WAVFILE on success, or NULL on failure.
 */
static WAVFILE *wav_open(ALLEGRO_FILE *f)
{
   WAVFILE *wavfile = NULL;
   char buffer[12];

   if (!f)
      goto wav_open_error;

   /* prepare default values */
   wavfile = al_malloc(sizeof(WAVFILE));
   if (!wavfile) {
      ALLEGRO_ERROR("Failed to allocate WAVFILE.\n");
      return NULL;
   }
   wavfile->f = f;
   wavfile->freq = 22050;
   wavfile->bits = 8;
   wavfile->channels = 1;

   /* check the header */
   if (al_fread(f, buffer, 12) != 12) {
      ALLEGRO_ERROR("Unexpected EOF while reading the header.\n");
      goto wav_open_error;
   }

   if (memcmp(buffer, "RIFF", 4) || memcmp(buffer+8, "WAVE", 4)) {
      ALLEGRO_ERROR("Bad magic number.\n");
      goto wav_open_error;
   }

   /* Read as many leading fmt chunks as exist, then read until a data chunk
    * is found.
    */
   while (true) {
      int length = 0;
      short pcm = 0;

      if (al_fread(f, buffer, 4) != 4) {
         ALLEGRO_ERROR("Unexpected EOF while reading RIFF type.\n");
         goto wav_open_error;
      }

      /* check to see if it's a fmt chunk */
      if (!memcmp(buffer, "fmt ", 4)) {

         length = al_fread32le(f);
         if (length < 16) {
            ALLEGRO_ERROR("Bad length: %d.\n", length);
            goto wav_open_error;
         }

         /* should be 1 for PCM data */
         pcm = al_fread16le(f);
         if (pcm != 1) {
            ALLEGRO_ERROR("Bad PCM value: %d.\n", pcm);
            goto wav_open_error;
         }

         /* mono or stereo data */
         wavfile->channels = al_fread16le(f);

         if ((wavfile->channels != 1) && (wavfile->channels != 2)) {
            ALLEGRO_ERROR("Bad number of channels: %d.\n", wavfile->channels);
            goto wav_open_error;
         }

         /* sample frequency */
         wavfile->freq = al_fread32le(f);

         /* skip six bytes */
         al_fseek(f, 6, ALLEGRO_SEEK_CUR);

         /* 8 or 16 bit data? */
         wavfile->bits = al_fread16le(f);
         if ((wavfile->bits != 8) && (wavfile->bits != 16)) {
            ALLEGRO_ERROR("Bad number of bits: %d.\n", wavfile->bits);
            goto wav_open_error;
         }

         /* Skip remainder of chunk */
         length -= 16;
         if (length > 0)
            al_fseek(f, length, ALLEGRO_SEEK_CUR);
      }
      else {
         if (!memcmp(buffer, "data", 4)) {
            ALLEGRO_ERROR("Bad RIFF type.\n");
            break;
         }
         ALLEGRO_INFO("Ignoring chunk: %c%c%c%c\n", buffer[0], buffer[1],
            buffer[2], buffer[3]);
         length = al_fread32le(f);
         al_fseek(f, length, ALLEGRO_SEEK_CUR);
      }
   }

   /* find out how many samples exist */
   wavfile->samples = al_fread32le(f);

   if (wavfile->channels == 2) {
      wavfile->samples = (wavfile->samples + 1) / 2;
   }

   if (wavfile->bits == 16) {
      wavfile->samples /= 2;
   }

   wavfile->sample_size = wavfile->channels * wavfile->bits / 8;

   wavfile->dpos = al_ftell(f);

   return wavfile;

wav_open_error:

   if (wavfile)
      al_free(wavfile);

   return NULL;
}

/* wav_read:
 *  Reads up to 'samples' number of samples from the wav ALLEGRO_FILE into 'data'.
 *  Returns the actual number of samples written to 'data'.
 */
static size_t wav_read(WAVFILE *wavfile, void *data, size_t samples)
{
   size_t bytes_read;
   size_t cur_samples;

   ASSERT(wavfile);

   cur_samples = (al_ftell(wavfile->f) - wavfile->dpos) / wavfile->sample_size;
   if (cur_samples + samples > (size_t)wavfile->samples)
      samples = wavfile->samples - cur_samples;

   bytes_read = al_fread(wavfile->f, data, samples * wavfile->sample_size);

   /* PCM data in RIFF WAV files is little endian.
    * PCM data in RIFX WAV files is big endian (which we don't support).
    */
#ifdef ALLEGRO_BIG_ENDIAN
   if (wavfile->bits == 16) {
      uint16_t *p = data;
      const uint16_t *const end = p + (bytes_read >> 1);

      /* swap high/low bytes */
      while (p < end) {
         *p = ((*p << 8) | (*p >> 8));
         p++;
      }
   }
#endif

   return bytes_read / wavfile->sample_size;
}


/* wav_close:
 *  Closes the ALLEGRO_FILE and frees the WAVFILE struct.
 */
static void wav_close(WAVFILE *wavfile)
{
   ASSERT(wavfile);

   al_free(wavfile);
}


static bool wav_stream_seek(ALLEGRO_AUDIO_STREAM * stream, double time)
{
   WAVFILE *wavfile = (WAVFILE *) stream->extra;
   int align = (wavfile->bits / 8) * wavfile->channels;
   unsigned long cpos = time * (double)(wavfile->freq * (wavfile->bits / 8) * wavfile->channels);
   if (time >= wavfile->loop_end)
      return false;
   cpos += cpos % align;
   return al_fseek(wavfile->f, wavfile->dpos + cpos, ALLEGRO_SEEK_SET);
}


/* wav_stream_rewind:
 *  Rewinds 'stream' to the beginning of the data chunk.
 *  Returns true on success, false on failure.
 */
static bool wav_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   WAVFILE *wavfile = (WAVFILE *) stream->extra;
   return wav_stream_seek(stream, wavfile->loop_start);
}


static double wav_stream_get_position(ALLEGRO_AUDIO_STREAM * stream)
{
   WAVFILE *wavfile = (WAVFILE *) stream->extra;
   double samples_per = (double)((wavfile->bits / 8) * wavfile->channels) * (double)(wavfile->freq);
   return ((double)(al_ftell(wavfile->f) - wavfile->dpos) / samples_per);
}


static double wav_stream_get_length(ALLEGRO_AUDIO_STREAM * stream)
{
   WAVFILE *wavfile = (WAVFILE *) stream->extra;
   double total_time = (double)(wavfile->samples) / (double)(wavfile->freq);
   return total_time;
}


static bool wav_stream_set_loop(ALLEGRO_AUDIO_STREAM * stream, double start, double end)
{
   WAVFILE *wavfile = (WAVFILE *) stream->extra;
   wavfile->loop_start = start;
   wavfile->loop_end = end;
   return true;
}


/* wav_stream_update:
 *  Updates 'stream' with the next chunk of data.
 *  Returns the actual number of bytes written.
 */
static size_t wav_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   int bytes_per_sample, samples, samples_read;
   double ctime, btime;

   WAVFILE *wavfile = (WAVFILE *) stream->extra;
   bytes_per_sample = (wavfile->bits / 8) * wavfile->channels;
   ctime = wav_stream_get_position(stream);
   btime = ((double)buf_size / (double)bytes_per_sample) / (double)(wavfile->freq);

   if (stream->spl.loop != _ALLEGRO_PLAYMODE_STREAM_ONCE && ctime + btime > wavfile->loop_end) {
      samples = ((wavfile->loop_end - ctime) * (double)(wavfile->freq));
   }
   else {
      samples = buf_size / bytes_per_sample;
   }
   if (samples < 0)
      return 0;

   samples_read = wav_read(wavfile, data, samples);

   return samples_read * bytes_per_sample;
}


/* wav_stream_close:
 *  Closes the 'stream'.
 */
static void wav_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   WAVFILE *wavfile = (WAVFILE *) stream->extra;

   _al_acodec_stop_feed_thread(stream);

   al_fclose(wavfile->f);
   wav_close(wavfile);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}


/* _al_load_wav:
 *  Reads a RIFF WAV format sample ALLEGRO_FILE, returning an ALLEGRO_SAMPLE
 *  structure, or NULL on error.
 */
ALLEGRO_SAMPLE *_al_load_wav(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   spl = _al_load_wav_f(f);

   al_fclose(f);

   return spl;
}

ALLEGRO_SAMPLE *_al_load_wav_f(ALLEGRO_FILE *fp)
{
   WAVFILE *wavfile = wav_open(fp);
   ALLEGRO_SAMPLE *spl = NULL;

   if (wavfile) {
      size_t n = (wavfile->bits / 8) * wavfile->channels * wavfile->samples;
      char *data = al_malloc(n);

      if (data) {
         spl = al_create_sample(data, wavfile->samples, wavfile->freq,
            _al_word_size_to_depth_conf(wavfile->bits / 8),
            _al_count_to_channel_conf(wavfile->channels), true);

         if (spl) {
            memset(data, 0, n);
            wav_read(wavfile, data, wavfile->samples);
         }
         else {
            al_free(data);
         }
      }
      wav_close(wavfile);
   }

   return spl;
}


/* _al_load_wav_audio_stream:
*/
ALLEGRO_AUDIO_STREAM *_al_load_wav_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = _al_load_wav_audio_stream_f(f, buffer_count, samples);
   if (!stream) {
      ALLEGRO_ERROR("Failed to load wav stream.\n");
      al_fclose(f);
   }

   return stream;
}

/* _al_load_wav_audio_stream_f:
*/
ALLEGRO_AUDIO_STREAM *_al_load_wav_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples)
{
   WAVFILE* wavfile;
   ALLEGRO_AUDIO_STREAM* stream;

   wavfile = wav_open(f);

   if (wavfile == NULL) {
      ALLEGRO_ERROR("Failed to load wav file.\n");
      return NULL;
   }

   stream = al_create_audio_stream(buffer_count, samples, wavfile->freq,
      _al_word_size_to_depth_conf(wavfile->bits / 8),
      _al_count_to_channel_conf(wavfile->channels));

   if (stream) {
      stream->extra = wavfile;
      wavfile->loop_start = 0.0;
      wavfile->loop_end = wav_stream_get_length(stream);
      stream->feeder = wav_stream_update;
      stream->unload_feeder = wav_stream_close;
      stream->rewind_feeder = wav_stream_rewind;
      stream->seek_feeder = wav_stream_seek;
      stream->get_feeder_position = wav_stream_get_position;
      stream->get_feeder_length = wav_stream_get_length;
      stream->set_feeder_loop = wav_stream_set_loop;
      _al_acodec_start_feed_thread(stream);
   }
   else {
      ALLEGRO_ERROR("Failed to load wav stream.\n");
      wav_close(wavfile);
   }

   return stream;
}


/* _al_save_wav:
 * Writes a sample into a wav ALLEGRO_FILE.
 * Returns true on success, false on error.
 */
bool _al_save_wav(const char *filename, ALLEGRO_SAMPLE *spl)
{
   ALLEGRO_FILE *pf = al_fopen(filename, "wb");

   if (pf) {
      bool rvsave = _al_save_wav_f(pf, spl);
      bool rvclose = al_fclose(pf);
      return rvsave && rvclose;
   }
   else {
      ALLEGRO_ERROR("Unable to open %s for writing.\n", filename);
   }

   return false;
}


/* _al_save_wav_f:
 * Writes a sample into a wav packfile.
 * Returns true on success, false on error.
 */
bool _al_save_wav_f(ALLEGRO_FILE *pf, ALLEGRO_SAMPLE *spl)
{
   size_t channels, bits;
   size_t data_size;
   size_t samples;
   size_t i, n;

   ASSERT(spl);
   ASSERT(pf);

   /* XXX: makes use of ALLEGRO_SAMPLE internals */

   channels = (spl->chan_conf >> 4) + (spl->chan_conf & 0xF);
   bits = (spl->depth == ALLEGRO_AUDIO_DEPTH_INT8 ||
           spl->depth == ALLEGRO_AUDIO_DEPTH_UINT8) ? 8 : 16;

   if (channels < 1 || channels > 2) {
      ALLEGRO_ERROR("Can only save samples with 1 or 2 channels as WAV.\n");
      return false;
   }

   samples = spl->len;
   data_size = samples * channels * bits / 8;
   n = samples * channels;

   al_fputs(pf, "RIFF");
   al_fwrite32le(pf, 36 + data_size);
   al_fputs(pf, "WAVE");

   al_fputs(pf, "fmt ");
   al_fwrite32le(pf, 16);
   al_fwrite16le(pf, 1);
   al_fwrite16le(pf, (int16_t)channels);
   al_fwrite32le(pf, spl->frequency);
   al_fwrite32le(pf, spl->frequency * channels * bits / 8);
   al_fwrite16le(pf, (int16_t)(channels * bits / 8));
   al_fwrite16le(pf, (int16_t)bits);

   al_fputs(pf, "data");
   al_fwrite32le(pf, data_size);


   if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT8) {
      al_fwrite(pf, spl->buffer.u8, samples * channels);
   }
   else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT16) {
      al_fwrite(pf, spl->buffer.s16, samples * channels * 2);
   }
   else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT8) {
      int8_t *data = spl->buffer.s8;
      for (i = 0; i < samples; ++i) {
         al_fputc(pf, *data++ + 0x80);
      }
   }
   else if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT16) {
      uint16_t *data = spl->buffer.u16;
      for (i = 0; i < n; ++i) {
         al_fwrite16le(pf, *data++ - 0x8000);
      }
   }
   else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT24) {
      int32_t *data = spl->buffer.s24;
      for (i = 0; i < n; ++i) {
         const int v = ((float)(*data++ + 0x800000) / 0x7FFFFF) * 0x7FFF - 0x8000;
         al_fwrite16le(pf, v);
      }
   }
   else if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT24) {
      uint32_t *data = spl->buffer.u24;
      for (i = 0; i < n; ++i) {
         const int v = ((float)(*data++) / 0x7FFFFF) * 0x7FFF - 0x8000;
         al_fwrite16le(pf, v);
      }
   }
   else if (spl->depth == ALLEGRO_AUDIO_DEPTH_FLOAT32) {
      float *data = spl->buffer.f32;
      for (i = 0; i < n; ++i) {
         al_fwrite16le(pf, *data * 0x7FFF);
         data++;
      }
   }
   else {
      ALLEGRO_ERROR("Unknown audio depth (%d) when saving wav ALLEGRO_FILE.\n",
         spl->depth);
      return false;
   }

   return true;
}


bool _al_identify_wav(ALLEGRO_FILE *f)
{
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "RIFF", 4) != 0)
      return false;
   if (!al_fseek(f, 4, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "WAVE", 4) == 0)
      return true;
   return false;
}

/* vim: set sts=3 sw=3 et: */
