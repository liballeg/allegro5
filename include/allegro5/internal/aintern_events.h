#ifndef _al_included_aintern_events_h
#define _al_included_aintern_events_h

#include "allegro5/internal/aintern_atomicops.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_vector.h"

AL_BEGIN_EXTERN_C


union ALLEGRO_EVENT;

struct ALLEGRO_EVENT_SOURCE
{
   _AL_MUTEX mutex;
   _AL_VECTOR queues;
};

typedef struct ALLEGRO_USER_EVENT_DESCRIPTOR
{
   void (*dtor)(ALLEGRO_USER_EVENT *event);
   volatile _AL_ATOMIC refcount;
} ALLEGRO_USER_EVENT_DESCRIPTOR;


AL_FUNC(void, _al_event_source_init, (ALLEGRO_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_free, (ALLEGRO_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_lock, (ALLEGRO_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_unlock, (ALLEGRO_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_on_registration_to_queue, (ALLEGRO_EVENT_SOURCE*, ALLEGRO_EVENT_QUEUE*));
AL_FUNC(void, _al_event_source_on_unregistration_from_queue, (ALLEGRO_EVENT_SOURCE*, ALLEGRO_EVENT_QUEUE*));
AL_FUNC(bool, _al_event_source_needs_to_generate_event, (ALLEGRO_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_emit_event, (ALLEGRO_EVENT_SOURCE *, ALLEGRO_EVENT*));

AL_FUNC(void, _al_event_queue_push_event, (ALLEGRO_EVENT_QUEUE*, const ALLEGRO_EVENT*));


AL_END_EXTERN_C

#endif

/* vi ts=8 sts=3 sw=3 et */
