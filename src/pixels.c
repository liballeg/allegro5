/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Pixel formats.
 *
 *      By Trent Gamblin.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_pixels.h"

ALLEGRO_DEBUG_CHANNEL("pixels")


/* lookup table for scaling 8 bit integers up to floats [0.0, 1.0] */
float _al_u8_to_float[256];
int _al_rgb_scale_1[2];
int _al_rgb_scale_4[16];
int _al_rgb_scale_5[32];
int _al_rgb_scale_6[64];

static int pixel_sizes[] = {
   0, /* ALLEGRO_PIXEL_FORMAT_ANY */
   0,
   0,
   2,
   2,
   2,
   3,
   4,
   4,
   4, /* ALLEGRO_PIXEL_FORMAT_ARGB_8888 */
   4,
   2,
   3,
   2,
   2,
   2,
   2,
   4,
   4,
   3,
   2,
   2,
   4,
   4,
   16, /* ALLEGRO_PIXEL_FORMAT_ABGR_F32 */
   4, /* ALLEGRO_PIXEL_FORMAT_ABGR_LE */
   2 /* ALLEGRO_PIXEL_FORMAT_RGBA_4444 */
};

static int pixel_bits[] = {
   0, /* ALLEGRO_PIXEL_FORMAT_ANY */
   0,
   0,
   15,
   16,
   16,
   24,
   32,
   32,
   32, /* ALLEGRO_PIXEL_FORMAT_ARGB_8888 */
   32,
   16,
   24,
   16,
   15,
   16,
   16,
   32,
   32,
   24,
   16,
   15,
   32,
   32,
   128, /* ALLEGRO_PIXEL_FORMAT_ABGR_F32 */
   32, /* ALLEGRO_PIXEL_FORMAT_ABGR_LE */
   16 /* ALLEGRO_PIXEL_FORMAT_RGBA_4444 */
};

static bool format_alpha_table[ALLEGRO_NUM_PIXEL_FORMATS] = {
   false, /* neutral (ALLEGRO_PIXEL_FORMAT_ANY) */
   false,
   true,
   false,
   false,
   true,
   false,
   false,
   true,
   true, /* ALLEGRO_PIXEL_FORMAT_ARGB_8888 */
   true,
   true,
   false,
   false,
   false,
   true,
   true,
   true,
   false,
   false,
   false,
   false,
   false,
   false,
   true, /* ALLEGRO_PIXEL_FORMAT_ABGR_F32 */
   true, /* ALLEGRO_PIXEL_FORMAT_ABGR_LE */
   true /* ALLEGRO_PIXEL_FORMAT_RGBA_4444 */
};

static char const *pixel_format_names[ALLEGRO_NUM_PIXEL_FORMATS + 1] = {
   "ANY",
   "ANY_NO_ALPHA",
   "ANY_WITH_ALPHA",
   "ANY_15_NO_ALPHA",
   "ANY_16_NO_ALPHA",
   "ANY_16_WITH_ALPHA",
   "ANY_24_NO_ALPHA",
   "ANY_32_NO_ALPHA",
   "ANY_32_WITH_ALPHA",
   "ARGB_8888",
   "RGBA_8888",
   "ARGB_4444",
   "RGB_888",
   "RGB_565",
   "RGB_555",
   "RGBA_5551",
   "ARGB_1555",
   "ABGR_8888",
   "XBGR_8888",
   "BGR_888",
   "BGR_565",
   "BGR_555",
   "RGBX_8888",
   "XRGB_8888",
   "ABGR_F32",
   "ABGR_8888_LE",
   "RGBA_4444",
   "INVALID"
};

static bool format_is_real[ALLEGRO_NUM_PIXEL_FORMATS] =
{
   false, /* ALLEGRO_PIXEL_FORMAT_ANY */
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   true, /* ALLEGRO_PIXEL_FORMAT_ARGB_8888 */
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true,
   true, /* ALLEGRO_PIXEL_FORMAT_ABGR_F32 */
   true, /* ALLEGRO_PIXEL_FORMAT_ABGR_LE */
   true /* ALLEGRO_PIXEL_FORMAT_RGBA_4444 */
};


void _al_init_pixels(void)
{
   int i;
   for (i = 0; i < 256; i++)
      _al_u8_to_float[i] = i / 255.0;

   for (i = 0; i < 2; i++)
      _al_rgb_scale_1[i] = i * 255 / 1;

   for (i = 0; i < 16; i++)
      _al_rgb_scale_4[i] = i * 255 / 15;

   for (i = 0; i < 32; i++)
      _al_rgb_scale_5[i] = i * 255 / 31;

   for (i = 0; i < 64; i++)
      _al_rgb_scale_6[i] = i * 255 / 63;
}


/* Function: al_get_pixel_size
 */
int al_get_pixel_size(int format)
{
   return pixel_sizes[format];
}


/* Function: al_get_pixel_format_bits
 */
int al_get_pixel_format_bits(int format)
{
   return pixel_bits[format];
}


bool _al_pixel_format_has_alpha(int format)
{
   return format_alpha_table[format];
}


bool _al_pixel_format_is_real(int format)
{
   ASSERT(format >= 0);
   ASSERT(format < ALLEGRO_NUM_PIXEL_FORMATS);

   return format_is_real[format];
}


/* We use al_get_display_format() as a hint for the preferred RGB ordering when
 * nothing else is specified.
 */
static bool try_display_format(ALLEGRO_DISPLAY *display, int *format)
{
   int best_format;
   int bytes;

   if (!display) {
      return false;
   }

   best_format = al_get_display_format(display);
   if (!_al_pixel_format_is_real(best_format))
      return false;

   bytes = al_get_pixel_size(*format);
   if (bytes && bytes != al_get_pixel_size(best_format))
      return false;

   if (_al_pixel_format_has_alpha(*format) &&
      !_al_pixel_format_has_alpha(best_format))
   {
      switch (best_format) {
         case ALLEGRO_PIXEL_FORMAT_RGBX_8888:
            *format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
            return true;
         case ALLEGRO_PIXEL_FORMAT_XRGB_8888:
             *format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
            return true;
         case ALLEGRO_PIXEL_FORMAT_XBGR_8888:
            *format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
            return true;
         default:
            return false;
      }
   }
   *format = best_format;
   return true;
}


int _al_get_real_pixel_format(ALLEGRO_DISPLAY *display, int format)
{
   /* Pick an appropriate format if the user is vague */
   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         if (!try_display_format(display, &format))
            format = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY:
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         if (!try_display_format(display, &format))
            format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_RGB_555;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         if (!try_display_format(display, &format))
            format = ALLEGRO_PIXEL_FORMAT_RGB_565;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_RGBA_4444;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_RGB_888;
         break;
      default:
         /* Already a real format - don't change it. */
         break;
   }

   ASSERT(_al_pixel_format_is_real(format));
   return format;
}


char const *_al_pixel_format_name(ALLEGRO_PIXEL_FORMAT format)
{
   if (format >= ALLEGRO_NUM_PIXEL_FORMATS) format = ALLEGRO_NUM_PIXEL_FORMATS;
   return pixel_format_names[format];
}


/* Color mapping functions */

/* Function: al_map_rgba
 */
ALLEGRO_COLOR al_map_rgba(
   unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   ALLEGRO_COLOR color;
   _AL_MAP_RGBA(color, r, g, b, a);
   return color;
}


/* Function: al_map_rgb
 */
ALLEGRO_COLOR al_map_rgb(
   unsigned char r, unsigned char g, unsigned char b)
{
   return al_map_rgba(r, g, b, 255);
}


/* Function: al_map_rgba_f
 */
ALLEGRO_COLOR al_map_rgba_f(float r, float g, float b, float a)
{
   ALLEGRO_COLOR color;
   color.r = r;
   color.g = g;
   color.b = b;
   color.a = a;
   return color;
}


/* Function: al_map_rgb_f
 */
ALLEGRO_COLOR al_map_rgb_f(float r, float g, float b)
{
   return al_map_rgba_f(r, g, b, 1.0f);
}


/* unmapping functions */


/* Function: al_unmap_rgba
 */
void al_unmap_rgba(ALLEGRO_COLOR color,
   unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
   *r = color.r * 255.0f;
   *g = color.g * 255.0f;
   *b = color.b * 255.0f;
   *a = color.a * 255.0f;
}


/* Function: al_unmap_rgb
 */
void al_unmap_rgb(ALLEGRO_COLOR color,
   unsigned char *r, unsigned char *g, unsigned char *b)
{
   unsigned char tmp;

   al_unmap_rgba(color, r, g, b, &tmp);
}


/* Function: al_unmap_rgba_f
 */
void al_unmap_rgba_f(ALLEGRO_COLOR color,
   float *r, float *g, float *b, float *a)
{
   *r = color.r;
   *g = color.g;
   *b = color.b;
   *a = color.a;
}


/* Function: al_unmap_rgb_f
 */
void al_unmap_rgb_f(ALLEGRO_COLOR color, float *r, float *g, float *b)
{
   float tmp;

   al_unmap_rgba_f(color, r, g, b, &tmp);
}


/* vim: set sts=3 sw=3 et: */
