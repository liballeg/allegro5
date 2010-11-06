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
#include "allegro5/internal/aintern_vector.h"

/* XXX The dependency on tls.c is not nice but the DllMain stuff for Windows
 * does not it easy to make abstract away TLS API differences.
 */

ALLEGRO_DEBUG_CHANNEL("dtor")


struct _AL_DTOR_LIST {
   _AL_MUTEX mutex;
   _AL_VECTOR dtors;
};


typedef struct DTOR {
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
   _al_vector_init(&dtors->dtors, sizeof(DTOR));

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
      while (!_al_vector_is_empty(&dtors->dtors)) {
         DTOR *dtor = _al_vector_ref_back(&dtors->dtors);
         void *object = dtor->object;
         void (*func)(void *) = dtor->func;

         ALLEGRO_DEBUG("calling dtor for object %p, func %p\n", object, func);
         _al_mutex_unlock(&dtors->mutex);
         {
            (*func)(object);
         }
         _al_mutex_lock(&dtors->mutex);
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
   ASSERT(_al_vector_size(&dtors->dtors) == 0);
   _al_vector_free(&dtors->dtors);

   _al_mutex_destroy(&dtors->mutex);

   al_free(dtors);
}



/* Internal function: _al_register_destructor
 *  Register OBJECT to be destroyed by FUNC during Allegro shutdown.
 *  This would be done in the object's constructor function.
 *
 *  [thread-safe]
 */
void _al_register_destructor(_AL_DTOR_LIST *dtors, void *object,
   void (*func)(void*))
{
   int *dtor_owner_count;
   ASSERT(object);
   ASSERT(func);

   dtor_owner_count = _al_tls_get_dtor_owner_count();
   if (*dtor_owner_count > 0)
      return;

   _al_mutex_lock(&dtors->mutex);
   {
#ifdef DEBUGMODE
      /* make sure the object is not registered twice */
      {
         unsigned int i;

         for (i = 0; i < _al_vector_size(&dtors->dtors); i++) {
            DTOR *dtor = _al_vector_ref(&dtors->dtors, i);
            ASSERT(dtor->object != object);
         }
      }
#endif /* DEBUGMODE */

      /* add the destructor to the list */
      {
         DTOR *new_dtor = _al_vector_alloc_back(&dtors->dtors);
         if (new_dtor) {
            new_dtor->object = object;
            new_dtor->func = func;
            ALLEGRO_DEBUG("added dtor for object %p, func %p\n", object, func);
         }
         else {
            ALLEGRO_WARN("failed to add dtor for object %p\n", object);
         }
      }
   }
   _al_mutex_unlock(&dtors->mutex);
}



/* Internal function: _al_unregister_destructor
 *  Unregister a previously registered object.  This must be called
 *  in the normal object destroyer routine, e.g. al_destroy_timer.
 *
 *  [thread-safe]
 */
void _al_unregister_destructor(_AL_DTOR_LIST *dtors, void *object)
{
   ASSERT(object);

   _al_mutex_lock(&dtors->mutex);
   {
      unsigned int i;

      for (i = 0; i < _al_vector_size(&dtors->dtors); i++) {
         DTOR *dtor = _al_vector_ref(&dtors->dtors, i);
         if (dtor->object == object) {
            _al_vector_delete_at(&dtors->dtors, i);
            ALLEGRO_DEBUG("removed dtor for object %p\n", object);
            break;
         }
      }

      /* We cannot assert that the destructor was found because it might not
       * have been registered if the owner count was non-zero at the time.
       */
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
      unsigned int i;

      for (i = 0; i < _al_vector_size(&dtors->dtors); i++) {
         DTOR *dtor = _al_vector_ref(&dtors->dtors, i);
         callback(dtor->object, dtor->func, userdata);
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
