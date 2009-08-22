#include "allegro5/allegro5.h"

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_COLOR_SRC
         #define _A5_COLOR_DLL __declspec(dllexport)
      #else
         #define _A5_COLOR_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_COLOR_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_COLOR_FUNC(type, name, args)      _A5_COLOR_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_COLOR_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_COLOR_FUNC(type, name, args)      extern _A5_COLOR_DLL type name args
#else
   #define A5_COLOR_FUNC      AL_FUNC
#endif

#ifdef __cplusplus
   extern "C" {
#endif

A5_COLOR_FUNC(uint32_t, al_get_allegro_color_version, (void));

A5_COLOR_FUNC(void, al_color_hsv_to_rgb, (float hue, float saturation,
   float value, float *red, float *green, float *blue));
A5_COLOR_FUNC(void, al_color_rgb_to_hsl, (float red, float green, float blue,
   float *hue, float *saturation, float *lightness));
A5_COLOR_FUNC(void, al_color_rgb_to_hsv, (float red, float green, float blue,
   float *hue, float *saturation, float *value));
A5_COLOR_FUNC(void, al_color_hsl_to_rgb, (float hue, float saturation, float lightness,
   float *red, float *green, float *blue));
A5_COLOR_FUNC(int, al_color_name_to_rgb, (char const *name, float *r, float *g,
   float *b));
A5_COLOR_FUNC(const char*, al_color_rgb_to_name, (float r, float g, float b));
A5_COLOR_FUNC(void, al_color_cmyk_to_rgb, (float cyan, float magenta, float yellow,
    float key, float *red, float *green, float *blue));
A5_COLOR_FUNC(void, al_color_rgb_to_cmyk, (float red, float green, float blue,
   float *cyan, float *magenta, float *yellow, float *key));
A5_COLOR_FUNC(void, al_color_yuv_to_rgb, (float y, float u, float v,
    float *red, float *green, float *blue));
A5_COLOR_FUNC(void, al_color_rgb_to_yuv, (float red, float green, float blue,
   float *y, float *u, float *v));
A5_COLOR_FUNC(void, al_color_rgb_to_html, (float red, float green, float blue,
    char *string));
A5_COLOR_FUNC(void, al_color_html_to_rgb, (char const *string,
   float *red, float *green, float *blue));
A5_COLOR_FUNC(ALLEGRO_COLOR, al_color_yuv, (float y, float u, float v));
A5_COLOR_FUNC(ALLEGRO_COLOR, al_color_cmyk, (float c, float m, float y, float k));
A5_COLOR_FUNC(ALLEGRO_COLOR, al_color_hsl, (float h, float s, float l));
A5_COLOR_FUNC(ALLEGRO_COLOR, al_color_hsv, (float h, float s, float v));
A5_COLOR_FUNC(ALLEGRO_COLOR, al_color_name, (char const *name));
A5_COLOR_FUNC(ALLEGRO_COLOR, al_color_html, (char const *string));

#ifdef __cplusplus
   }
#endif
