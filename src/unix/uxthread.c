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


#define _XOPEN_SOURCE 500       /* for Unix98 recursive mutexes */
                                /* XXX: added configure test */

#include <sys/time.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"



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
   ASSERT(mutex->inited);

   pthread_mutex_destroy(&mutex->mutex);
   mutex->inited = false;
}


/* condition variables */
/* most of the condition variable implementation is actually inline */

int _al_cond_timedwait(_AL_COND *cond, _AL_MUTEX *mutex, unsigned long abstime)
{
   long msecs;
   struct timeval now;
   struct timespec timeout;
   int retcode = 0;

   /* FIXME: It is rather stupid that abstime is referenced to
    * al_current_time() base.  A different interface is needed, which
    * takes into account that win32 uses relative timeouts for
    * WaitFor*, but relative timeouts are bad for _al_cond_timedwait().
    */
   
   msecs = abstime - ALLEGRO_SECS_TO_MSECS(al_current_time());
   if (msecs < 0)
      return -1;

   gettimeofday(&now, NULL);
   timeout.tv_sec = now.tv_sec + (msecs / 1000);
   timeout.tv_nsec = (now.tv_usec + (msecs % 1000) * 1000) * 1000;
   timeout.tv_sec += timeout.tv_nsec / 1000000000L;
   timeout.tv_nsec = timeout.tv_nsec % 1000000000L;

   retcode = pthread_cond_timedwait(&cond->cond, &mutex->mutex, &timeout);

   return (retcode == ETIMEDOUT) ? -1 : 0;
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
