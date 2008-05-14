/**
 * allegro image codec loader
 * author: Ryan Dickie (c) 2008
 */

#ifndef ICODEC_H
#define ICODEC_H

#include "allegro5/allegro5.h"

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_ICODEC_SRC
         #define _A5_ICODEC_DLL __declspec(dllexport)
      #else
         #define _A5_ICODEC_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_ICODEC_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_ICODEC_FUNC(type, name, args)      _A5_ICODEC_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_ICODEC_FUNC(type, name, args)      extern type name args
#endif


A5_ICODEC_FUNC(int, al_save_image, (ALLEGRO_BITMAP* bmp, const char* path));
A5_ICODEC_FUNC(ALLEGRO_BITMAP*, al_load_image, (const char* path));

#endif
