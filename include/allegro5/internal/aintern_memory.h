#ifndef _al_included_aintern_memory_h
#define _al_included_aintern_memory_h


/* these should be migrated later */
#define _AL_MALLOC(n)         al_malloc(n)
#define _AL_MALLOC_ATOMIC(n)  al_malloc(n)
#define _AL_FREE(p)           al_free(p)
#define _AL_REALLOC(p, n)     al_realloc((p), (n))


#endif   /* _al_included_aintern_memory_h */

/* vim: set sts=3 sw=3 et: */
