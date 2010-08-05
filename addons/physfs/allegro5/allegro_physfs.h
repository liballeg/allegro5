#ifndef __al_included_allegro5_allegro_physfs_h
#define __al_included_allegro5_allegro_physfs_h

#include "allegro5/allegro.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_PHYSFS_SRC
         #define _ALLEGRO_PHYSFS_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_PHYSFS_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_PHYSFS_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_PHYSFS_FUNC(type, name, args)      _ALLEGRO_PHYSFS_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_PHYSFS_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_PHYSFS_FUNC(type, name, args)      extern _ALLEGRO_PHYSFS_DLL type name args
#else
   #define ALLEGRO_PHYSFS_FUNC      AL_FUNC
#endif


ALLEGRO_PHYSFS_FUNC(void, al_set_physfs_file_interface, (void));
ALLEGRO_PHYSFS_FUNC(uint32_t, al_get_allegro_physfs_version, (void));


#ifdef __cplusplus
}
#endif

#endif
