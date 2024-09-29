#ifndef __al_included_allegro5_allegro_image_h
#define __al_included_allegro5_allegro_image_h

#include "allegro5/base.h"

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_IIO_SRC
         #define _A5O_IIO_DLL __declspec(dllexport)
      #else
         #define _A5O_IIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_IIO_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_IIO_FUNC(type, name, args)      _A5O_IIO_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_IIO_FUNC(type, name, args)      extern type name args
#elif defined A5O_BCC32
   #define A5O_IIO_FUNC(type, name, args)      extern _A5O_IIO_DLL type name args
#else
   #define A5O_IIO_FUNC      AL_FUNC
#endif


#ifdef __cplusplus
extern "C" {
#endif


A5O_IIO_FUNC(bool, al_init_image_addon, (void));
A5O_IIO_FUNC(bool, al_is_image_addon_initialized, (void));
A5O_IIO_FUNC(void, al_shutdown_image_addon, (void));
A5O_IIO_FUNC(uint32_t, al_get_allegro_image_version, (void));


#ifdef __cplusplus
}
#endif

#endif

