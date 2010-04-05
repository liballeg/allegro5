#ifndef __al_included_ALLEGRO_MODAUDIO_h
#define __al_included_ALLEGRO_MODAUDIO_h

#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_MODAUDIO_SRC
         #define _ALLEGRO_MODAUDIO_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_MODAUDIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_MODAUDIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_MODAUDIO_FUNC(type, name, args)      _ALLEGRO_MODAUDIO_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_MODAUDIO_FUNC(type, name, args)      extern type name args
#else
   #define ALLEGRO_MODAUDIO_FUNC      AL_FUNC
#endif

ALLEGRO_MODAUDIO_FUNC(bool, al_init_modaudio_addon, (void));
ALLEGRO_MODAUDIO_FUNC(uint32_t, al_get_allegro_modaudio_version, (void));
   
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_mod_audio_stream, (
   const char *filename, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_mod_audio_stream_f, (
   ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_it_audio_stream, (
   const char *filename, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_it_audio_stream_f, (
   ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_xm_audio_stream, (
   const char *filename, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_xm_audio_stream_f, (
   ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_s3m_audio_stream, (
   const char *filename, size_t buffer_count, unsigned int samples));
ALLEGRO_MODAUDIO_FUNC(ALLEGRO_AUDIO_STREAM *, al_load_s3m_audio_stream_f, (
   ALLEGRO_FILE* f, size_t buffer_count, unsigned int samples));



#ifdef __cplusplus
}
#endif

#endif
