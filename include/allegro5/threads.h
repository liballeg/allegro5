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
 *      Thread routines.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_allegro5_threads_h
#define __al_included_allegro5_threads_h

#include "allegro5/altime.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: A5O_THREAD
 */
typedef struct A5O_THREAD A5O_THREAD;

/* Type: A5O_MUTEX
 */
typedef struct A5O_MUTEX A5O_MUTEX;

/* Type: A5O_COND
 */
typedef struct A5O_COND A5O_COND;


AL_FUNC(A5O_THREAD *, al_create_thread,
   (void *(*proc)(A5O_THREAD *thread, void *arg), void *arg));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(A5O_THREAD *, al_create_thread_with_stacksize,
   (void *(*proc)(A5O_THREAD *thread, void *arg), void *arg, size_t stacksize));
#endif
AL_FUNC(void, al_start_thread, (A5O_THREAD *outer));
AL_FUNC(void, al_join_thread, (A5O_THREAD *outer, void **ret_value));
AL_FUNC(void, al_set_thread_should_stop, (A5O_THREAD *outer));
AL_FUNC(bool, al_get_thread_should_stop, (A5O_THREAD *outer));
AL_FUNC(void, al_destroy_thread, (A5O_THREAD *thread));
AL_FUNC(void, al_run_detached_thread, (void *(*proc)(void *arg), void *arg));

AL_FUNC(A5O_MUTEX *, al_create_mutex, (void));
AL_FUNC(A5O_MUTEX *, al_create_mutex_recursive, (void));
AL_FUNC(void, al_lock_mutex, (A5O_MUTEX *mutex));
AL_FUNC(void, al_unlock_mutex, (A5O_MUTEX *mutex));
AL_FUNC(void, al_destroy_mutex, (A5O_MUTEX *mutex));

AL_FUNC(A5O_COND *, al_create_cond, (void));
AL_FUNC(void, al_destroy_cond, (A5O_COND *cond));
AL_FUNC(void, al_wait_cond, (A5O_COND *cond, A5O_MUTEX *mutex));
AL_FUNC(int, al_wait_cond_until, (A5O_COND *cond, A5O_MUTEX *mutex,
                    const A5O_TIMEOUT *timeout));
AL_FUNC(void, al_broadcast_cond, (A5O_COND *cond));
AL_FUNC(void, al_signal_cond, (A5O_COND *cond));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
