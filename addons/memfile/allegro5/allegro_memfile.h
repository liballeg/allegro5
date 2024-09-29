#ifndef __al_included_allegro5_memfile_h
#define __al_included_allegro5_memfile_h

#include "allegro5/allegro.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_MEMFILE_SRC
         #define _A5O_MEMFILE_DLL __declspec(dllexport)
      #else
         #define _A5O_MEMFILE_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_MEMFILE_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_MEMFILE_FUNC(type, name, args)      _A5O_MEMFILE_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_MEMFILE_FUNC(type, name, args)      extern type name args
#elif defined A5O_BCC32
   #define A5O_MEMFILE_FUNC(type, name, args)      extern _A5O_MEMFILE_DLL type name args
#else
   #define A5O_MEMFILE_FUNC      AL_FUNC
#endif


A5O_MEMFILE_FUNC(A5O_FILE *, al_open_memfile, (void *mem, int64_t size, const char *mode));
A5O_MEMFILE_FUNC(uint32_t, al_get_allegro_memfile_version, (void));

#ifdef __cplusplus
}
#endif

#endif
