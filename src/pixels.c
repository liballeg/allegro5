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

A5O_DEBUG_CHANNEL("pixels")


/* lookup table for scaling 8 bit integers up to floats [0.0, 1.0] */
#include "pixel_tables.inc"

static int pixel_sizes[] = {
   0, /* A5O_PIXEL_FORMAT_ANY */
   0,
   0,
   2,
   2,
   2,
   3,
   4,
   4,
   4, /* A5O_PIXEL_FORMAT_ARGB_8888 */
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
   16, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   4, /* A5O_PIXEL_FORMAT_ABGR_LE */
   2, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   1, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   0,
   0,
   0,
};

static int pixel_bits[] = {
   0, /* A5O_PIXEL_FORMAT_ANY */
   0,
   0,
   15,
   16,
   16,
   24,
   32,
   32,
   32, /* A5O_PIXEL_FORMAT_ARGB_8888 */
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
   128, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   32, /* A5O_PIXEL_FORMAT_ABGR_LE */
   16, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   8, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   0,
   0,
   0,
};

static int pixel_block_widths[] = {
   0, /* A5O_PIXEL_FORMAT_ANY */
   0,
   0,
   1,
   1,
   1,
   1,
   1,
   1,
   1, /* A5O_PIXEL_FORMAT_ARGB_8888 */
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   1, /* A5O_PIXEL_FORMAT_ABGR_LE */
   1, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   1, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   4,
   4,
   4,
};

static int pixel_block_heights[] = {
   0, /* A5O_PIXEL_FORMAT_ANY */
   0,
   0,
   1,
   1,
   1,
   1,
   1,
   1,
   1, /* A5O_PIXEL_FORMAT_ARGB_8888 */
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1,
   1, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   1, /* A5O_PIXEL_FORMAT_ABGR_LE */
   1, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   1, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   4,
   4,
   4,
};

static int pixel_block_sizes[] = {
   0,  /* A5O_PIXEL_FORMAT_ANY */
   0,
   0,
   2,
   2,
   2,
   3,
   4,
   4,
   4,  /* A5O_PIXEL_FORMAT_ARGB_8888 */
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
   16, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   4,  /* A5O_PIXEL_FORMAT_ABGR_LE */
   2,  /* A5O_PIXEL_FORMAT_RGBA_4444 */
   1,  /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   8,
   16,
   16,
};

static bool format_alpha_table[A5O_NUM_PIXEL_FORMATS] = {
   false, /* neutral (A5O_PIXEL_FORMAT_ANY) */
   false,
   true,
   false,
   false,
   true,
   false,
   false,
   true,
   true, /* A5O_PIXEL_FORMAT_ARGB_8888 */
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
   true, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   true, /* A5O_PIXEL_FORMAT_ABGR_LE */
   true, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   false, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   true,
   true,
   true,
};

static char const *pixel_format_names[A5O_NUM_PIXEL_FORMATS + 1] = {
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
   "SINGLE_CHANNEL_8",
   "RGBA_DXT1",
   "RGBA_DXT3",
   "RGBA_DXT5",
   "INVALID"
};

static bool format_is_real[A5O_NUM_PIXEL_FORMATS] =
{
   false, /* A5O_PIXEL_FORMAT_ANY */
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   true, /* A5O_PIXEL_FORMAT_ARGB_8888 */
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
   true, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   true, /* A5O_PIXEL_FORMAT_ABGR_LE */
   true, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   true, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   true,
   true,
   true,
};

static bool format_is_video_only[A5O_NUM_PIXEL_FORMATS] =
{
   false, /* A5O_PIXEL_FORMAT_ANY */
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false, /* A5O_PIXEL_FORMAT_ARGB_8888 */
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   false, /* A5O_PIXEL_FORMAT_ABGR_LE */
   false, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   false, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   true,
   true,
   true,
};

static bool format_is_compressed[A5O_NUM_PIXEL_FORMATS] =
{
   false, /* A5O_PIXEL_FORMAT_ANY */
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false, /* A5O_PIXEL_FORMAT_ARGB_8888 */
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false,
   false, /* A5O_PIXEL_FORMAT_ABGR_F32 */
   false, /* A5O_PIXEL_FORMAT_ABGR_LE */
   false, /* A5O_PIXEL_FORMAT_RGBA_4444 */
   false, /* A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8 */
   true,
   true,
   true,
};


/* Function: al_get_pixel_block_size
 */
int al_get_pixel_block_size(int format)
{
   return pixel_block_sizes[format];
}


/* Function: al_get_pixel_block_width
 */
int al_get_pixel_block_width(int format)
{
   return pixel_block_widths[format];
}


/* Function: al_get_pixel_block_height
 */
int al_get_pixel_block_height(int format)
{
   return pixel_block_heights[format];
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
   ASSERT(format < A5O_NUM_PIXEL_FORMATS);

   return format_is_real[format];
}

bool _al_pixel_format_is_video_only(int format)
{
   ASSERT(format >= 0);
   ASSERT(format < A5O_NUM_PIXEL_FORMATS);

   return format_is_video_only[format];
}

bool _al_pixel_format_is_compressed(int format)
{
   ASSERT(format >= 0);
   ASSERT(format < A5O_NUM_PIXEL_FORMATS);

   return format_is_compressed[format];
}


/* We use al_get_display_format() as a hint for the preferred RGB ordering when
 * nothing else is specified.
 */
static bool try_display_format(A5O_DISPLAY *display, int *format)
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
      !_al_pixel_format_has_alpha(best_format)) {
      switch (best_format) {
         case A5O_PIXEL_FORMAT_RGBX_8888:
            *format = A5O_PIXEL_FORMAT_RGBA_8888;
            return true;
         case A5O_PIXEL_FORMAT_XRGB_8888:
             *format = A5O_PIXEL_FORMAT_ARGB_8888;
            return true;
         case A5O_PIXEL_FORMAT_XBGR_8888:
            *format = A5O_PIXEL_FORMAT_ABGR_8888;
            return true;
         default:
            return false;
      }
   }
   if (!_al_pixel_format_has_alpha(*format) &&
      _al_pixel_format_has_alpha(best_format)) {
      switch (best_format) {
         case A5O_PIXEL_FORMAT_RGBA_8888:
            *format = A5O_PIXEL_FORMAT_RGBX_8888;
            return true;
         case A5O_PIXEL_FORMAT_ARGB_8888:
             *format = A5O_PIXEL_FORMAT_XRGB_8888;
            return true;
         case A5O_PIXEL_FORMAT_ABGR_8888:
            *format = A5O_PIXEL_FORMAT_XBGR_8888;
            return true;
         default:
            return false;
      }
   }
   *format = best_format;
   return true;
}


int _al_get_real_pixel_format(A5O_DISPLAY *display, int format)
{
   /* Pick an appropriate format if the user is vague */
   switch (format) {
      case A5O_PIXEL_FORMAT_ANY_NO_ALPHA:
      case A5O_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         if (!try_display_format(display, &format))
            format = A5O_PIXEL_FORMAT_XRGB_8888;
         break;
      case A5O_PIXEL_FORMAT_ANY:
      case A5O_PIXEL_FORMAT_ANY_WITH_ALPHA:
      case A5O_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         if (!try_display_format(display, &format))
#if defined A5O_CFG_OPENGLES // OPENGLES doesn't have ARGB_8888
            format = A5O_PIXEL_FORMAT_ABGR_8888_LE;
#else
            format = A5O_PIXEL_FORMAT_ARGB_8888;
#endif
         break;
      case A5O_PIXEL_FORMAT_ANY_15_NO_ALPHA:
         format = A5O_PIXEL_FORMAT_RGB_555;
         break;
      case A5O_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         if (!try_display_format(display, &format))
            format = A5O_PIXEL_FORMAT_RGB_565;
         break;
      case A5O_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         format = A5O_PIXEL_FORMAT_RGBA_4444;
         break;
      case A5O_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         format = A5O_PIXEL_FORMAT_RGB_888;
         break;
      default:
         /* Already a real format - don't change it. */
         break;
   }

   ASSERT(_al_pixel_format_is_real(format));
   return format;
}


char const *_al_pixel_format_name(A5O_PIXEL_FORMAT format)
{
   if (format >= A5O_NUM_PIXEL_FORMATS) format = A5O_NUM_PIXEL_FORMATS;
   return pixel_format_names[format];
}


/* Color mapping functions */

/* Function: al_map_rgba
 */
A5O_COLOR al_map_rgba(
   unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   A5O_COLOR color;
   _AL_MAP_RGBA(color, r, g, b, a);
   return color;
}


/* Function: al_premul_rgba
 */
A5O_COLOR al_premul_rgba(
   unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   A5O_COLOR color;
   _AL_MAP_RGBA(color, r * a / 255, g * a / 255, b * a / 255, a);
   return color;
}


/* Function: al_map_rgb
 */
A5O_COLOR al_map_rgb(
   unsigned char r, unsigned char g, unsigned char b)
{
   return al_map_rgba(r, g, b, 255);
}


/* Function: al_map_rgba_f
 */
A5O_COLOR al_map_rgba_f(float r, float g, float b, float a)
{
   A5O_COLOR color;
   color.r = r;
   color.g = g;
   color.b = b;
   color.a = a;
   return color;
}


/* Function: al_premul_rgba_f
 */
A5O_COLOR al_premul_rgba_f(float r, float g, float b, float a)
{
   A5O_COLOR color;
   color.r = r * a;
   color.g = g * a;
   color.b = b * a;
   color.a = a;
   return color;
}


/* Function: al_map_rgb_f
 */
A5O_COLOR al_map_rgb_f(float r, float g, float b)
{
   return al_map_rgba_f(r, g, b, 1.0f);
}


/* unmapping functions */


/* Function: al_unmap_rgba
 */
void al_unmap_rgba(A5O_COLOR color,
   unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
   *r = color.r * 255.0f;
   *g = color.g * 255.0f;
   *b = color.b * 255.0f;
   *a = color.a * 255.0f;
}


/* Function: al_unmap_rgb
 */
void al_unmap_rgb(A5O_COLOR color,
   unsigned char *r, unsigned char *g, unsigned char *b)
{
   unsigned char tmp;

   al_unmap_rgba(color, r, g, b, &tmp);
}


/* Function: al_unmap_rgba_f
 */
void al_unmap_rgba_f(A5O_COLOR color,
   float *r, float *g, float *b, float *a)
{
   *r = color.r;
   *g = color.g;
   *b = color.b;
   *a = color.a;
}


/* Function: al_unmap_rgb_f
 */
void al_unmap_rgb_f(A5O_COLOR color, float *r, float *g, float *b)
{
   float tmp;

   al_unmap_rgba_f(color, r, g, b, &tmp);
}


/* vim: set sts=3 sw=3 et: */
