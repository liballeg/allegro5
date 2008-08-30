/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: add streaming support
 */

#ifndef ACODEC_H
#define ACODEC_H

#include "allegro5/allegro5.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_ACODEC_SRC
         #define _A5_ACODEC_DLL __declspec(dllexport)
      #else
         #define _A5_ACODEC_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_ACODEC_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_ACODEC_FUNC(type, name, args)      _A5_ACODEC_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_ACODEC_FUNC(type, name, args)      extern type name args
#else
   #define A5_ACODEC_FUNC      AL_FUNC
#endif

#include "allegro5/kcm_audio.h"

A5_ACODEC_FUNC(ALLEGRO_SAMPLE_DATA *, al_load_sample, (const char *filename));
A5_ACODEC_FUNC(ALLEGRO_STREAM *, al_load_stream, (const char *filename));

#ifdef __cplusplus
}
#endif

#endif
