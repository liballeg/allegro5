#ifndef ALLEGRO_PHYSFS_H
#define ALLEGRO_PHYSFS_H

#include "allegro5/allegro5.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_PHYSFS_SRC
         #define _A5_PHYSFS_DLL __declspec(dllexport)
      #else
         #define _A5_PHYSFS_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_PHYSFS_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_PHYSFS_FUNC(type, name, args)      _A5_PHYSFS_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_PHYSFS_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_PHYSFS_FUNC(type, name, args)      extern _A5_PHYSFS_DLL type name args
#else
   #define A5_PHYSFS_FUNC      AL_FUNC
#endif


A5_PHYSFS_FUNC(void, al_set_physfs_file_interface, (void));


#ifdef __cplusplus
}
#endif

#endif /* ALLEGRO_PHYSFS_H */
