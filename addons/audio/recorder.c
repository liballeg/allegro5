/*
 * Allegro audio recording
 */

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"
#include "allegro5/internal/aintern.h"

ALLEGRO_DEBUG_CHANNEL("audio")

/* Function: al_create_audio_recorder
 */
ALLEGRO_AUDIO_RECORDER *al_create_audio_recorder(unsigned int frequency,
   size_t sample_count, ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_AUDIO_RECORDER *r;
   ASSERT(_al_kcm_driver);
   
   if (!_al_kcm_driver->allocate_recorder) {
      ALLEGRO_ERROR("Audio driver does not support recording.\n");
      return false;
   }
   
   r = al_calloc(1, sizeof(*r));
   if (!r) {
      ALLEGRO_ERROR("Unable to allocate memory for ALLEGRO_AUDIO_RECORDER\n");
      return false;
   }
   
   r->frequency = frequency,
   r->sample_count = sample_count;
   r->depth = depth;
   r->chan_conf = chan_conf;
   
   r->sample_size = al_get_channel_count(chan_conf) * al_get_audio_depth_size(depth);

   if (_al_kcm_driver->allocate_recorder(r)) {
      ALLEGRO_ERROR("Failed to allocate recorder from driver\n");
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
bool al_start_audio_recorder(ALLEGRO_AUDIO_RECORDER *r, void *buffer, size_t buffer_bytes)
{
   ALLEGRO_ASSERT(r);
   ALLEGRO_ASSERT(buffer);
   
   al_lock_mutex(r->mutex);
   r->buffer = buffer;
   r->remaining_buffer_size = buffer_bytes;
   r->is_recording = true;
   al_signal_cond(r->cond);
   al_unlock_mutex(r->mutex);
   
   return true;
}

/* Function: al_stop_audio_recorder
 */
void al_stop_audio_recorder(ALLEGRO_AUDIO_RECORDER *r)
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
bool al_is_audio_recorder_recording(ALLEGRO_AUDIO_RECORDER *r)
{
   bool is_recording;
   
   al_lock_mutex(r->mutex);
   is_recording = r->is_recording;
   al_unlock_mutex(r->mutex);
   
   return is_recording;
}

/* Function: al_update_audio_recorder_buffer
 */
void al_update_audio_recorder_buffer(ALLEGRO_AUDIO_RECORDER *r, void *buffer, size_t buffer_bytes) 
{
   al_lock_mutex(r->mutex);
   r->buffer = buffer;
   r->remaining_buffer_size = buffer_bytes;
   al_signal_cond(r->cond);
   al_unlock_mutex(r->mutex);
}

/* Function: al_get_audio_recorder_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_audio_recorder_event_source(ALLEGRO_AUDIO_RECORDER *r)
{
   return &r->source;
}

/* Function: al_destroy_audio_recorder
 */
void al_destroy_audio_recorder(ALLEGRO_AUDIO_RECORDER *r)
{
   if (r->thread) {
      al_set_thread_should_stop(r->thread);
      
      al_lock_mutex(r->mutex);
      r->is_recording = false;
      al_signal_cond(r->cond);
      al_unlock_mutex(r->mutex);
   
      al_join_thread(r->thread, NULL);
      al_destroy_thread(r->thread);
    }
   
   al_destroy_user_event_source(&r->source);     
   al_destroy_mutex(r->mutex);
   al_destroy_cond(r->cond);
   
   al_free(r);
}