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
 *      Internal cross-platform threading API for Windows.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


/* Native condition variables need Vista. */
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600
   #undef _WIN32_WINNT
   #define _WIN32_WINNT 0x0600
#endif

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"

#include <mmsystem.h>
#include <process.h>



/* threads */

static unsigned __stdcall thread_proc_trampoline(void *data)
{
   _AL_THREAD *thread = data;
   (*thread->proc)(thread, thread->arg);

   /* _endthreadex does not automatically close the thread handle,
    * unlike _endthread.  We rely on this in al_join_thread().
    */
   _endthreadex(0);
   return 0;
}


void _al_thread_create(_AL_THREAD *thread, void (*proc)(_AL_THREAD*, void*), void *arg)
{
   ASSERT(thread);
   ASSERT(proc);
   {
      InitializeCriticalSection(&thread->cs);

      thread->should_stop = false;
      thread->proc = proc;
      thread->arg = arg;

      thread->thread = (void *)_beginthreadex(NULL, 0,
         thread_proc_trampoline, thread, 0, NULL);
   }
}


void _al_thread_create_with_stacksize(_AL_THREAD* thread, void (*proc)(_AL_THREAD*, void*), void *arg, size_t stacksize)
{
   ASSERT(thread);
   ASSERT(proc);
   {
      InitializeCriticalSection(&thread->cs);

      thread->should_stop = false;
      thread->proc = proc;
      thread->arg = arg;

      thread->thread = (void *)_beginthreadex(NULL, stacksize,
         thread_proc_trampoline, thread, 0, NULL);
   }
}


void _al_thread_set_should_stop(_AL_THREAD *thread)
{
   ASSERT(thread);

   EnterCriticalSection(&thread->cs);
   {
      thread->should_stop = true;
   }
   LeaveCriticalSection(&thread->cs);
}



void _al_thread_join(_AL_THREAD *thread)
{
   ASSERT(thread);

   _al_thread_set_should_stop(thread);
   WaitForSingleObject(thread->thread, INFINITE);

   CloseHandle(thread->thread);
   DeleteCriticalSection(&thread->cs);
}


void _al_thread_detach(_AL_THREAD *thread)
{
   ASSERT(thread);

   CloseHandle(thread->thread);
   DeleteCriticalSection(&thread->cs);
}



/* mutexes */

void _al_mutex_init(_AL_MUTEX *mutex)
{
   ASSERT(mutex);

   if (!mutex->cs)
      mutex->cs = al_malloc(sizeof *mutex->cs);
   ASSERT(mutex->cs);
   if (mutex->cs)
      InitializeCriticalSection(mutex->cs);
   else
      abort();
}


void _al_mutex_init_recursive(_AL_MUTEX *mutex)
{
   /* These are the same on Windows. */
   _al_mutex_init(mutex);
}


void _al_mutex_destroy(_AL_MUTEX *mutex)
{
   ASSERT(mutex);

   if (mutex->cs) {
      DeleteCriticalSection(mutex->cs);
      al_free(mutex->cs);
      mutex->cs = NULL;
   }
}


/* condition variables */

/* These used to be emulated with a semaphore plus a CRITICAL_SECTION "gate"
 * (pthread-win32's algorithm 8a). But that algorithm needs a gate that one
 * thread can close and another can open, and a CRITICAL_SECTION is owned by
 * the thread that entered it: a signaller re-entering the gate recursively
 * while a woken waiter left it raced on the section's recursion count, and
 * could fault with STATUS_RESOURCE_NOT_OWNED. Native condition variables
 * (Vista and later) wait on the CRITICAL_SECTION of an ALLEGRO_MUTEX
 * directly. Like pthreads, they may wake spuriously, so callers must wait in
 * a loop - which all callers already do.
 */


void _al_cond_init(_AL_COND *cond)
{
   InitializeConditionVariable((PCONDITION_VARIABLE)&cond->cv);
}


void _al_cond_destroy(_AL_COND *cond)
{
   /* Native condition variables have nothing to free. */
   (void)cond;
}


/* returns -1 on timeout */
static int cond_wait(_AL_COND *cond, _AL_MUTEX *mtxExternal, DWORD timeout)
{
   ASSERT(mtxExternal->cs);

   if (!SleepConditionVariableCS((PCONDITION_VARIABLE)&cond->cv,
         mtxExternal->cs, timeout)) {
      ASSERT(GetLastError() == ERROR_TIMEOUT);
      return -1;
   }

   return 0;
}


void _al_cond_wait(_AL_COND *cond, _AL_MUTEX *mtxExternal)
{
   int result;

   ASSERT(cond);
   ASSERT(mtxExternal);

   result = cond_wait(cond, mtxExternal, INFINITE);
   ASSERT(result != -1);
   (void)result;
}


int _al_cond_timedwait(_AL_COND *cond, _AL_MUTEX *mtxExternal,
   const ALLEGRO_TIMEOUT *timeout)
{
   ALLEGRO_TIMEOUT_WIN *win_timeout = (ALLEGRO_TIMEOUT_WIN *) timeout;
   DWORD now;
   DWORD rel_msecs;

   ASSERT(cond);
   ASSERT(mtxExternal);

   now = timeGetTime();
   rel_msecs = win_timeout->abstime - now;
   if (rel_msecs > INT_MAX) {
      rel_msecs = 0;
   }

   return cond_wait(cond, mtxExternal, rel_msecs);
}


void _al_cond_broadcast(_AL_COND *cond)
{
   WakeAllConditionVariable((PCONDITION_VARIABLE)&cond->cv);
}


void _al_cond_signal(_AL_COND *cond)
{
   WakeConditionVariable((PCONDITION_VARIABLE)&cond->cv);
}


/* vim: set sts=3 sw=3 et: */
