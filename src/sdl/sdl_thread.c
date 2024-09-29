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
 *      SDL thread support.
 *
 *      See LICENSE.txt for copyright information.
 */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/platform/allegro_internal_sdl.h"

A5O_DEBUG_CHANNEL("thread")

static int thread_trampoline(void* data)
{
   _AL_THREAD *thread = data;
   (*thread->proc)(thread, thread->arg);
   return 0;
}

void _al_thread_create(_AL_THREAD *thread, void (*proc)(_AL_THREAD*, void*),
   void *arg)
{
   ASSERT(thread);
   ASSERT(proc);
   thread->should_stop = false;
   thread->proc = proc;
   thread->arg = arg;
   thread->thread = SDL_CreateThread(thread_trampoline, "allegro", thread);
}

void _al_thread_create_with_stacksize(_AL_THREAD *thread, void (*proc)(_AL_THREAD*, void*),
   void *arg, size_t stacksize)
{
   ASSERT(thread);
   ASSERT(proc);
   thread->should_stop = false;
   thread->proc = proc;
   thread->arg = arg;
#if SDL_VERSION_ATLEAST(2,0,9)
   thread->thread = SDL_CreateThreadWithStackSize(thread_trampoline, "allegro", stacksize, thread);
#else
   (void)stacksize;
   A5O_WARN("Creating a thread with a custom thread size is not supported "
      "on this version of SDL, it is too old.\n");
   thread->thread = SDL_CreateThread(thread_trampoline, "allegro", thread);
#endif
}

void _al_thread_set_should_stop(_AL_THREAD *thread)
{
   ASSERT(thread);
   thread->should_stop = true;
}

void _al_thread_join(_AL_THREAD *thread)
{
   ASSERT(thread);
   _al_thread_set_should_stop(thread);
   int r;
   SDL_WaitThread(thread->thread, &r);
}

void _al_thread_detach(_AL_THREAD *thread)
{
   ASSERT(thread);
   SDL_DetachThread(thread->thread);
}

/* mutexes */

void _al_mutex_init(_AL_MUTEX *mutex)
{
   ASSERT(mutex);
    
   mutex->mutex = SDL_CreateMutex();
}

void _al_mutex_init_recursive(_AL_MUTEX *mutex)
{
   _al_mutex_init(mutex);
}

void _al_mutex_destroy(_AL_MUTEX *mutex)
{
   ASSERT(mutex);

   if (mutex->mutex) {
      SDL_DestroyMutex(mutex->mutex);
      mutex->mutex = NULL;
   }
}

/* condition variables */
/* most of the condition variable implementation is actually inline */

int _al_cond_timedwait(_AL_COND *cond, _AL_MUTEX *mutex,
   const A5O_TIMEOUT *timeout)
{
   A5O_TIMEOUT_SDL *timeout_sdl = (void *)timeout;
   int r = SDL_CondWaitTimeout(cond->cond, mutex->mutex, timeout_sdl->ms);

   return (r == SDL_MUTEX_TIMEDOUT) ? -1 : 0;
}
