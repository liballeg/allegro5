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
 *      Original line drawing by Michael Bukin.
 *      Conversion & filled rectangle/clear by Trent Gamblin.
 *
 */

/* Memory bitmap drawing functions */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_bitmap.h"

#define DO_FILLED_RECTANGLE(lr, size, set, w, h, value) \
{ \
   int x, y; \
   unsigned char *this_line = lr.data; \
   unsigned char *curr_ptr = lr.data; \
 \
   for (y = 0; y < h; y++) { \
      for (x = 0; x < w; x++) { \
         set(curr_ptr, value); \
	 curr_ptr += size; \
      } \
      curr_ptr = this_line = this_line + lr.pitch; \
   } \
}

void _al_draw_filled_rectangle_memory(int x1, int y1, int x2, int y2,
   AL_COLOR *color)
{
   AL_BITMAP *bitmap = al_get_target_bitmap();
   AL_LOCKED_REGION lr;
   int w, h;
   int tmp;
   int pixel_value;

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
   if (bitmap->clip) {
      if (x1 < bitmap->cl) {
         w -= (bitmap->cl - x1);
         x1 = bitmap->cl;
      }
      if (y1 < bitmap->ct) {
         h -= (bitmap->ct - y1);
         y1 = bitmap->ct;
      }
      if (x2 > bitmap->cr) {
         x2 = bitmap->cr;
      }
      if (y2 > bitmap->cb) {
         y2 = bitmap->cb;
      }
   }
   else {
      if (x1 < 0)
         x1 = 0;
      if (y1 < 0)
         y1 = 0;
      if (x2 > bitmap->w-1)
         x2 = bitmap->w-1;
      if (y2 > bitmap->h-1)
         y2 = bitmap->h-1;
   }

   w = x2 - x1 + 1;
   h = y2 - y1 + 1;

   if (w <= 0 || h <= 0)
      return;

   pixel_value = _al_get_pixel_value(bitmap->format, color);

   al_lock_bitmap_region(bitmap, x1, y1, w, h, &lr, 0);

   switch (al_get_pixel_size(bitmap->format)) {
      case 1:
         DO_FILLED_RECTANGLE(lr, 1, bmp_write8, w, h, pixel_value);
	 break;
      case 2:
         DO_FILLED_RECTANGLE(lr, 2, bmp_write16, w, h, pixel_value);
	 break;
      case 3:
         DO_FILLED_RECTANGLE(lr, 3, WRITE3BYTES, w, h, pixel_value);
	 break;
      case 4:
         DO_FILLED_RECTANGLE(lr, 4, bmp_write32, w, h, pixel_value);
	 break;
   }

   al_unlock_bitmap(bitmap);
}

void _al_clear_memory(AL_COLOR *color)
{
   AL_BITMAP *bitmap = al_get_target_bitmap();

   _al_draw_filled_rectangle_memory(0, 0, bitmap->w-1, bitmap->h-1, color);
}

/* FIXME: do a seperate horizontal and vertical line? */

/* Copied from do_line in gfx.c */
void _al_draw_line_memory(int x1, int y1, int x2, int y2, AL_COLOR *color)
{
   int dx;
   int dy;
   int i1, i2;
   int x, y;
   int dd;
   AL_LOCKED_REGION lr;
   int tmp;
   int d;
   AL_BITMAP *bitmap = al_get_target_bitmap();
   int sx, sy, t;
   bool clip;
   int clip_x1, clip_y1, clip_x2, clip_y2;

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond,     \
      set, size)                                                             \
   {                                                                         \
      if (d##pri_c == 0) {                                                   \
	 set(lr.data+y1*lr.pitch+x1*size, d);                                \
	 return;                                                             \
      }                                                                      \
									     \
      i1 = 2 * d##sec_c;                                                     \
      dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
      i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
									     \
      x = x1;                                                                \
      y = y1;                                                                \
									     \
      while (pri_c pri_cond pri_c##2) {                                      \
	 set(lr.data+y*lr.pitch+x*size, d);                                  \
									     \
	 if (dd sec_cond 0) {                                                \
	    sec_c = sec_c sec_sign 1;                                        \
	    dd += i2;                                                        \
	 }                                                                   \
	 else                                                                \
	    dd += i1;                                                        \
									     \
	 pri_c = pri_c pri_sign 1;                                           \
      }                                                                      \
   }

   #define NOT_CLIPPED(x, y) \
      (x >= clip_x1 && y >= clip_y1 && x <= clip_x2 && y <= clip_y2)

   /* worker macro with clipping */
   #define DO_LINE_CLIP(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond,\
      set, size)                                                             \
   {                                                                         \
      if (d##pri_c == 0) {                                                   \
         if (NOT_CLIPPED(x1, y1))                                            \
   	    set(lr.data+y1*lr.pitch+x1*size, d);                             \
	 return;                                                             \
      }                                                                      \
									     \
      i1 = 2 * d##sec_c;                                                     \
      dd = i1 - (sec_sign (pri_sign d##pri_c));                              \
      i2 = dd - (sec_sign (pri_sign d##pri_c));                              \
									     \
      x = x1;                                                                \
      y = y1;                                                                \
									     \
      while (pri_c pri_cond pri_c##2) {                                      \
         if (NOT_CLIPPED(x, y))                                              \
	    set(lr.data+y*lr.pitch+x*size, d);                               \
									     \
	 if (dd sec_cond 0) {                                                \
	    sec_c = sec_c sec_sign 1;                                        \
	    dd += i2;                                                        \
	 }                                                                   \
	 else                                                                \
	    dd += i1;                                                        \
									     \
	 pri_c = pri_c pri_sign 1;                                           \
      }                                                                      \
   }

   /* use a bounding box to check if the line needs clipping */
   if (bitmap->clip) {
      sx = x1;
      sy = y1;
      dx = x2;
      dy = y2;

      if (sx > dx) {
	 t = sx;
	 sx = dx;
	 dx = t;
      }

      if (sy > dy) {
	 t = sy;
	 sy = dy;
	 dy = t;
      }

      if ((sx >= bitmap->cr) || (sy >= bitmap->cb) || (dx < bitmap->cl) || (dy < bitmap->ct))
	 return;

      if ((sx >= bitmap->cl) && (sy >= bitmap->ct) && (dx < bitmap->cr) && (dy < bitmap->cb))
	 clip = FALSE;
      else
         clip = TRUE;
   }
   else
      clip = FALSE;

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

   if (x1 < 0)
      x1 = 0;
   if (y1 < 0)
      y1 = 0;
   if (x2 > bitmap->w-1)
      x2 = bitmap->w-1;
   if (y2 > bitmap->h-1)
      y2 = bitmap->h-1;

   dx = x2 - x1;
   dy = y2 - y1;

   if (dx <= 0 || dy <= 0)
      return;

   d = _al_get_pixel_value(bitmap->format, color);

   al_lock_bitmap_region(bitmap, x1, y1, dx, dy, &lr, 0);

   if (clip) {
      clip_x1 = bitmap->cl - x1;
      clip_y1 = bitmap->ct - y1;
      clip_x2 = (x1 + dx - 1) - x1;
      clip_y2 = (y1 + dy - 1) - y1;
   }

   x2 -= x1;
   y2 -= y1;
   x1 = y1 = 0;

   if (clip) {
      switch (al_get_pixel_size(bitmap->format)) {
         case 1:
            DO_LINE_CLIP(+, x, <=, +, y, >=, bmp_write8, 1);
            break;
         case 2:
            DO_LINE_CLIP(+, x, <=, +, y, >=, bmp_write16, 2);
            break;
         case 3:
            DO_LINE_CLIP(+, x, <=, +, y, >=, WRITE3BYTES, 3);
            break;
         case 4:
            DO_LINE_CLIP(+, x, <=, +, y, >=, bmp_write32, 4);
            break;
      }
   }
   else {
      switch (al_get_pixel_size(bitmap->format)) {
         case 1:
            DO_LINE(+, x, <=, +, y, >=, bmp_write8, 1);
            break;
         case 2:
            DO_LINE(+, x, <=, +, y, >=, bmp_write16, 2);
            break;
         case 3:
            DO_LINE(+, x, <=, +, y, >=, WRITE3BYTES, 3);
            break;
         case 4:
            DO_LINE(+, x, <=, +, y, >=, bmp_write32, 4);
            break;
      }
   }

   al_unlock_bitmap(bitmap);

   #undef DO_LINE
}

