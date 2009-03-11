/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Sample Instance functions
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
void _al_kcm_stream_set_mutex(ALLEGRO_SAMPLE_INSTANCE *stream, ALLEGRO_MUTEX *mutex)
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
         ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);
         ALLEGRO_SAMPLE_INSTANCE *spl = *slot;
         _al_kcm_stream_set_mutex(spl, mutex);
      }
   }
}


/* stream_free:
 *  This function is ALLEGRO_MIXER aware and frees the memory associated with
 *  the sample or mixer, and detaches any attached streams or mixers.
 */
static void stream_free(ALLEGRO_SAMPLE_INSTANCE *spl)
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
            ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);
            ALLEGRO_SAMPLE_INSTANCE *spl = *slot;
            spl->parent.u.ptr = NULL;
         }

         _al_vector_free(&mixer->streams);

         if (spl->spl_data.buffer.ptr) {
            ASSERT(spl->spl_data.free_buf);
            free(spl->spl_data.buffer.ptr);
            spl->spl_data.buffer.ptr = NULL;
         }
         spl->spl_data.free_buf = false;
      }

      ASSERT(! spl->spl_data.free_buf);

      free(spl);
   }
}


/* _al_kcm_detach_from_parent:
 *  This detaches the sample, stream, or mixer from anything it may be attached
 *  to.
 */
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE_INSTANCE *spl)
{
   ALLEGRO_MIXER *mixer;
   int i;

   if (!spl || !spl->parent.u.ptr)
      return;

   if (spl->parent.is_voice) {
      al_detach_voice(spl->parent.u.voice);
      return;
   }
   
   mixer = spl->parent.u.mixer;

   free(spl->matrix);
   spl->matrix = NULL;

   /* Search through the streams and check for this one */
   for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);

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


/* Function: al_create_sample_instance
 */
ALLEGRO_SAMPLE_INSTANCE *al_create_sample_instance(ALLEGRO_SAMPLE *sample_data)
{
   ALLEGRO_SAMPLE_INSTANCE *spl;

   spl = calloc(1, sizeof(*spl));
   if (!spl) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating sample object");
      return NULL;
   }

   if (sample_data) {
      spl->spl_data = *sample_data;
   }
   spl->spl_data.free_buf = false;

   spl->loop = ALLEGRO_PLAYMODE_ONCE;
   spl->speed = 1.0f;
   spl->gain = 1.0f;
   spl->pan = 0.0f;
   spl->pos = 0;
   spl->loop_start = 0;
   spl->loop_end = sample_data ? sample_data->len : 0;
   spl->step = 0;

   spl->matrix = NULL;

   spl->spl_read = NULL;

   spl->mutex = NULL;
   spl->parent.u.ptr = NULL;

   _al_kcm_register_destructor(spl, (void (*)(void *)) al_destroy_sample);

   return spl;
}


/* This function is ALLEGRO_MIXER aware */
/* Function: al_destroy_sample_instance
 */
void al_destroy_sample_instance(ALLEGRO_SAMPLE_INSTANCE *spl)
{
   _al_kcm_destroy_sample(spl, true);
}


/* Internal function: _al_kcm_destroy_sample
 */
void _al_kcm_destroy_sample(ALLEGRO_SAMPLE_INSTANCE *spl, bool unregister)
{
   if (spl) {
      if (unregister) {
         _al_kcm_unregister_destructor(spl);
      }

      _al_kcm_detach_from_parent(spl);
      stream_free(spl);
   }
}


/* Function: al_play_sample_instance
 */
int al_play_sample_instance(ALLEGRO_SAMPLE_INSTANCE *spl)
{
   ASSERT(spl);

   return al_set_sample_instance_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, true);
}


/* Function: al_stop_sample_instance
 */
int al_stop_sample_instance(ALLEGRO_SAMPLE_INSTANCE *spl)
{
   ASSERT(spl);

   return al_set_sample_instance_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, false);
}


/* Function: al_get_sample_instance_long
 */
int al_get_sample_instance_long(const ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = spl->spl_data.frequency;
         return 0;

      case ALLEGRO_AUDIOPROP_LENGTH:
         *val = spl->spl_data.len >> MIXER_FRAC_SHIFT;
         return 0;

      case ALLEGRO_AUDIOPROP_POSITION:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            return al_get_voice_long(voice, ALLEGRO_AUDIOPROP_POSITION, val);
         }

         *val = spl->pos >> MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample long setting");
         return 1;
   }
}


/* Function: al_get_sample_instance_float
 */
int al_get_sample_instance_float(const ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, float *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_SPEED:
         *val = spl->speed;
         return 0;

      case ALLEGRO_AUDIOPROP_GAIN:
         *val = spl->gain;
         return 0;

      case ALLEGRO_AUDIOPROP_PAN:
         *val = spl->pan;
         return 0;

      case ALLEGRO_AUDIOPROP_TIME:
         *val = (float)(spl->spl_data.len >> MIXER_FRAC_SHIFT)
                  / (float)spl->spl_data.frequency;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample float setting");
         return 1;
   }
}


/* Function: al_get_sample_instance_enum
 */
int al_get_sample_instance_enum(const ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, int *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_DEPTH:
         *val = spl->spl_data.depth;
         return 0;

      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = spl->spl_data.chan_conf;
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


/* Function: al_get_sample_instance_bool
 */
int al_get_sample_instance_bool(const ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, bool *val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            return al_get_voice_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, val);
         }

         *val = spl->is_playing;
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         *val = (spl->parent.u.ptr != NULL);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample bool setting");
         return 1;
   }
}


/* Function: al_get_sample_instance_ptr
 */
int al_get_sample_instance_ptr(const ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, void **val)
{
   ASSERT(spl);

   (void)spl;
   (void)val;

   switch (setting) {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample pointer setting");
         return 1;
   }
}


/* Function: al_set_sample_instance_long
 */
int al_set_sample_instance_long(ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_POSITION:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            ALLEGRO_VOICE *voice = spl->parent.u.voice;
            if (al_set_voice_long(voice, ALLEGRO_AUDIOPROP_POSITION, val) != 0)
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
         spl->spl_data.len = val << MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* Function: al_set_sample_instance_float
 */
int al_set_sample_instance_float(ALLEGRO_SAMPLE_INSTANCE *spl,
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

            spl->step = (spl->spl_data.frequency << MIXER_FRAC_SHIFT) *
                        spl->speed / mixer->ss.spl_data.frequency;
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

      case ALLEGRO_AUDIOPROP_GAIN:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Could not set gain of sample attached to voice");
            return 1;
         }

         if (spl->gain == val) {
            return 0;
         }
         spl->gain = val;

         /* If attached to a mixer already, need to recompute the sample
          * matrix to take into account the gain.
          */
         if (spl->parent.u.mixer) {
            ALLEGRO_MIXER *mixer = spl->parent.u.mixer;

            al_lock_mutex(spl->mutex);
            _al_kcm_mixer_rejig_sample_matrix(mixer, spl);
            al_unlock_mutex(spl->mutex);
         }

         return 0;

      case ALLEGRO_AUDIOPROP_PAN:
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Could not set panning of sample attached to voice");
            return 1;
         }
         if (val != ALLEGRO_AUDIO_PAN_NONE && (val < -1.0 || val > 1.0)) {
            _al_set_error(ALLEGRO_GENERIC_ERROR, "Invalid pan value");
            return 1;
         }

         if (spl->pan == val) {
            return 0;
         }
         spl->pan = val;

         /* If attached to a mixer already, need to recompute the sample
          * matrix to take into account the panning.
          */
         if (spl->parent.u.mixer) {
            ALLEGRO_MIXER *mixer = spl->parent.u.mixer;

            al_lock_mutex(spl->mutex);
            _al_kcm_mixer_rejig_sample_matrix(mixer, spl);
            al_unlock_mutex(spl->mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample float setting");
         return 1;
   }
}


/* Function: al_set_sample_instance_enum
 */
int al_set_sample_instance_enum(ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, int val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_LOOPMODE:
         if (val < ALLEGRO_PLAYMODE_ONCE || val > ALLEGRO_PLAYMODE_BIDIR) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Invalid loop mode");
            return 1;
         }
         if (spl->parent.u.ptr && spl->parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Unable to set voice loop mode");
            return 1;
         }

         if (spl->parent.u.ptr)
            al_lock_mutex(spl->mutex);

         spl->loop = val;
         if (spl->loop != ALLEGRO_PLAYMODE_ONCE) {
            if (spl->pos < spl->loop_start)
               spl->pos = spl->loop_start;
            else if (spl->pos > spl->loop_end-MIXER_FRAC_ONE)
               spl->pos = spl->loop_end-MIXER_FRAC_ONE;
         }

         if (spl->parent.u.ptr)
            al_unlock_mutex(spl->mutex);

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample enum setting");
         return 1;
   }
}


/* Function: al_set_sample_instance_bool
 */
int al_set_sample_instance_bool(ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, bool val)
{
   ASSERT(spl);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (!spl->parent.u.ptr) {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Sample has no parent");
            return 1;
         }
         if (!spl->spl_data.buffer.ptr) {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Sample has no data");
            return 1;
         }

         if (spl->parent.is_voice) {
            /* parent is voice */
            ALLEGRO_VOICE *voice = spl->parent.u.voice;

            /* FIXME: there is no else. what does this do? */
            if (al_set_voice_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, val) == 0) {
               unsigned long pos = spl->pos >> MIXER_FRAC_SHIFT;
               if (al_get_voice_long(voice, ALLEGRO_AUDIOPROP_POSITION, &pos) == 0)
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
               "Attempted to set sample attachment status true");
            return 1;
         }
         _al_kcm_detach_from_parent(spl);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* Function: al_set_sample_instance_ptr
 */
int al_set_sample_instance_ptr(ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, void *val)
{
   ASSERT(spl);

   (void)spl;
   (void)val;

   switch (setting) {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* Function: al_set_sample
 */
int al_set_sample(ALLEGRO_SAMPLE_INSTANCE *spl, ALLEGRO_SAMPLE *data)
{
   sample_parent_t old_parent;
   bool need_reattach;

   ASSERT(spl);

   /* Stop the sample if it is playing. */
   if (spl->is_playing) {
      if (al_set_sample_instance_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, false) != 0) {
         /* Shouldn't happen. */
         ASSERT(false);
         return 1;
      }
   }

   if (!data) {
      if (spl->parent.u.ptr) {
         _al_kcm_detach_from_parent(spl);
      }
      spl->spl_data.buffer.ptr = NULL;
      return 0;
   }

   /* Have data. */

   need_reattach = false;
   if (spl->parent.u.ptr != NULL) {
      if (spl->spl_data.frequency != data->frequency ||
            spl->spl_data.depth != data->depth ||
            spl->spl_data.chan_conf != data->chan_conf) {
         old_parent = spl->parent;
         need_reattach = true;
         _al_kcm_detach_from_parent(spl);
      }
   }

   spl->spl_data = *data;
   spl->spl_data.free_buf = false;
   spl->pos = 0;
   spl->loop_start = 0;
   spl->loop_end = data->len;
   /* Should we reset the loop mode? */

   if (need_reattach) {
      if (old_parent.is_voice) {
         if (al_attach_sample_to_voice(old_parent.u.voice, spl) != 0) {
            spl->spl_data.buffer.ptr = NULL;
            return 1;
         }
      }
      else {
         if (al_attach_sample_to_mixer(old_parent.u.mixer, spl) != 0) {
            spl->spl_data.buffer.ptr = NULL;
            return 1;
         }
      }
   }

   return 0;
}


/* Function: al_get_sample
 *  XXX
 */
ALLEGRO_SAMPLE *al_get_sample(ALLEGRO_SAMPLE_INSTANCE *spl)
{
   return &(spl->spl_data);
}


/* vim: set sts=3 sw=3 et: */
