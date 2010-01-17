#ifndef __al_included_allegro_flac_h
#define __al_included_allegro_flac_h

#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_FLAC_SRC
         #define _ALLEGRO_FLAC_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_FLAC_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_FLAC_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_FLAC_FUNC(type, name, args)      _ALLEGRO_FLAC_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_FLAC_FUNC(type, name, args)      extern type name args
#else
   #define ALLEGRO_FLAC_FUNC      AL_FUNC
#endif


ALLEGRO_FLAC_FUNC(bool, al_init_flac_addon, (void));
ALLEGRO_FLAC_FUNC(ALLEGRO_SAMPLE *, al_load_flac, (const char *filename));
ALLEGRO_FLAC_FUNC(uint32_t, al_get_allegro_flac_version, (void));
ALLEGRO_FLAC_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_audio_stream_flac, (
   const char *filename, size_t buffer_count, unsigned int samples));


#ifdef __cplusplus
}
#endif

#endif
