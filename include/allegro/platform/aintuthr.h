/*
 * UNIX threads
 */
#ifndef _AINTUTHR_H
#define _AINTUTHR_H

#include <pthread.h>

AL_BEGIN_EXTERN_C

/* threads */
struct _AL_THREAD
{
    /* private: */
    pthread_t thread;
    pthread_mutex_t mutex;
    bool should_stop;
    void (*proc)(struct _AL_THREAD *self, void *arg);
    void *arg;
};

struct _AL_MUTEX
{
    bool inited;
    pthread_mutex_t mutex;
};

#define _AL_MUTEX_UNINITED	       { false, PTHREAD_MUTEX_INITIALIZER }
				       /* makes no sense, but shuts gcc up */
#define _AL_MARK_MUTEX_UNINITED(M)     do { M.inited = false; } while (0)

struct _AL_COND
{
   pthread_cond_t cond;
};


AL_INLINE(bool, _al_thread_should_stop, (struct _AL_THREAD *t),
{
    bool ret;
    pthread_mutex_lock(&t->mutex);
    ret = t->should_stop;
    pthread_mutex_unlock(&t->mutex);
    return ret;
})


AL_FUNC(void, _al_mutex_init, (struct _AL_MUTEX*));
AL_FUNC(void, _al_mutex_destroy, (struct _AL_MUTEX*));
AL_INLINE(void, _al_mutex_lock, (struct _AL_MUTEX *m),
{
   if (m->inited)
      pthread_mutex_lock(&m->mutex);
})
AL_INLINE(void, _al_mutex_unlock, (struct _AL_MUTEX *m),
{
   if (m->inited)
      pthread_mutex_unlock(&m->mutex);
})

AL_INLINE(void, _al_cond_init, (struct _AL_COND *cond),
{
   pthread_cond_init(&cond->cond, NULL);
})

AL_INLINE(void, _al_cond_destroy, (struct _AL_COND *cond),
{
   pthread_cond_destroy(&cond->cond);
})

AL_INLINE(void, _al_cond_wait, (struct _AL_COND *cond, struct _AL_MUTEX *mutex),
{
   pthread_cond_wait(&cond->cond, &mutex->mutex);
})

AL_INLINE(void, _al_cond_broadcast, (struct _AL_COND *cond),
{
   pthread_cond_broadcast(&cond->cond);
})

AL_INLINE(void, _al_cond_signal, (struct _AL_COND *cond),
{
   pthread_cond_signal(&cond->cond);
})

AL_END_EXTERN_C

#endif
