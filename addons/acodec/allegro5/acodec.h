/**
 * Allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 */

#ifndef ACODEC_H
#define ACODEC_H

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"

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


A5_ACODEC_FUNC(bool, al_register_sample_loader, (const char *ext,
	ALLEGRO_SAMPLE *(*loader)(const char *filename)));
A5_ACODEC_FUNC(bool, al_register_sample_saver, (const char *ext,
	bool (*saver)(const char *filename, ALLEGRO_SAMPLE *spl)));
A5_ACODEC_FUNC(bool, al_register_stream_loader, (const char *ext,
	ALLEGRO_STREAM *(*stream_loader)(const char *filename,
	    size_t buffer_count, unsigned int samples)));

A5_ACODEC_FUNC(ALLEGRO_SAMPLE *, al_load_sample, (const char *filename));
A5_ACODEC_FUNC(bool, al_save_sample, (const char *filename,
	ALLEGRO_SAMPLE *spl));
A5_ACODEC_FUNC(ALLEGRO_STREAM *, al_stream_from_file, (const char *filename,
	size_t buffer_count, unsigned int samples));

/* WAV handlers */
A5_ACODEC_FUNC(ALLEGRO_SAMPLE *, al_load_sample_wav, (const char *filename));
A5_ACODEC_FUNC(bool, al_save_sample_wav, (const char *filename, ALLEGRO_SAMPLE *spl));
A5_ACODEC_FUNC(bool, al_save_sample_wav_pf, (ALLEGRO_FILE *pf, ALLEGRO_SAMPLE *spl));
A5_ACODEC_FUNC(ALLEGRO_STREAM *, al_load_stream_wav, (const char *filename,
	size_t buffer_count, unsigned int samples));


#ifdef __cplusplus
}
#endif

#endif
