/*
 * Allegro audio recording
 */

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"
#include "allegro5/internal/aintern.h"

A5O_DEBUG_CHANNEL("audio")

A5O_STATIC_ASSERT(recorder,
   sizeof(A5O_AUDIO_RECORDER_EVENT) <= sizeof(A5O_EVENT));


/* Function: al_create_audio_recorder
 */
A5O_AUDIO_RECORDER *al_create_audio_recorder(size_t fragment_count,
   unsigned int samples, unsigned int frequency,
   A5O_AUDIO_DEPTH depth, A5O_CHANNEL_CONF chan_conf)
{
   size_t i;

   A5O_AUDIO_RECORDER *r;
   ASSERT(_al_kcm_driver);
   
   if (!_al_kcm_driver->allocate_recorder) {
      A5O_ERROR("Audio driver does not support recording.\n");
      return false;
   }
   
   r = al_calloc(1, sizeof(*r));
   if (!r) {
      A5O_ERROR("Unable to allocate memory for A5O_AUDIO_RECORDER\n");
      return false;
   }
   
   r->fragment_count = fragment_count;
   r->samples = samples;
   r->frequency = frequency,   
   r->depth = depth;
   r->chan_conf = chan_conf;
   
   r->sample_size = al_get_channel_count(chan_conf) * al_get_audio_depth_size(depth);

   r->fragments = al_malloc(r->fragment_count * sizeof(uint8_t *));
   if (!r->fragments) {
      al_free(r);
      A5O_ERROR("Unable to allocate memory for A5O_AUDIO_RECORDER fragments\n");
      return false;
   }

   r->fragment_size = r->samples * r->sample_size;
   for (i = 0; i < fragment_count; ++i) {
      r->fragments[i] = al_malloc(r->fragment_size);
      if (!r->fragments[i]) {
         size_t j;
         for (j = 0; j < i; ++j) {
            al_free(r->fragments[j]);
         }
         al_free(r->fragments);

         A5O_ERROR("Unable to allocate memory for A5O_AUDIO_RECORDER fragments\n");
         return false;
      }
   }

   if (_al_kcm_driver->allocate_recorder(r)) {
      A5O_ERROR("Failed to allocate recorder from driver\n");
      return false;
   }
  
   r->is_recording = false;
   r->mutex = al_create_mutex();
   r->cond = al_create_cond();
   
   al_init_user_event_source(&r->source);
   
   if (r->thread) {
      /* the driver should have created a thread */
      al_start_thread(r->thread);
   }
   
   return r;  
};

/* Function: al_start_audio_recorder
 */
bool al_start_audio_recorder(A5O_AUDIO_RECORDER *r)
{
   A5O_ASSERT(r);
   
   al_lock_mutex(r->mutex);
   r->is_recording = true;
   al_signal_cond(r->cond);
   al_unlock_mutex(r->mutex);
   
   return true;
}

/* Function: al_stop_audio_recorder
 */
void al_stop_audio_recorder(A5O_AUDIO_RECORDER *r)
{
   al_lock_mutex(r->mutex);
   if (r->is_recording) {
      r->is_recording = false;
      al_signal_cond(r->cond);
   }
   al_unlock_mutex(r->mutex);
}

/* Function: al_is_audio_recorder_recording
 */
bool al_is_audio_recorder_recording(A5O_AUDIO_RECORDER *r)
{
   bool is_recording;
   
   al_lock_mutex(r->mutex);
   is_recording = r->is_recording;
   al_unlock_mutex(r->mutex);
   
   return is_recording;
}

/* Function: al_get_audio_recorder_event
 */
A5O_AUDIO_RECORDER_EVENT *al_get_audio_recorder_event(A5O_EVENT *event)
{
   ASSERT(event->any.type == A5O_EVENT_AUDIO_RECORDER_FRAGMENT);
   return (A5O_AUDIO_RECORDER_EVENT *) event;
}

/* Function: al_get_audio_recorder_event_source
 */
A5O_EVENT_SOURCE *al_get_audio_recorder_event_source(A5O_AUDIO_RECORDER *r)
{
   return &r->source;
}

/* Function: al_destroy_audio_recorder
 */
void al_destroy_audio_recorder(A5O_AUDIO_RECORDER *r)
{
   size_t i;

   if (!r)
      return;

   if (r->thread) {
      al_set_thread_should_stop(r->thread);
      
      al_lock_mutex(r->mutex);
      r->is_recording = false;
      al_signal_cond(r->cond);
      al_unlock_mutex(r->mutex);
   
      al_join_thread(r->thread, NULL);
      al_destroy_thread(r->thread);
    }
   
   if (_al_kcm_driver->deallocate_recorder) {
      _al_kcm_driver->deallocate_recorder(r);
   }
   
   al_destroy_user_event_source(&r->source);     
   al_destroy_mutex(r->mutex);
   al_destroy_cond(r->cond);
   
   for (i = 0; i < r->fragment_count; ++i) {
      al_free(r->fragments[i]);
   }
   al_free(r->fragments);
   al_free(r);
}
