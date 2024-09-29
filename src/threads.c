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
 *      Public threads interface.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_system.h"



typedef enum THREAD_STATE {
   THREAD_STATE_CREATED,   /* -> starting or -> joining */
   THREAD_STATE_STARTING,  /* -> started */
   THREAD_STATE_STARTED,   /* -> joining */
   THREAD_STATE_JOINING,   /* -> joined */
   THREAD_STATE_JOINED,    /* -> destroyed */
   THREAD_STATE_DESTROYED,
   THREAD_STATE_DETACHED
} THREAD_STATE;


struct A5O_THREAD {
   _AL_THREAD thread;
   _AL_MUTEX mutex;
   _AL_COND cond;
   THREAD_STATE thread_state;
   void *proc;
   void *arg;
   void *retval;
};


struct A5O_MUTEX {
   _AL_MUTEX mutex;
};


struct A5O_COND {
   _AL_COND cond;
};


static void thread_func_trampoline(_AL_THREAD *inner, void *_outer)
{
   A5O_THREAD *outer = (A5O_THREAD *) _outer;
   A5O_SYSTEM *system = al_get_system_driver();
   (void)inner;

   if (system && system->vt && system->vt->thread_init) {
      system->vt->thread_init(outer);
   }

   /* Wait to start the actual user thread function.  The thread could also be
    * destroyed before ever running the user function.
    */
   _al_mutex_lock(&outer->mutex);
   while (outer->thread_state == THREAD_STATE_CREATED) {
      _al_cond_wait(&outer->cond, &outer->mutex);
   }
   _al_mutex_unlock(&outer->mutex);

   if (outer->thread_state == THREAD_STATE_STARTING) {
      outer->thread_state = THREAD_STATE_STARTED;
      outer->retval =
         ((void *(*)(A5O_THREAD *, void *))outer->proc)(outer, outer->arg);
   }

   if (system && system->vt && system->vt->thread_exit) {
      system->vt->thread_exit(outer);
   }
}


static void detached_thread_func_trampoline(_AL_THREAD *inner, void *_outer)
{
   A5O_THREAD *outer = (A5O_THREAD *) _outer;
   (void)inner;

   ((void *(*)(void *))outer->proc)(outer->arg);
   al_free(outer);
}


static A5O_THREAD *create_thread(void)
{
   A5O_THREAD *outer;

   outer = al_malloc(sizeof(*outer));
   if (!outer) {
      return NULL;
   }
   _AL_MARK_MUTEX_UNINITED(outer->mutex); /* required */
   outer->retval = NULL;
   return outer;
}


/* Function: al_create_thread
 */
A5O_THREAD *al_create_thread(
   void *(*proc)(A5O_THREAD *thread, void *arg), void *arg)
{
   A5O_THREAD *outer = create_thread();
   outer->thread_state = THREAD_STATE_CREATED;
   _al_mutex_init(&outer->mutex);
   _al_cond_init(&outer->cond);
   outer->arg = arg;
   outer->proc = proc;
   _al_thread_create(&outer->thread, thread_func_trampoline, outer);
   /* XXX _al_thread_create should return an error code */
   return outer;
}

/* Function: al_create_thread_with_stacksize
 */
A5O_THREAD *al_create_thread_with_stacksize(
   void *(*proc)(A5O_THREAD *thread, void *arg), void *arg, size_t stacksize)
{
   A5O_THREAD *outer = create_thread();
   outer->thread_state = THREAD_STATE_CREATED;
   _al_mutex_init(&outer->mutex);
   _al_cond_init(&outer->cond);
   outer->arg = arg;
   outer->proc = proc;
   _al_thread_create_with_stacksize(&outer->thread, thread_func_trampoline, outer, stacksize);
   /* XXX _al_thread_create should return an error code */
   return outer;
}

/* Function: al_run_detached_thread
 */
void al_run_detached_thread(void *(*proc)(void *arg), void *arg)
{
   A5O_THREAD *outer = create_thread();
   outer->thread_state = THREAD_STATE_DETACHED;
   outer->arg = arg;
   outer->proc = proc;
   _al_thread_create(&outer->thread, detached_thread_func_trampoline, outer);
   _al_thread_detach(&outer->thread);
}


/* Function: al_start_thread
 */
void al_start_thread(A5O_THREAD *thread)
{
   ASSERT(thread);

   switch (thread->thread_state) {
      case THREAD_STATE_CREATED:
         _al_mutex_lock(&thread->mutex);
         thread->thread_state = THREAD_STATE_STARTING;
         _al_cond_broadcast(&thread->cond);
         _al_mutex_unlock(&thread->mutex);
         break;
      case THREAD_STATE_STARTING:
         break;
      case THREAD_STATE_STARTED:
         break;
      /* invalid cases */
      case THREAD_STATE_JOINING:
         ASSERT(thread->thread_state != THREAD_STATE_JOINING);
         break;
      case THREAD_STATE_JOINED:
         ASSERT(thread->thread_state != THREAD_STATE_JOINED);
         break;
      case THREAD_STATE_DESTROYED:
         ASSERT(thread->thread_state != THREAD_STATE_DESTROYED);
         break;
      case THREAD_STATE_DETACHED:
         ASSERT(thread->thread_state != THREAD_STATE_DETACHED);
         break;
   }
}


/* Function: al_join_thread
 */
void al_join_thread(A5O_THREAD *thread, void **ret_value)
{
   ASSERT(thread);

   /* If al_join_thread() is called soon after al_start_thread(), the thread
    * function may not yet have noticed the STARTING state and executed the
    * user's thread function.  Hence we must wait until the thread enters the
    * STARTED state.
    */
   while (thread->thread_state == THREAD_STATE_STARTING) {
      al_rest(0.001);
   }

   switch (thread->thread_state) {
      case THREAD_STATE_CREATED: /* fall through */
      case THREAD_STATE_STARTED:
         _al_mutex_lock(&thread->mutex);
         thread->thread_state = THREAD_STATE_JOINING;
         _al_cond_broadcast(&thread->cond);
         _al_mutex_unlock(&thread->mutex);
         _al_cond_destroy(&thread->cond);
         _al_mutex_destroy(&thread->mutex);
         _al_thread_join(&thread->thread);
         thread->thread_state = THREAD_STATE_JOINED;
         break;
      case THREAD_STATE_STARTING:
         ASSERT(thread->thread_state != THREAD_STATE_STARTING);
         break;
      case THREAD_STATE_JOINING:
         ASSERT(thread->thread_state != THREAD_STATE_JOINING);
         break;
      case THREAD_STATE_JOINED:
         ASSERT(thread->thread_state != THREAD_STATE_JOINED);
         break;
      case THREAD_STATE_DESTROYED:
         ASSERT(thread->thread_state != THREAD_STATE_DESTROYED);
         break;
      case THREAD_STATE_DETACHED:
         ASSERT(thread->thread_state != THREAD_STATE_DETACHED);
         break;
   }

   if (ret_value) {
      *ret_value = thread->retval;
   }
}


/* Function: al_set_thread_should_stop
 */
void al_set_thread_should_stop(A5O_THREAD *thread)
{
   ASSERT(thread);
   _al_thread_set_should_stop(&thread->thread);
}


/* Function: al_get_thread_should_stop
 */
bool al_get_thread_should_stop(A5O_THREAD *thread)
{
   ASSERT(thread);
   return _al_get_thread_should_stop(&thread->thread);
}


/* Function: al_destroy_thread
 */
void al_destroy_thread(A5O_THREAD *thread)
{
   if (!thread) {
      return;
   }

   /* Join if required. */
   switch (thread->thread_state) {
      case THREAD_STATE_CREATED: /* fall through */
      case THREAD_STATE_STARTING: /* fall through */
      case THREAD_STATE_STARTED:
         al_join_thread(thread, NULL);
         break;
      case THREAD_STATE_JOINING:
         ASSERT(thread->thread_state != THREAD_STATE_JOINING);
         break;
      case THREAD_STATE_JOINED:
         break;
      case THREAD_STATE_DESTROYED:
         ASSERT(thread->thread_state != THREAD_STATE_DESTROYED);
         break;
      case THREAD_STATE_DETACHED:
         ASSERT(thread->thread_state != THREAD_STATE_DETACHED);
         break;
   }

   /* May help debugging. */
   thread->thread_state = THREAD_STATE_DESTROYED;
   al_free(thread);
}


/* Function: al_create_mutex
 */
A5O_MUTEX *al_create_mutex(void)
{
   A5O_MUTEX *mutex = al_malloc(sizeof(*mutex));
   if (mutex) {
      _AL_MARK_MUTEX_UNINITED(mutex->mutex);
      _al_mutex_init(&mutex->mutex);
   }
   return mutex;
}


/* Function: al_create_mutex_recursive
 */
A5O_MUTEX *al_create_mutex_recursive(void)
{
   A5O_MUTEX *mutex = al_malloc(sizeof(*mutex));
   if (mutex) {
      _AL_MARK_MUTEX_UNINITED(mutex->mutex);
      _al_mutex_init_recursive(&mutex->mutex);
   }
   return mutex;
}


/* Function: al_lock_mutex
 */
void al_lock_mutex(A5O_MUTEX *mutex)
{
   ASSERT(mutex);
   _al_mutex_lock(&mutex->mutex);
}



/* Function: al_unlock_mutex
 */
void al_unlock_mutex(A5O_MUTEX *mutex)
{
   ASSERT(mutex);
   _al_mutex_unlock(&mutex->mutex);
}


/* Function: al_destroy_mutex
 */
void al_destroy_mutex(A5O_MUTEX *mutex)
{
   if (mutex) {
      _al_mutex_destroy(&mutex->mutex);
      al_free(mutex);
   }
}


/* Function: al_create_cond
 */
A5O_COND *al_create_cond(void)
{
   A5O_COND *cond = al_malloc(sizeof(*cond));
   if (cond) {
      _al_cond_init(&cond->cond);
   }
   return cond;
}


/* Function: al_destroy_cond
 */
void al_destroy_cond(A5O_COND *cond)
{
   if (cond) {
      _al_cond_destroy(&cond->cond);
      al_free(cond);
   }
}


/* Function: al_wait_cond
 */
void al_wait_cond(A5O_COND *cond, A5O_MUTEX *mutex)
{
   ASSERT(cond);
   ASSERT(mutex);

   _al_cond_wait(&cond->cond, &mutex->mutex);
}


/* Function: al_wait_cond_until
 */
int al_wait_cond_until(A5O_COND *cond, A5O_MUTEX *mutex,
   const A5O_TIMEOUT *timeout)
{
   ASSERT(cond);
   ASSERT(mutex);
   ASSERT(timeout);

   return _al_cond_timedwait(&cond->cond, &mutex->mutex, timeout);
}


/* Function: al_broadcast_cond
 */
void al_broadcast_cond(A5O_COND *cond)
{
   ASSERT(cond);

   _al_cond_broadcast(&cond->cond);
}


/* Function: al_signal_cond
 */
void al_signal_cond(A5O_COND *cond)
{
   ASSERT(cond);

   _al_cond_signal(&cond->cond);
}


/* vim: set sts=3 sw=3 et: */
