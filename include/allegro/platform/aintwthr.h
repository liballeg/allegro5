/*
 * Windows threads
 */
#ifndef _AINTWTHR_H
#define _AINTWTHR_H

#include "winalleg.h"

AL_BEGIN_EXTERN_C

/* threads */
struct _AL_THREAD
{
   /* private: */
   HANDLE thread;
   CRITICAL_SECTION cs;
   bool should_stop; /* XXX: use a dedicated terminate Event object? */
   void (*proc)(struct _AL_THREAD *self, void *arg);
   void *arg;
};

struct _AL_MUTEX
{
   PCRITICAL_SECTION cs;
};

#define _AL_MUTEX_UNINITED	       { NULL }
#define _AL_MARK_MUTEX_UNINITED(M)     do { M.cs = NULL; } while (0)

struct _AL_COND
{
   long nWaitersBlocked;
   long nWaitersGone;
   long nWaitersToUnblock;
   HANDLE semBlockQueue;
   CRITICAL_SECTION semBlockLock;
   CRITICAL_SECTION mtxUnblockLock;
};


AL_INLINE(bool, _al_thread_should_stop, (struct _AL_THREAD *t),
{
   bool ret;
   EnterCriticalSection(&t->cs);
   ret = t->should_stop;
   LeaveCriticalSection(&t->cs);
   return ret;
})

AL_INLINE(void, _al_mutex_lock, (struct _AL_MUTEX *m),
{
   if (m->cs)
      EnterCriticalSection(m->cs);
})
AL_INLINE(void, _al_mutex_unlock, (struct _AL_MUTEX *m),
{
   if (m->cs)
      LeaveCriticalSection(m->cs);
})

AL_END_EXTERN_C

#endif
