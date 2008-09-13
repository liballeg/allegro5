#ifndef _al_included_aintern_memory_h
#define _al_included_aintern_memory_h

#ifdef __cplusplus
   extern "C" {
#endif


/* malloc wrappers */
AL_VAR(void *, _al_memory_opaque);
AL_FUNCPTR(void *, _al_malloc, (void *opaque, size_t size));
AL_FUNCPTR(void *, _al_malloc_atomic, (void *opaque, size_t size));
AL_FUNCPTR(void, _al_free, (void *opaque, void *ptr));
AL_FUNCPTR(void *, _al_realloc, (void *opaque, void *ptr, size_t size));
AL_FUNCPTR(void *, _al_debug_malloc, (int line, const char *file, const char *func,
   void *opaque, size_t size));
AL_FUNCPTR(void *, _al_debug_malloc_atomic, (int line, const char *file, const char *func,
   void *opaque, size_t size));
AL_FUNCPTR(void, _al_debug_free, (int line, const char *file, const char *func,
   void *opaque, void *ptr));
AL_FUNCPTR(void *, _al_debug_realloc, (int line, const char *file, const char *func,
   void *opaque, void *ptr, size_t size));

#ifdef DEBUGMODE
   #define _AL_MALLOC(SIZE)         (_al_debug_malloc(__LINE__, __FILE__, __func__, _al_memory_opaque, SIZE))
   #define _AL_MALLOC_ATOMIC(SIZE)  (_al_debug_malloc_atomic(__LINE__, __FILE__, __func__, _al_memory_opaque, SIZE))
   #define _AL_FREE(PTR)            (_al_debug_free(__LINE__, __FILE__, __func__, _al_memory_opaque, PTR))
   #define _AL_REALLOC(PTR, SIZE)   (_al_debug_realloc(__LINE__, __FILE__, __func__, _al_memory_opaque, PTR, SIZE))
#else
   #define _AL_MALLOC(SIZE)         (_al_malloc(_al_memory_opaque, SIZE))
   #define _AL_MALLOC_ATOMIC(SIZE)  (_al_malloc_atomic(_al_memory_opaque, SIZE))
   #define _AL_FREE(PTR)            (_al_free(_al_memory_opaque, PTR))
   #define _AL_REALLOC(PTR, SIZE)   (_al_realloc(_al_memory_opaque, PTR, SIZE))
#endif


#ifdef __cplusplus
   }
#endif

#endif   /* _al_included_aintern_memory_h */

/* vim: set sts=3 sw=3 et: */
