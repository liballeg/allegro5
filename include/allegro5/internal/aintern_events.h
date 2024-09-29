#ifndef __al_included_allegro5_aintern_events_h
#define __al_included_allegro5_aintern_events_h

#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_vector.h"

#ifdef __cplusplus
   extern "C" {
#endif


typedef struct A5O_EVENT_SOURCE_REAL A5O_EVENT_SOURCE_REAL;

struct A5O_EVENT_SOURCE_REAL
{
   _AL_MUTEX mutex;
   _AL_VECTOR queues;
   intptr_t data;
};

typedef struct A5O_USER_EVENT_DESCRIPTOR
{
   void (*dtor)(A5O_USER_EVENT *event);
   int refcount;
} A5O_USER_EVENT_DESCRIPTOR;


void _al_init_events(void);

void _al_event_source_init(A5O_EVENT_SOURCE*);
void _al_event_source_free(A5O_EVENT_SOURCE*);
void _al_event_source_lock(A5O_EVENT_SOURCE*);
void _al_event_source_unlock(A5O_EVENT_SOURCE*);
void _al_event_source_on_registration_to_queue(A5O_EVENT_SOURCE*, A5O_EVENT_QUEUE*);
void _al_event_source_on_unregistration_from_queue(A5O_EVENT_SOURCE*, A5O_EVENT_QUEUE*);
bool _al_event_source_needs_to_generate_event(A5O_EVENT_SOURCE*);
void _al_event_source_emit_event(A5O_EVENT_SOURCE *, A5O_EVENT*);

void _al_event_queue_push_event(A5O_EVENT_QUEUE*, const A5O_EVENT*);


#ifdef __cplusplus
   }
#endif

#endif

/* vi ts=8 sts=3 sw=3 et */
