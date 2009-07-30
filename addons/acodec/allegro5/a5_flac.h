#ifndef __al_included_a5_flac_h
#define __al_included_a5_flac_h

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_FLAC_SRC
         #define _A5_FLAC_DLL __declspec(dllexport)
      #else
         #define _A5_FLAC_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_FLAC_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_FLAC_FUNC(type, name, args)      _A5_FLAC_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_FLAC_FUNC(type, name, args)      extern type name args
#else
   #define A5_FLAC_FUNC      AL_FUNC
#endif


A5_FLAC_FUNC(bool, al_init_flac_addon, (void));
A5_FLAC_FUNC(ALLEGRO_SAMPLE *, al_load_sample_flac, (const char *filename));


#ifdef __cplusplus
}
#endif

#endif
