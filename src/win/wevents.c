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
 *      Windows event queues.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER



#define INITIAL_QUEUE_SIZE      32
#define MAX_QUEUE_SIZE          512 /* power of 2 for max use of _AL_VECTOR */


struct AL_EVENT_QUEUE
{
   _AL_VECTOR events; /* XXX: better a deque */
   _AL_VECTOR sources;
   _AL_MUTEX mutex;
   HANDLE semaphore;
};



/* al_create_event_queue: [primary thread]
 */
AL_EVENT_QUEUE *al_create_event_queue(void)
{
   AL_EVENT_QUEUE *queue = malloc(sizeof *queue);

   ASSERT(queue);

   if (queue) {
      _al_vector_init(&queue->events, sizeof(AL_EVENT *));
      _al_vector_init(&queue->sources, sizeof(AL_EVENT_SOURCE *));

      _al_mutex_init(&queue->mutex);

      /* Poor man's condition variable:
       *
       * Initially, there are 0 events, corresponding to a count of 0 in
       * the semaphore.  When an event is enqueued, the semaphore's count
       * is increased by 1 using ReleaseSemaphore().  When the user calls
       * al_wait_for_event() on the queue, we can call
       * WaitForSingleObject() so that the user's thread will block until
       * the semaphore's count is not zero.  WaitForSingleObject()
       * decrements the count when it returns.
       *
       * Note that this requires us to synchronise the semaphore count
       * with the number of events in the queue, even if the user uses
       * al_get_next_event() instead of al_wait_for_event.
       *
       * Note further that this strategy won't work if two or more
       * threads are trying to pull events out of the queue
       * simultaneously.  As yet, our API simply doesn't allow that.  We
       * would need proper condition variables, I think, which Win32
       * doesn't provide, and are extremely hairy to implement using
       * Win32 synchronisation primitives.
       */

      queue->semaphore = CreateSemaphore(NULL, 0, INT_MAX, NULL);

      _al_register_destructor(queue, (void (*)(void *)) al_destroy_event_queue);
   }

   return queue;
}



/* al_destroy_event_queue: [primary thread]
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

   _al_mutex_destroy(&queue->mutex);
   CloseHandle(queue->semaphore);

   free(queue);
}



/* al_register_event_source: [primary thread]
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
 */
bool al_event_queue_is_empty(AL_EVENT_QUEUE *queue)
{
   ASSERT(queue);

   return _al_vector_is_empty(&queue->events);
}



/* wait_and_get_event: [primary thread]
 *  Does the real work of al_get_next_event and al_wait_for_event.
 */
static bool wait_and_get_event(AL_EVENT_QUEUE *queue, AL_EVENT *ret_event, DWORD timeout_msecs)
{
   DWORD status = WaitForSingleObject(queue->semaphore, timeout_msecs);

   switch (status) {

   case WAIT_OBJECT_0: {
      AL_EVENT *event = NULL;

      _al_mutex_lock(&queue->mutex);
      {
         if (_al_vector_is_nonempty(&queue->events)) {
            AL_EVENT **slot = _al_vector_ref_front(&queue->events);
            event = *slot;
            _al_vector_delete_at(&queue->events, 0); /* inefficient */
         }
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

   default:
      ASSERT(status == WAIT_TIMEOUT);
      return false;
   }
}



/*  al_get_next_event: [primary thread]
 */
bool al_get_next_event(AL_EVENT_QUEUE *queue, AL_EVENT *ret_event)
{
   ASSERT(queue);
   ASSERT(ret_event);

   return wait_and_get_event(queue, ret_event, 0);
}



/* al_peek_next_event: [primary thread]
 *  Copy the first event in the queue into RET_EVENT and return
 *  true.  The first event must NOT be removed from the queue.  If
 *  the queue is initially empty, return false.
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
 *  Remove the first event from the queue, if there is one.
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



/*  al_wait_for_event: [primary thread]
 */
bool al_wait_for_event(AL_EVENT_QUEUE *queue, AL_EVENT *ret_event, long msecs)
{
   ASSERT(queue);
   ASSERT(ret_event);
   ASSERT(msecs >= 0);

   return wait_and_get_event(queue, ret_event, (msecs == 0) ? INFINITE : (unsigned long)msecs);
}



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

	 ReleaseSemaphore(queue->semaphore, 1, NULL);
      }
   }
   _al_mutex_unlock(&queue->mutex);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
