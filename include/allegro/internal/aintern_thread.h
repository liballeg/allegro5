#ifndef _al_included_aintern_thread_h
#define _al_included_aintern_thread_h

#ifndef SCAN_EXPORT
#include ALLEGRO_INTERNAL_THREAD_HEADER
#endif

AL_BEGIN_EXTERN_C


typedef struct _AL_THREAD _AL_THREAD;
typedef struct _AL_MUTEX _AL_MUTEX;
typedef struct _AL_COND _AL_COND;


AL_FUNC(void, _al_thread_create, (_AL_THREAD*,
				  void (*proc)(_AL_THREAD*, void*),
				  void *arg));
/* static inline bool _al_thread_should_stop(_AL_THREAD *); */
AL_FUNC(void, _al_thread_join, (_AL_THREAD*));


AL_FUNC(void, _al_mutex_init, (_AL_MUTEX*));
AL_FUNC(void, _al_mutex_init_recursive, (_AL_MUTEX*));
AL_FUNC(void, _al_mutex_destroy, (_AL_MUTEX*));
/* static inline void _al_mutex_lock(_AL_MUTEX*); */
/* static inline void _al_mutex_unlock(_AL_MUTEX*); */


AL_FUNC(void, _al_cond_init, (_AL_COND*));
AL_FUNC(void, _al_cond_destroy, (_AL_COND*));
AL_FUNC(void, _al_cond_wait, (_AL_COND*, _AL_MUTEX*));
AL_FUNC(int, _al_cond_timedwait, (_AL_COND*, _AL_MUTEX*, unsigned long abstime));
AL_FUNC(void, _al_cond_broadcast, (_AL_COND*));
AL_FUNC(void, _al_cond_signal, (_AL_COND*));


AL_END_EXTERN_C

#endif

/* vi ts=8 sts=3 sw=3 et */
