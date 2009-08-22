#ifndef __al_included_allegro_vorbis_h
#define __al_included_allegro_vorbis_h

#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_VORBIS_SRC
         #define _A5_VORBIS_DLL __declspec(dllexport)
      #else
         #define _A5_VORBIS_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_VORBIS_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_VORBIS_FUNC(type, name, args)      _A5_VORBIS_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_VORBIS_FUNC(type, name, args)      extern type name args
#else
   #define A5_VORBIS_FUNC      AL_FUNC
#endif


A5_VORBIS_FUNC(bool, al_init_ogg_vorbis_addon, (void));
A5_VORBIS_FUNC(ALLEGRO_SAMPLE *, al_load_sample_ogg_vorbis, (
	const char *filename));
A5_VORBIS_FUNC(ALLEGRO_STREAM *, al_load_stream_ogg_vorbis, (
	const char *filename, size_t buffer_count, unsigned int samples));
A5_VORBIS_FUNC(uint32_t, al_get_allegro_ogg_vorbis_version, (void));


#ifdef __cplusplus
}
#endif

#endif
