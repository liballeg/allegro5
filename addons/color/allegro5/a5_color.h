#include "allegro5/allegro5.h"

#ifdef __cplusplus
   extern "C" {
#endif

void al_color_hsv_to_rgb(float hue, float saturation,
   float value, float *red, float *green, float *blue);
void al_color_rgb_to_hsl(float red, float green, float blue,
   float *hue, float *saturation, float *lightness);
void al_color_rgb_to_hsv(float red, float green, float blue,
   float *hue, float *saturation, float *value);
void al_color_hsl_to_rgb(float hue, float saturation, float lightness,
   float *red, float *green, float *blue);
int al_color_name_to_rgb(char const *name, float *r, float *g,
   float *b);
char const *al_color_rgb_to_name(float r, float g, float b);
void al_color_cmyk_to_rgb(float cyan, float magenta, float yellow,
    float key, float *red, float *green, float *blue);
void al_color_rgb_to_cmyk(float red, float green, float blue,
   float *cyan, float *magenta, float *yellow, float *key);
void al_color_yuv_to_rgb(float y, float u, float v,
    float *red, float *green, float *blue);
void al_color_rgb_to_yuv(float red, float green, float blue,
   float *y, float *u, float *v);
void al_color_rgb_to_html(float red, float green, float blue,
    char *string);
void al_color_html_to_rgb(char const *string,
   float *red, float *green, float *blue);
ALLEGRO_COLOR al_color_yuv(float y, float u, float v);
ALLEGRO_COLOR al_color_cmyk(float c, float m, float y, float k);
ALLEGRO_COLOR al_color_hsl(float h, float s, float l);
ALLEGRO_COLOR al_color_hsv(float h, float s, float v);
ALLEGRO_COLOR al_color_name(char const *name);
ALLEGRO_COLOR al_color_html(char const *string);

#ifdef __cplusplus
   }
#endif
