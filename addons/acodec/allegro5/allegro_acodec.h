#ifndef __al_included_allegro5_allegro_acodec_h
#define __al_included_allegro5_allegro_acodec_h

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined A5O_MINGW32) || (defined A5O_MSVC)
   #ifndef A5O_STATICLINK
      #ifdef A5O_ACODEC_SRC
         #define _A5O_ACODEC_DLL __declspec(dllexport)
      #else
         #define _A5O_ACODEC_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_ACODEC_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_ACODEC_FUNC(type, name, args)      _A5O_ACODEC_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_ACODEC_FUNC(type, name, args)      extern type name args
#else
   #define A5O_ACODEC_FUNC      AL_FUNC
#endif


A5O_ACODEC_FUNC(bool, al_init_acodec_addon, (void));
A5O_ACODEC_FUNC(bool, al_is_acodec_addon_initialized, (void));
A5O_ACODEC_FUNC(uint32_t, al_get_allegro_acodec_version, (void));


#ifdef __cplusplus
}
#endif

#endif
