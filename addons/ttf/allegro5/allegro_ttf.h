#ifndef __al_included_allegro5_allegro_ttf_h
#define __al_included_allegro5_allegro_ttf_h

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define A5O_TTF_NO_KERNING  1
#define A5O_TTF_MONOCHROME  2
#define A5O_TTF_NO_AUTOHINT 4

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_TTF_SRC
         #define _A5O_TTF_DLL __declspec(dllexport)
      #else
         #define _A5O_TTF_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_TTF_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_TTF_FUNC(type, name, args)      _A5O_TTF_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_TTF_FUNC(type, name, args)      extern type name args
#elif defined A5O_BCC32
   #define A5O_TTF_FUNC(type, name, args)      extern _A5O_TTF_DLL type name args
#else
   #define A5O_TTF_FUNC      AL_FUNC
#endif

A5O_TTF_FUNC(A5O_FONT *, al_load_ttf_font, (char const *filename, int size, int flags));
A5O_TTF_FUNC(A5O_FONT *, al_load_ttf_font_f, (A5O_FILE *file, char const *filename, int size, int flags));
A5O_TTF_FUNC(A5O_FONT *, al_load_ttf_font_stretch, (char const *filename, int w, int h, int flags));
A5O_TTF_FUNC(A5O_FONT *, al_load_ttf_font_stretch_f, (A5O_FILE *file, char const *filename, int w, int h, int flags));
A5O_TTF_FUNC(bool, al_init_ttf_addon, (void));
A5O_TTF_FUNC(bool, al_is_ttf_addon_initialized, (void));
A5O_TTF_FUNC(void, al_shutdown_ttf_addon, (void));
A5O_TTF_FUNC(uint32_t, al_get_allegro_ttf_version, (void));

#ifdef __cplusplus
   }
#endif

#endif
