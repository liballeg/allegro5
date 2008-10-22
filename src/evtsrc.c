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
 *      Event sources.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Event sources
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_memory.h"



/* Internal function: _al_event_source_init
 *  Initialise an event source structure.
 */
void _al_event_source_init(ALLEGRO_EVENT_SOURCE *this)
{
   _AL_MARK_MUTEX_UNINITED(this->mutex);
   _al_mutex_init(&this->mutex);
   _al_vector_init(&this->queues, sizeof(ALLEGRO_EVENT_QUEUE *));
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
      }
   }
}



/* Function: al_create_user_event_source
 *  Allocate an event source for emitting user events.
 */
ALLEGRO_EVENT_SOURCE *al_create_user_event_source(void)
{
   ALLEGRO_EVENT_SOURCE *src;

   src = _AL_MALLOC(sizeof(*src));
   if (src) {
      _al_event_source_init(src);
      _al_register_destructor(src,
         (void (*)(void *)) al_destroy_user_event_source);
   }
   return src;
}



/* Function: al_destroy_user_event_source
 *  Destroy an event source created with <al_create_user_event_source>.
 */
void al_destroy_user_event_source(ALLEGRO_EVENT_SOURCE *src)
{
   if (src) {
      _al_unregister_destructor(src);
      _al_event_source_free(src);
      _AL_FREE(src);
   }
}



/* Function: al_emit_user_event
 *  Emit a user event.
 *  The event source must have been created with <al_create_user_event_source>.
 *  Some fields of the event being passed in may be modified.
 *  Returns `false' if the event source isn't registered with any queues,
 *  hence the event wouldn't have been delivered into any queues.
 *
 *  Reference counting will be performed on the event if `dtor' is non-NULL.
 *  When the reference count drops to zero `dtor' will be called with a copy of
 *  the event as an argument.  It should free the resources associated with
 *  the event.  If `dtor' is NULL then reference counting will not be
 *  performed.
 *
 *  You need to call <al_unref_user_event> when you are done with a reference
 *  counted user event that you have gotten from <al_get_next_event>,
 *  <al_peek_next_event>, <al_wait_for_event>, etc.  You may, but do not need
 *  to, call <al_unref_user_event> on non-reference counted user events.
 */
bool al_emit_user_event(ALLEGRO_EVENT_SOURCE *src,
   ALLEGRO_EVENT *event, void (*dtor)(ALLEGRO_USER_EVENT *))
{
   size_t num_queues;
   bool rc;

   ASSERT(src);
   ASSERT(event);
   ASSERT(ALLEGRO_EVENT_TYPE_IS_USER(event->any.type));

   if (dtor) {
      ALLEGRO_USER_EVENT_DESCRIPTOR *descr = _AL_MALLOC(sizeof(*descr));
      descr->refcount = 0;
      descr->dtor = dtor;
      event->user.__internal__descr = descr;
   }
   else {
      event->user.__internal__descr = NULL;
   }

   _al_event_source_lock(src);
   {
      num_queues = _al_vector_size(&src->queues);
      if (num_queues > 0) {
         event->user.timestamp = al_current_time();
         _al_event_source_emit_event(src, event);
         rc = true;
      }
      else {
         rc = false;
      }
   }
   _al_event_source_unlock(src);

   if (dtor && !rc) {
      dtor(&event->user);
      _AL_FREE(event->user.__internal__descr);
   }

   return rc;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
