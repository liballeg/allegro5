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
 *	Destructors (a list of objects to destroy when Allegro shuts down).
 *
 *	By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER



static void shutdown_destructors(void);



typedef struct DTOR {
   void *object;
   void (*func)(void*);
} DTOR;



static _AL_MUTEX mutex = _AL_MUTEX_UNINITED;
static _AL_VECTOR dtors = _AL_VECTOR_INITIALIZER(DTOR);



void _al_init_destructors(void)
{
   _al_mutex_init(&mutex);

   _add_exit_func(shutdown_destructors);
}



static void shutdown_destructors(void)
{
   TRACE("shutdown_destructors called\n");

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



void _al_register_destructor(void *object, void (*func)(void*))
{
   ASSERT(object);
   ASSERT(func);

   _al_mutex_lock(&mutex);
   {
      /* make sure the object is not registered twice */
      {
         unsigned int i;

         for (i = 0; i < _al_vector_size(&dtors); i++) {
            DTOR *dtor = _al_vector_ref(&dtors, i);
            ASSERT(dtor->object != object);
         }
      }
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
