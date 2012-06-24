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

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_vector.h"

ALLEGRO_DEBUG_CHANNEL("audio")


static ALLEGRO_VOICE *allegro_voice = NULL;
static ALLEGRO_MIXER *allegro_mixer = NULL;
static ALLEGRO_MIXER *default_mixer = NULL;

static _AL_VECTOR auto_samples = _AL_VECTOR_INITIALIZER(ALLEGRO_SAMPLE_INSTANCE *);
static _AL_VECTOR auto_sample_ids = _AL_VECTOR_INITIALIZER(int);


static bool create_default_mixer(void);
static bool do_play_sample(ALLEGRO_SAMPLE_INSTANCE *spl, ALLEGRO_SAMPLE *data,
      float gain, float pan, float speed, ALLEGRO_PLAYMODE loop);
static void free_sample_vector(void);


static int string_to_depth(const char *s)
{
   // FIXME: fill in the rest
   if (!_al_stricmp(s, "int16")) {
      return ALLEGRO_AUDIO_DEPTH_INT16;
   }
   else {
      return ALLEGRO_AUDIO_DEPTH_FLOAT32;
   }
}


/* Creates the default voice and mixer if they haven't been created yet. */
static bool create_default_mixer(void)
{
   int voice_frequency = 44100;
   int voice_depth = ALLEGRO_AUDIO_DEPTH_INT16;
   int mixer_frequency = 44100;
   int mixer_depth = ALLEGRO_AUDIO_DEPTH_FLOAT32;

   ALLEGRO_CONFIG *config = al_get_system_config();
   if (config) {
      const char *p;
      p = al_get_config_value(config, "audio", "primary_voice_frequency");
      if (p && p[0] != '\0') {
         voice_frequency = atoi(p);
      }
      p = al_get_config_value(config, "audio", "primary_mixer_frequency");
      if (p && p[0] != '\0') {
         mixer_frequency = atoi(p);
      }
      p = al_get_config_value(config, "audio", "primary_voice_depth");
      if (p && p[0] != '\0') {
         voice_depth = string_to_depth(p);
      }
      p = al_get_config_value(config, "audio", "primary_mixer_depth");
      if (p && p[0] != '\0') {
         mixer_depth = string_to_depth(p);
      }
   }

   if (!allegro_voice) {
      allegro_voice = al_create_voice(voice_frequency, voice_depth,
         ALLEGRO_CHANNEL_CONF_2);
      if (!allegro_voice) {
         ALLEGRO_ERROR("al_create_voice failed\n");
         goto Error;
      }
   }

   if (!allegro_mixer) {
      allegro_mixer = al_create_mixer(mixer_frequency, mixer_depth,
         ALLEGRO_CHANNEL_CONF_2);
      if (!allegro_mixer) {
         ALLEGRO_ERROR("al_create_voice failed\n");
         goto Error;
      }
   }

   if (!al_attach_mixer_to_voice(allegro_mixer, allegro_voice)) {
      ALLEGRO_ERROR("al_attach_mixer_to_voice failed\n");
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
 */
ALLEGRO_SAMPLE *al_create_sample(void *buf, unsigned int samples,
   unsigned int freq, ALLEGRO_AUDIO_DEPTH depth,
   ALLEGRO_CHANNEL_CONF chan_conf, bool free_buf)
{
   ALLEGRO_SAMPLE *spl;

   ASSERT(buf);

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid sample frequency");
      return NULL;
   }

   spl = al_calloc(1, sizeof(*spl));
   if (!spl) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating sample data object");
      return NULL;
   }

   spl->depth = depth;
   spl->chan_conf = chan_conf;
   spl->frequency = freq;
   spl->len = samples;
   spl->buffer.ptr = buf;
   spl->free_buf = free_buf;

   _al_kcm_register_destructor(spl, (void (*)(void *)) al_destroy_sample);

   return spl;
}


/* Stop any sample instances which are still playing a sample buffer which
 * is about to be destroyed.
 */
static void stop_sample_instances_helper(void *object, void (*func)(void *),
   void *userdata)
{
   ALLEGRO_SAMPLE_INSTANCE *splinst = object;

   /* This is ugly. */
   if (func == (void (*)(void *)) al_destroy_sample_instance
      && al_get_sample_data(al_get_sample(splinst)) == userdata
      && al_get_sample_instance_playing(splinst))
   {
      al_stop_sample_instance(splinst);
   }
}


/* Function: al_destroy_sample
 */
void al_destroy_sample(ALLEGRO_SAMPLE *spl)
{
   if (spl) {
      _al_kcm_foreach_destructor(stop_sample_instances_helper,
         al_get_sample_data(spl));
      _al_kcm_unregister_destructor(spl);

      if (spl->free_buf && spl->buffer.ptr) {
         al_free(spl->buffer.ptr);
      }
      spl->buffer.ptr = NULL;
      spl->free_buf = false;
      al_free(spl);
   }
}


/* Function: al_reserve_samples
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
      if (!al_restore_default_mixer())
         goto Error;
   }

   if (current_samples_count < reserve_samples) {
      /* We need to reserve more samples than currently are reserved. */
      for (i = 0; i < reserve_samples - current_samples_count; i++) {
         ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_alloc_back(&auto_samples);
         int *id = _al_vector_alloc_back(&auto_sample_ids);
         *id = 0;
         *slot = al_create_sample_instance(NULL);
         if (!*slot) {
            ALLEGRO_ERROR("al_create_sample failed\n");
            goto Error;
         }
         if (!al_attach_sample_instance_to_mixer(*slot, default_mixer)) {
            ALLEGRO_ERROR("al_attach_mixer_to_sample failed\n");
            goto Error;
         }
      }
   }
   else if (current_samples_count > reserve_samples) {
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
 */
ALLEGRO_MIXER *al_get_default_mixer(void)
{
   return default_mixer;
}


/* Function: al_set_default_mixer
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
            ALLEGRO_ERROR("al_create_sample failed\n");
            goto Error;
         }
         if (!al_attach_sample_instance_to_mixer(*slot, default_mixer)) {
            ALLEGRO_ERROR("al_attach_mixer_to_sample failed\n");
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
 */
bool al_restore_default_mixer(void)
{
   if (!create_default_mixer())
      return false;

   if (!al_set_default_mixer(allegro_mixer))
      return false;

   return true;
}


/* Function: al_play_sample
 */
bool al_play_sample(ALLEGRO_SAMPLE *spl, float gain, float pan, float speed,
   ALLEGRO_PLAYMODE loop, ALLEGRO_SAMPLE_ID *ret_id)
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

      if (!al_get_sample_instance_playing(splinst)) {
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

   return false;
}


static bool do_play_sample(ALLEGRO_SAMPLE_INSTANCE *splinst,
   ALLEGRO_SAMPLE *spl, float gain, float pan, float speed, ALLEGRO_PLAYMODE loop)
{
   if (!al_set_sample(splinst, spl)) {
      ALLEGRO_ERROR("al_set_sample failed\n");
      return false;
   }

   if (!al_set_sample_instance_gain(splinst, gain) ||
         !al_set_sample_instance_pan(splinst, pan) ||
         !al_set_sample_instance_speed(splinst, speed) ||
         !al_set_sample_instance_playmode(splinst, loop)) {
      return false;
   }

   if (!al_play_sample_instance(splinst)) {
      ALLEGRO_ERROR("al_play_sample_instance failed\n");
      return false;
   }

   return true;
}


/* Function: al_stop_sample
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


/* Function: al_get_sample_frequency
 */
unsigned int al_get_sample_frequency(const ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->frequency;
}


/* Function: al_get_sample_length
 */
unsigned int al_get_sample_length(const ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->len;
}


/* Function: al_get_sample_depth
 */
ALLEGRO_AUDIO_DEPTH al_get_sample_depth(const ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->depth;
}


/* Function: al_get_sample_channels
 */
ALLEGRO_CHANNEL_CONF al_get_sample_channels(const ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->chan_conf;
}


/* Function: al_get_sample_data
 */
void *al_get_sample_data(const ALLEGRO_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->buffer.ptr;
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
   al_destroy_mixer(allegro_mixer);
   al_destroy_voice(allegro_voice);

   allegro_mixer = NULL;
   allegro_voice = NULL;
   default_mixer = NULL;
}


/* vim: set sts=3 sw=3 et: */
