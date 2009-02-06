/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Sample audio interface.
 *
 *      By Peter Wang.
 *
 *      See LICENSE.txt for copyright information.
 */

/* Title: Sample audio interface
 */

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_vector.h"


static ALLEGRO_VOICE *allegro_voice = NULL;
static ALLEGRO_MIXER *allegro_mixer = NULL;
static ALLEGRO_MIXER *default_mixer = NULL;

static _AL_VECTOR auto_samples = _AL_VECTOR_INITIALIZER(ALLEGRO_SAMPLE_INSTANCE *);
static _AL_VECTOR auto_sample_ids = _AL_VECTOR_INITIALIZER(int);

static bool create_default_mixer(void);
static bool do_play_sample(ALLEGRO_SAMPLE_INSTANCE *spl, ALLEGRO_SAMPLE *data, float gain, float pan, float speed, int loop);
static void free_sample_vector(void);

/* Creates the default voice and mixer if they haven't been created yet. */
static bool create_default_mixer(void)
{
   if (!allegro_voice) {
      allegro_voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
         ALLEGRO_CHANNEL_CONF_2);
      if (!allegro_voice) {
         TRACE("al_create_voice failed\n");
         goto Error;
      }
   }

   if (!allegro_mixer) {
      allegro_mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
         ALLEGRO_CHANNEL_CONF_2);
      if (!allegro_mixer) {
         TRACE("al_create_voice failed\n");
         goto Error;
      }
   }

   if (al_attach_mixer_to_voice(allegro_voice, allegro_mixer) != 0) {
      TRACE("al_attach_mixer_to_voice failed\n");
      goto Error;
   }

   return true;

Error:

   if (allegro_mixer) {
      al_destroy_mixer(allegro_mixer);
      allegro_mixer = NULL;
   }

   if (allegro_voice) {
      al_destroy_voice(allegro_voice);
      allegro_voice = NULL;
   }

   return false;
}

/* Function: al_create_sample
 *  Create a sample data structure from the supplied buffer.
 *  If `free_buf' is true then the buffer will be freed as well when the
 *  sample data structure is destroyed.
 */
ALLEGRO_SAMPLE *al_create_sample(void *buf, unsigned long samples,
   unsigned long freq, ALLEGRO_AUDIO_DEPTH depth,
   ALLEGRO_CHANNEL_CONF chan_conf, bool free_buf)
{
   ALLEGRO_SAMPLE *spl;

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

   _al_kcm_register_destructor(spl, (void (*)(void *)) al_destroy_sample);

   return spl;
}


/* Function: al_destroy_sample
 *  Free the sample data structure. If it was created with the `free_buf'
 *  parameter set to true, then the buffer will be freed as well.
 *
 *  You must destroy any ALLEGRO_SAMPLE_INSTANCE structures which reference
 *  this ALLEGRO_SAMPLE beforehand.
 */
void al_destroy_sample(ALLEGRO_SAMPLE *spl)
{
   if (spl) {
      _al_kcm_unregister_destructor(spl);

      if (spl->free_buf && spl->buffer.ptr) {
         free(spl->buffer.ptr);
      }
      spl->buffer.ptr = NULL;
      spl->free_buf = false;
      free(spl);
   }
}

/* Function: al_reserve_samples
 *  Reserves 'reserve_samples' number of samples attached to the default mixer.
 *  <al_install_audio> must have been called first.  If no default mixer is set,
 *  then this function will create a voice with an attached mixer.
 *  Returns true on success, false on error.
 */
bool al_reserve_samples(int reserve_samples)
{
   int i;
   int current_samples_count = (int) _al_vector_size(&auto_samples);

   ASSERT(reserve_samples >= 0);

   /* If no default mixer has been set by the user, then create a voice
    * and a mixer, and set them to be the default one for use with
    * al_play_sample().
    */
   if (default_mixer == NULL) {
      if (!al_restore_default_mixer()) goto Error;
   }

   if (current_samples_count < reserve_samples) {
      /* We need to reserve more samples than currently are reserved. */
      for (i = 0; i < reserve_samples - current_samples_count; i++) {
         ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_alloc_back(&auto_samples);
         int *id = _al_vector_alloc_back(&auto_sample_ids);
         *id = 0;
         *slot = al_create_sample_instance(NULL);
         if (!*slot) {
            TRACE("al_create_sample failed\n");
            goto Error;
         }
         if (al_attach_sample_to_mixer(default_mixer, *slot) != 0) {
            TRACE("al_attach_mixer_to_sample failed\n");
            goto Error;
         }
      }
   }
   else if (current_samples_count == reserve_samples) {
      /* We need to reserve fewer samples than currently are reserved. */
      while (current_samples_count-- > reserve_samples) {
         _al_vector_delete_at(&auto_samples, current_samples_count);
         _al_vector_delete_at(&auto_sample_ids, current_samples_count);
      }
   }

   return true;

 Error:
   free_sample_vector();
   
   return false;
}

/* Function: al_get_default_mixer
 *  Return the default mixer.
 */
ALLEGRO_MIXER *al_get_default_mixer(void)
{
   return default_mixer;
}

/* Function: al_set_default_mixer
 *  Sets the default mixer. All samples started with <al_play_sample>
 *  will be stopped. If you are using your own mixer, this should be
 *  called before <al_reserve_samples>.
 *  Returns true on success, false on error.
 */
bool al_set_default_mixer(ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer != NULL);

   if (mixer != default_mixer) {
      int i;

      default_mixer = mixer;

      /* Destroy all current sample instances, recreate them, and
       * attach them to the new mixer */
      for (i = 0; i < (int) _al_vector_size(&auto_samples); i++) {
         ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&auto_samples, i);
         int *id = _al_vector_ref(&auto_sample_ids, i);

         *id = 0;
         al_destroy_sample_instance(*slot);

         *slot = al_create_sample_instance(NULL);
         if (!*slot) {
            TRACE("al_create_sample failed\n");
            goto Error;
         }
         if (al_attach_sample_to_mixer(default_mixer, *slot) != 0) {
            TRACE("al_attach_mixer_to_sample failed\n");
            goto Error;
         }
      }      
   }

   return true;

Error:
   free_sample_vector();
   default_mixer = NULL;   
   return false;
}

/* Function: al_restore_default_mixer
 *  Restores Allegro's default mixer. All samples started with <al_play_sample>
 *  will be stopped.
 *  Returns true on success, false on error.
 */
bool al_restore_default_mixer(void)
{
   if (!create_default_mixer()) return false;

   if (!al_set_default_mixer(allegro_mixer)) return false;

   return true;
}

/* Function: al_play_sample
 *  Plays a sample over the default mixer. <al_reserve_samples> must have
 *  previously been called. Returns true on success, false on failure.
 *  Playback may fail because all the reserved samples are currently used.
 */
bool al_play_sample(ALLEGRO_SAMPLE *spl, float gain, float pan, float speed, int loop, ALLEGRO_SAMPLE_ID *ret_id)
{
   static int next_id = 0;
   unsigned int i;
   
   ASSERT(spl);

   if (ret_id != NULL) {
      ret_id->_id = -1;
      ret_id->_index = 0;
   }

   for (i = 0; i < _al_vector_size(&auto_samples); i++) {
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&auto_samples, i);
      ALLEGRO_SAMPLE_INSTANCE *splinst = (*slot);
      bool playing;

      if (al_get_sample_instance_bool(splinst, ALLEGRO_AUDIOPROP_PLAYING, &playing) == 0) {
         if (playing == false) {
            int *id = _al_vector_ref(&auto_sample_ids, i);

            if (!do_play_sample(splinst, spl, gain, pan, speed, loop))
               break;

            if (ret_id != NULL) {
               ret_id->_index = (int) i;
               ret_id->_id = *id = ++next_id;
            }

            return true;
         }
      }
   }

   return false;
}

static bool do_play_sample(ALLEGRO_SAMPLE_INSTANCE *splinst, ALLEGRO_SAMPLE *spl, float gain, float pan, float speed, int loop)
{
   (void)pan;

   if (al_set_sample(splinst, spl) != 0) {
      TRACE("al_set_sample failed\n");
      return false;
   }

   if (al_set_sample_instance_float(splinst, ALLEGRO_AUDIOPROP_GAIN, gain) ||
      al_set_sample_instance_float(splinst, ALLEGRO_AUDIOPROP_SPEED, speed) ||
      al_set_sample_instance_enum(splinst, ALLEGRO_AUDIOPROP_LOOPMODE, loop)) {
      return false;
   }

   if (al_play_sample_instance(splinst) != 0) {
      TRACE("al_play_sample_instance failed\n");
      return false;
   }

   return true;
}

/* Function: al_stop_sample
 *  Stop the sample started by <al_start_sample>
 */
void al_stop_sample(ALLEGRO_SAMPLE_ID *spl_id)
{
   int *id;

   ASSERT(spl_id->_id != -1);
   ASSERT(spl_id->_index < (int) _al_vector_size(&auto_samples));
   ASSERT(spl_id->_index < (int) _al_vector_size(&auto_sample_ids));

   id = _al_vector_ref(&auto_sample_ids, spl_id->_index);
   if (*id == spl_id->_id) {
      ALLEGRO_SAMPLE_INSTANCE **slot, *spl;
      slot = _al_vector_ref(&auto_samples, spl_id->_index);
      spl = (*slot);
      al_stop_sample_instance(spl);
   }
}


/* Function: al_stop_samples
 *  Stop all samples started by <al_start_sample>
 */
void al_stop_samples(void)
{
   unsigned int i;

   for (i = 0; i < _al_vector_size(&auto_samples); i++) {
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&auto_samples, i);
      ALLEGRO_SAMPLE_INSTANCE *spl = (*slot);
      al_stop_sample_instance(spl);
   }
}

/* Destroy all sample instances, and frees the associated vectors. */
static void free_sample_vector(void)
{
   int j;

   for (j = 0; j < (int) _al_vector_size(&auto_samples); j++) {
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&auto_samples, j);
      al_destroy_sample_instance(*slot);
   }
   _al_vector_free(&auto_samples);
   _al_vector_free(&auto_sample_ids);
}

void _al_kcm_shutdown_default_mixer(void)
{
   free_sample_vector(); 
   if (allegro_mixer) al_destroy_mixer(allegro_mixer);
   if (allegro_voice) al_destroy_voice(allegro_voice);

   allegro_mixer = NULL;
   allegro_voice = NULL;
   default_mixer = NULL;
}


/* vim: set sts=3 sw=3 et: */
