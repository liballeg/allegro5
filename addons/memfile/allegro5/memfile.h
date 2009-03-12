#ifndef ALLEGRO_MEMFILE_H
#define ALLEGRO_MEMFILE_H

#include "allegro5/allegro5.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_MEMFILE_SRC
         #define _A5_MEMFILE_DLL __declspec(dllexport)
      #else
         #define _A5_MEMFILE_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_MEMFILE_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_MEMFILE_FUNC(type, name, args)      _A5_MEMFILE_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_MEMFILE_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_MEMFILE_FUNC(type, name, args)      extern _A5_MEMFILE_DLL type name args
#else
   #define A5_MEMFILE_FUNC      AL_FUNC
#endif


A5_MEMFILE_FUNC(ALLEGRO_FS_ENTRY *, al_open_memfile, (int64_t size, void *mem));

#ifdef __cplusplus
}
#endif

#endif /* ALLEGRO_MEMFILE_H */
