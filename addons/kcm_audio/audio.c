/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"

void _al_set_error(int error, char* string)
{
   TRACE("%s (error code: %d)", string, error);
}

ALLEGRO_AUDIO_DRIVER *_al_kcm_driver = NULL;

#if defined(ALLEGRO_CFG_KCM_OPENAL)
   extern struct ALLEGRO_AUDIO_DRIVER _openal_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_ALSA)
   extern struct ALLEGRO_AUDIO_DRIVER _alsa_driver;
#endif

/* Channel configuration helpers */
bool al_is_channel_conf(ALLEGRO_AUDIO_ENUM conf)
{
   return ((conf >= ALLEGRO_AUDIO_1_CH) && (conf <= ALLEGRO_AUDIO_7_1_CH));
}

size_t al_channel_count(ALLEGRO_AUDIO_ENUM conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
size_t al_depth_size(ALLEGRO_AUDIO_ENUM conf)
{
   ALLEGRO_AUDIO_ENUM depth = conf&~ALLEGRO_AUDIO_UNSIGNED;
   if(depth == ALLEGRO_AUDIO_8_BIT_INT)
      return sizeof(int8_t);
   if(depth == ALLEGRO_AUDIO_16_BIT_INT)
      return sizeof(int16_t);
   if(depth == ALLEGRO_AUDIO_24_BIT_INT)
      return sizeof(int32_t);
   if(depth == ALLEGRO_AUDIO_32_BIT_FLOAT)
      return sizeof(float);
   return 0;
}


/* This function sets a sample's mutex pointer to the specified value. It is
   ALLEGRO_MIXER aware, and will recursively set any attached streams' mutex to
   the same value */
void _al_kcm_stream_set_mutex(ALLEGRO_SAMPLE *stream, _AL_MUTEX *mutex)
{
   if(stream->mutex == mutex)
      return;
   stream->mutex = mutex;

   /* If this is a mixer, we need to make sure all the attached streams also
      set the same mutex */
   if(stream->read == mixer_read)
   {
      ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER*)stream;
      int i;

      for(i = 0;mixer->streams[i];++i)
         _al_kcm_stream_set_mutex(mixer->streams[i], mutex);
   }
}

/* This function is ALLEGRO_MIXER aware and frees the memory associated with the
   sample or mixer, and detaches any attached streams or mixers */
static void stream_free(ALLEGRO_SAMPLE *spl)
{
   if(spl)
   {
      /* Make sure we free the mixer buffer and de-reference the attached
         streams if this is a mixer stream */
      if(spl->read == mixer_read)
      {
         ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER*)spl;
         int i;

         _al_kcm_stream_set_mutex(&mixer->ss, NULL);

         for(i = 0;mixer->streams[i];++i)
            mixer->streams[i]->parent.ptr = NULL;

         free(mixer->streams);
      }

      if(spl->orphan_buffer)
         free(spl->buffer.ptr);

      free(spl);
   }
}


static inline int32_t clamp(int32_t val, int32_t min, int32_t max)
{
   /* Clamp to min */
   val -= min;
   val &= (~val) >> 31;
   val += min;

   /* Clamp to max */
   val -= max;
   val &= val >> 31;
   val += max;

   return val;
}


/* Mixes the streams attached to the mixer and writes additively to the
   specified buffer (or if *buf is NULL, indicating a voice, convert it and
   set it to the buffer pointer) */
void mixer_read(void *source, void **buf, unsigned long samples, ALLEGRO_AUDIO_ENUM buffer_depth, size_t dest_maxc)
{
   const ALLEGRO_MIXER *mixer;
   ALLEGRO_MIXER *m = (ALLEGRO_MIXER*)source;
   int maxc = al_channel_count(m->ss.chan_conf);
   ALLEGRO_SAMPLE **spl;

   if(!m->ss.playing)
      return;

   /* make sure the mixer buffer is big enough */
   if(m->ss.len*maxc < samples*maxc)
   {
      free(m->ss.buffer.ptr);
      m->ss.buffer.ptr = malloc(samples*maxc*sizeof(float));
      if(!m->ss.buffer.ptr)
      {
         _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating mixer buffer");
         m->ss.len = 0;
         return;
      }
      m->ss.len = samples;
   }

   mixer = m;

   /* Clear the buffer to silence */
   memset(mixer->ss.buffer.ptr, 0, samples*maxc*sizeof(float));
   /* mix the streams into the mixer buffer */
   for(spl = mixer->streams;*spl;++spl)
      (*spl)->read(*spl, (void**)&mixer->ss.buffer.ptr, samples, ALLEGRO_AUDIO_32_BIT_FLOAT, maxc);
   /* Call the post-processing callback */
   if(mixer->post_process)
      mixer->post_process(mixer->ss.buffer.ptr, mixer->ss.len, mixer->cb_ptr);


   samples *= maxc;

   if(*buf)
   {
      /* We don't need to clamp in the mixer yet */
      float *lbuf = *buf;
      float *src = mixer->ss.buffer.f32;

      while(samples > 0)
      {
         *(lbuf++) += *(src++);
         --samples;
      }
      return;
   }

   /* We're feeding to a voice, so we pass it back the mixed data (make sure
      to clamp and convert it) */
   *buf = mixer->ss.buffer.ptr;
   switch(buffer_depth&~ALLEGRO_AUDIO_UNSIGNED)
   {
      case ALLEGRO_AUDIO_24_BIT_INT:
      {
         int32_t off = ((buffer_depth&ALLEGRO_AUDIO_UNSIGNED)?0x800000:0);
         int32_t *lbuf = mixer->ss.buffer.s24;
         float *src = mixer->ss.buffer.f32;

         while(samples > 0)
         {
            *lbuf  = clamp(*(src++) * ((float)0x7FFFFF+0.5f), ~0x7FFFFF, 0x7FFFFF);
            *lbuf += off;
            ++lbuf;
            --samples;
         }
         break;
      }
      case ALLEGRO_AUDIO_16_BIT_INT:
      {
         int16_t off = ((buffer_depth&ALLEGRO_AUDIO_UNSIGNED)?0x8000:0);
         int16_t *lbuf = mixer->ss.buffer.s16;
         float *src = mixer->ss.buffer.f32;

         while(samples > 0)
         {
            *lbuf  = clamp(*(src++) * ((float)0x7FFF+0.5f), ~0x7FFF, 0x7FFF);
            *lbuf += off;
            ++lbuf;
            --samples;
         }
         break;
      }
      case ALLEGRO_AUDIO_8_BIT_INT:
      default:
      {
         int8_t off = ((buffer_depth&ALLEGRO_AUDIO_UNSIGNED)?0x80:0);
         int8_t *lbuf = mixer->ss.buffer.s8;
         float *src = mixer->ss.buffer.f32;

         while(samples > 0)
         {
            *lbuf  = clamp(*(src++) * ((float)0x7F+0.5f), ~0x7F, 0x7F);
            *lbuf += off;
            ++lbuf;
            --samples;
         }
         break;
      }
   }
   (void)dest_maxc;
}


/* This detaches the sample, stream, or mixer from anything it may be attached
   to */
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE *spl)
{
   if(!spl || !spl->parent.ptr)
      return;

   if(!spl->parent_is_voice)
   {
      ALLEGRO_MIXER *mixer = spl->parent.mixer;
      int i;

      free(spl->matrix);
      spl->matrix = NULL;

      /* Search through the streams and check for this one */
      for(i = 0;mixer->streams[i];++i)
      {
         if(mixer->streams[i] == spl)
         {
            _al_mutex_lock(mixer->ss.mutex);

            do {
               mixer->streams[i] = mixer->streams[i+1];
            } while(mixer->streams[++i]);
            spl->parent.mixer = NULL;
            _al_kcm_stream_set_mutex(spl, NULL);

            if(spl->read != mixer_read)
               spl->read = NULL;

            _al_mutex_unlock(mixer->ss.mutex);

            return;
         }
      }
   }
   else
      al_voice_detach(spl->parent.voice);
}


/* al_sample_create:
 *   Creates a sample stream, using the supplied buffer at the specified size,
 *   channel configuration, sample depth, and frequency. This must be attached
 *   to a voice or mixer before it can be played.
 */
ALLEGRO_SAMPLE *al_sample_create(void *buf, unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf)
{
   ALLEGRO_SAMPLE *spl;

   if(!freq)
   {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid sample frequency");
      return NULL;
   }

   spl = calloc(1, sizeof(*spl));
   if(!spl)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating sample object");
      return NULL;
   }

   spl->loop       = ALLEGRO_AUDIO_PLAY_ONCE;
   spl->depth      = depth;
   spl->chan_conf  = chan_conf;
   spl->frequency  = freq;
   spl->speed      = 1.0f;
   spl->parent.ptr = NULL;
   spl->buffer.ptr = buf;

   spl->step = 0;
   spl->pos = 0;
   spl->len = samples<<MIXER_FRAC_SHIFT;

   spl->loop_start = 0;
   spl->loop_end = spl->len;

   return spl;
}

/* al_sample_destroy:
 *   Detaches the sample stream from anything it may be attached to and frees
 *   its data (note, the sample data is *not* freed!).
 */
/* This function is ALLEGRO_MIXER aware */
void al_sample_destroy(ALLEGRO_SAMPLE *spl)
{
   _al_kcm_detach_from_parent(spl);
   stream_free(spl);
}


void al_sample_play(ALLEGRO_SAMPLE *spl)
{
   al_sample_set_bool(spl, ALLEGRO_AUDIO_PLAYING, 1);
}

void al_sample_stop(ALLEGRO_SAMPLE *spl)
{
   al_sample_set_bool(spl, ALLEGRO_AUDIO_PLAYING, 0);
}

int al_sample_get_long(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, unsigned long *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
         *val = spl->frequency;
         return 0;

      case ALLEGRO_AUDIO_LENGTH:
         *val = spl->len>>MIXER_FRAC_SHIFT;
         return 0;

      case ALLEGRO_AUDIO_POSITION:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            return al_voice_get_long(voice, ALLEGRO_AUDIO_POSITION, val);
         }

         *val = spl->pos >> MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample long setting");
         break;
   }
   return 1;
}

int al_sample_get_float(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, float *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_SPEED:
         *val = spl->speed;
         return 0;
      case ALLEGRO_AUDIO_TIME:
         *val = (float)(spl->len>>MIXER_FRAC_SHIFT) / (float)spl->frequency;
         return 0;
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample float setting");
         break;
   }
   return 1;
}

int al_sample_get_enum(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_DEPTH:
         *val = spl->depth;
         return 0;

      case ALLEGRO_AUDIO_CHANNELS:
         *val = spl->chan_conf;
         return 0;

      case ALLEGRO_AUDIO_LOOPMODE:
         *val = spl->loop;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample enum setting");
         break;
   }
   return 1;
}

int al_sample_get_bool(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, bool *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            return al_voice_get_bool(voice, ALLEGRO_AUDIO_PLAYING, val);
         }

         *val = spl->playing;
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         *val = (spl->parent.ptr != NULL);
         return 0;

      case ALLEGRO_AUDIO_ORPHAN_BUFFER:
         *val = spl->orphan_buffer;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample bool setting");
         break;
   }
   return 1;
}

int al_sample_get_ptr(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, void **val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_BUFFER:
         if(spl->playing || spl->orphan_buffer)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, (spl->orphan_buffer ?
                                    "Attempted to get an orphaned buffer" :
                                    "Attempted to get a playing buffer"));
            return 1;
         }
         *val = spl->buffer.ptr;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample pointer setting");
         break;
   }
   return 1;
}



int al_sample_set_long(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, unsigned long val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_POSITION:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            if(al_voice_set_long(voice, ALLEGRO_AUDIO_POSITION, val) != 0)
               return 1;
         }
         else
         {
            _al_mutex_lock(spl->mutex);
            spl->pos = val << MIXER_FRAC_SHIFT;
            _al_mutex_unlock(spl->mutex);
         }

         return 0;

      case ALLEGRO_AUDIO_LENGTH:
         if(spl->playing)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to change the length of a playing sample");
            return 1;
         }
         spl->len = val << MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample long setting");
         break;
   }
   return 1;
}

int al_sample_set_float(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, float val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_SPEED:
         if(fabs(val) < (1.0f/64.0f))
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set zero speed");
            return 1;
         }

         if(spl->parent.ptr && spl->parent_is_voice)
         {
            _al_set_error(ALLEGRO_GENERIC_ERROR, "Could not set voice playback speed");
            return 1;
         }

         spl->speed = val;
         if(spl->parent.mixer)
         {
            ALLEGRO_MIXER *mixer = spl->parent.mixer;

            _al_mutex_lock(spl->mutex);

            spl->step = (spl->frequency<<MIXER_FRAC_SHIFT) *
                        spl->speed / mixer->ss.frequency;
            /* Don't wanna be trapped with a step value of 0 */
            if(spl->step == 0)
            {
               if(spl->speed > 0.0f)
                  spl->step = 1;
               else
                  spl->step = -1;
            }

            _al_mutex_unlock(spl->mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample float setting");
         break;
   }
   return 1;
}

int al_sample_set_enum(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_LOOPMODE:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to set voice loop mode");
            return 1;
         }

         _al_mutex_lock(spl->mutex);
         spl->loop = val;
         if(spl->loop != ALLEGRO_AUDIO_PLAY_ONCE)
         {
            if(spl->pos < spl->loop_start)
               spl->pos = spl->loop_start;
            else if(spl->pos > spl->loop_end-MIXER_FRAC_ONE)
               spl->pos = spl->loop_end-MIXER_FRAC_ONE;
         }
         _al_mutex_unlock(spl->mutex);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample enum setting");
         break;
   }
   return 1;
}

int al_sample_set_bool(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, bool val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if (!spl->parent.ptr)
         {
            fprintf(stderr, "Sample has no parent\n");
            return 1;
         }

         if(spl->parent_is_voice)
         {
            /* parent is voice */
            ALLEGRO_VOICE *voice = spl->parent.voice;

            /* FIXME: there is no else. what does this do? */
            if(al_voice_set_bool(voice, ALLEGRO_AUDIO_PLAYING, val) == 0)
            {
               unsigned long pos = spl->pos >> MIXER_FRAC_SHIFT;
               if(al_voice_get_long(voice, ALLEGRO_AUDIO_POSITION, &pos) == 0)
               {
                  spl->pos = pos << MIXER_FRAC_SHIFT;
                  spl->playing = val;
                  return 0;
               }
            }
            return 1;
         }
         else
         {
            /* parent is mixer */
            _al_mutex_lock(spl->mutex);
            spl->playing = val;
            if(!val)
               spl->pos = 0;
            _al_mutex_unlock(spl->mutex);
         }
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         if(val)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set sample attachment ststus true");
            return 1;
         }
         _al_kcm_detach_from_parent(spl);
         return 0;

      case ALLEGRO_AUDIO_ORPHAN_BUFFER:
         if(spl->orphan_buffer && !val)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to adopt an orphaned buffer");
            return 1;
         }
         spl->orphan_buffer = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample long setting");
         break;
   }
   return 1;
}

int al_sample_set_ptr(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, void *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_BUFFER:
         if(spl->playing)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to change the buffer of a playing sample");
            return 1;
         }
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            voice->driver->unload_voice(voice);
            if(voice->driver->load_voice(voice, val))
            {
               _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to load sample into voice");
               return 1;
            }
         }
         if(spl->orphan_buffer)
            free(spl->buffer.ptr);
         spl->orphan_buffer = false;
         spl->buffer.ptr = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample long setting");
         break;
   }
   return 1;
}


/* TODO: possibly take extra parameters
 * (freq, channel, etc) and test if you 
 * can create a voice with them.. if not
 * try another driver.
 */
int al_audio_init(ALLEGRO_AUDIO_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (_al_kcm_driver)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A Driver already running"); 
      return 1;
   }

   switch (mode)
   {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         /* check openal first then fallback on others */
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal == 0)
            return 0;
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_ALSA);
         if (retVal == 0)
            return 0;
         _al_kcm_driver = NULL;
         return 1;

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         #if defined(ALLEGRO_CFG_KCM_OPENAL)
            if (_openal_driver.open() == 0) {
               fprintf(stderr, "Using OpenAL driver\n"); 
               _al_kcm_driver = &_openal_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OpenAL not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_CFG_KCM_ALSA)
            if(_alsa_driver.open() == 0) {
               fprintf(stderr, "Using ALSA driver\n"); 
               _al_kcm_driver = &_alsa_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "ALSA not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound driver not yet implemented");
            return 1;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid audio driver");
         return 1;
   }

}

void al_audio_deinit()
{
   if(_al_kcm_driver)
      _al_kcm_driver->close();
   _al_kcm_driver = NULL;
}
