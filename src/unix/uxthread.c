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
 *        Internal cross-platform threading API for Unix.
 *
 *        By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#define _XOPEN_SOURCE 500       /* for Unix98 recursive mutexes */
                                /* XXX: added configure test */

#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/platform/aintunix.h"

#ifdef ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif



/* threads */

static void *thread_proc_trampoline(void *data)
{
   _AL_THREAD *thread = data;
   /* Android is special and needs to attach/detach threads with Java */
#ifdef ALLEGRO_ANDROID
   _al_android_thread_created();
#endif
   (*thread->proc)(thread, thread->arg);
#ifdef ALLEGRO_ANDROID
   _al_android_thread_ended();
#endif
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


void _al_thread_create_with_stacksize(_AL_THREAD* thread, void (*proc)(_AL_THREAD*, void*), void *arg, size_t stacksize)
{
#ifndef __GNU__
   ASSERT(stacksize >= PTHREAD_STACK_MIN);
#endif
   ASSERT(thread);
   ASSERT(proc);
   {
      int status;

      pthread_mutex_init(&thread->mutex, NULL);

      thread->should_stop = false;
      thread->proc = proc;
      thread->arg = arg;

      pthread_attr_t thread_attr;
      int result = 0;
      result = pthread_attr_init(&thread_attr);
      ASSERT(result == 0);

      // On some systems, pthread_attr_setstacksize() can fail
      // if stacksize is not a multiple of the system page size.
      result = pthread_attr_setstacksize(&thread_attr, stacksize);
      ASSERT(result == 0);

      status = pthread_create(&thread->thread, &thread_attr, thread_proc_trampoline, thread);
      ASSERT(status == 0);
      if (status != 0)
         abort();
   }
}


void _al_thread_set_should_stop(_AL_THREAD *thread)
{
   ASSERT(thread);

   pthread_mutex_lock(&thread->mutex);
   {
      thread->should_stop = true;
   }
   pthread_mutex_unlock(&thread->mutex);
}



void _al_thread_join(_AL_THREAD *thread)
{
   ASSERT(thread);

   _al_thread_set_should_stop(thread);
   pthread_join(thread->thread, NULL);

   pthread_mutex_destroy(&thread->mutex);
}


void _al_thread_detach(_AL_THREAD *thread)
{
   ASSERT(thread);
   pthread_mutex_destroy(&thread->mutex);
   pthread_detach(thread->thread);
}


/* mutexes */

void _al_mutex_init(_AL_MUTEX *mutex)
{
   ASSERT(mutex);

   pthread_mutex_init(&mutex->mutex, NULL);
   mutex->inited = true;
}


void _al_mutex_init_recursive(_AL_MUTEX *mutex)
{
   pthread_mutexattr_t attr;

   ASSERT(mutex);

   pthread_mutexattr_init(&attr);
   if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) == EINVAL) {
      pthread_mutexattr_destroy(&attr);
      abort(); /* XXX */
   }

   pthread_mutex_init(&mutex->mutex, &attr);
   mutex->inited = true;

   pthread_mutexattr_destroy(&attr);
}


void _al_mutex_destroy(_AL_MUTEX *mutex)
{
   ASSERT(mutex);

   if (mutex->inited) {
      pthread_mutex_destroy(&mutex->mutex);
      mutex->inited = false;
   }
}


/* condition variables */
/* most of the condition variable implementation is actually inline */

int _al_cond_timedwait(_AL_COND *cond, _AL_MUTEX *mutex,
   const ALLEGRO_TIMEOUT *timeout)
{
   ALLEGRO_TIMEOUT_UNIX *unix_timeout = (ALLEGRO_TIMEOUT_UNIX *) timeout;
   int retcode;

   retcode = pthread_cond_timedwait(&cond->cond, &mutex->mutex,
      &unix_timeout->abstime);

   return (retcode == ETIMEDOUT) ? -1 : 0;
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
