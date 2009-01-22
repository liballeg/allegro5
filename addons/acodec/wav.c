/*
 * Allegro5 WAV/PCM/AIFF reader
 * author: Ryan Dickie (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"
#include "allegro5/internal/aintern_memory.h"


#ifdef ALLEGRO_CFG_ACODEC_SNDFILE

#include <sndfile.h>

typedef struct _sf_private _sf_private;
struct _sf_private {
   SF_INFO sfinfo;
   SNDFILE *sndfile;
   ALLEGRO_FS_ENTRY *fh;
};

static sf_count_t _sf_vio_get_filelen(void *dptr)
{
   ALLEGRO_FS_ENTRY *fh = (ALLEGRO_FS_ENTRY *)dptr;

   return (sf_count_t)al_get_entry_size(fh);
}

static sf_count_t _sf_vio_seek(sf_count_t offset, int whence, void *dptr)
{
   ALLEGRO_FS_ENTRY *fh = (ALLEGRO_FS_ENTRY *)dptr;

   switch(whence) {
      case SEEK_SET: whence = ALLEGRO_SEEK_SET; break;
      case SEEK_CUR: whence = ALLEGRO_SEEK_CUR; break;
      case SEEK_END: whence = ALLEGRO_SEEK_END; break;
   }
   
   return (sf_count_t)al_fseek(fh, offset, whence);
}

static sf_count_t _sf_vio_read(void *ptr, sf_count_t count, void *dptr)
{
   ALLEGRO_FS_ENTRY *fh = (ALLEGRO_FS_ENTRY *)dptr;

   /* is this a bug waiting to happen? what does libsndfile expect to happen? */
   /* undocumented api's ftw */
   return (sf_count_t)al_fread(fh, count, ptr);
}

static sf_count_t _sf_vio_write(const void *ptr, sf_count_t count, void *dptr)
{
   ALLEGRO_FS_ENTRY *fh = (ALLEGRO_FS_ENTRY *)dptr;

   return (sf_count_t)al_fwrite(fh, count, ptr);
}

static sf_count_t _sf_vio_tell(void *dptr)
{
   ALLEGRO_FS_ENTRY *fh = (ALLEGRO_FS_ENTRY *)dptr;

   return (sf_count_t)al_ftell(fh);
}


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

_sf_private *_sf_open_private(AL_CONST char *filename)
{
   _sf_private *priv = NULL;
   SF_VIRTUAL_IO vio;

   /* Set up pointers to the locally defined vio functions. */
   vio.get_filelen = _sf_vio_get_filelen;
   vio.seek = _sf_vio_seek;
   vio.read = _sf_vio_read;
   vio.write = _sf_vio_write;
   vio.tell = _sf_vio_tell;

   priv = _AL_MALLOC(sizeof(_sf_private));
   if (priv == NULL)
      return NULL;

   memset(priv, 0, sizeof(_sf_private));

   priv->fh = al_fopen(filename, "rb");
   if (priv->fh == NULL)
      goto _sf_open_private_fatal;

   priv->sndfile = sf_open_virtual(&vio, SFM_READ, &(priv->sfinfo), (void*)(priv->fh));
   if(priv->sndfile == NULL)
      goto _sf_open_private_fatal;
   
   return priv;
   
_sf_open_private_fatal:
   if(priv->sndfile)
      sf_close(priv->sndfile);
   
   if(priv->fh)
      al_fclose(priv->fh);
   
   _AL_FREE(priv);
   
   return NULL;
}

void _sf_close_private(_sf_private *priv)
{
   ASSERT(priv);

   if(priv->sndfile)
      sf_close(priv->sndfile);

   if(priv->fh)
      al_fclose(priv->fh);

   memset(priv, 0, sizeof(_sf_private));
   
   _AL_FREE(priv);
}

ALLEGRO_SAMPLE_DATA *al_load_sample_data_sndfile(const char *filename)
{
   ALLEGRO_AUDIO_DEPTH depth;
   _sf_private *priv = NULL;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   void *buffer;
   ALLEGRO_SAMPLE_DATA *sample;

   priv = _sf_open_private(filename);
   if (priv == NULL)
      return NULL;

   word_size = 0;
   depth = _get_depth_enum(priv->sfinfo.format, &word_size);  
   if (depth == 0) {
      /* XXX set error */
      _sf_close_private(priv);
      return NULL;
   }
   channels = priv->sfinfo.channels;
   rate = priv->sfinfo.samplerate;
   total_samples = priv->sfinfo.frames;
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
      _sf_close_private(priv);
      return NULL;
   }

   /* XXX error handling */
   switch (depth) {
      case ALLEGRO_AUDIO_DEPTH_INT16:
         sf_readf_short(priv->sndfile, buffer, total_samples);
         break;
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         sf_readf_float(priv->sndfile, buffer, total_samples);
         break;
      case ALLEGRO_AUDIO_DEPTH_UINT8:
      case ALLEGRO_AUDIO_DEPTH_INT24:
         sf_read_raw(priv->sndfile, buffer, total_samples);
         break;
      default:
         ASSERT(false);
         break;
   }

   _sf_close_private(priv);

   sample = al_create_sample_data(buffer, total_samples, rate, depth,
         _al_count_to_channel_conf(channels), true);

   /* XXX set error if !sample */

   return sample;
}


static size_t _sndfile_stream_update(ALLEGRO_STREAM *stream, void *data,
   size_t buf_size)
{
   int bytes_per_sample, samples, num_read, bytes_read, silence;
   ALLEGRO_AUDIO_DEPTH depth = stream->spl.spl_data.depth;

   _sf_private *priv = (_sf_private *) stream->extra;
   bytes_per_sample = al_get_channel_count(stream->spl.spl_data.chan_conf)
                    * al_get_depth_size(depth);
   samples = buf_size / bytes_per_sample;

   if (depth == ALLEGRO_AUDIO_DEPTH_INT16) {
      num_read = sf_readf_short(priv->sndfile, data, samples);
   }
   else if (depth == ALLEGRO_AUDIO_DEPTH_FLOAT32) {
      num_read = sf_readf_float(priv->sndfile, data, samples);
   }
   else {
      num_read = sf_read_raw(priv->sndfile, data, samples);
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
   _sf_private *priv = (_sf_private *) stream->extra;
   return (sf_seek(priv->sndfile, 0, SEEK_SET) != -1);
}


static void _sndfile_stream_close(ALLEGRO_STREAM *stream)
{
   _sf_private *priv = (_sf_private *) stream->extra;
   ALLEGRO_EVENT quit_event;

   quit_event.type = _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE;
   al_emit_user_event(*((ALLEGRO_EVENT_SOURCE**)stream), &quit_event, NULL);
   al_join_thread(stream->feed_thread, NULL);
   al_destroy_thread(stream->feed_thread);

   _sf_close_private(priv);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}


ALLEGRO_STREAM *al_load_stream_sndfile(size_t buffer_count,
                                       unsigned long samples,
                                       const char *filename)
{
   ALLEGRO_AUDIO_DEPTH depth;
   _sf_private *priv = NULL;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   ALLEGRO_STREAM* stream;

   priv = _sf_open_private(filename);
   if (priv == NULL)
      return NULL;

   /* supports 16-bit, 32-bit (and float) */
   word_size = 0;
   depth = _get_depth_enum(priv->sfinfo.format, &word_size);
   channels = priv->sfinfo.channels;
   rate = priv->sfinfo.samplerate;
   total_samples = priv->sfinfo.frames;
   total_size = total_samples * channels * word_size;

   TRACE("loaded sample %s with properties:\n", filename);
   TRACE("      channels %d\n", channels);
   TRACE("      word_size %d\n", word_size);
   TRACE("      rate %ld\n", rate);
   TRACE("      total_samples %ld\n", total_samples);
   TRACE("      total_size %ld\n", total_size);

   stream = al_create_stream(buffer_count, samples, rate, depth,
                             _al_count_to_channel_conf(channels));

   stream->extra = priv;
   stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
   stream->feeder = _sndfile_stream_update;
   stream->unload_feeder = _sndfile_stream_close;
   stream->rewind_feeder = _sndfile_stream_rewind;
   al_start_thread(stream->feed_thread);

   return stream;
}


#endif /* ALLEGRO_CFG_ACODEC_SNDFILE */

/* vim: set sts=3 sw=3 et: */
