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

A5O_DEBUG_CHANNEL("audio")


static A5O_VOICE *allegro_voice = NULL;
static A5O_MIXER *allegro_mixer = NULL;
static A5O_MIXER *default_mixer = NULL;


typedef struct AUTO_SAMPLE {
   A5O_SAMPLE_INSTANCE *instance;
   int id;
   bool locked;
} AUTO_SAMPLE;

static _AL_VECTOR auto_samples = _AL_VECTOR_INITIALIZER(AUTO_SAMPLE);


static bool create_default_mixer(void);
static bool do_play_sample(A5O_SAMPLE_INSTANCE *spl, A5O_SAMPLE *data,
      float gain, float pan, float speed, A5O_PLAYMODE loop);
static void free_sample_vector(void);


static int string_to_depth(const char *s)
{
   // FIXME: fill in the rest
   if (!_al_stricmp(s, "int16")) {
      return A5O_AUDIO_DEPTH_INT16;
   }
   else {
      return A5O_AUDIO_DEPTH_FLOAT32;
   }
}


/* Creates the default voice and mixer if they haven't been created yet. */
static bool create_default_mixer(void)
{
   int voice_frequency = 44100;
   int voice_depth = A5O_AUDIO_DEPTH_INT16;
   int mixer_frequency = 44100;
   int mixer_depth = A5O_AUDIO_DEPTH_FLOAT32;

   A5O_CONFIG *config = al_get_system_config();
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

   if (!allegro_voice) {
      allegro_voice = al_create_voice(voice_frequency, voice_depth,
         A5O_CHANNEL_CONF_2);
      if (!allegro_voice) {
         A5O_ERROR("al_create_voice failed\n");
         goto Error;
      }
   }

   if (!allegro_mixer) {
      allegro_mixer = al_create_mixer(mixer_frequency, mixer_depth,
         A5O_CHANNEL_CONF_2);
      if (!allegro_mixer) {
         A5O_ERROR("al_create_voice failed\n");
         goto Error;
      }
   }

   /* In case this function is called multiple times. */
   al_detach_mixer(allegro_mixer);

   if (!al_attach_mixer_to_voice(allegro_mixer, allegro_voice)) {
      A5O_ERROR("al_attach_mixer_to_voice failed\n");
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
A5O_SAMPLE *al_create_sample(void *buf, unsigned int samples,
   unsigned int freq, A5O_AUDIO_DEPTH depth,
   A5O_CHANNEL_CONF chan_conf, bool free_buf)
{
   A5O_SAMPLE *spl;

   ASSERT(buf);

   if (!freq) {
      _al_set_error(A5O_INVALID_PARAM, "Invalid sample frequency");
      return NULL;
   }

   spl = al_calloc(1, sizeof(*spl));
   if (!spl) {
      _al_set_error(A5O_GENERIC_ERROR,
         "Out of memory allocating sample data object");
      return NULL;
   }

   spl->depth = depth;
   spl->chan_conf = chan_conf;
   spl->frequency = freq;
   spl->len = samples;
   spl->buffer.ptr = buf;
   spl->free_buf = free_buf;

   spl->dtor_item = _al_kcm_register_destructor("sample", spl, (void (*)(void *)) al_destroy_sample);

   return spl;
}


/* Stop any sample instances which are still playing a sample buffer which
 * is about to be destroyed.
 */
static void stop_sample_instances_helper(void *object, void (*func)(void *),
   void *userdata)
{
   A5O_SAMPLE_INSTANCE *splinst = object;

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
void al_destroy_sample(A5O_SAMPLE *spl)
{
   if (spl) {
      _al_kcm_foreach_destructor(stop_sample_instances_helper,
         al_get_sample_data(spl));
      _al_kcm_unregister_destructor(spl->dtor_item);

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
         AUTO_SAMPLE *slot = _al_vector_alloc_back(&auto_samples);
         slot->id = 0;
         slot->instance = al_create_sample_instance(NULL);
         slot->locked = false;
         if (!slot->instance) {
            A5O_ERROR("al_create_sample failed\n");
            goto Error;
         }
         if (!al_attach_sample_instance_to_mixer(slot->instance, default_mixer)) {
            A5O_ERROR("al_attach_mixer_to_sample failed\n");
            goto Error;
         }
      }
   }
   else if (current_samples_count > reserve_samples) {
      /* We need to reserve fewer samples than currently are reserved. */
      while (current_samples_count-- > reserve_samples) {
         AUTO_SAMPLE *slot = _al_vector_ref(&auto_samples, current_samples_count);
         al_destroy_sample_instance(slot->instance);
         _al_vector_delete_at(&auto_samples, current_samples_count);
      }
   }

   return true;

 Error:
   free_sample_vector();
   
   return false;
}


/* Function: al_get_default_mixer
 */
A5O_MIXER *al_get_default_mixer(void)
{
   return default_mixer;
}


/* Function: al_set_default_mixer
 */
bool al_set_default_mixer(A5O_MIXER *mixer)
{
   ASSERT(mixer != NULL);

   if (mixer != default_mixer) {
      int i;

      default_mixer = mixer;

      /* Destroy all current sample instances, recreate them, and
       * attach them to the new mixer */
      for (i = 0; i < (int) _al_vector_size(&auto_samples); i++) {
         AUTO_SAMPLE *slot = _al_vector_ref(&auto_samples, i);

         slot->id = 0;
         al_destroy_sample_instance(slot->instance);
         slot->locked = false;

         slot->instance = al_create_sample_instance(NULL);
         if (!slot->instance) {
            A5O_ERROR("al_create_sample failed\n");
            goto Error;
         }
         if (!al_attach_sample_instance_to_mixer(slot->instance, default_mixer)) {
            A5O_ERROR("al_attach_mixer_to_sample failed\n");
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


/* Function: al_get_default_voice
 */
A5O_VOICE *al_get_default_voice(void)
{
   return allegro_voice;
}


/* Function: al_set_default_voice
 */
void al_set_default_voice(A5O_VOICE *voice)
{
   if (allegro_voice) {
      al_destroy_voice(allegro_voice);
   }
   allegro_voice = voice;
}


/* Function: al_play_sample
 */
bool al_play_sample(A5O_SAMPLE *spl, float gain, float pan, float speed,
   A5O_PLAYMODE loop, A5O_SAMPLE_ID *ret_id)
{
   static int next_id = 0;
   unsigned int i;
   
   ASSERT(spl);

   if (ret_id != NULL) {
      ret_id->_id = -1;
      ret_id->_index = 0;
   }

   for (i = 0; i < _al_vector_size(&auto_samples); i++) {
      AUTO_SAMPLE *slot = _al_vector_ref(&auto_samples, i);

      if (!al_get_sample_instance_playing(slot->instance) && !slot->locked) {
         if (!do_play_sample(slot->instance, spl, gain, pan, speed, loop))
            break;

         if (ret_id != NULL) {
            ret_id->_index = (int) i;
            ret_id->_id = slot->id = ++next_id;
         }

         return true;
      }
   }

   return false;
}


static bool do_play_sample(A5O_SAMPLE_INSTANCE *splinst,
   A5O_SAMPLE *spl, float gain, float pan, float speed, A5O_PLAYMODE loop)
{
   if (!al_set_sample(splinst, spl)) {
      A5O_ERROR("al_set_sample failed\n");
      return false;
   }

   if (!al_set_sample_instance_gain(splinst, gain) ||
         !al_set_sample_instance_pan(splinst, pan) ||
         !al_set_sample_instance_speed(splinst, speed) ||
         !al_set_sample_instance_playmode(splinst, loop)) {
      return false;
   }

   if (!al_play_sample_instance(splinst)) {
      A5O_ERROR("al_play_sample_instance failed\n");
      return false;
   }

   return true;
}


/* Function: al_stop_sample
 */
void al_stop_sample(A5O_SAMPLE_ID *spl_id)
{
   AUTO_SAMPLE *slot;

   ASSERT(spl_id->_id != -1);
   ASSERT(spl_id->_index < (int) _al_vector_size(&auto_samples));

   slot = _al_vector_ref(&auto_samples, spl_id->_index);
   if (slot->id == spl_id->_id) {
      al_stop_sample_instance(slot->instance);
   }
}


/* Function: al_lock_sample_id
 */
A5O_SAMPLE_INSTANCE* al_lock_sample_id(A5O_SAMPLE_ID *spl_id)
{
   AUTO_SAMPLE *slot;

   ASSERT(spl_id->_id != -1);
   ASSERT(spl_id->_index < (int) _al_vector_size(&auto_samples));

   slot = _al_vector_ref(&auto_samples, spl_id->_index);
   if (slot->id == spl_id->_id) {
      slot->locked = true;
      return slot->instance;
   }
   return NULL;
}


/* Function: al_unlock_sample_id
 */
void al_unlock_sample_id(A5O_SAMPLE_ID *spl_id)
{
   AUTO_SAMPLE *slot;

   ASSERT(spl_id->_id != -1);
   ASSERT(spl_id->_index < (int) _al_vector_size(&auto_samples));

   slot = _al_vector_ref(&auto_samples, spl_id->_index);
   if (slot->id == spl_id->_id) {
      slot->locked = false;
   }
}


/* Function: al_stop_samples
 */
void al_stop_samples(void)
{
   unsigned int i;

   for (i = 0; i < _al_vector_size(&auto_samples); i++) {
      AUTO_SAMPLE *slot = _al_vector_ref(&auto_samples, i);
      al_stop_sample_instance(slot->instance);
   }
}


/* Function: al_get_sample_frequency
 */
unsigned int al_get_sample_frequency(const A5O_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->frequency;
}


/* Function: al_get_sample_length
 */
unsigned int al_get_sample_length(const A5O_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->len;
}


/* Function: al_get_sample_depth
 */
A5O_AUDIO_DEPTH al_get_sample_depth(const A5O_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->depth;
}


/* Function: al_get_sample_channels
 */
A5O_CHANNEL_CONF al_get_sample_channels(const A5O_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->chan_conf;
}


/* Function: al_get_sample_data
 */
void *al_get_sample_data(const A5O_SAMPLE *spl)
{
   ASSERT(spl);

   return spl->buffer.ptr;
}


/* Destroy all sample instances, and frees the associated vectors. */
static void free_sample_vector(void)
{
   int j;

   for (j = 0; j < (int) _al_vector_size(&auto_samples); j++) {
      AUTO_SAMPLE *slot = _al_vector_ref(&auto_samples, j);
      al_destroy_sample_instance(slot->instance);
   }
   _al_vector_free(&auto_samples);
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
