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

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_bitmap.h"


#define DEFINE_PUT_PIXEL(name, size, get, set) \
static void name(AL_BITMAP *dst, void *dst_addr, int dx, int dy, int color, int flags) \
{ \
   ASSERT(dst); \
 \
   if (dst->clip && ((dx < dst->cl) || (dx >= dst->cr) || (dy < dst->ct) || (dy >= dst->cb))) \
      return; \
 \
   if (flags & AL_PATTERNED) { \
      color = get(dst->pattern_copy->memory + \
         ((dy - dst->drawing_y_anchor) & dst->drawing_y_mask)*dst->pattern_pitch + \
         ((dx - dst->drawing_x_anchor) & dst->drawing_x_mask)*size); \
   } \
 \
   set(dst_addr, color); \
}

#define DEFINE_PUT_PIXEL_NC(name, size, get, set) \
static void name(AL_BITMAP *dst, void *dst_addr, int dx, int dy, int color, int flags) \
{ \
   ASSERT(dst); \
 \
   if (flags & AL_PATTERNED) { \
      color = get(dst->pattern_copy->memory + \
         ((dy - dst->drawing_y_anchor) & dst->drawing_y_mask)*dst->pattern_pitch + \
         ((dx - dst->drawing_x_anchor) & dst->drawing_x_mask)*size); \
   } \
 \
   set(dst_addr, color); \
}


DEFINE_PUT_PIXEL(_al_put_pixel8, 1, bmp_read8, bmp_write8)
DEFINE_PUT_PIXEL(_al_put_pixel16, 2, bmp_read16, bmp_write16)
DEFINE_PUT_PIXEL(_al_put_pixel24, 3, READ3BYTES, WRITE3BYTES)
DEFINE_PUT_PIXEL(_al_put_pixel32, 4, bmp_read32, bmp_write32)

DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc8, 1, bmp_read8, bmp_write8)
DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc16, 2, bmp_read16, bmp_write16)
DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc24, 3, READ3BYTES, WRITE3BYTES)
DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc32, 4, bmp_read32, bmp_write32)


#define DEFINE_HLINE(name, size, get, set) \
static void name(AL_BITMAP *dst, unsigned char *dst_addr, int dx1, int dy, int dx2, \
   int color, int flags) \
{ \
   int w; \
 \
   ASSERT(dst); \
 \
   if (dx1 > dx2) { \
      int tmp = dx1; \
      dx1 = dx2; \
      dx2 = tmp; \
   } \
   if (dst->clip) { \
      if (dx1 < dst->cl) \
	 dx1 = dst->cl; \
      if (dx2 >= dst->cr) \
	 dx2 = dst->cr - 1; \
      if ((dx1 > dx2) || (dy < dst->ct) || (dy >= dst->cb)) \
	 return; \
   } \
 \
   w = dx2 - dx1; \
    \
   if (flags & AL_PATTERNED) { \
      int x, curw; \
      unsigned char *sline = dst->pattern_copy->memory + \
         (((dy - dst->drawing_y_anchor) & dst->drawing_y_mask)*dst->pattern_pitch); \
      unsigned char *s; \
 \
      x = (dx1 - dst->drawing_x_anchor) & dst->drawing_x_mask; \
      s = sline + (x*size); \
      w++; \
      curw = dst->drawing_x_mask + 1 - x; \
      if (curw > w) \
	 curw = w; \
 \
      do { \
         w -= curw; \
         do { \
            unsigned long c = get(s); \
            set(dst_addr, c); \
            s += size; \
            dst_addr += size; \
         } while (--curw > 0); \
         s = sline; \
         curw = MIN(w, (int)dst->drawing_x_mask+1); \
      } while (curw > 0); \
   } \
   else { \
      do { \
	 set(dst_addr, color); \
         dst_addr += size; \
      } while (--w >= 0); \
   } \
}


DEFINE_HLINE(_hline8, 1, bmp_read8, bmp_write8)
DEFINE_HLINE(_hline16, 2, bmp_read16, bmp_write16)
DEFINE_HLINE(_hline24, 3, READ3BYTES, WRITE3BYTES)
DEFINE_HLINE(_hline32, 4, bmp_read32, bmp_write32)


void _al_draw_hline_memory(int dx1, int dy, int dx2, AL_COLOR *color, int flags)
{
   AL_BITMAP *target;
   int color_value;
   AL_LOCKED_REGION lr;

   target = al_get_target_bitmap();
   color_value = _al_get_pixel_value(target->format, color);

   if (!al_lock_bitmap_region(target, dx1, dy, dx2-dx1+1, 1, &lr, 0))
      return;

   switch (al_get_pixel_size(target->format)) {
      case 1:
         _hline8(target, lr.data, dx1, dy, dx2, color_value, flags);
         break;
      case 2:
         _hline16(target, lr.data, dx1, dy, dx2, color_value, flags);
         break;
      case 3:
         _hline24(target, lr.data, dx1, dy, dx2, color_value, flags);
         break;
      case 4:
         _hline32(target, lr.data, dx1, dy, dx2, color_value, flags);
         break;
   }

   al_unlock_bitmap(target);
}


void _al_draw_vline_memory(int dx, int dy1, int dy2, AL_COLOR *color, int flags)
{
   int y;
   AL_BITMAP *dst = al_get_target_bitmap();
   AL_LOCKED_REGION lr;
   int size;
   int color_value;

   ASSERT(dst);

   if (dy1 > dy2) {
      int tmp = dy1;
      dy1 = dy2;
      dy2 = tmp;
   }
   if (dst->clip) {
      if (dy1 < dst->ct)
	 dy1 = dst->ct;
      if (dy2 >= dst->cb)
	 dy2 = dst->cb - 1;
      if ((dx < dst->cl) || (dx >= dst->cr) || (dy1 > dy2))
	 return;
   }

   if (!al_lock_bitmap_region(dst, dx, dy1, 1, dy2-dy1+1, &lr, 0))
      return;

   size = al_get_pixel_size(dst->format);
   color_value = _al_get_pixel_value(dst->format, color);

   if (flags & AL_PATTERNED) {
      int clip = dst->clip;
      unsigned char *d = lr.data;
      void (*pp)(AL_BITMAP *, void *, int, int, int, int);

      switch (size) {
         case 1:
            pp = _al_put_pixel8;
            break;
         case 2:
            pp = _al_put_pixel16;
            break;
         case 3:
            pp = _al_put_pixel24;
            break;
         case 4:
            pp = _al_put_pixel32;
            break;
      }

      dst->clip = 0;
      for (y = dy1; y <= dy2; y++) {
         pp(dst, d, dx, y, color_value, flags);
         d += lr.pitch;
      }
      dst->clip = clip;
   }
   else {
      #define DO_SOLID_VLINE(set, size) \
      { \
         for (y = dy1; y <= dy2; y++) { \
            set(d, color_value); \
            d += lr.pitch; \
         } \
      }

      unsigned char *d = lr.data;

      switch (size) {
         case 1:
            DO_SOLID_VLINE(bmp_write8, size)
            break;
         case 2:
            DO_SOLID_VLINE(bmp_write16, size)
            break;
         case 3:
            DO_SOLID_VLINE(WRITE3BYTES, size)
            break;
         case 4:
            DO_SOLID_VLINE(bmp_write32, size)
            break;
      }

      #undef DO_SOLID_VLINE
   }

   al_unlock_bitmap(dst);
}


/* Copied from do_line in gfx.c */
void _al_draw_line_memory(int x1, int y1, int x2, int y2, AL_COLOR *color, int flags)
{
   int dx;
   int dy;
   int i1, i2;
   int x, y;
   int dd;
   AL_LOCKED_REGION lr;
   int tmp;
   int d;
   AL_BITMAP *bitmap;
   int sx, sy, t;
   bool clip;
   int clip_x1, clip_y1, clip_x2, clip_y2;
   int xo, yo; /* offset to top left */

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond,     \
      set, size)                                                             \
   {                                                                         \
      if (d##pri_c == 0) {                                                   \
         set(bitmap, lr.data+y1*lr.pitch+x1*size, x1+xo, y1+yo, d, flags);   \
         al_unlock_bitmap(bitmap);                                           \
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
	 set(bitmap, lr.data+y*lr.pitch+x*size, x+xo, y+yo, d, flags);       \
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

   if (y1 == y2) {
      _al_draw_hline_memory(x1, y1, x2, color, flags);
      return;
   }
   else if (x1 == x2) {
      _al_draw_vline_memory(x1, y1, y2, color, flags);
      return;
   }

   bitmap = al_get_target_bitmap();

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

   d = _al_get_pixel_value(bitmap->format, color);

   al_lock_bitmap_region(bitmap, x1, y1, dx+1, dy+1, &lr, 0);

   if (clip) {
      clip_x1 = bitmap->cl - x1;
      clip_y1 = bitmap->ct - y1;
      clip_x2 = (x1 + dx - 1) - x1;
      clip_y2 = (y1 + dy - 1) - y1;
   }

   xo = x1;
   yo = y1;

   x2 -= x1;
   y2 -= y1;
   x1 = y1 = 0;

   if (clip) {
      switch (al_get_pixel_size(bitmap->format)) {
         case 1:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel8, 1);
            break;
         case 2:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel16, 2);
            break;
         case 3:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel24, 3);
            break;
         case 4:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel32, 4);
            break;
      }
   }
   else {
      switch (al_get_pixel_size(bitmap->format)) {
         case 1:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel_nc8, 1);
            break;
         case 2:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel_nc16, 2);
            break;
         case 3:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel_nc24, 3);
            break;
         case 4:
            DO_LINE(+, x, <=, +, y, >=, _al_put_pixel_nc32, 4);
            break;
      }
   }

   al_unlock_bitmap(bitmap);

   #undef DO_LINE
}

#define DO_FILLED_RECTANGLE(dst, dst_addr, func, dx, dy, w, h, value, flags) \
{ \
   int y; \
   unsigned char *line_ptr = dst_addr; \
 \
   for (y = 0; y < h; y++) { \
      func(dst, line_ptr, dx, dy+y, dx+w-1, value, flags); \
      line_ptr += lr.pitch; \
   } \
}


void _al_draw_rectangle_memory(int x1, int y1, int x2, int y2,
   AL_COLOR *color, int flags)
{
   AL_BITMAP *bitmap;
   AL_LOCKED_REGION lr;
   int w, h;
   int tmp;
   int pixel_value;

   if (!(flags & AL_FILLED)) {
      _al_draw_line_memory(x1, y1, x2, y1, color, flags);
      _al_draw_line_memory(x2, y1, x2, y2, color, flags);
      _al_draw_line_memory(x1, y1, x1, y2, color, flags);
      _al_draw_line_memory(x1, y2, x2, y2, color, flags);
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
         DO_FILLED_RECTANGLE(bitmap, lr.data, _hline8, x1, y1, w, h, pixel_value, flags)
	 break;
      case 2:
         DO_FILLED_RECTANGLE(bitmap, lr.data, _hline16, x1, y1, w, h, pixel_value, flags)
	 break;
      case 3:
         DO_FILLED_RECTANGLE(bitmap, lr.data, _hline24, x1, y1, w, h, pixel_value, flags)
	 break;
      case 4:
         DO_FILLED_RECTANGLE(bitmap, lr.data, _hline32, x1, y1, w, h, pixel_value, flags)
	 break;
   }

   al_unlock_bitmap(bitmap);
}


void _al_clear_memory(AL_COLOR *color)
{
   AL_BITMAP *bitmap = al_get_target_bitmap();

   _al_draw_rectangle_memory(0, 0, bitmap->w-1, bitmap->h-1, color, AL_FILLED);
}

