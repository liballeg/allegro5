#ifndef __al_included_allegro5_allegro_ttf_h
#define __al_included_allegro5_allegro_ttf_h

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define ALLEGRO_TTF_NO_KERNING  1
#define ALLEGRO_TTF_MONOCHROME  2
#define ALLEGRO_TTF_NO_AUTOHINT 4

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_TTF_SRC
         #define _ALLEGRO_TTF_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_TTF_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_TTF_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_TTF_FUNC(type, name, args)      _ALLEGRO_TTF_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_TTF_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_TTF_FUNC(type, name, args)      extern _ALLEGRO_TTF_DLL type name args
#else
   #define ALLEGRO_TTF_FUNC      AL_FUNC
#endif

ALLEGRO_TTF_FUNC(ALLEGRO_FONT *, al_load_ttf_font, (char const *filename, int size, int flags));
ALLEGRO_TTF_FUNC(ALLEGRO_FONT *, al_load_ttf_font_f, (ALLEGRO_FILE *file, char const *filename, int size, int flags));
ALLEGRO_TTF_FUNC(ALLEGRO_FONT *, al_load_ttf_font_stretch, (char const *filename, int w, int h, int flags));
ALLEGRO_TTF_FUNC(ALLEGRO_FONT *, al_load_ttf_font_stretch_f, (ALLEGRO_FILE *file, char const *filename, int w, int h, int flags));
ALLEGRO_TTF_FUNC(bool, al_init_ttf_addon, (void));
ALLEGRO_TTF_FUNC(void, al_shutdown_ttf_addon, (void));
ALLEGRO_TTF_FUNC(uint32_t, al_get_allegro_ttf_version, (void));

#ifdef __cplusplus
   }
#endif

#endif
