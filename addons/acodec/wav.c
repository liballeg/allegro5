/*
 * Allegro5 WAV/PCM/AIFF reader
 * author: Ryan Dickie (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"
#include "allegro5/internal/aintern_memory.h"


#ifdef ALLEGRO_CFG_ACODEC_SNDFILE

#include <sndfile.h>


static ALLEGRO_AUDIO_DEPTH _get_depth_enum(int format, int *word_size)
{
   switch (format & 0xFFFF) {
      case SF_FORMAT_PCM_U8:
         *word_size = 1;
         return ALLEGRO_AUDIO_DEPTH_UINT8;

      case SF_FORMAT_PCM_16:
         *word_size = 2;
         return ALLEGRO_AUDIO_DEPTH_INT16;

      case SF_FORMAT_PCM_24:
         *word_size = 3;
         return ALLEGRO_AUDIO_DEPTH_INT24;

      case SF_FORMAT_FLOAT:
         *word_size = 4;
         return ALLEGRO_AUDIO_DEPTH_FLOAT32;

      default:
         TRACE("Unsupported sndfile depth format (%X)\n", format);
         *word_size = 0;
         return 0;
   }
}


ALLEGRO_SAMPLE_DATA *al_load_sample_sndfile(const char *filename)
{
   ALLEGRO_AUDIO_DEPTH depth;
   SF_INFO sfinfo;
   SNDFILE *sndfile;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   void *buffer;
   ALLEGRO_SAMPLE_DATA *sample;

   sfinfo.format = 0;
   sndfile = sf_open(filename, SFM_READ, &sfinfo); 

   if (sndfile == NULL)
      return NULL;

   word_size = 0;
   depth = _get_depth_enum(sfinfo.format, &word_size);  
   if (depth == 0) {
      /* XXX set error */
      sf_close(sndfile);
      return NULL;
   }
   channels = sfinfo.channels;
   rate = sfinfo.samplerate;
   total_samples = sfinfo.frames;
   total_size = total_samples * channels * word_size; 

   TRACE("Loaded sample %s with properties:\n", filename);
   TRACE("    channels %d\n", channels);
   TRACE("    word_size %d\n", word_size);
   TRACE("    rate %ld\n", rate);
   TRACE("    total_samples %ld\n", total_samples);
   TRACE("    total_size %ld\n", total_size);

   buffer = _AL_MALLOC_ATOMIC(total_size);
   if (!buffer) {
      /* XXX set error */
      sf_close(sndfile);
      return NULL;
   }

   /* XXX error handling */
   switch (depth) {
      case ALLEGRO_AUDIO_DEPTH_INT16:
         sf_readf_short(sndfile, buffer, total_samples);
         break;
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         sf_readf_float(sndfile, buffer, total_samples);
         break;
      case ALLEGRO_AUDIO_DEPTH_UINT8:
      case ALLEGRO_AUDIO_DEPTH_INT24:
         sf_read_raw(sndfile, buffer, total_samples);
         break;
      default:
         ASSERT(false);
         break;
   }

   sf_close(sndfile);

   sample = al_sample_data_create(buffer, total_samples, rate, depth,
         _al_count_to_channel_conf(channels), true);

   /* XXX set error if !sample */

   return sample;
}


static size_t _sndfile_stream_update(ALLEGRO_STREAM *stream, void *data,
   size_t buf_size)
{
   int bytes_per_sample, samples, num_read, bytes_read, silence;
   ALLEGRO_AUDIO_DEPTH depth = stream->spl.spl_data.depth;

   SNDFILE *sndfile = (SNDFILE *) stream->extra;
   bytes_per_sample = al_channel_count(stream->spl.spl_data.chan_conf)
                    * al_depth_size(depth);
   samples = buf_size / bytes_per_sample;

   if (depth == ALLEGRO_AUDIO_DEPTH_INT16) {
      num_read = sf_readf_short(sndfile, data, samples);
   }
   else if (depth == ALLEGRO_AUDIO_DEPTH_FLOAT32) {
      num_read = sf_readf_float(sndfile, data, samples);
   }
   else {
      num_read = sf_read_raw(sndfile, data, samples);
   }

   if (num_read != samples) {
      /* stream is dry */
      bytes_read = num_read * bytes_per_sample;
      silence = _al_audio_get_silence(depth);
      memset((char*)data + bytes_read, silence, buf_size - bytes_read);
   }

   /* return the number of usefull bytes written */
   return num_read * bytes_per_sample;
}


static bool _sndfile_stream_rewind(ALLEGRO_STREAM *stream)
{
   SNDFILE *sndfile = (SNDFILE *) stream->extra;
   return (sf_seek(sndfile, 0, SEEK_SET) != -1);
}


static void _sndfile_stream_close(ALLEGRO_STREAM *stream)
{
   SNDFILE *sndfile = (SNDFILE *) stream->extra;

   stream->quit_feed_thread = true;
   al_join_thread(stream->feed_thread, NULL);
   al_destroy_thread(stream->feed_thread);

   sf_close(sndfile);
   stream->extra = NULL;
}


ALLEGRO_STREAM *al_load_stream_sndfile(size_t buffer_count,
                                       unsigned long samples,
                                       const char *filename)
{
   ALLEGRO_AUDIO_DEPTH depth;
   SF_INFO sfinfo;
   SNDFILE* sndfile;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   ALLEGRO_STREAM* stream;

   sfinfo.format = 0;
   sndfile = sf_open(filename, SFM_READ, &sfinfo);

   if (sndfile == NULL)
      return NULL;

   /* supports 16-bit, 32-bit (and float) */
   word_size = 0;
   depth = _get_depth_enum(sfinfo.format, &word_size);
   channels = sfinfo.channels;
   rate = sfinfo.samplerate;
   total_samples = sfinfo.frames;
   total_size = total_samples * channels * word_size;

   TRACE("loaded sample %s with properties:\n", filename);
   TRACE("      channels %d\n", channels);
   TRACE("      word_size %d\n", word_size);
   TRACE("      rate %ld\n", rate);
   TRACE("      total_samples %ld\n", total_samples);
   TRACE("      total_size %ld\n", total_size);

   stream = al_stream_create(buffer_count, samples, rate, depth,
                             _al_count_to_channel_conf(channels));

   stream->extra = sndfile;
   stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
   stream->quit_feed_thread = false;
   stream->feeder = _sndfile_stream_update;
   stream->unload_feeder = _sndfile_stream_close;
   stream->rewind_feeder = _sndfile_stream_rewind;
   al_start_thread(stream->feed_thread);

   return stream;
}


#endif /* ALLEGRO_CFG_ACODEC_SNDFILE */

/* vim: set sts=3 sw=3 et: */
