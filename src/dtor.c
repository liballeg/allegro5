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
 *	Destructors.
 *
 *	By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Destructors
 *
 * This file records a list of objects created by the user and/or Allegro
 * itself, that need to be destroyed when Allegro is shut down.  Strictly
 * speaking, this list should not be necessary if the user is careful to
 * destroy all the objects he creates.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_tls.h"
#include "allegro5/internal/aintern_list.h"

/* XXX The dependency on tls.c is not nice but the DllMain stuff for Windows
 * does not it easy to make abstract away TLS API differences.
 */

A5O_DEBUG_CHANNEL("dtor")


struct _AL_DTOR_LIST {
   _AL_MUTEX mutex;
   _AL_LIST* dtors;
};


typedef struct DTOR {
   char const *name;
   void *object;
   void (*func)(void*);
} DTOR;


/* Internal function: _al_init_destructors
 *  Initialise a list of destructors.
 */
_AL_DTOR_LIST *_al_init_destructors(void)
{
   _AL_DTOR_LIST *dtors = al_malloc(sizeof(*dtors));

   _AL_MARK_MUTEX_UNINITED(dtors->mutex);
   _al_mutex_init(&dtors->mutex);
   dtors->dtors = _al_list_create();

   return dtors;
}



/* _al_push_destructor_owner:
 *  Increase the owner count for the current thread.  When it is greater than
 *  zero, _al_register_destructor will do nothing.
 *
 *  This is required if an object to-be-registered (B) should be "owned" by an
 *  object (A), which is responsible for destroying (B).  (B) should not be
 *  destroyed independently of (A).
 */
void _al_push_destructor_owner(void)
{
   int *dtor_owner_count = _al_tls_get_dtor_owner_count();
   (*dtor_owner_count)++;
}



/* _al_push_destructor_owner:
 *  Decrease the owner count for the current thread.
 */
void _al_pop_destructor_owner(void)
{
   int *dtor_owner_count = _al_tls_get_dtor_owner_count();
   (*dtor_owner_count)--;
   ASSERT(*dtor_owner_count >= 0);
}



/* _al_run_destructors:
 *  Run all the destructors on the list in reverse order.
 */
void _al_run_destructors(_AL_DTOR_LIST *dtors)
{
   if (!dtors) {
      return;
   }

   /* call the destructors in reverse order */
   _al_mutex_lock(&dtors->mutex);
   {
      _AL_LIST_ITEM *iter = _al_list_back(dtors->dtors);
      while (iter) {
         DTOR *dtor = _al_list_item_data(iter);
         void *object = dtor->object;
         void (*func)(void *) = dtor->func;

         A5O_DEBUG("calling dtor for %s %p, func %p\n",
            dtor->name, object, func);
         _al_mutex_unlock(&dtors->mutex);
         {
            (*func)(object);
         }
         _al_mutex_lock(&dtors->mutex);
         /* Don't do normal iteration as the destructors will possibly run
          * multiple destructors at once. */
         iter = _al_list_back(dtors->dtors);
      }
   }
   _al_mutex_unlock(&dtors->mutex);
}



/* _al_shutdown_destructors:
 *  Free the list of destructors. The list should be empty now.
 */
void _al_shutdown_destructors(_AL_DTOR_LIST *dtors)
{
   if (!dtors) {
      return;
   }

   /* free resources used by the destructor subsystem */
   ASSERT(_al_list_is_empty(dtors->dtors));
   _al_list_destroy(dtors->dtors);

   _al_mutex_destroy(&dtors->mutex);

   al_free(dtors);
}



/* Internal function: _al_register_destructor
 *  Register OBJECT to be destroyed by FUNC during Allegro shutdown.
 *  This would be done in the object's constructor function.
 *
 *  Returns a list item representing the destructor's position in the list
 *  (possibly null).
 *
 *  [thread-safe]
 */
_AL_LIST_ITEM *_al_register_destructor(_AL_DTOR_LIST *dtors, char const *name,
   void *object, void (*func)(void*))
{
   int *dtor_owner_count;
   _AL_LIST_ITEM *ret = NULL;
   ASSERT(object);
   ASSERT(func);

   dtor_owner_count = _al_tls_get_dtor_owner_count();
   if (*dtor_owner_count > 0)
      return NULL;

   _al_mutex_lock(&dtors->mutex);
   {
#ifdef DEBUGMODE
      /* make sure the object is not registered twice */
      {
         _AL_LIST_ITEM *iter = _al_list_front(dtors->dtors);

         while (iter) {
            DTOR *dtor = _al_list_item_data(iter);
            ASSERT(dtor->object != object);
            iter = _al_list_next(dtors->dtors, iter);
         }
      }
#endif /* DEBUGMODE */

      /* add the destructor to the list */
      {
         DTOR *new_dtor = al_malloc(sizeof(DTOR));
         if (new_dtor) {
            new_dtor->object = object;
            new_dtor->func = func;
            new_dtor->name = name;
            A5O_DEBUG("added dtor for %s %p, func %p\n", name,
               object, func);
            ret = _al_list_push_back(dtors->dtors, new_dtor);
         }
         else {
            A5O_WARN("failed to add dtor for %s %p\n", name,
               object);
         }
      }
   }
   _al_mutex_unlock(&dtors->mutex);
   return ret;
}



/* Internal function: _al_unregister_destructor
 *  Unregister a previously registered object.  This must be called
 *  in the normal object destroyer routine, e.g. al_destroy_timer.
 *
 *  [thread-safe]
 */
void _al_unregister_destructor(_AL_DTOR_LIST *dtors, _AL_LIST_ITEM *dtor_item)
{
   if (!dtor_item) {
      return;
   }

   _al_mutex_lock(&dtors->mutex);
   {
      DTOR *dtor = _al_list_item_data(dtor_item);
      A5O_DEBUG("removed dtor for %s %p\n", dtor->name, dtor->object);
      al_free(dtor);
      _al_list_erase(dtors->dtors, dtor_item);
   }
   _al_mutex_unlock(&dtors->mutex);
}



/* Internal function: _al_foreach_destructor
 *  Call the callback for each registered object.
 *  [thread-safe]
 */
void _al_foreach_destructor(_AL_DTOR_LIST *dtors,
   void (*callback)(void *object, void (*func)(void *), void *udata),
   void *userdata)
{
   _al_mutex_lock(&dtors->mutex);
   {
      _AL_LIST_ITEM *iter = _al_list_front(dtors->dtors);

      while (iter) {
         DTOR *dtor = _al_list_item_data(iter);
         callback(dtor->object, dtor->func, userdata);
         iter = _al_list_next(dtors->dtors, iter);
      }
   }
   _al_mutex_unlock(&dtors->mutex);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
