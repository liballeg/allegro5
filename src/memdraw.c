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

#define DEFINE_PUT_PIXEL(name, size, get, set)                               \
static void name(ALLEGRO_BITMAP *dst, void *dst_addr, int dx, int dy,        \
   int color)                                                                \
{                                                                            \
   ASSERT(dst);                                                              \
                                                                             \
   if ((dx < dst->cl) || (dx >= dst->cr) ||                                  \
       (dy < dst->ct) || (dy >= dst->cb))                                    \
      return;                                                                \
                                                                             \
   set(dst_addr, color);                                                     \
}

#define DEFINE_PUT_PIXEL_NC(name, size, get, set)                            \
static void name(ALLEGRO_BITMAP *dst, void *dst_addr, int dx, int dy,        \
   int color)                                                                \
{                                                                            \
   ASSERT(dst);                                                              \
                                                                             \
   set(dst_addr, color);                                                     \
}


DEFINE_PUT_PIXEL(_al_put_pixel8, 1, bmp_read8, bmp_write8)
DEFINE_PUT_PIXEL(_al_put_pixel16, 2, bmp_read16, bmp_write16)
DEFINE_PUT_PIXEL(_al_put_pixel24, 3, READ3BYTES, WRITE3BYTES)
DEFINE_PUT_PIXEL(_al_put_pixel32, 4, bmp_read32, bmp_write32)

DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc8, 1, bmp_read8, bmp_write8)
DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc16, 2, bmp_read16, bmp_write16)
DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc24, 3, READ3BYTES, WRITE3BYTES)
DEFINE_PUT_PIXEL_NC(_al_put_pixel_nc32, 4, bmp_read32, bmp_write32)


#define DEFINE_HLINE(name, size, get, set)                                   \
static void name(ALLEGRO_BITMAP *dst, unsigned char *dst_addr,               \
   int dx1, int dy, int dx2, int color)                                      \
{                                                                            \
   int w;                                                                    \
                                                                             \
   ASSERT(dst);                                                              \
                                                                             \
   w = dx2 - dx1;                                                            \
                                                                             \
   do {                                                                      \
      set(dst_addr, color);                                                  \
      dst_addr += size;                                                      \
   } while (--w >= 0);                                                       \
}


DEFINE_HLINE(_hline8, 1, bmp_read8, bmp_write8)
DEFINE_HLINE(_hline16, 2, bmp_read16, bmp_write16)
DEFINE_HLINE(_hline24, 3, READ3BYTES, WRITE3BYTES)
DEFINE_HLINE(_hline32, 4, bmp_read32, bmp_write32)


void _al_draw_hline_memory_fast(int dx1, int dy, int dx2, ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target;
   int color_value;
   ALLEGRO_LOCKED_REGION lr;

   target = al_get_target_bitmap();
   color_value = _al_get_pixel_value(target->format, color);

   if (dx1 > dx2) {
      int tmp = dx1;
      dx1 = dx2;
      dx2 = tmp;
   }

   if (dx1 < target->cl)
      dx1 = target->cl;
   if (dx2 >= target->cr)
      dx2 = target->cr - 1;
   if ((dx1 > dx2) || (dy < target->ct) || (dy >= target->cb))
      return;

   if (!al_lock_bitmap_region(target, dx1, dy, dx2-dx1+1, 1, &lr, 0))
      return;

   switch (al_get_pixel_size(target->format)) {
      case 1:
         _hline8(target, lr.data, dx1, dy, dx2, color_value);
         break;
      case 2:
         _hline16(target, lr.data, dx1, dy, dx2, color_value);
         break;
      case 3:
         _hline24(target, lr.data, dx1, dy, dx2, color_value);
         break;
      case 4:
         _hline32(target, lr.data, dx1, dy, dx2, color_value);
         break;
   }

   al_unlock_bitmap(target);
}


void _al_draw_vline_memory_fast(int dx, int dy1, int dy2, ALLEGRO_COLOR *color)
{
   int y;
   ALLEGRO_BITMAP *dst = al_get_target_bitmap();
   ALLEGRO_LOCKED_REGION lr;
   int size;
   int color_value;
   unsigned char *d;

   ASSERT(dst);

   if (dy1 > dy2) {
      int tmp = dy1;
      dy1 = dy2;
      dy2 = tmp;
   }

   if (dy1 < dst->ct)
      dy1 = dst->ct;
   if (dy2 >= dst->cb)
      dy2 = dst->cb - 1;
   if ((dx < dst->cl) || (dx >= dst->cr) || (dy1 > dy2))
      return;

   if (!al_lock_bitmap_region(dst, dx, dy1, 1, dy2-dy1+1, &lr, 0))
      return;

   size = al_get_pixel_size(dst->format);
   color_value = _al_get_pixel_value(dst->format, color);

   #define DO_SOLID_VLINE(set, size)                                         \
   do {                                                                      \
      for (y = dy1; y <= dy2; y++) {                                         \
         set(d, color_value);                                                \
         d += lr.pitch;                                                      \
      }                                                                      \
   } while (0)

   d = lr.data;

   switch (size) {
      case 1:
         DO_SOLID_VLINE(bmp_write8, size);
         break;
      case 2:
         DO_SOLID_VLINE(bmp_write16, size);
         break;
      case 3:
         DO_SOLID_VLINE(WRITE3BYTES, size);
         break;
      case 4:
         DO_SOLID_VLINE(bmp_write32, size);
         break;
   }

   #undef DO_SOLID_VLINE

   al_unlock_bitmap(dst);
}


/* Copied from do_line in gfx.c */
void _al_draw_line_memory_fast(int x1, int y1, int x2, int y2,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_LOCKED_REGION lr;
   ALLEGRO_BITMAP *bitmap;
   int bb_x1, bb_y1, bb_x2, bb_y2;  /* bounding box */
   int dx, dy;                      /* slope */
   int d;
   int clip;

   if (y1 == y2) {
      _al_draw_hline_memory_fast(x1, y1, x2, color);
      return;
   }
   else if (x1 == x2) {
      _al_draw_vline_memory_fast(x1, y1, y2, color);
      return;
   }

   /* make left coordinate first -- fewer cases to consider */
   if (x1 > x2) {
      int tmp;

      tmp = x1;
      x1 = x2;
      x2 = tmp;

      tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond,     \
      set, size)                                                             \
   do {                                                                      \
      int i1, i2;                                                            \
      int dd;                                                                \
      int x, y;                                                              \
                                                                             \
      if (d##pri_c == 0) {                                                   \
         set(bitmap, (void *)((intptr_t)lr.data + y1 * lr.pitch + x1 * size),\
            x1 + bb_x1, y1 + bb_y1, d);                                      \
         break;                                                              \
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
         set(bitmap, (void *)((intptr_t)lr.data + y * lr.pitch + x * size),  \
            x + bb_x1, y + bb_y1, d);                                        \
                                                                             \
         if (dd sec_cond 0) {                                                \
            sec_c = sec_c sec_sign 1;                                        \
            dd += i2;                                                        \
         }                                                                   \
         else {                                                              \
            dd += i1;                                                        \
         }                                                                   \
                                                                             \
         pri_c = pri_c pri_sign 1;                                           \
      }                                                                      \
   } while (0)

   bitmap = al_get_target_bitmap();

   /* compute bounding box */
   bb_x1 = x1;
   bb_x2 = x2;
   if (y1 < y2) {
      bb_y1 = y1;
      bb_y2 = y2;
   }
   else {
      bb_y1 = y2;
      bb_y2 = y1;
   }

   /* use a bounding box to check if the line needs clipping */
   if ((bb_x1 >= bitmap->cr) || (bb_y1 >= bitmap->cb) ||
       (bb_x2 <  bitmap->cl) || (bb_y2 <  bitmap->ct)) {
      return;
   }

   if ((bb_x1 >= bitmap->cl) && (bb_y1 >= bitmap->ct) &&
       (bb_x2 <  bitmap->cr) && (bb_y2 <  bitmap->cb))
      clip = FALSE;
   else
      clip = TRUE;

   /* XXX this is completely wrong */
   if (x1 < 0)
      x1 = 0;
   if (y1 < 0)
      y1 = 0;
   if (x2 > bitmap->w-1)
      x2 = bitmap->w-1;
   if (y2 > bitmap->h-1)
      y2 = bitmap->h-1;

   if (!al_lock_bitmap_region(bitmap, bb_x1, bb_y1,
         bb_x2 - bb_x1 + 1, bb_y2 - bb_y1 + 1, &lr, 0)) {
      TRACE("_al_draw_line_memory_fast failed to lock bitmap\n");
      return;
   }

   d = _al_get_pixel_value(bitmap->format, color);

   dx = x2 - x1;
   dy = y2 - y1;

   x1 -= bb_x1;
   y1 -= bb_y1;
   x2 -= bb_x1;
   y2 -= bb_y1;

   #define CALL_DO_LINE(set, size)                                           \
   do {                                                                      \
      ASSERT(dx >= 0);                                                       \
      if (dy >= 0) {                                                         \
         if (dx >= dy) {                                                     \
            /* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */                     \
            DO_LINE(+, x, <=, +, y, >=, set, size);                          \
         }                                                                   \
         else {                                                              \
            /* (x1 <= x2) && (y1 <= y2) && (dx < dy) */                      \
            DO_LINE(+, y, <=, +, x, >=, set, size);                          \
         }                                                                   \
      }                                                                      \
      else {                                                                 \
         if (dx >= -dy) {                                                    \
            /* (x1 <= x2) && (y1 > y2) && (dx >= dy) */                      \
            DO_LINE(+, x, <=, -, y, <=, set, size);                          \
         }                                                                   \
         else {                                                              \
            /* (x1 <= x2) && (y1 > y2) && (dx < dy) */                       \
            DO_LINE(-, y, >=, +, x, >=, set, size);                          \
         }                                                                   \
      }                                                                      \
   } while (0)

   /* XXX if the line was properly pre-clipped we wouldn't need to check every
    * pixel
    */
   if (clip) {
      switch (al_get_pixel_size(bitmap->format)) {
         case 1:
            CALL_DO_LINE(_al_put_pixel8, 1);
            break;
         case 2:
            CALL_DO_LINE(_al_put_pixel16, 2);
            break;
         case 3:
            CALL_DO_LINE(_al_put_pixel24, 3);
            break;
         case 4:
            CALL_DO_LINE(_al_put_pixel32, 4);
            break;
      }
   }
   else {
      switch (al_get_pixel_size(bitmap->format)) {
         case 1:
            CALL_DO_LINE(_al_put_pixel_nc8, 1);
            break;
         case 2:
            CALL_DO_LINE(_al_put_pixel_nc16, 2);
            break;
         case 3:
            CALL_DO_LINE(_al_put_pixel_nc24, 3);
            break;
         case 4:
            CALL_DO_LINE(_al_put_pixel_nc32, 4);
            break;
      }
   }

   al_unlock_bitmap(bitmap);

   #undef DO_LINE
   #undef CALL_DO_LINE
}

#define DO_FILLED_RECTANGLE_FAST(dst, dst_addr, func, dx, dy, w, h, value)   \
do {                                                                         \
   int y;                                                                    \
   unsigned char *line_ptr = dst_addr;                                       \
                                                                             \
   for (y = 0; y < h; y++) {                                                 \
      func(dst, line_ptr, dx, dy+y, dx+w-1, value);                          \
      line_ptr += lr.pitch;                                                  \
   }                                                                         \
} while (0)


void _al_draw_rectangle_memory_fast(int x1, int y1, int x2, int y2,
   ALLEGRO_COLOR *color, int flags)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION lr;
   int w, h;
   int tmp;
   int pixel_value;

   if (!(flags & ALLEGRO_FILLED)) {
      _al_draw_hline_memory_fast(x1, y1, x2, color);
      _al_draw_vline_memory_fast(x2, y1, y2, color);
      _al_draw_vline_memory_fast(x1, y1, y2, color);
      _al_draw_hline_memory_fast(x1, y2, x2, color);
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

   w = x2 - x1 + 1;
   h = y2 - y1 + 1;

   /* Do clipping */
   if (x1 < bitmap->cl) {
      w -= (bitmap->cl - x1);
      x1 = bitmap->cl;
   }
   if (y1 < bitmap->ct) {
      h -= (bitmap->ct - y1);
      y1 = bitmap->ct;
   }
   if (x2 > bitmap->cr) {
      w -= (x2 - bitmap->cr);
      x2 = bitmap->cr;
   }
   if (y2 > bitmap->cb) {
      h -= (y2 - bitmap->cb);
      y2 = bitmap->cb;
   }

   if (w <= 0 || h <= 0)
      return;

   pixel_value = _al_get_pixel_value(bitmap->format, color);

   al_lock_bitmap_region(bitmap, x1, y1, w, h, &lr, 0);

   switch (al_get_pixel_size(bitmap->format)) {
      case 1:
         DO_FILLED_RECTANGLE_FAST(bitmap, lr.data, _hline8, x1, y1, w, h,
            pixel_value);
         break;
      case 2:
         DO_FILLED_RECTANGLE_FAST(bitmap, lr.data, _hline16, x1, y1, w, h,
            pixel_value);
         break;
      case 3:
         DO_FILLED_RECTANGLE_FAST(bitmap, lr.data, _hline24, x1, y1, w, h,
            pixel_value);
         break;
      case 4:
         DO_FILLED_RECTANGLE_FAST(bitmap, lr.data, _hline32, x1, y1, w, h,
            pixel_value);
         break;
   }

   al_unlock_bitmap(bitmap);
}


void _al_clear_memory(ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   _al_draw_rectangle_memory_fast(0, 0, bitmap->w-1, bitmap->h-1, color,
      ALLEGRO_FILLED);
}



static void _hline(int x1, int y, int x2, ALLEGRO_COLOR *color)
{
   int x;

   for (x = x1; x <= x2; x++) {
      ALLEGRO_COLOR result;
      _al_blend(color, x, y, &result);
      al_put_pixel(x, y, result);
   }
}



static void _vline(int x, int y1, int y2, ALLEGRO_COLOR *color)
{
   int y;

   for (y = y1; y <= y2; y++) {
      ALLEGRO_COLOR result;
      _al_blend(color, x, y, &result);
      al_put_pixel(x, y, result);
   }
}



void _al_draw_hline_memory(int dx1, int dy, int dx2, ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target;
   ALLEGRO_LOCKED_REGION lr;

   target = al_get_target_bitmap();

   if (dx1 > dx2) {
      int tmp = dx1;
      dx1 = dx2;
      dx2 = tmp;
   }

   if (dx1 < target->cl)
      dx1 = target->cl;
   if (dx2 >= target->cr)
      dx2 = target->cr - 1;
   if ((dx1 > dx2) || (dy < target->ct) || (dy >= target->cb))
      return;

   if (!al_lock_bitmap_region(target, dx1, dy, dx2-dx1+1, 1, &lr, 0))
      return;

   _hline(dx1, dy, dx2, color);

   al_unlock_bitmap(target);
}


void _al_draw_vline_memory(int dx, int dy1, int dy2, ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *dst = al_get_target_bitmap();
   ALLEGRO_LOCKED_REGION lr;

   ASSERT(dst);

   if (dy1 > dy2) {
      int tmp = dy1;
      dy1 = dy2;
      dy2 = tmp;
   }

   if (dy1 < dst->ct)
      dy1 = dst->ct;
   if (dy2 >= dst->cb)
      dy2 = dst->cb - 1;
   if ((dx < dst->cl) || (dx >= dst->cr) || (dy1 > dy2))
      return;

   if (!al_lock_bitmap_region(dst, dx, dy1, 1, dy2-dy1+1, &lr, 0))
      return;

   _vline(dx, dy1, dy2, color);

   al_unlock_bitmap(dst);
}


/* Copied from do_line in gfx.c */
void _al_draw_line_memory(int x1, int y1, int x2, int y2, ALLEGRO_COLOR *color)
{
   ALLEGRO_LOCKED_REGION lr;
   ALLEGRO_BITMAP *bitmap;
   int bb_x1, bb_y1, bb_x2, bb_y2;  /* bounding box */
   int dx, dy;                      /* slope */

   /* special cases */
   {
      int src_mode, dst_mode;
      ALLEGRO_COLOR *ic;

      al_get_blender(&src_mode, &dst_mode, NULL);
      ic = _al_get_blend_color();
      if (src_mode == ALLEGRO_ONE && dst_mode == ALLEGRO_ZERO &&
            ic->r == 1.0f && ic->g == 1.0f && ic->b == 1.0f && ic->a == 1.0f)
      {
         _al_draw_line_memory_fast(x1, y1, x2, y2, color);
         return;
      }
   }

   if (y1 == y2) {
      _al_draw_hline_memory(x1, y1, x2, color);
      return;
   }
   else if (x1 == x2) {
      _al_draw_vline_memory(x1, y1, y2, color);
      return;
   }

   /* make left coordinate first -- fewer cases to consider */
   if (x1 > x2) {
      int tmp;

      tmp = x1;
      x1 = x2;
      x2 = tmp;

      tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   /* worker macro */
   #define DO_LINE(pri_sign, pri_c, pri_cond, sec_sign, sec_c, sec_cond,     \
      set)                                                                   \
   do {                                                                      \
      int i1, i2;                                                            \
      int dd;                                                                \
      int x, y;                                                              \
                                                                             \
      if (d##pri_c == 0) {                                                   \
         set(x1, y1, color);                                                 \
         break;                                                              \
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
         set(x, y, color);                                                   \
                                                                             \
         if (dd sec_cond 0) {                                                \
            sec_c = sec_c sec_sign 1;                                        \
            dd += i2;                                                        \
         }                                                                   \
         else {                                                              \
            dd += i1;                                                        \
         }                                                                   \
                                                                             \
         pri_c = pri_c pri_sign 1;                                           \
      }                                                                      \
   } while (0)

   bitmap = al_get_target_bitmap();

   /* compute bounding box */
   bb_x1 = x1;
   bb_x2 = x2;
   if (y1 < y2) {
      bb_y1 = y1;
      bb_y2 = y2;
   }
   else {
      bb_y1 = y2;
      bb_y2 = y1;
   }

   /* use a bounding box to check if the line needs clipping */
   if ((bb_x1 >= bitmap->cr) || (bb_y1 >= bitmap->cb) ||
       (bb_x2 <  bitmap->cl) || (bb_y2 <  bitmap->ct)) {
      return;
   }

   /* XXX this is completely wrong */
   /* XXX we need to clip against the bitmap's clipping rectangle as well
    * so that we don't lock anything outside of the clipping rectangle
    */
   if (x1 < 0)
      x1 = 0;
   if (y1 < 0)
      y1 = 0;
   if (x2 > bitmap->w-1)
      x2 = bitmap->w-1;
   if (y2 > bitmap->h-1)
      y2 = bitmap->h-1;

   if (!al_lock_bitmap_region(bitmap, bb_x1, bb_y1,
         bb_x2 - bb_x1 + 1, bb_y2 - bb_y1 + 1, &lr, 0)) {
      TRACE("_al_draw_line_memory failed to lock bitmap\n");
      return;
   }

   dx = x2 - x1;
   dy = y2 - y1;

   ASSERT(dx >= 0);
   if (dy >= 0) {
      if (dx >= dy) {
         /* (x1 <= x2) && (y1 <= y2) && (dx >= dy) */
         DO_LINE(+, x, <=, +, y, >=, _al_draw_pixel_memory);
      }
      else {
         /* (x1 <= x2) && (y1 <= y2) && (dx < dy) */
         DO_LINE(+, y, <=, +, x, >=, _al_draw_pixel_memory);
      }
   }
   else {
      if (dx >= -dy) {
         /* (x1 <= x2) && (y1 > y2) && (dx >= dy) */
         DO_LINE(+, x, <=, -, y, <=, _al_draw_pixel_memory);
      }
      else {
         /* (x1 <= x2) && (y1 > y2) && (dx < dy) */
         DO_LINE(-, y, >=, +, x, >=, _al_draw_pixel_memory);
      }
   }

   al_unlock_bitmap(bitmap);

   #undef DO_LINE
}

#define DO_FILLED_RECTANGLE(func, dx, dy, w, h, color)                       \
do {                                                                         \
   int y;                                                                    \
                                                                             \
   for (y = 0; y < h; y++) {                                                 \
      func(dx, dy+y, dx+w-1, color);                                         \
   }                                                                         \
} while (0)


void _al_draw_rectangle_memory(int x1, int y1, int x2, int y2,
   ALLEGRO_COLOR *color, int flags)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION lr;
   int w, h;
   int tmp;
   int src_mode, dst_mode;
   ALLEGRO_COLOR *ic;

   al_get_blender(&src_mode, &dst_mode, NULL);
   ic = _al_get_blend_color();
   if (src_mode == ALLEGRO_ONE && dst_mode == ALLEGRO_ZERO &&
         ic->r == 1.0f && ic->g == 1.0f && ic->b == 1.0f && ic->a == 1.0f)
   {
      _al_draw_rectangle_memory_fast(x1, y1, x2, y2, color, flags);
      return;
   }

   if (!(flags & ALLEGRO_FILLED)) {
      _al_draw_hline_memory(x1, y1, x2, color);
      _al_draw_vline_memory(x2, y1, y2, color);
      _al_draw_vline_memory(x1, y1, y2, color);
      _al_draw_hline_memory(x1, y2, x2, color);
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
   if (x1 < bitmap->cl) {
      x1 = bitmap->cl;
   }
   if (y1 < bitmap->ct) {
      y1 = bitmap->ct;
   }
   if (x2 > bitmap->cr) {
      x2 = bitmap->cr;
   }
   if (y2 > bitmap->cb) {
      y2 = bitmap->cb;
   }

   w = x2 - x1 + 1;
   h = y2 - y1 + 1;

   if (w <= 0 || h <= 0)
      return;

   al_lock_bitmap_region(bitmap, x1, y1, w, h, &lr, 0);

   DO_FILLED_RECTANGLE(_hline, x1, y1, w, h, color);

   al_unlock_bitmap(bitmap);
}

void _al_draw_pixel_memory(int x, int y, ALLEGRO_COLOR *color)
{
   ALLEGRO_COLOR result;
   _al_blend(color, x, y, &result);
   al_put_pixel(x, y, result);
}

/* vim: set sts=3 sw=3 et: */
