#ifndef __al_included_allegro5_memfile_h
#define __al_included_allegro5_memfile_h

#include "allegro5/allegro.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_MEMFILE_SRC
         #define _ALLEGRO_MEMFILE_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_MEMFILE_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_MEMFILE_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_MEMFILE_FUNC(type, name, args)      _ALLEGRO_MEMFILE_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_MEMFILE_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_MEMFILE_FUNC(type, name, args)      extern _ALLEGRO_MEMFILE_DLL type name args
#else
   #define ALLEGRO_MEMFILE_FUNC      AL_FUNC
#endif


ALLEGRO_MEMFILE_FUNC(ALLEGRO_FILE *, al_open_memfile, (void *mem, int64_t size, const char *mode));
ALLEGRO_MEMFILE_FUNC(uint32_t, al_get_allegro_memfile_version, (void));

#ifdef __cplusplus
}
#endif

#endif
