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
 *      Event queues.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern2.h"



#define INITIAL_QUEUE_SIZE      32
#define MAX_QUEUE_SIZE          512 /* power of 2 for max use of _AL_VECTOR */


struct AL_EVENT_QUEUE
{
   _AL_VECTOR events; /* XXX: better a deque */
   _AL_VECTOR sources;
   _AL_MUTEX mutex;
   _AL_COND cond;
};



/* al_create_event_queue: [primary thread]
 *  Create a new, empty event queue, returning a pointer to object if
 *  successful.  Returns NULL on error.
 */
AL_EVENT_QUEUE *al_create_event_queue(void)
{
   AL_EVENT_QUEUE *queue = malloc(sizeof *queue);

   ASSERT(queue);

   if (queue) {
      _al_vector_init(&queue->events, sizeof(AL_EVENT *));
      _al_vector_init(&queue->sources, sizeof(AL_EVENT_SOURCE *));
      _al_mutex_init(&queue->mutex);
      _al_cond_init(&queue->cond);

      _al_register_destructor(queue, (void (*)(void *)) al_destroy_event_queue);
   }

   return queue;
}



/* al_destroy_event_queue: [primary thread]
 *  Destroy the event queue specified.  All event sources currently
 *  registered with the queue will be automatically unregistered before
 *  the queue is destroyed.
 */
void al_destroy_event_queue(AL_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   _al_unregister_destructor(queue);

   /* Unregister any event sources registered with this queue.  */
   while (_al_vector_is_nonempty(&queue->sources)) {
      AL_EVENT_SOURCE **slot = _al_vector_ref_back(&queue->sources);
      al_unregister_event_source(queue, *slot);
   }

   ASSERT(_al_vector_is_empty(&queue->events));
   ASSERT(_al_vector_is_empty(&queue->sources));

   _al_vector_free(&queue->events);
   _al_vector_free(&queue->sources);

   _al_cond_destroy(&queue->cond);
   _al_mutex_destroy(&queue->mutex);

   free(queue);
}



/* al_register_event_source: [primary thread]
 *  Register the event source with the event queue specified.  An
 *  event source may be registered with any number of event queues
 *  simultaneously, or none.  Trying to register an event source with
 *  the same event queue more than once does nothing.
 */
void al_register_event_source(AL_EVENT_QUEUE *queue, AL_EVENT_SOURCE *source)
{
   ASSERT(queue);
   ASSERT(source);
   {
      AL_EVENT_SOURCE **slot;

      if (_al_vector_contains(&queue->sources, source))
         return;

      _al_event_source_on_registration_to_queue(source, queue);

      slot = _al_vector_alloc_back(&queue->sources);
      *slot = source;
   }
}



/* al_unregister_event_source: [primary thread]
 *  Unregister an event source with an event queue.  If the event
 *  source is not actually registered with the event queue, nothing
 *  happens.
 *
 *  If the queue had any events in it which originated from the event
 *  source, they will no longer be in the queue after this call.
 */
void al_unregister_event_source(AL_EVENT_QUEUE *queue, AL_EVENT_SOURCE *source)
{
   ASSERT(queue);
   ASSERT(source);

   /* Remove SOURCE from our list.  */
   if (!_al_vector_find_and_delete(&queue->sources, &source))
      return;

   /* Tell the event source that it was unregistered.  */
   _al_event_source_on_unregistration_from_queue(source, queue);

   /* Drop all the events in the queue that belonged to the source.  */
   {
      unsigned int i = 0;
      AL_EVENT **slot;
      AL_EVENT *event;

      while (i < _al_vector_size(&queue->events)) {
         slot = _al_vector_ref(&queue->events, i);
         event = *slot;

         if (event->any.source == source) {
            _al_release_event(event);
            _al_vector_delete_at(&queue->events, i);
         }
         else
            i++;
      }
   }
}



/* al_event_queue_is_empty: [primary thread]
 *  Return true if the event queue specified is currently empty.
 */
bool al_event_queue_is_empty(AL_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   return _al_vector_is_empty(&queue->events);
}



/* really_get_next_event: [primary thread]
 *  Helper function.  The event queue must be locked before entering
 *  this function.
 */
static AL_EVENT* really_get_next_event(AL_EVENT_QUEUE *queue)
{
   if (_al_vector_is_empty(&queue->events))
      return NULL;
   else {
      AL_EVENT **slot  = _al_vector_ref_front(&queue->events);
      AL_EVENT  *event = *slot;
      _al_vector_delete_at(&queue->events, 0); /* inefficient */
      return event;
   }
}



/* al_get_next_event: [primary thread]
 *  Take the next event packet out of the event queue specified, and
 *  copy the contents into RET_EVENT, returning true.  The original
 *  event packet will be removed from the queue.  If the event queue is
 *  empty, return false and the contents of RET_EVENT are unspecified.
 */
bool al_get_next_event(AL_EVENT_QUEUE *queue, AL_EVENT *ret_event)
{
   ASSERT(queue);
   ASSERT(ret_event);
   {
      AL_EVENT *event;

      _al_mutex_lock(&queue->mutex);
      {
         event = really_get_next_event(queue);
      }
      _al_mutex_unlock(&queue->mutex);

      if (event) {
         _al_copy_event(ret_event, event);
         _al_release_event(event);
         return true;
      }
      else
         return false;
   }
}



/* al_peek_next_event: [primary thread]
 *  Copy the contents of the next event packet in the event queue
 *  specified into RET_EVENT and return true.  The original event
 *  packet will remain at the head of the queue.  If the event queue is
 *  actually empty, this function returns false and the contents of
 *  RET_EVENT are unspecified.
 */
bool al_peek_next_event(AL_EVENT_QUEUE *queue, AL_EVENT *ret_event)
{
   ASSERT(queue);
   ASSERT(ret_event);
   {
      AL_EVENT *event = NULL;

      _al_mutex_lock(&queue->mutex);
      {
         if (_al_vector_is_nonempty(&queue->events)) {
            AL_EVENT **slot = _al_vector_ref_front(&queue->events);
            event = *slot;
         }
      }
      _al_mutex_unlock(&queue->mutex);

      if (event) {
         _al_copy_event(ret_event, event);
         /* Don't release the event, since we are only peeking.  */
         return true;
      }
      else
         return false;
   }
}



/* al_drop_next_event: [primary thread]
 *  Drop the next event packet from the queue.  If the queue is empty,
 *  nothing happens.
 */
void al_drop_next_event(AL_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   _al_mutex_lock(&queue->mutex);
   {
      if (_al_vector_is_nonempty(&queue->events)) {
         AL_EVENT **slot  = _al_vector_ref_front(&queue->events);
         _al_release_event(*slot);
         _al_vector_delete_at(&queue->events, 0); /* inefficient */
      }
   }
   _al_mutex_unlock(&queue->mutex);
}



/* al_flush_event_queue: [primary thread]
 *  Drops all events, if any, from the queue.
 */
void al_flush_event_queue(AL_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   _al_mutex_lock(&queue->mutex);
   {
      while (_al_vector_is_nonempty(&queue->events)) {
         unsigned int i = _al_vector_size(&queue->events) - 1;
         AL_EVENT **slot = _al_vector_ref(&queue->events, i);
         _al_release_event(*slot);
         _al_vector_delete_at(&queue->events, i);
      }
   }
   _al_mutex_unlock(&queue->mutex);
}



/* al_wait_for_event: [primary thread]
 *  Wait until the event queue specified is non-empty, then copy the
 *  first event packet in the queue into RET_EVENT (the event packet
 *  will no longer be in the queue).  TIMEOUT_MSECS determines
 *  approximately how many milliseconds to wait.  If TIMEOUT_MSECS is
 *  zero, the call will wait indefinitely.  If the call times out,
 *  false is returned.  Otherwise true is returned.
 */
bool al_wait_for_event(AL_EVENT_QUEUE *queue, AL_EVENT *ret_event, long msecs)
{
   ASSERT(queue);
   ASSERT(ret_event);
   ASSERT(msecs >= 0);
   {
      AL_EVENT *next_event;

      if (msecs == 0)
      {
         /* "Infinite" timeout case.  */

         _al_mutex_lock(&queue->mutex);
         {
            while (_al_vector_is_empty(&queue->events))
               _al_cond_wait(&queue->cond, &queue->mutex);

            next_event = really_get_next_event(queue);
            ASSERT(next_event);
         }
         _al_mutex_unlock(&queue->mutex);
      }
      else
      {
         /* Finite timeout case.  */

         _al_mutex_lock(&queue->mutex);
         {
            unsigned long timeout = al_current_time() + msecs;
            int result = 0;

            /* Is the queue is non-empty?  If not, block on a condition
             * variable, which will be signaled when an event is placed into
             * the queue.
             */
            while (_al_vector_is_empty(&queue->events) && (result != -1))
               result = _al_cond_timedwait(&queue->cond, &queue->mutex, timeout);

            if (result == -1)   /* timed out */
               next_event = NULL;
            else {
               next_event = really_get_next_event(queue);
               ASSERT(next_event);
            }
         }
         _al_mutex_unlock(&queue->mutex);
      }

      if (next_event) {
         _al_copy_event(ret_event, next_event);
         _al_release_event(next_event);
         return true;
      }
      else
         return false;
   }
}



/* al_wait_for_specific_event: [primary thread]
 *  
 *  Wait until the head of the event queue contains an event packet
 *  that satisfies the following criteria, then copy that event packet
 *  into RET_EVENT:
 *
 *    1. If SRC_OR_NULL is not NULL, then the event must come from the
 *       event source specified.  If SRC_OR_NULL is NULL, there is no
 *       such restriction.
 *
 *    2. The event must match the bitfield EVENT_MASK.
 *
 *  All events at the front of the queue which do not satisfy those
 *  criteria _will be discarded_.
 *
 *  TIMEOUT_MSECS determines how long for the function call to wait.
 *  If TIMEOUT_MSECS is zero, the call will wait indefinitely.  If the
 *  call times out, false is returned.  Otherwise true is returned.
 */
bool al_wait_for_specific_event(AL_EVENT_QUEUE *queue,
                                AL_EVENT *ret_event,
                                long msecs,
                                AL_EVENT_SOURCE *source,
                                unsigned long event_mask)
{
   ASSERT(msecs >= 0);

   if (msecs == 0) {
      while (true) {
         al_wait_for_event(queue, ret_event, 0);

         if (((!source) || (ret_event->any.source == source)) &&
             (ret_event->type & event_mask))
            return true;
      }
   }
   else {
      /* XXX: Should get rid of long longs at some stage. */
      /* XXX: Watch out for al_current_time(), which could wrap. */
      long long end, remaining;

      end = al_current_time() + msecs;

      while (true) {
         remaining = end - al_current_time();
         if (remaining <= 0LL)
            return false;

         /* Don't bother trying to wait a second time unless we missed
          * the mark by a large enough amount.  The 1ms below is
          * pretty arbitrary.
          */
         if (remaining < 1)
            return false;

         if (!al_wait_for_event(queue, ret_event, remaining))
            return false;

         if (((!source) || (ret_event->any.source != source)) &&
             (ret_event->type & event_mask))
            return true;
      }
   }
}



/*----------------------------------------------------------------------*/



/* _al_event_queue_push_event:
 *  Event sources call this function when they have something to add to
 *  the queue.  If a queue cannot accept the event, the event's
 *  refcount will not be incremented.
 *
 *  If no event queues can accept the event, the event should be
 *  returned to the event source's list of recyclable events.
 */
void _al_event_queue_push_event(AL_EVENT_QUEUE *queue, AL_EVENT *event)
{
   ASSERT(queue);
   ASSERT(event);

   _al_mutex_lock(&queue->mutex);
   {
      if (_al_vector_size(&queue->events) < MAX_QUEUE_SIZE)
      {
         event->any._refcount++;
         ASSERT(event->any._refcount > 0);

         {
            AL_EVENT **slot = _al_vector_alloc_back(&queue->events);
            *slot = event;
         }

         /* Wake up threads that are waiting for an event to be placed in
          * the queue.
          *
          * TODO: Test if multiple threads waiting on the same event
          * queue actually works.
          */
         _al_cond_broadcast(&queue->cond);
      }
   }
   _al_mutex_unlock(&queue->mutex);
}



/* _al_copy_event:
 *  Copies the contents of the event SRC to DEST.  How many bytes are
 *  actually copied depends on what type of event SRC is.
 */
void _al_copy_event(AL_EVENT *dest, AL_CONST AL_EVENT *src)
{
   ASSERT(dest);
   ASSERT(src->any._size > 0);

   memcpy(dest, src, src->any._size);

   dest->any._refcount = 0;
   dest->any._next = NULL;
   dest->any._next_free = NULL;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
