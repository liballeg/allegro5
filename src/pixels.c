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
 *      Pixel manipulation.
 *
 *      By Trent Gamblin.
 *
 */

/* Title: Pixel manipulation
 */


#include <string.h> /* for memset */
#include "allegro5/allegro5.h"
#include "allegro5/bitmap_new.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"



/*
 * This table is used to unmap 8 bit color components
 * into the 0-INT_MAX range.
 */
static int _integer_unmap_table[256];

/* Generate _integer_unmap_table */
void _al_generate_integer_unmap_table(void)
{
   int c = INT_MAX;
   int d = INT_MAX / 255;
   int i;
   
   for (i = 255; i > 0; i--) {
      _integer_unmap_table[i] = c;
      c -= d;
   }
   _integer_unmap_table[i] = 0;
}


static int pixel_sizes[] = {
   0,
   0,
   2,
   2,
   2,
   2,
   3,
   3,
   4,
   4,
   4,
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
   4
};

static int pixel_bits[] = {
   0,
   0,
   15,
   15,
   16,
   16,
   24,
   24,
   32,
   32,
   32,
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
   32
};


int al_get_pixel_size(int format)
{
   return pixel_sizes[format];
}

int al_get_pixel_format_bits(int format)
{
   return pixel_bits[format];
}

static bool format_alpha_table[ALLEGRO_NUM_PIXEL_FORMATS] = {
   false,
   true,
   false,
   true,
   false,
   true,
   false,
   true,
   false,
   true,
   true,
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
   false
};

bool _al_format_has_alpha(int format)
{
   return format_alpha_table[format];
}

static bool format_is_real[ALLEGRO_NUM_PIXEL_FORMATS] =
{
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
   true
};

bool _al_pixel_format_is_real(int format)
{
   ASSERT(format >= 0);
   ASSERT(format < ALLEGRO_NUM_PIXEL_FORMATS);

   return format_is_real[format];
}

/* Returns true if real format1 fits into format2. */
bool _al_pixel_format_fits(int format1, int format2)
{
   ASSERT(format1 >= 0);
   ASSERT(format1 < ALLEGRO_NUM_PIXEL_FORMATS);
   ASSERT(format_is_real[format1]);
   ASSERT(format2 >= 0);
   ASSERT(format2 < ALLEGRO_NUM_PIXEL_FORMATS);

   if (format1 == format2)
      return true;

   if (format_alpha_table[format1] && format2 == ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA)
      return true;

   if (!format_alpha_table[format1] && format2 == ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA)
      return true;

   if (pixel_sizes[format1] == pixel_sizes[format2]
   && format_alpha_table[format1] == format_alpha_table[format2]
   && !format_is_real[format2])
      return true;

   return false;
}


/* Color mapping functions */

/* Function: al_map_rgba
 *
 * Convert r, g, b, a  (ranging from 0-255) into an ALLEGRO_COLOR.
 *
 * See also: <al_map_rgb> <al_map_rgba_f) <al_map_rgb_f>
 */
ALLEGRO_COLOR al_map_rgba(
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    ALLEGRO_COLOR color;
	color.r = (float)r / 255.0f;
	color.g = (float)g / 255.0f;
	color.b = (float)b / 255.0f;
	color.a = (float)a / 255.0f;
	return color;
}


/* Function: al_map_rgb
 *
 * Convert r, g, b (ranging from 0-255) into an ALLEGRO_COLOR,
 * using 255 for alpha.
 *
 * See also: <al_map_rgba> <al_map_rgba_f) <al_map_rgb_f>
 */
ALLEGRO_COLOR al_map_rgb(
	unsigned char r, unsigned char g, unsigned char b)
{
    return al_map_rgba(r, g, b, 255);
}


/* Function: al_map_rgba_f
 *
 * Convert r, g, b, a (ranging from 0.0f-1.0f) into an ALLEGRO_COLOR.
 *
 * See also: <al_map_rgba> <al_map_rgb> <al_map_rgb_f>
 */
ALLEGRO_COLOR al_map_rgba_f(
	float r, float g, float b, float a)
{
    ALLEGRO_COLOR color;
	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color;
}


/* Function: al_map_rgb_f
 *
 * Convert r, g, b, (ranging from 0.0f-1.0f) into an ALLEGRO_COLOR,
 * using 1.0f for alpha.
 *
 * See also: <al_map_rgba> <al_map_rgb> <al_map_rgba_f>
 */
ALLEGRO_COLOR al_map_rgb_f(
	float r, float g, float b)
{
	return al_map_rgba_f(r, g, b, 1.0f);
}


/* Get pixel functions */

void _get_pixel_argb_8888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = *(uint32_t *)(data);
   *color = al_map_rgba(
      (pixel & 0x00FF0000) >> 16,
      (pixel & 0x0000FF00) >>  8,
      (pixel & 0x000000FF) >>  0,
      (pixel & 0xFF000000) >> 24);
}

void _get_pixel_rgba_8888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = *(uint32_t *)(data);
   *color = al_map_rgba(
      (pixel & 0xFF000000) >> 24,
      (pixel & 0x00FF0000) >> 16,
      (pixel & 0x0000FF00) >>  8,
      (pixel & 0x000000FF) >>  0);
}

void _get_pixel_argb_4444(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_4[(pixel & 0x0F00) >> 8],
      _rgb_scale_4[(pixel & 0x00F0) >> 4],
      _rgb_scale_4[(pixel & 0x000F)],
      _rgb_scale_4[(pixel & 0xF000) >>  12]);
}

void _get_pixel_rgb_888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = READ3BYTES(data);   
   *color = al_map_rgba(
      (pixel & 0xFF0000) >> 16,
      (pixel & 0x00FF00) >>  8,
      (pixel & 0x0000FF) >>  0,
      255);
}

void _get_pixel_rgb_565(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_5[(pixel & 0xF800) >> 11],
      _rgb_scale_6[(pixel & 0x07E0) >> 5],
      _rgb_scale_5[(pixel & 0x001F)],
      255);
}

void _get_pixel_rgb_555(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_5[(pixel & 0x7C00) >> 10],
      _rgb_scale_5[(pixel & 0x03E0) >> 5],
      _rgb_scale_5[(pixel & 0x001F)],
      255);
}

void _get_pixel_rgba_5551(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_5[(pixel & 0xF800) >> 11],
      _rgb_scale_5[(pixel & 0x07C0) >> 6],
      _rgb_scale_5[(pixel & 0x003E) >> 1],
      255);
}

void _get_pixel_argb_1555(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_5[(pixel & 0x7C00) >> 10],
      _rgb_scale_5[(pixel & 0x03E0) >> 5],
      _rgb_scale_5[(pixel & 0x001F)],
      255);
}

void _get_pixel_abgr_8888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = *(uint32_t *)(data);
   *color = al_map_rgba(
      (pixel & 0x000000FF) >>  0,
      (pixel & 0x0000FF00) >>  8,
      (pixel & 0x00FF0000) >> 16,
      (pixel & 0xFF000000) >> 24);
}

void _get_pixel_xbgr_8888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = *(uint32_t *)(data);
   *color = al_map_rgba(
      (pixel & 0x000000FF) >>  0,
      (pixel & 0x0000FF00) >>  8,
      (pixel & 0x00FF0000) >> 16,
      255);
}

void _get_pixel_bgr_888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = READ3BYTES(data);
   *color = al_map_rgba(
      (pixel & 0x000000FF) >>  0,
      (pixel & 0x0000FF00) >>  8,
      (pixel & 0x00FF0000) >> 16,
      255);
}

void _get_pixel_bgr_565(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_5[(pixel & 0x001F)],
      _rgb_scale_5[(pixel & 0x07E0) >> 5],
      _rgb_scale_5[(pixel & 0xF800) >> 11],
      255);
}

void _get_pixel_bgr_555(void *data, ALLEGRO_COLOR *color)
{
   uint16_t pixel = *(uint16_t *)(data);
   *color = al_map_rgba(
      _rgb_scale_5[(pixel & 0x001F)],
      _rgb_scale_5[(pixel & 0x03E0) >> 5],
      _rgb_scale_5[(pixel & 0x7C00) >> 10],
      255);
}

void _get_pixel_rgbx_8888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = *(uint32_t *)(data);
   *color = al_map_rgba(
      (pixel & 0xFF000000) >> 24,
      (pixel & 0x00FF0000) >> 16,
      (pixel & 0x0000FF00) >>  8,
      255);
}

void _get_pixel_xrgb_8888(void *data, ALLEGRO_COLOR *color)
{
   uint32_t pixel = *(uint32_t *)(data);
   *color = al_map_rgba(
      (pixel & 0x00FF0000) >> 16,
      (pixel & 0x0000FF00) >>  8,
      (pixel & 0x000000FF),
      255);
}

/* get pixel lookup table */

typedef void (*p_get_pixel_func)(void *, ALLEGRO_COLOR *);

p_get_pixel_func get_pixel_funcs[ALLEGRO_NUM_PIXEL_FORMATS] = {
   /* Fake pixel formats */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   /* End fake pixel formats */
   _get_pixel_argb_8888,
   _get_pixel_rgba_8888,
   _get_pixel_argb_4444,
   _get_pixel_rgb_888,
   _get_pixel_rgb_565,
   _get_pixel_rgb_555,
   _get_pixel_rgba_5551,
   _get_pixel_argb_1555,
   _get_pixel_abgr_8888,
   _get_pixel_xbgr_8888,
   _get_pixel_bgr_888,
   _get_pixel_bgr_565,
   _get_pixel_bgr_555,
   _get_pixel_rgbx_8888,
   _get_pixel_xrgb_8888
};

ALLEGRO_COLOR *_al_get_pixel(ALLEGRO_BITMAP *bitmap, void *data, ALLEGRO_COLOR *color)
{
   (*get_pixel_funcs[bitmap->format])(data, color);

   return color;
}

/* Function: al_get_pixel
 *
 * Get a pixel's color value from the specified bitmap.
 */
ALLEGRO_COLOR al_get_pixel(ALLEGRO_BITMAP *bitmap, int x, int y)
{
   ALLEGRO_LOCKED_REGION lr;
   ALLEGRO_COLOR color;

   if (bitmap->locked) {
      x -= bitmap->lock_x;
      y -= bitmap->lock_y;
      if (x < 0 || y < 0 || x >= bitmap->lock_w || y >= bitmap->lock_h) {
         TRACE("al_get_pixel out of bounds\n");
         memset(&color, 0, sizeof(ALLEGRO_COLOR));
         return color;
      }

      _al_get_pixel(bitmap, (void*)(((char*)bitmap->locked_region.data)
            + y * bitmap->locked_region.pitch
            + x * al_get_pixel_size(bitmap->format)),
            &color);
   }
   else {
      /* FIXME: must use clip not full bitmap */
      if (x < 0 || y < 0 || x >= bitmap->w || y >= bitmap->h) {
         memset(&color, 0, sizeof(ALLEGRO_COLOR));
         return color;
      }

      if (!al_lock_bitmap_region(bitmap, x, y, 1, 1, &lr,
            ALLEGRO_LOCK_READONLY))
      {
         memset(&color, 0, sizeof(ALLEGRO_COLOR));
         return color;
      }
      
      /* FIXME: check for valid pixel format */

      _al_get_pixel(bitmap, lr.data, &color);

      al_unlock_bitmap(bitmap);
   }

   return color;
}

/* get_pixel_value functions */

int _get_pixel_value_argb_8888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0;
   pixel |= (int)(p->a * 255) << 24;
   pixel |= (int)(p->r * 255) << 16;
   pixel |= (int)(p->g * 255) <<  8;
   pixel |= (int)(p->b * 255);
   return pixel;
}

int _get_pixel_value_rgba_8888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0;
   pixel |= (int)(p->r * 255) << 24;
   pixel |= (int)(p->g * 255) << 16;
   pixel |= (int)(p->b * 255) <<  8;
   pixel |= (int)(p->a * 255);
   return pixel;
}


int _get_pixel_value_argb_4444(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->a * 15) << 12;
   pixel |= (int)(p->r * 15) <<  8;
   pixel |= (int)(p->g * 15) <<  4;
   pixel |= (int)(p->b * 15);
   return pixel;
}

int _get_pixel_value_rgb_888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0;
   pixel |= (int)(p->r * 255) << 16;
   pixel |= (int)(p->g * 255) << 8;
   pixel |= (int)(p->b * 255);
   return pixel;
}

int _get_pixel_value_rgb_565(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->r * 0x1f) << 11;
   pixel |= (int)(p->g * 0x3f) << 5;
   pixel |= (int)(p->b * 0x1f);

   return pixel;
}

int _get_pixel_value_rgb_555(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->r * 0x1f) << 10;
   pixel |= (int)(p->g * 0x1f) << 5;
   pixel |= (int)(p->b * 0x1f);
   return pixel;
}

int _get_pixel_value_rgba_5551(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->r * 0x1f) << 11;
   pixel |= (int)(p->g * 0x1f) << 6;
   pixel |= (int)(p->b * 0x1f) << 1;
   pixel |= (int)p->a;
   return pixel;
}

int _get_pixel_value_argb_1555(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->a) << 15;
   pixel |= (int)(p->r * 0x1f) << 10;
   pixel |= (int)(p->g * 0x1f) <<  5;
   pixel |= (int)(p->b * 0x1f);
   return pixel;
}

int _get_pixel_value_abgr_8888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0;
   pixel |= (int)(p->a * 0xff) << 24;
   pixel |= (int)(p->b * 0xff) << 16;
   pixel |= (int)(p->g * 0xff) << 8;
   pixel |= (int)(p->r * 0xff);
   return pixel;
}

int _get_pixel_value_xbgr_8888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0xff000000;
   pixel |= (int)(p->b * 0xff) << 16;
   pixel |= (int)(p->g * 0xff) << 8;
   pixel |= (int)(p->b * 0xff);
   return pixel;
}

int _get_pixel_value_bgr_888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0;
   pixel |= (int)(p->b * 0xff) << 16;
   pixel |= (int)(p->g * 0xff) << 8;
   pixel |= (int)(p->r * 0xff);
   return pixel;
}

int _get_pixel_value_bgr_565(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->b * 0x1f) << 11;
   pixel |= (int)(p->g * 0x3f) << 5;
   pixel |= (int)(p->r * 0x1f);
   return pixel;
}

int _get_pixel_value_bgr_555(ALLEGRO_COLOR *p)
{
   uint16_t pixel = 0;
   pixel |= (int)(p->b * 0x1f) << 10;
   pixel |= (int)(p->g * 0x1f) << 5;
   pixel |= (int)(p->r * 0x1f);
   return pixel;
}
   
int _get_pixel_value_rgbx_8888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0xff;
   pixel |= (int)(p->r * 0xff) << 24;
   pixel |= (int)(p->g * 0xff) << 16;
   pixel |= (int)(p->b * 0xff) << 8;
   return pixel;
}

int _get_pixel_value_xrgb_8888(ALLEGRO_COLOR *p)
{
   uint32_t pixel = 0xff000000;
   pixel |= (int)(p->r * 0xff) << 16;
   pixel |= (int)(p->g * 0xff) << 8;
   pixel |= (int)(p->b * 0xff);
   return pixel;
}

typedef int (*_get_pixel_value_func)(ALLEGRO_COLOR *color);

static _get_pixel_value_func _get_pixel_value_funcs[ALLEGRO_NUM_PIXEL_FORMATS] = {
   /* Fake formats */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   /* End fake formats */
   _get_pixel_value_argb_8888,
   _get_pixel_value_rgba_8888,
   _get_pixel_value_argb_4444,
   _get_pixel_value_rgb_888,
   _get_pixel_value_rgb_565,
   _get_pixel_value_rgb_555,
   _get_pixel_value_rgba_5551,
   _get_pixel_value_argb_1555,
   _get_pixel_value_abgr_8888,
   _get_pixel_value_xbgr_8888,
   _get_pixel_value_bgr_888,
   _get_pixel_value_bgr_565,
   _get_pixel_value_bgr_555,
   _get_pixel_value_rgbx_8888,
   _get_pixel_value_xrgb_8888
};

int _al_get_pixel_value(int src_format, ALLEGRO_COLOR *src_color)
{
   ASSERT(_al_pixel_format_is_real(src_format));

   return (*_get_pixel_value_funcs[src_format])(src_color);
}

/* put_pixel functions */

void _put_pixel_argb_8888(void *data, int pixel)
{
   *(uint32_t *)(data) = pixel;
}

void _put_pixel_rgba_8888(void *data, int pixel)
{
   *(uint32_t *)(data) = pixel;
}

void _put_pixel_argb_4444(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_rgb_888(void *data, int pixel)
{
   WRITE3BYTES(data, pixel);
}

void _put_pixel_rgb_565(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_rgb_555(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_rgba_5551(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_argb_1555(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_abgr_8888(void *data, int pixel)
{
   *(uint32_t *)(data) = pixel;
}

void _put_pixel_xbgr_8888(void *data, int pixel)
{
   *(uint32_t *)(data) = pixel;
}

void _put_pixel_bgr_888(void *data, int pixel)
{
   WRITE3BYTES(data, pixel);
}

void _put_pixel_bgr_565(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_bgr_555(void *data, int pixel)
{
   *(uint16_t *)(data) = pixel;
}

void _put_pixel_rgbx_8888(void *data, int pixel)
{
   *(uint32_t *)(data) = pixel;
}

void _put_pixel_xrgb_8888(void *data, int pixel)
{
   *(uint32_t *)(data) = pixel;
}

/* put pixel lookup table */

typedef void (*p_put_pixel_func)(void *data, int pixel);

p_put_pixel_func put_pixel_funcs[ALLEGRO_NUM_PIXEL_FORMATS] = {
   /* Fake pixel formats */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   /* End fake pixel formats */
   _put_pixel_argb_8888,
   _put_pixel_rgba_8888,
   _put_pixel_argb_4444,
   _put_pixel_rgb_888,
   _put_pixel_rgb_565,
   _put_pixel_rgb_555,
   _put_pixel_rgba_5551,
   _put_pixel_argb_1555,
   _put_pixel_abgr_8888,
   _put_pixel_xbgr_8888,
   _put_pixel_bgr_888,
   _put_pixel_bgr_565,
   _put_pixel_bgr_555,
   _put_pixel_rgbx_8888,
   _put_pixel_xrgb_8888
};

void _al_put_pixel(void *data, int format, int color)
{
   ASSERT(_al_pixel_format_is_real(format));

   (*put_pixel_funcs[format])(data, color);
}

/* Function: al_put_pixel
 *
 * Draw a single pixel on the target bitmap.
 */
void al_put_pixel(int x, int y, ALLEGRO_COLOR color)
{
   ALLEGRO_LOCKED_REGION lr;
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();
   int color_value;

   color_value = _al_get_pixel_value(bitmap->format, &color);

   if (bitmap->locked) {
      x -= bitmap->lock_x;
      y -= bitmap->lock_y;
      if (x < 0 || y < 0 || x >= bitmap->lock_w || y >= bitmap->lock_h) { 
         return;
      }

      _al_put_pixel(
         (char *)bitmap->locked_region.data
            + y * bitmap->locked_region.pitch
            + x * al_get_pixel_size(bitmap->format),
         bitmap->format, color_value);
   }
   else {
      if (x < bitmap->cl || y < bitmap->ct ||
          x >= bitmap->cr || y >= bitmap->cb)
      {
         return;
      }

      if (!al_lock_bitmap_region(bitmap, x, y, 1, 1, &lr, 0))
         return;

      /* FIXME: check for valid pixel format */

      _al_put_pixel(lr.data, bitmap->format, color_value);

      al_unlock_bitmap(bitmap);
   }
}


/* unmapping functions */


/* Function: al_unmap_rgba
 *
 * Retrieves components of an ALLEGRO_COLOR.
 * Components will range from 0-255.
 *
 * See also: <al_unmap_rgb>, <al_unmap_rgba_f>, <al_unmap_rgb_f>
 */
void al_unmap_rgba(ALLEGRO_COLOR color,
	unsigned char *r,  unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = color.r * 255.0f;
	*g = color.g * 255.0f;
	*b = color.b * 255.0f;
	*a = color.a * 255.0f;
}


/* Function: al_unmap_rgb
 *
 * Retrieves components of an ALLEGRO_COLOR, ignoring alpha
 * Components will range from 0-255.
 *
 * See also: <al_unmap_rgba>, <al_unmap_rgba_f>, <al_unmap_rgb_f>
 */
void al_unmap_rgb(ALLEGRO_COLOR color,
	unsigned char *r,  unsigned char *g, unsigned char *b)
{
	unsigned char tmp;

	al_unmap_rgba(color, r, g, b, &tmp);
}


/* Function: al_unmap_rgba_f
 *
 * Retrieves components of an ALLEGRO_COLOR.
 * Components will range from 0.0f-1.0f.
 *
 * See also: <al_unmap_rgba>, <al_unmap_rgb>, <al_unmap_rgb_f>
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
 *
 * Retrieves components of an ALLEGRO_COLOR, ignoring alpha.
 * Components will range from 0.0f-1.0f.
 *
 * See also: <al_unmap_rgba>, <al_unmap_rgb>, <al_unmap_rgba_f>
 */
void al_unmap_rgb_f(ALLEGRO_COLOR color,
	float *r, float *g, float *b)
{
	float tmp;

	al_unmap_rgba_f(color, r, g, b, &tmp);
}

