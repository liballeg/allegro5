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


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER



/*----------------------------------------------------------------------*
 *                                                                      *
 *      User API                                                        *
 *                                                                      *
 *----------------------------------------------------------------------*/

/* al_event_source_mask:
 *
 */
unsigned long al_event_source_mask(AL_EVENT_SOURCE *source)
{
   ASSERT(source);

   return source->event_mask;
}



/* al_event_source_set_mask:
 *
 */
void al_event_source_set_mask(AL_EVENT_SOURCE *source, unsigned long mask)
{
   ASSERT(source);

   source->event_mask = mask;
}



/*----------------------------------------------------------------------*
 *                                                                      *
 *      Internal event source API                                       *
 *                                                                      *
 *----------------------------------------------------------------------*/


void _al_event_source_init(AL_EVENT_SOURCE *this, size_t event_size)
{
   this->event_size = event_size;
   _al_mutex_init(&this->mutex);
   _al_vector_init(&this->queues, sizeof(AL_EVENT_QUEUE *));
   this->all_events = NULL;
   this->free_events = NULL;
}


void _al_event_source_free(AL_EVENT_SOURCE *this)
{
   /* Unregister from all queues. */
   while (!_al_vector_is_empty(&this->queues)) {
      AL_EVENT_QUEUE **slot = _al_vector_ref_back(&this->queues);
      al_unregister_event_source(*slot, this);
   }

   _al_vector_free(&this->queues);

   /* Free all allocated event structures.  */
   {
      AL_EVENT *event, *next_event;

      for (event = this->all_events; event != NULL; event = next_event) {
         ASSERT(event->any._refcount == 0);
         next_event = event->any._next;
         free(event);
      }
   }

   _al_mutex_destroy(&this->mutex);
}


void _al_event_source_lock(AL_EVENT_SOURCE *this)
{
   _al_mutex_lock(&this->mutex);
}


void _al_event_source_unlock(AL_EVENT_SOURCE *this)
{
   _al_mutex_unlock(&this->mutex);
}


static AL_EVENT *make_new_event(AL_EVENT_SOURCE *this)
{
   AL_EVENT *ret;

   ASSERT(this->event_size > 0);

   ret = malloc(this->event_size);
   ASSERT(ret);

   if (ret) {
      ret->any._size = this->event_size;
      ret->any._refcount = 0;
      ret->any._next = NULL;
      ret->any._next_free = NULL;
   }

   return ret;
}


void _al_event_source_on_registration_to_queue(AL_EVENT_SOURCE *this, AL_EVENT_QUEUE *queue)
{
   _al_event_source_lock(this);
   {
      /* Add the queue to the source's list.  */
      AL_EVENT_QUEUE **slot = _al_vector_alloc_back(&this->queues);
      *slot = queue;
   }
   _al_event_source_unlock(this);
}


void _al_event_source_on_unregistration_from_queue(AL_EVENT_SOURCE *this, AL_EVENT_QUEUE *queue)
{
   _al_event_source_lock(this);
   {
      _al_vector_find_and_delete(&this->queues, &queue);
   }
   _al_event_source_unlock(this);
}


bool _al_event_source_needs_to_generate_event(AL_EVENT_SOURCE *this, unsigned long event_type)
{
   return !_al_vector_is_empty(&this->queues) && (this->event_mask & event_type);
}


AL_EVENT *_al_event_source_get_unused_event(AL_EVENT_SOURCE *this)
{
   AL_EVENT *event;

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


void _al_event_source_emit_event(AL_EVENT_SOURCE *this, AL_EVENT *event)
{
   event->any.source = this;
   ASSERT(event->any._refcount == 0);

   /* Push the event to all the queues that this event source is
    * registered to.
    */
   {
      size_t num_queues = _al_vector_size(&this->queues);
      unsigned int i;
      AL_EVENT_QUEUE **slot;

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


void _al_release_event(AL_EVENT *event)
{
   ASSERT(event);
   {
      AL_EVENT_SOURCE *source = event->any.source;

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
