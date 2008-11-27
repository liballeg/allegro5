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
void _al_kcm_detach_from_parent(ALLEGRO_SAMPLE *spl)
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


/* Function: al_create_sample_data
 *  Create a sample data structure from the supplied buffer.
 *  If `free_buf' is true then the buffer will be freed as well when the
 *  sample data structure is destroyed.
 */
ALLEGRO_SAMPLE_DATA *al_create_sample_data(void *buf, unsigned long samples,
   unsigned long freq, ALLEGRO_AUDIO_DEPTH depth,
   ALLEGRO_CHANNEL_CONF chan_conf, bool free_buf)
{
   ALLEGRO_SAMPLE_DATA *spl;

   ASSERT(buf);

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid sample frequency");
      return NULL;
   }

   spl = calloc(1, sizeof(*spl));
   if (!spl) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating sample data object");
      return NULL;
   }

   spl->depth = depth;
   spl->chan_conf = chan_conf;
   spl->frequency = freq;
   spl->len = samples << MIXER_FRAC_SHIFT;
   spl->buffer.ptr = buf;
   spl->free_buf = free_buf;

   return spl;
}


/* Function: al_destroy_sample_data
 *  Free the sample data structure. If it was created with the `free_buf'
 *  parameter set to true, then the buffer will be freed as well.
 *
 *  You must destroy any ALLEGRO_SAMPLE structures which reference
 *  this ALLEGRO_SAMPLE_DATA beforehand.
 */
void al_destroy_sample_data(ALLEGRO_SAMPLE_DATA *spl)
{
   if (spl) {
      if (spl->free_buf && spl->buffer.ptr) {
         free(spl->buffer.ptr);
      }
      spl->buffer.ptr = NULL;
      spl->free_buf = false;
      free(spl);
   }
}


/* Function: al_create_sample
 *  Creates a sample stream, using the supplied data.  This must be attached
 *  to a voice or mixer before it can be played.
 *  The argument may be NULL. You can then set the data later with
 *  <al_set_sample_data>.
 */
ALLEGRO_SAMPLE *al_create_sample(ALLEGRO_SAMPLE_DATA *sample_data)
{
   ALLEGRO_SAMPLE *spl;

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
   spl->pos = 0;
   spl->loop_start = 0;
   spl->loop_end = sample_data ? sample_data->len : 0;
   spl->step = 0;

   spl->matrix = NULL;

   spl->spl_read = NULL;

   spl->mutex = NULL;
   spl->parent.u.ptr = NULL;

   return spl;
}


/* Function: al_destroy_sample
 *  Detaches the sample stream from anything it may be attached to and frees
 *  it (the sample data is *not* freed!).
 */
/* This function is ALLEGRO_MIXER aware */
void al_destroy_sample(ALLEGRO_SAMPLE *spl)
{
   if (spl) {
      _al_kcm_detach_from_parent(spl);
      stream_free(spl);
   }
}


/* Function: al_play_sample
 */
int al_play_sample(ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return al_set_sample_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, true);
}


/* Function: al_stop_sample
 */
int al_stop_sample(ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return al_set_sample_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, false);
}


/* Function: al_get_sample_long
 */
int al_get_sample_long(const ALLEGRO_SAMPLE *spl,
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


/* Function: al_get_sample_float
 */
int al_get_sample_float(const ALLEGRO_SAMPLE *spl,
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


/* Function: al_get_sample_enum
 */
int al_get_sample_enum(const ALLEGRO_SAMPLE *spl,
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


/* Function: al_get_sample_bool
 */
int al_get_sample_bool(const ALLEGRO_SAMPLE *spl,
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


/* Function: al_get_sample_ptr
 */
int al_get_sample_ptr(const ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, void **val)
{
   ASSERT(spl);

   switch (setting) {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid sample pointer setting");
         return 1;
   }
}


/* Function: al_set_sample_long
 */
int al_set_sample_long(ALLEGRO_SAMPLE *spl,
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


/* Function: al_set_sample_float
 */
int al_set_sample_float(ALLEGRO_SAMPLE *spl,
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

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample float setting");
         return 1;
   }
}


/* Function: al_set_sample_enum
 */
int al_set_sample_enum(ALLEGRO_SAMPLE *spl,
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


/* Function: al_set_sample_bool
 */
int al_set_sample_bool(ALLEGRO_SAMPLE *spl,
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


/* Function: al_set_sample_ptr
 */
int al_set_sample_ptr(ALLEGRO_SAMPLE *spl,
   ALLEGRO_AUDIO_PROPERTY setting, void *val)
{
   ASSERT(spl);

   switch (setting) {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid sample long setting");
         return 1;
   }
}


/* Function: al_set_sample_data
 *  Change the sample data that a sample plays.  This can be quite an involved
 *  process.
 *
 *  First, the sample is stopped if it is not already.
 *
 *  Next, if data is NULL, the sample is detached from its parent (if any).
 *
 *  If data is not NULL, the sample may be detached and reattached to its
 *  parent (if any).  This is not necessary if the old sample data and new
 *  sample data have the same frequency, depth and channel configuration.
 *  Reattaching may not always succeed.
 *
 *  On success, the sample remains stopped.  The playback position and loop
 *  end points are reset to their default values.  The loop mode remains
 *  unchanged.
 *
 *  Returns zero on success, non-zero on failure.  On failure, the sample will
 *  be stopped and detached from its parent.
 */
int al_set_sample_data(ALLEGRO_SAMPLE *spl, ALLEGRO_SAMPLE_DATA *data)
{
   sample_parent_t old_parent;
   bool need_reattach;

   ASSERT(spl);

   /* Stop the sample if it is playing. */
   if (spl->is_playing) {
      if (al_set_sample_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, false) != 0) {
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


/* vim: set sts=3 sw=3 et: */
