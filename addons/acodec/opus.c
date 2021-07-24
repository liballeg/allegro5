/*
 * Allegro5 Opus reader.
 * author: Boris Carvajal (c) 2016
 * Based on ogg.c code
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

#ifndef ALLEGRO_CFG_ACODEC_OPUS
   #error configuration problem, ALLEGRO_CFG_ACODEC_OPUS not set
#endif

ALLEGRO_DEBUG_CHANNEL("acodec")

#include <opus/opusfile.h>

typedef struct AL_OP_DATA AL_OP_DATA;

struct AL_OP_DATA {
   OggOpusFile *of;
   ALLEGRO_FILE *file;
   int channels;
   int bitstream;
   double loop_start;
   double loop_end;
};

/* dynamic loading support (Windows only currently) */
#ifdef ALLEGRO_CFG_ACODEC_OPUSFILE_DLL
static void *op_dll = NULL;
static bool op_virgin = true;
#endif

static struct
{
   void (*op_free)(OggOpusFile *_of);
   int (*op_channel_count)(const OggOpusFile *_of,int _li);
   OggOpusFile *(*op_open_callbacks)(void *_source, const OpusFileCallbacks *_cb, const unsigned char *_initial_data, size_t _initial_bytes, int *_error);
   ogg_int64_t (*op_pcm_total)(const OggOpusFile *_of, int _li);
   int (*op_pcm_seek)(OggOpusFile *_of, ogg_int64_t _pcm_offset);
   ogg_int64_t (*op_pcm_tell)(const OggOpusFile *_of);
   int (*op_read)(OggOpusFile *_of, opus_int16 *_pcm, int _buf_size, int *_li);
} lib;


#ifdef ALLEGRO_CFG_ACODEC_OPUSFILE_DLL
static void shutdown_dynlib(void)
{
   if (op_dll) {
      _al_close_library(op_dll);
      op_dll = NULL;
      op_virgin = true;
   }
}
#endif


static bool init_dynlib(void)
{
#ifdef ALLEGRO_CFG_ACODEC_OPUSFILE_DLL
   if (op_dll) {
      return true;
   }

   if (!op_virgin) {
      return false;
   }

   op_virgin = false;

   op_dll = _al_open_library(ALLEGRO_CFG_ACODEC_OPUSFILE_DLL);
   if (!op_dll) {
      ALLEGRO_WARN("Could not load " ALLEGRO_CFG_ACODEC_OPUSFILE_DLL "\n");
      return false;
   }

   _al_add_exit_func(shutdown_dynlib, "shutdown_dynlib");

   #define INITSYM(x)                                                         \
      do                                                                      \
      {                                                                       \
         lib.x = _al_import_symbol(op_dll, #x);                               \
         if (lib.x == 0) {                                                    \
            ALLEGRO_ERROR("undefined symbol in lib structure: " #x "\n");     \
            return false;                                                     \
         }                                                                    \
      } while(0)
#else
   #define INITSYM(x)   (lib.x = (x))
#endif

   memset(&lib, 0, sizeof(lib));

   INITSYM(op_free);
   INITSYM(op_channel_count);
   INITSYM(op_open_callbacks);
   INITSYM(op_pcm_total);
   INITSYM(op_pcm_seek);
   INITSYM(op_pcm_tell);
   INITSYM(op_read);

   return true;

#undef INITSYM
}


static int read_callback(void *stream, unsigned char *ptr, int nbytes)
{
   AL_OP_DATA *op = (AL_OP_DATA *)stream;
   size_t ret = 0;

   ret = al_fread(op->file, ptr, nbytes);

   return ret;
}


static int seek_callback(void *stream, opus_int64 offset, int whence)
{
   AL_OP_DATA *op = (AL_OP_DATA *)stream;

   switch(whence) {
      case SEEK_SET: whence = ALLEGRO_SEEK_SET; break;
      case SEEK_CUR: whence = ALLEGRO_SEEK_CUR; break;
      case SEEK_END: whence = ALLEGRO_SEEK_END; break;
   }

   if (!al_fseek(op->file, offset, whence)) {
      return -1;
   }

   return 0;
}


static opus_int64 tell_callback(void *stream)
{
   AL_OP_DATA *op = (AL_OP_DATA *)stream;
   int64_t ret = 0;

   ret = al_ftell(op->file);
   if (ret == -1)
      return -1;

   return ret;
}


static int close_callback(void *stream)
{
   /* Don't close stream->file here. */
   (void)stream;
   return 0;
}


static OpusFileCallbacks callbacks = {
   read_callback,
   seek_callback,
   tell_callback,
   close_callback
};


ALLEGRO_SAMPLE *_al_load_ogg_opus(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);

   ALLEGRO_INFO("Loading sample %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   spl = _al_load_ogg_opus_f(f);

   al_fclose(f);

   return spl;
}


ALLEGRO_SAMPLE *_al_load_ogg_opus_f(ALLEGRO_FILE *file)
{
   /* Note: decoding library can return 16-bit or floating-point output,
    * both using native endian ordering. (TODO: Implement float output,
    * individual links in the stream...)
    */
   int word_size = 2; /* 2 = 16-bit. */
   const int packet_size = 5760; /* suggestion for size to read at a time */
   OggOpusFile *of;
   opus_int16 *buffer;
   ALLEGRO_SAMPLE *sample;
   int channels;
   long rate;
   ogg_int64_t total_samples;
   int bitstream;
   ogg_int64_t total_size;
   AL_OP_DATA op;
   ogg_int64_t pos;
   long read;

   if (!init_dynlib()) {
      return NULL;
   }

   op.file = file;
   of = lib.op_open_callbacks(&op, &callbacks, NULL, 0, NULL);
   if (!of) {
      ALLEGRO_ERROR("Audio file does not appear to be an Ogg bitstream.\n");
      return NULL;
   }

   bitstream = -1;
   channels = lib.op_channel_count(of, bitstream);
   rate = 48000;
   total_samples = lib.op_pcm_total(of, bitstream);
   total_size = total_samples * channels * word_size;

   ALLEGRO_DEBUG("channels %d\n", channels);
   ALLEGRO_DEBUG("word_size %d\n", word_size);
   ALLEGRO_DEBUG("rate %ld\n", rate);
   ALLEGRO_DEBUG("total_samples %ld\n", (long)total_samples);
   ALLEGRO_DEBUG("total_size %ld\n", (long)total_size);

   buffer = al_malloc(total_size);
   if (!buffer) {
      return NULL;
   }

   pos = 0;
   while (pos < total_samples) {
      const int read_size = _ALLEGRO_MIN(packet_size, total_samples - pos);
      ASSERT(pos + read_size <= total_samples);

      /* XXX error handling */
      read = lib.op_read(of, buffer + pos * channels, read_size, NULL);

      pos += read;
      if (read == 0)
         break;
   }

   lib.op_free(of);

   sample = al_create_sample(buffer, total_samples, rate,
      _al_word_size_to_depth_conf(word_size),
      _al_count_to_channel_conf(channels), true);

   if (!sample) {
      al_free(buffer);
   }

   return sample;
}


static bool ogg_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   if (time >= extra->loop_end)
      return false;

   return !lib.op_pcm_seek(extra->of, time * 48000);;
}


static bool ogg_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   return ogg_stream_seek(stream, extra->loop_start);
}


static double ogg_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   return lib.op_pcm_tell(extra->of)/48000.0;
}


static double ogg_stream_get_length(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   return lib.op_pcm_total(extra->of, -1)/48000.0;
}


static bool ogg_stream_set_loop(ALLEGRO_AUDIO_STREAM *stream, double start, double end)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   extra->loop_start = start;
   extra->loop_end = end;

   return true;
}


/* To be called when stream is destroyed */
static void ogg_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   _al_acodec_stop_feed_thread(stream);

   al_fclose(extra->file);

   lib.op_free(extra->of);
   al_free(extra);
   stream->extra = NULL;
}


static size_t ogg_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
                                size_t buf_size)
{
   AL_OP_DATA *extra = (AL_OP_DATA *) stream->extra;

   const int word_size = 2;   /* 2 = 16-bit. opus_int16 size, for use in op_read */

   unsigned long pos = 0;
   int read_length = buf_size;
   int buf_in_word;
   long rate = 48000;
   int channels = extra->channels;

   double ctime = lib.op_pcm_tell(extra->of)/(double)rate;

   double btime = ((double)buf_size / (word_size * channels)) / rate;
   long read;

   if (stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR) {
      if (ctime + btime > extra->loop_end) {
         read_length = (extra->loop_end - ctime) * rate * word_size * channels;
         if (read_length < 0)
            return 0;
         read_length += read_length % word_size;
      }
   }

   buf_in_word= read_length/word_size;

   while (pos < (unsigned long) read_length) {
      read = lib.op_read(extra->of, (opus_int16 *)data + pos, buf_in_word - pos, NULL);
      pos += read * channels;

      if (read == 0) {
         /* Return the number of useful bytes written. */
         return pos*word_size;
      }
   }

   return pos;
}


ALLEGRO_AUDIO_STREAM *_al_load_ogg_opus_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   ALLEGRO_INFO("Loading stream %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   stream = _al_load_ogg_opus_audio_stream_f(f, buffer_count, samples);
   if (!stream) {
      al_fclose(f);
   }

   return stream;
}


ALLEGRO_AUDIO_STREAM *_al_load_ogg_opus_audio_stream_f(ALLEGRO_FILE *file,
   size_t buffer_count, unsigned int samples)
{
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   OggOpusFile* of;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   int bitstream;
   AL_OP_DATA* extra;
   ALLEGRO_AUDIO_STREAM* stream;

   if (!init_dynlib()) {
      return NULL;
   }

   extra = al_malloc(sizeof(AL_OP_DATA));
   if (extra == NULL) {
      ALLEGRO_ERROR("Failed to allocate AL_OP_DATA struct.\n");
      return NULL;
   }

   extra->file = file;

   of = lib.op_open_callbacks(extra, &callbacks, NULL, 0, NULL);
   if (!of) {
      ALLEGRO_ERROR("ogg: Input does not appear to be an Ogg bitstream.\n");
      return NULL;
   }

   extra->of = of;
   extra->bitstream = -1;
   bitstream = extra->bitstream;

   extra->channels = lib.op_channel_count(of, bitstream);
   channels = extra->channels;

   rate = 48000;
   total_samples = lib.op_pcm_total(of, bitstream);
   total_size = total_samples * channels * word_size;


   ALLEGRO_DEBUG("channels %d\n", channels);
   ALLEGRO_DEBUG("word_size %d\n", word_size);
   ALLEGRO_DEBUG("rate %ld\n", rate);
   ALLEGRO_DEBUG("total_samples %ld\n", total_samples);
   ALLEGRO_DEBUG("total_size %ld\n", total_size);

   stream = al_create_audio_stream(buffer_count, samples, rate,
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels));
   if (!stream) {
      lib.op_free(of);
      return NULL;
   }

   stream->extra = extra;

   extra->loop_start = 0.0;
   extra->loop_end = ogg_stream_get_length(stream);
   stream->quit_feed_thread = false;
   stream->feeder = ogg_stream_update;
   stream->rewind_feeder = ogg_stream_rewind;
   stream->seek_feeder = ogg_stream_seek;
   stream->get_feeder_position = ogg_stream_get_position;
   stream->get_feeder_length = ogg_stream_get_length;
   stream->set_feeder_loop = ogg_stream_set_loop;
   stream->unload_feeder = ogg_stream_close;
   _al_acodec_start_feed_thread(stream);

   return stream;
}


bool _al_identify_ogg_opus(ALLEGRO_FILE *f)
{
   uint8_t x[10];
   if (al_fread(f, x, 4) < 4)
      return false;
   if (memcmp(x, "OggS", 4) != 0)
      return false;
   if (!al_fseek(f, 22, ALLEGRO_SEEK_CUR))
      return false;
   if (al_fread(f, x, 10) < 10)
      return false;
   if (memcmp(x, "\x01\x13OpusHead", 10) == 0)
      return true;
   return false;
}

/* vim: set sts=3 sw=3 et: */
