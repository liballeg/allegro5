#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define ALLEGRO_TTF_NO_KERNING 1

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_TTF_SRC
         #define _A5_TTF_DLL __declspec(dllexport)
      #else
         #define _A5_TTF_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_TTF_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_TTF_FUNC(type, name, args)      _A5_TTF_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_TTF_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_TTF_FUNC(type, name, args)      extern _A5_TTF_DLL type name args
#else
   #define A5_TTF_FUNC      AL_FUNC
#endif

A5_TTF_FUNC(ALLEGRO_FONT *, al_load_ttf_font, (char const *filename, int size, int flags));
A5_TTF_FUNC(bool, al_init_ttf_addon, (void));

#ifdef __cplusplus
   }
#endif
