#ifndef __al_included_allegro5_allegro_color_h
#define __al_included_allegro5_allegro_color_h

#include "allegro5/allegro.h"

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_COLOR_SRC
         #define _ALLEGRO_COLOR_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_COLOR_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_COLOR_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_COLOR_FUNC(type, name, args)      _ALLEGRO_COLOR_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_COLOR_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_COLOR_FUNC(type, name, args)      extern _ALLEGRO_COLOR_DLL type name args
#else
   #define ALLEGRO_COLOR_FUNC      AL_FUNC
#endif

#ifdef __cplusplus
   extern "C" {
#endif

ALLEGRO_COLOR_FUNC(uint32_t, al_get_allegro_color_version, (void));

ALLEGRO_COLOR_FUNC(void, al_color_hsv_to_rgb, (float hue, float saturation,
   float value, float *red, float *green, float *blue));
ALLEGRO_COLOR_FUNC(void, al_color_rgb_to_hsl, (float red, float green, float blue,
   float *hue, float *saturation, float *lightness));
ALLEGRO_COLOR_FUNC(void, al_color_rgb_to_hsv, (float red, float green, float blue,
   float *hue, float *saturation, float *value));
ALLEGRO_COLOR_FUNC(void, al_color_hsl_to_rgb, (float hue, float saturation, float lightness,
   float *red, float *green, float *blue));
ALLEGRO_COLOR_FUNC(bool, al_color_name_to_rgb, (char const *name, float *r, float *g,
   float *b));
ALLEGRO_COLOR_FUNC(const char*, al_color_rgb_to_name, (float r, float g, float b));
ALLEGRO_COLOR_FUNC(void, al_color_cmyk_to_rgb, (float cyan, float magenta, float yellow,
    float key, float *red, float *green, float *blue));
ALLEGRO_COLOR_FUNC(void, al_color_rgb_to_cmyk, (float red, float green, float blue,
   float *cyan, float *magenta, float *yellow, float *key));
ALLEGRO_COLOR_FUNC(void, al_color_yuv_to_rgb, (float y, float u, float v,
    float *red, float *green, float *blue));
ALLEGRO_COLOR_FUNC(void, al_color_rgb_to_yuv, (float red, float green, float blue,
   float *y, float *u, float *v));
ALLEGRO_COLOR_FUNC(void, al_color_rgb_to_html, (float red, float green, float blue,
    char *string));
ALLEGRO_COLOR_FUNC(bool, al_color_html_to_rgb, (char const *string,
   float *red, float *green, float *blue));
ALLEGRO_COLOR_FUNC(ALLEGRO_COLOR, al_color_yuv, (float y, float u, float v));
ALLEGRO_COLOR_FUNC(ALLEGRO_COLOR, al_color_cmyk, (float c, float m, float y, float k));
ALLEGRO_COLOR_FUNC(ALLEGRO_COLOR, al_color_hsl, (float h, float s, float l));
ALLEGRO_COLOR_FUNC(ALLEGRO_COLOR, al_color_hsv, (float h, float s, float v));
ALLEGRO_COLOR_FUNC(ALLEGRO_COLOR, al_color_name, (char const *name));
ALLEGRO_COLOR_FUNC(ALLEGRO_COLOR, al_color_html, (char const *string));

#ifdef __cplusplus
   }
#endif

#endif
