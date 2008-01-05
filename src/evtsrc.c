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
 *      Event sources (mostly internal stuff).
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Event sources
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_events.h"



/*----------------------------------------------------------------------*
 *                                                                      *
 *      Internal event source API                                       *
 *                                                                      *
 *----------------------------------------------------------------------*/


/* Internal function: _al_event_source_init
 *  Initialise an event source structure.
 */
void _al_event_source_init(ALLEGRO_EVENT_SOURCE *this)
{
   _AL_MARK_MUTEX_UNINITED(this->mutex);
   _al_mutex_init(&this->mutex);
   _al_vector_init(&this->queues, sizeof(ALLEGRO_EVENT_QUEUE *));
   this->all_events = NULL;
   this->free_events = NULL;
}



/* Internal function: _al_event_source_free
 *  Free the resources using by an event source structure.  It
 *  automatically unregisters the event source from all the event
 *  queues it is currently registered with.
 */
void _al_event_source_free(ALLEGRO_EVENT_SOURCE *this)
{
   /* Unregister from all queues. */
   while (!_al_vector_is_empty(&this->queues)) {
      ALLEGRO_EVENT_QUEUE **slot = _al_vector_ref_back(&this->queues);
      al_unregister_event_source(*slot, this);
   }

   _al_vector_free(&this->queues);

   /* Free all allocated event structures.  */
   {
      ALLEGRO_EVENT *event, *next_event;

      for (event = this->all_events; event != NULL; event = next_event) {
         ASSERT(event->any._refcount == 0);
         next_event = event->any._next;
         _AL_FREE(event);
      }
   }

   _al_mutex_destroy(&this->mutex);
}



/* Internal function: _al_event_source_lock
 *  Lock the event source.  See below for when you should call this function.
 */
void _al_event_source_lock(ALLEGRO_EVENT_SOURCE *this)
{
   _al_mutex_lock(&this->mutex);
}



/* Internal function: _al_event_source_unlock
 *  Unlock the event source.
 */
void _al_event_source_unlock(ALLEGRO_EVENT_SOURCE *this)
{
   _al_mutex_unlock(&this->mutex);
}



/* make_new_event:
 *  Helper to allocate event structures for the event source.
 */
static ALLEGRO_EVENT *make_new_event(ALLEGRO_EVENT_SOURCE *this)
{
   ALLEGRO_EVENT *ret;

   ret = _AL_MALLOC(sizeof *ret);
   ASSERT(ret);

   if (ret) {
      ret->any._refcount = 0;
      ret->any._next = NULL;
      ret->any._next_free = NULL;
   }

   return ret;
}



/* Internal function: _al_event_source_on_registration_to_queue
 *  This function is called by al_register_event_source() when an
 *  event source is registered to an event queue.  This gives the
 *  event source a chance to remember which queues it is registered
 *  to.
 */
void _al_event_source_on_registration_to_queue(ALLEGRO_EVENT_SOURCE *this, ALLEGRO_EVENT_QUEUE *queue)
{
   _al_event_source_lock(this);
   {
      /* Add the queue to the source's list.  */
      ALLEGRO_EVENT_QUEUE **slot = _al_vector_alloc_back(&this->queues);
      *slot = queue;
   }
   _al_event_source_unlock(this);
}



/* Internal function: _al_event_source_on_unregistration_from_queue
 *  This function is called by al_unregister_event_source() when an
 *  event source is unregistered from a queue.
 */
void _al_event_source_on_unregistration_from_queue(ALLEGRO_EVENT_SOURCE *this, ALLEGRO_EVENT_QUEUE *queue)
{
   _al_event_source_lock(this);
   {
      _al_vector_find_and_delete(&this->queues, &queue);
   }
   _al_event_source_unlock(this);
}



/* Internal function: _al_event_source_needs_to_generate_event
 *  This function is called by modules that implement event sources
 *  when some interesting thing happens.  They call this to check if
 *  they should bother generating an event of the given type, i.e. if
 *  the given event source is actually registered with one or more
 *  event queues.  This is an optimisation to avoid allocating and
 *  filling in unwanted event structures.
 *
 *  The event source must be _locked_ before calling this function.
 *
 *  [runs in background threads]
 */
bool _al_event_source_needs_to_generate_event(ALLEGRO_EVENT_SOURCE *this)
{
   return !_al_vector_is_empty(&this->queues);
}



/* Internal function: _al_event_source_get_unused_event
 *  Once it is determined that an event source needs to generate an
 *  event, this function is called.  It returns an allocated event
 *  structure that can be filled in, or NULL if no event structure is
 *  available.
 *
 *  The event source must be _locked_ before calling this function.
 *
 *  [runs in background threads]
 */
ALLEGRO_EVENT *_al_event_source_get_unused_event(ALLEGRO_EVENT_SOURCE *this)
{
   ALLEGRO_EVENT *event;

   event = this->free_events;
   if (event) {
      this->free_events = event->any._next_free;
      return event;
   }

   event = make_new_event(this);
   if (event) {
      event->any._next = this->all_events;
      this->all_events = event;
   }
   return event;
}



/* Internal function: _al_event_source_emit_event
 *  After an event structure has been filled in, it is time for the
 *  event source to tell the event queues it knows of about the new
 *  event.  Afterwards, the caller of this function should not touch
 *  the event any more.
 *
 *  The event source must be _locked_ before calling this function.
 *
 *  [runs in background threads]
 */
void _al_event_source_emit_event(ALLEGRO_EVENT_SOURCE *this, ALLEGRO_EVENT *event)
{
   event->any.source = this;
   ASSERT(event->any._refcount == 0);

   /* Push the event to all the queues that this event source is
    * registered to.
    */
   {
      size_t num_queues = _al_vector_size(&this->queues);
      unsigned int i;
      ALLEGRO_EVENT_QUEUE **slot;

      for (i = 0; i < num_queues; i++) {
         slot = _al_vector_ref(&this->queues, i);
         _al_event_queue_push_event(*slot, event);
         /* The event queue will increment the event's _refcount field
          * if the event was accepted into the queue.
          */
      }
   }

   /* Note: if a thread was waiting on a event queue, the
    * event_queue->push_event() call may have woken it up.
    * Futhermore, it may have grabbed the event just pushed, and
    * already released it!
    *
    * If the event source's `release_event' method does locking, and
    * the event source is locked before entering this function,
    * everything will (should!) work out fine.
    */

   if (event->any._refcount == 0) {
      /* No queues could accept this event, add to free list.  */
      event->any._next_free = this->free_events;
      this->free_events = event;
   }
}



/* Internal function: _al_release_event
 *  This function is called by event queue implementations when they
 *  no longer need a reference to the given event structure.  It
 *  decrements the refcount of the event by one.  Once the refcount
 *  reaches zero, the event structure is put on a 'free list', where
 *  it can be reused for later events.
 */
void _al_release_event(ALLEGRO_EVENT *event)
{
   ASSERT(event);
   {
      ALLEGRO_EVENT_SOURCE *source = event->any.source;

      _al_event_source_lock(source);
      {
         /* Decrement the refcount...  */
         ASSERT(event->any._refcount > 0);
         event->any._refcount--;

         /* ... then return the event to the free list if appropriate.  */
         if (event->any._refcount == 0) {
            event->any._next_free = source->free_events;
            source->free_events = event;
         }
      }
      _al_event_source_unlock(source);
   }
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
