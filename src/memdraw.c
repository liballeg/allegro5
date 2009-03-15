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
 *      Memory bitmap drawing routines
 *
 *      Based on Michael Bukin's C drawing functions
 *      Conversion to the new API by Trent Gamblin.
 *
 */

/* Memory bitmap drawing functions */

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"

void _al_draw_pixel_memory(ALLEGRO_BITMAP *bitmap, int x, int y,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_COLOR result;
   _al_blend(color, bitmap, x, y, &result);
   _al_put_pixel(bitmap, x, y, result);
}

#define DEFINE_PUT_PIXEL(name, size, get, set)                               \
static void name(ALLEGRO_BITMAP *dst, void *dst_addr, int dx, int dy,        \
   int color)                                                                \
{                                                                            \
   ASSERT(dst);                                                              \
                                                                             \
   if ((dx < dst->cl) || (dx >= dst->cr_excl) ||                             \
       (dy < dst->ct) || (dy >= dst->cb_excl))                               \
      return;                                                                \
                                                                             \
   set(dst_addr, color);                                                     \
}



#define DEFINE_HLINE(name, size, get, set)                                   \
static void name(ALLEGRO_BITMAP *dst, unsigned char *dst_addr,               \
   int dx1, int dy, int dx2, int color)                                      \
{                                                                            \
   int w;                                                                    \
                                                                             \
   ASSERT(dst);                                                              \
   (void)dst;                                                                \
   (void)dy;                                                                 \
                                                                             \
   w = dx2 - dx1;                                                            \
                                                                             \
   do {                                                                      \
      set(dst_addr, color);                                                  \
      dst_addr += size;                                                      \
   } while (--w >= 0);                                                       \
}


DEFINE_HLINE(_hline32, 4, bmp_read32, bmp_write32)



#define DO_FILLED_RECTANGLE_FAST(dst, dst_addr, func, dx, dy, w, h, value)   \
do {                                                                         \
   int y;                                                                    \
   unsigned char *line_ptr = dst_addr;                                       \
                                                                             \
   for (y = 0; y < h; y++) {                                                 \
      func(dst, line_ptr, dx, dy+y, dx+w-1, value);                          \
      line_ptr += lr->pitch;                                                 \
   }                                                                         \
} while (0)


/* Coordinates are inclusive full-pixel positions. So (0, 0, 0, 0) draws a
 * single pixel at 0/0.
 */
static void _al_draw_filled_rectangle_memory_fast(int x1, int y1, int x2, int y2,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION *lr;
   int w, h;
   int tmp;
   int pixel_value;

   bitmap = al_get_target_bitmap();

   /* Make sure it's top left first */
   if (x1 > x2) {
      tmp = x1;
      x1 = x2;
      x2 = tmp;
   }
   if (y1 > y2) {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   /* Do clipping */
   if (x1 < bitmap->cl) x1 = bitmap->cl;
   if (y1 < bitmap->ct) y1 = bitmap->ct;
   if (x2 > bitmap->cr_excl - 1) x2 = bitmap->cr_excl - 1;
   if (y2 > bitmap->cb_excl - 1) y2 = bitmap->cb_excl - 1;

   w = x2 - x1 + 1;
   h = y2 - y1 + 1;

   if (w <= 0 || h <= 0)
      return;

   pixel_value = _al_get_pixel_value(bitmap->format, color);

   lr = al_lock_bitmap_region(bitmap, x1, y1, w, h, ALLEGRO_PIXEL_FORMAT_ARGB_8888, 0);

   DO_FILLED_RECTANGLE_FAST(bitmap, lr->data, _hline32, x1, y1, w, h, pixel_value);

   al_unlock_bitmap(bitmap);
}


void _al_clear_memory(ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   _al_draw_filled_rectangle_memory_fast(0, 0, bitmap->w-1, bitmap->h-1,
      color);
}



static void _hline(ALLEGRO_BITMAP *target, int x1, int y, int x2,
   ALLEGRO_COLOR *color)
{
   int x;

   for (x = x1; x <= x2; x++) {
      ALLEGRO_COLOR result;
      _al_blend(color, target, x, y, &result);
      _al_put_pixel(target, x, y, result);
   }
}



#define DO_FILLED_RECTANGLE(func, bitmap, dx, dy, w, h, color)               \
do {                                                                         \
   int y;                                                                    \
                                                                             \
   for (y = 0; y < h; y++) {                                                 \
      func(bitmap, dx, dy+y, dx+w-1, color);                                 \
   }                                                                         \
} while (0)


void _al_draw_filled_rectangle_memory(int x1, int y1, int x2, int y2,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION *lr;
   int w, h;
   int tmp;
   int src_mode, dst_mode;
   ALLEGRO_COLOR *ic;

   al_get_blender(&src_mode, &dst_mode, NULL);
   ic = _al_get_blend_color();
   if (src_mode == ALLEGRO_ONE && dst_mode == ALLEGRO_ZERO &&
         ic->r == 1.0f && ic->g == 1.0f && ic->b == 1.0f && ic->a == 1.0f)
   {
      _al_draw_filled_rectangle_memory_fast(x1, y1, x2, y2, color);
      return;
   }

   bitmap = al_get_target_bitmap();

   /* Make sure it's top left first */
   if (x1 > x2) {
      tmp = x1;
      x1 = x2;
      x2 = tmp;
   }
   if (y1 > y2) {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   /* Do clipping */
   if (x1 < bitmap->cl) x1 = bitmap->cl;
   if (y1 < bitmap->ct) y1 = bitmap->ct;
   if (x2 > bitmap->cr_excl - 1) x2 = bitmap->cr_excl - 1;
   if (y2 > bitmap->cb_excl - 1) y2 = bitmap->cb_excl - 1;

   w = x2 - x1 + 1;
   h = y2 - y1 + 1;

   if (w <= 0 || h <= 0)
      return;

   lr = al_lock_bitmap_region(bitmap, x1, y1, w, h, ALLEGRO_PIXEL_FORMAT_ANY, 0);

   DO_FILLED_RECTANGLE(_hline, bitmap, x1, y1, w, h, color);

   al_unlock_bitmap(bitmap);
}

/* vim: set sts=3 sw=3 et: */
