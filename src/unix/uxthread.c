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
 *	Internal cross-platform threading API for Unix.
 *
 *	By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER



/* threads */

static void *thread_proc_trampoline(void *data)
{
   _AL_THREAD *thread = data;
   (*thread->proc)(thread, thread->arg);
   return NULL;
}


void _al_thread_create(_AL_THREAD *thread, void (*proc)(_AL_THREAD*, void*), void *arg)
{
   ASSERT(thread);
   ASSERT(proc);
   {
      int status;

      pthread_mutex_init(&thread->mutex, NULL);

      thread->should_stop = false;
      thread->proc = proc;
      thread->arg = arg;

      status = pthread_create(&thread->thread, NULL, thread_proc_trampoline, thread);
      ASSERT(status == 0);
      if (status != 0)
	 abort();
   }
}


void _al_thread_join(_AL_THREAD *thread)
{
   ASSERT(thread);

   pthread_mutex_lock(&thread->mutex);
   {
      thread->should_stop = true;
   }
   pthread_mutex_unlock(&thread->mutex);
   pthread_join(thread->thread, NULL);

   pthread_mutex_destroy(&thread->mutex);
}


/* mutexes */

void _al_mutex_init(_AL_MUTEX *mutex)
{
   ASSERT(mutex);
    
   pthread_mutex_init(&mutex->mutex, NULL);
   mutex->inited = true;
}


void _al_mutex_destroy(_AL_MUTEX *mutex)
{
   ASSERT(mutex);
   ASSERT(mutex->inited);

   pthread_mutex_destroy(&mutex->mutex);
   mutex->inited = false;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
