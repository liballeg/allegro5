#ifndef _al_included_aintern_events_h
#define _al_included_aintern_events_h

AL_BEGIN_EXTERN_C


struct AL_EVENT_SOURCE
{
   unsigned long event_mask;
   _AL_MUTEX mutex;
   _AL_VECTOR queues;
   AL_EVENT *all_events;
   AL_EVENT *free_events;
};

AL_FUNC(void, _al_event_source_init, (AL_EVENT_SOURCE*, unsigned long event_mask));
AL_FUNC(void, _al_event_source_free, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_lock, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_unlock, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_on_registration_to_queue, (AL_EVENT_SOURCE*, AL_EVENT_QUEUE*));
AL_FUNC(void, _al_event_source_on_unregistration_from_queue, (AL_EVENT_SOURCE*, AL_EVENT_QUEUE*));
AL_FUNC(bool, _al_event_source_needs_to_generate_event, (AL_EVENT_SOURCE*, unsigned long event_type));
AL_FUNC(AL_EVENT*, _al_event_source_get_unused_event, (AL_EVENT_SOURCE*));
AL_FUNC(void, _al_event_source_emit_event, (AL_EVENT_SOURCE *, AL_EVENT*));

AL_FUNC(void, _al_release_event, (AL_EVENT*));

AL_FUNC(void, _al_event_queue_push_event, (AL_EVENT_QUEUE*, AL_EVENT*));


#endif

/* vi ts=8 sts=3 sw=3 et */
