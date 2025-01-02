#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "helper.h"

void _al_acodec_start_feed_thread(ALLEGRO_AUDIO_STREAM *stream)
{
   stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
   stream->feed_thread_started_cond = al_create_cond();
   stream->feed_thread_started_mutex = al_create_mutex();
   al_start_thread(stream->feed_thread);

   /* Need to wait for the thread to start, otherwise the quit event may be
    * sent before the event source is registered with the queue.
    *
    * This also makes the pre-fill system thread safe, as it needs to operate
    * before the mutexes are set up.
    */
   al_lock_mutex(stream->feed_thread_started_mutex);
   while (!stream->feed_thread_started) {
      al_wait_cond(stream->feed_thread_started_cond, stream->feed_thread_started_mutex);
   }
   al_unlock_mutex(stream->feed_thread_started_mutex);
}

void _al_acodec_stop_feed_thread(ALLEGRO_AUDIO_STREAM *stream)
{
   ALLEGRO_EVENT quit_event;

   quit_event.type = _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE;
   al_emit_user_event(al_get_audio_stream_event_source(stream), &quit_event, NULL);
   al_join_thread(stream->feed_thread, NULL);
   al_destroy_thread(stream->feed_thread);
   al_destroy_cond(stream->feed_thread_started_cond);
   al_destroy_mutex(stream->feed_thread_started_mutex);

   stream->feed_thread = NULL;
}
