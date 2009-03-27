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


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_vector.h"

ALLEGRO_DEBUG_CHANNEL("system")


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
   _AL_DTOR_LIST *dtors = _AL_MALLOC(sizeof(*dtors));

   _AL_MARK_MUTEX_UNINITED(dtors->mutex);
   _al_mutex_init(&dtors->mutex);
   _al_vector_init(&dtors->dtors, sizeof(DTOR));

   return dtors;
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

         ALLEGRO_INFO("calling dtor for object %p, func %p\n", dtor->object, dtor->func);
         _al_mutex_unlock(&dtors->mutex);
         {
            (*dtor->func)(dtor->object);
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

   _AL_FREE(dtors);
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
   ASSERT(object);
   ASSERT(func);

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
         }
      }
   }
   _al_mutex_unlock(&dtors->mutex);
}



/* Internal function: _al_unregister_destructor
 *  Unregister a previously registered object.  This must be called
 *  in the normal object destroyer routine, e.g. al_uninstall_timer.
 *
 *  [thread-safe]
 */
void _al_unregister_destructor(_AL_DTOR_LIST *dtors, void *object)
{
   ASSERT(object);

   _al_mutex_lock(&dtors->mutex);
   {
      unsigned int i;
      bool found = false;

      for (i = 0; i < _al_vector_size(&dtors->dtors); i++) {
         DTOR *dtor = _al_vector_ref(&dtors->dtors, i);
         if (dtor->object == object) {
            _al_vector_delete_at(&dtors->dtors, i);
            found = true;
            break;
         }
      }

      ASSERT(found);
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
