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
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_vector.h"



static void shutdown_destructors(void);



typedef struct DTOR {
   void *object;
   void (*func)(void*);
} DTOR;



static _AL_MUTEX mutex = _AL_MUTEX_UNINITED;
static _AL_VECTOR dtors = _AL_VECTOR_INITIALIZER(DTOR);



/* Internal function: _al_init_destructors
 *  This is called from allegro_init() and nowhere else.  It adds an exit
 *  function using _add_exit_func.  When that exit func is called, all the
 *  registered destructors will be called in reverse order to the order in
 *  which they were registered.
 */
void _al_init_destructors(void)
{
   _al_mutex_init(&mutex);

   _add_exit_func(shutdown_destructors, "shutdown_destructors");
}



/* shutdown_destructors:
 *  The function that is called on exit by the _add_exit_func() mechanism.
 */
static void shutdown_destructors(void)
{
   //TRACE("shutdown_destructors called\n");

   /* call the destructors in reverse order */
   _al_mutex_lock(&mutex);
   {
      while (!_al_vector_is_empty(&dtors))
      {
         DTOR *dtor = _al_vector_ref_back(&dtors);

         TRACE("calling dtor for object %p, func %p\n", dtor->object, dtor->func);
         _al_mutex_unlock(&mutex);
         {
            (*dtor->func)(dtor->object);
         }
         _al_mutex_lock(&mutex);
      }
   }
   _al_mutex_unlock(&mutex);

   /* free resources used by the destructor subsystem */
   ASSERT(_al_vector_size(&dtors) == 0);
   _al_vector_free(&dtors);

   _al_mutex_destroy(&mutex);

   /* this is an _add_exit_func'd function */
   _remove_exit_func(shutdown_destructors);
}



/* Internal function: _al_register_destructor
 *  Register OBJECT to be destroyed by FUNC during allegro_exit().
 *  This would be done in the object's constructor function.
 *
 *  [thread-safe]
 */
void _al_register_destructor(void *object, void (*func)(void*))
{
   ASSERT(object);
   ASSERT(func);

   _al_mutex_lock(&mutex);
   {
#ifdef DEBUGMODE
      /* make sure the object is not registered twice */
      {
         unsigned int i;

         for (i = 0; i < _al_vector_size(&dtors); i++) {
            DTOR *dtor = _al_vector_ref(&dtors, i);
            ASSERT(dtor->object != object);
         }
      }
#endif /* DEBUGMODE */

      /* add the destructor to the list */
      {
         DTOR *new_dtor = _al_vector_alloc_back(&dtors);
         if (new_dtor) {
            new_dtor->object = object;
            new_dtor->func = func;
         }
      }
   }
   _al_mutex_unlock(&mutex);
}



/* Internal function: _al_unregister_destructor
 *  Unregister a previously registered object.  This must be called
 *  in the normal object destroyer routine, e.g. al_uninstall_timer.
 *
 *  [thread-safe]
 */
void _al_unregister_destructor(void *object)
{
   ASSERT(object);

   _al_mutex_lock(&mutex);
   {
      unsigned int i;
      bool found = false;

      for (i = 0; i < _al_vector_size(&dtors); i++) {
         DTOR *dtor = _al_vector_ref(&dtors, i);
         if (dtor->object == object) {
            _al_vector_delete_at(&dtors, i);
            found = true;
            break;
         }
      }

      ASSERT(found);
   }
   _al_mutex_unlock(&mutex);
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
