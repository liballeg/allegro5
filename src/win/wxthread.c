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
 *	Internal cross-platform threading API for Windows.
 *
 *	By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER

#ifndef SCAN_DEPEND
   #include <process.h>
#endif



/* threads */

static void thread_proc_trampoline(void *data)
{
   _AL_THREAD *thread = data;
   (*thread->proc)(thread, thread->arg);
}


void _al_thread_create(_AL_THREAD *thread, void (*proc)(_AL_THREAD*, void*), void *arg)
{
   ASSERT(thread);
   ASSERT(proc);
   {
      int status;

      InitializeCriticalSection(&thread->cs);

      thread->should_stop = false;
      thread->proc = proc;
      thread->arg = arg;

      thread->thread = (void *)_beginthread(thread_proc_trampoline, 0, thread);
   }
}


void _al_thread_join(_AL_THREAD *thread)
{
   ASSERT(thread);

   EnterCriticalSection(&thread->cs);
   {
      thread->should_stop = true;
   }
   LeaveCriticalSection(&thread->cs);
   WaitForSingleObject(thread->thread, INFINITE);

   DeleteCriticalSection(&thread->cs);
}


/* mutexes */

void _al_mutex_init(_AL_MUTEX *mutex)
{
   ASSERT(mutex);

   InitializeCriticalSection(&mutex->cs);
   mutex->inited = true;
}


void _al_mutex_destroy(_AL_MUTEX *mutex)
{
   ASSERT(mutex);
   ASSERT(mutex->inited);

   DeleteCriticalSection(&mutex->cs);
   mutex->inited = false;
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
