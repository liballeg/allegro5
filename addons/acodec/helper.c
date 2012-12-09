#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_system.h"
#include "helper.h"

void _al_acodec_stop_feed_thread(ALLEGRO_AUDIO_STREAM *stream)
{
   ALLEGRO_EVENT quit_event;

   quit_event.type = _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE;
   al_emit_user_event(al_get_audio_stream_event_source(stream), &quit_event, NULL);
   al_join_thread(stream->feed_thread, NULL);
   al_destroy_thread(stream->feed_thread);

   stream->feed_thread = NULL;
}
