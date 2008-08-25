/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Sample functions
 */

#include <math.h>
#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"


/* _al_kcm_stream_set_mutex:
 *  This function sets a sample's mutex pointer to the specified value. It is
 *  ALLEGRO_MIXER aware, and will recursively set any attached streams' mutex
 *  to the same value.
 */
void _al_kcm_stream_set_mutex(ALLEGRO_SAMPLE *stream, ALLEGRO_MUTEX *mutex)
{
   ASSERT(stream);

   if (stream->mutex == mutex)
      return;
   stream->mutex = mutex;

   /* If this is a mixer, we need to make sure all the attached streams also
    * set the same mutex.
    */
   if (stream->spl_read == _al_kcm_mixer_read) {
      ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER *)stream;
      int i;

      for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
         ALLEGRO_SAMPLE **slot = _al_vector_ref(&mixer->streams, i);
         ALLEGRO_SAMPLE *spl = *slot;
         _al_kcm_stream_set_mutex(spl, mutex);
      }
   }
}


/* stream_free:
 *  This function is ALLEGRO_MIXER aware and frees the memory associated with
 *  the sample or mixer, and detaches any attached streams or mixers.
 */
static void stream_free(ALLEGRO_SAMPLE *spl)
{
   if (spl) {
      /* Make sure we free the mixer buffer and de-reference the attached
       * streams if this is a mixer stream.
       */
      if (spl->spl_read == _al_kcm_mixer_read) {
         ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER *)spl;
         int i;

         _al_kcm_stream_set_mutex(&mixer->ss, NULL);

         for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
            ALLEGRO_SAMPLE **slot = _al_vector_ref(&mixer->streams, i);
            ALLEGRO_SAMPLE *spl = *slot;
            spl->parent.u.ptr = NULL;
         }

         _al_vector_free(&mixer->streams);
      }

      if (spl->orphan_buffer) {
         free(spl->buffer.ptr);
      }

      free(spl);
   }
}


/* _al_kcm_detach_from_parent:
 *  This detaches the sample, stream, or mixer from anything it may be attached
 *  to.
 */
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE *spl)
{
   ALLEGRO_MIXER *mixer;
   int i;

   if (!spl || !spl->parent.u.ptr)
      return;

   if (spl->parent.is_voice) {
      al_voice_detach(spl->parent.u.voice);
      return;
   }
   
   mixer = spl->parent.u.mixer;

   free(spl->matrix);
   spl->matrix = NULL;

   /* Search through the streams and check for this one */
   for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
      ALLEGRO_SAMPLE **slot = _al_vector_ref(&mixer->streams, i);

      if (*slot == spl) {
         al_lock_mutex(mixer->ss.mutex);

         _al_vector_delete_at(&mixer->streams, i);
         spl->parent.u.mixer = NULL;
         _al_kcm_stream_set_mutex(spl, NULL);

         if (spl->spl_read != _al_kcm_mixer_read)
            spl->spl_read = NULL;

         al_unlock_mutex(mixer->ss.mutex);

         break;
      }
   }
}


/* Function: al_sample_create
 *  Creates a sample stream, using the supplied buffer at the specified size,
 *  channel configuration, sample depth, and frequency. This must be attached
 *  to a voice or mixer before it can be played. Set free_buf to true if you
 *  want the supplied buffer to be freed using free() in al_sample_destroy().
 */
ALLEGRO_SAMPLE *al_sample_create(void *buf, unsigned long samples,
   unsigned long freq, ALLEGRO_AUDIO_DEPTH depth,
   ALLEGRO_CHANNEL_CONF chan_conf, bool free_buf)
{
   ALLEGRO_SAMPLE *spl;

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid sample frequency");
      return NULL;
   }

   spl = calloc(1, sizeof(*spl));
   if (!spl) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating sample object");
      return NULL;
   }

   spl->loop       = ALLEGRO_PLAYMODE_ONCE;
   spl->depth      = depth;
   spl->chan_conf  = chan_conf;
   spl->frequency  = freq;
   spl->speed      = 1.0f;
   spl->parent.u.ptr = NULL;
   spl->buffer.ptr = buf;

   spl->step = 0;
   spl->pos = 0;
   spl->len = samples << MIXER_FRAC_SHIFT;

   spl->loop_start = 0;
   spl->loop_end = spl->len;

   spl->free_buf = free_buf;

   return spl;
}


/* Function: al_sample_create_clone
 *  Creates a new sample from an existing sample. The new sample will share
 *  the buffer with the original, and inherit its properties.
 *  This is equivalent to calling <al_sample_create> with 'free_buf'
 *  set to true.
 */
ALLEGRO_SAMPLE *al_sample_create_clone(const ALLEGRO_SAMPLE *spl)
{
   unsigned long length;

   length = spl->len >> MIXER_FRAC_SHIFT;
   return al_sample_create(spl->buffer.ptr, length, spl->frequency,
      spl->depth, spl->chan_conf, false);
}


/* Function: al_sample_destroy
 *  Detaches the sample stream from anything it may be attached to and frees
 *  its data (note, the sample data is *not* freed!).
 */
/* This function is ALLEGRO_MIXER aware */
void al_sample_destroy(ALLEGRO_SAMPLE *spl)
{
   if (spl) {
      if (spl->free_buf)
         free(spl->buffer.ptr);

      _al_kcm_detach_from_parent(spl);
      stream_free(spl);
   }
}


/* Function: al_sample_play
 */
int al_sample_play(ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return al_sample_set_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, true);
}


/* Function: al_sample_stop
 */
int al_sample_stop(ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return al_sample_set_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, false);
}


/* Function: al_sample_get_long
 */
int al_sample_get_long(const ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = spl->frequency;
         return 0;

      case ALLEGRO_AUDIOPROP_LENGTH:
         *val = spl->len>>MIXER_FRAC_SHIFT;
         return 0;

      case ALLEGRO_AUDIOPROP_POSITION:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            return al_voice_get_long(voice, ALLEGRO_AUDIOPROP_POSITION, val);
         }

         *val = spl->pos >> MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample long setting");
         return 1;
   }
}


/* Function: al_sample_get_float
 */
int al_sample_get_float(const ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, float *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_SPEED:
         *val = spl->speed;
         return 0;

      case ALLEGRO_AUDIOPROP_TIME:
         *val = (float)(spl->len>>MIXER_FRAC_SHIFT) / (float)spl->frequency;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample float setting");
         return 1;
   }
}


/* Function: al_sample_get_enum
 */
int al_sample_get_enum(const ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, int *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_DEPTH:
         *val = spl->depth;
         return 0;

      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = spl->chan_conf;
         return 0;

      case ALLEGRO_AUDIOPROP_LOOPMODE:
         *val = spl->loop;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample enum setting");
         return 1;
   }
}


/* Function: al_sample_get_bool
 */
int al_sample_get_bool(const ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, bool *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            return al_voice_get_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, val);
         }

         *val = spl->is_playing;
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         *val = (spl->parent.u.ptr != NULL);
         return 0;

      case ALLEGRO_AUDIOPROP_ORPHAN_BUFFER:
         *val = spl->orphan_buffer;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample bool setting");
         return 1;
   }
}


/* Function: al_sample_get_ptr
 */
int al_sample_get_ptr(const ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, void **val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_BUFFER:
         if (spl->is_playing) {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to get a playing buffer");
            return 1;
         }
         if (spl->orphan_buffer) {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to get an orphaned buffer");
            return 1;
         }
         *val = spl->buffer.ptr;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample pointer setting");
         return 1;
   }
}


/* Function: al_sample_set_long
 */
int al_sample_set_long(ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_POSITION:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            if (al_voice_set_long(voice, ALLEGRO_AUDIOPROP_POSITION, val) != 0)
               return 1;
         }
         else {
            al_lock_mutex(spl->mutex);
            spl->pos = val << MIXER_FRAC_SHIFT;
            al_unlock_mutex(spl->mutex);
         }

         return 0;

      case ALLEGRO_AUDIOPROP_LENGTH:
         if (spl->is_playing) {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to change the length of a playing sample");
            return 1;
         }
         spl->len = val << MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* Function: al_sample_set_float
 */
int al_sample_set_float(ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, float val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_SPEED:
         if (fabs(val) < (1.0f/64.0f)) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to set zero speed");
            return 1;
         }

         if (spl->parent.u.ptr && spl->parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Could not set voice playback speed");
            return 1;
         }

         spl->speed = val;
         if (spl->parent.u.mixer) {
            ALLEGRO_MIXER *mixer = spl->parent.u.mixer;

            al_lock_mutex(spl->mutex);

            spl->step = (spl->frequency<<MIXER_FRAC_SHIFT) *
                        spl->speed / mixer->ss.frequency;
            /* Don't wanna be trapped with a step value of 0 */
            if (spl->step == 0) {
               if (spl->speed > 0.0f)
                  spl->step = 1;
               else
                  spl->step = -1;
            }

            al_unlock_mutex(spl->mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample float setting");
         return 1;
   }
}


/* Function: al_sample_set_enum
 */
int al_sample_set_enum(ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, int val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_LOOPMODE:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Unable to set voice loop mode");
            return 1;
         }

         al_lock_mutex(spl->mutex);
         spl->loop = val;
         if (spl->loop != ALLEGRO_PLAYMODE_ONCE) {
            if (spl->pos < spl->loop_start)
               spl->pos = spl->loop_start;
            else if (spl->pos > spl->loop_end-MIXER_FRAC_ONE)
               spl->pos = spl->loop_end-MIXER_FRAC_ONE;
         }
         al_unlock_mutex(spl->mutex);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample enum setting");
         return 1;
   }
}


/* Function: al_sample_set_bool
 */
int al_sample_set_bool(ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, bool val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (!spl->parent.u.ptr) {
            fprintf(stderr, "Sample has no parent\n");
            return 1;
         }

         if (spl->parent.is_voice) {
            /* parent is voice */
            ALLEGRO_VOICE *voice = spl->parent.u.voice;

            /* FIXME: there is no else. what does this do? */
            if (al_voice_set_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, val) == 0) {
               unsigned long pos = spl->pos >> MIXER_FRAC_SHIFT;
               if (al_voice_get_long(voice, ALLEGRO_AUDIOPROP_POSITION, &pos) == 0)
               {
                  spl->pos = pos << MIXER_FRAC_SHIFT;
                  spl->is_playing = val;
                  return 0;
               }
            }
            return 1;
         }
         else {
            /* parent is mixer */
            al_lock_mutex(spl->mutex);
            spl->is_playing = val;
            if (!val)
               spl->pos = 0;
            al_unlock_mutex(spl->mutex);
         }
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         if (val) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to set sample attachment ststus true");
            return 1;
         }
         _al_kcm_detach_from_parent(spl);
         return 0;

      case ALLEGRO_AUDIOPROP_ORPHAN_BUFFER:
         if (spl->orphan_buffer && !val) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to adopt an orphaned buffer");
            return 1;
         }
         spl->orphan_buffer = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* Function: al_sample_set_ptr
 */
int al_sample_set_ptr(ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, void *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_BUFFER:
         if (spl->is_playing) {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to change the buffer of a playing sample");
            return 1;
         }
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            voice->driver->unload_voice(voice);
            if (voice->driver->load_voice(voice, val)) {
               _al_set_error(ALLEGRO_GENERIC_ERROR,
                  "Unable to load sample into voice");
               return 1;
            }
         }
         if (spl->orphan_buffer)
            free(spl->buffer.ptr);
         spl->orphan_buffer = false;
         spl->buffer.ptr = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* vim: set sts=3 sw=3 et: */
