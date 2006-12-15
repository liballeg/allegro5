#ifndef _al_included_aintern_events_h
#define _al_included_aintern_events_h

#include "allegro/internal/aintern_thread.h"
#include "allegro/internal/aintern_vector.h"

AL_BEGIN_EXTERN_C


union AL_EVENT;

struct AL_EVENT_SOURCE
{
   AL_EVENT_TYPE event_mask;
   _AL_MUTEX mutex;
   _AL_VECTOR queues;
   AL_EVENT *all_events;
   AL_EVENT *free_events;
};


AL_FUNC(void, _al_copy_event, (union AL_EVENT *dest, const union AL_EVENT *src));

AL_FUNC(void, _al_event_source_init, (AL_EVENT_SOURCE*, AL_EVENT_TYPE event_mask));
AL_FUNC(void, _al_event_source_free, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_lock, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_unlock, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_on_registration_to_queue, (AL_EVENT_SOURCE*, AL_EVENT_QUEUE*));
AL_FUNC(void, _al_event_source_on_unregistration_from_queue, (AL_EVENT_SOURCE*, AL_EVENT_QUEUE*));
AL_FUNC(bool, _al_event_source_needs_to_generate_event, (AL_EVENT_SOURCE*, AL_EVENT_TYPE event_type));
AL_FUNC(AL_EVENT*, _al_event_source_get_unused_event, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_emit_event, (AL_EVENT_SOURCE *, AL_EVENT*));

AL_FUNC(void, _al_release_event, (AL_EVENT*));

AL_FUNC(void, _al_event_queue_push_event, (AL_EVENT_QUEUE*, AL_EVENT*));


AL_END_EXTERN_C

#endif

/* vi ts=8 sts=3 sw=3 et */
