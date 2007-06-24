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

void _al_clear_memory(AL_COLOR *color)
{
   AL_BITMAP *bitmap = al_get_target_bitmap();
   AL_LOCKED_REGION lr;
   int pixel_value = _al_get_pixel_value(bitmap->format, color);

   al_lock_bitmap(bitmap, &lr, 0);

   switch (_al_get_pixel_size(bitmap->format)) {
      case 1:
         DO_FILLED_RECTANGLE(lr, 1, bmp_write8, bitmap->w, bitmap->h, pixel_value);
	 break;
      case 2:
         DO_FILLED_RECTANGLE(lr, 2, bmp_write16, bitmap->w, bitmap->h, pixel_value);
	 break;
      case 3:
         DO_FILLED_RECTANGLE(lr, 3, WRITE3BYTES, bitmap->w, bitmap->h, pixel_value);
	 break;
      case 4:
         DO_FILLED_RECTANGLE(lr, 4, bmp_write32, bitmap->w, bitmap->h, pixel_value);
	 break;
   }

   al_unlock_bitmap(bitmap);
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

   if (x1 < 0)
      x1 = 0;
   if (y1 < 0)
      y1 = 0;
   if (x2 > bitmap->w-1)
      x2 = bitmap->w-1;
   if (y2 > bitmap->h-1)
      y2 = bitmap->h-1;

   w = x2 - x1;
   h = y2 - y1;

   if (w <= 0 || h <= 0)
      return;

   pixel_value = _al_get_pixel_value(bitmap->format, color);

   al_lock_bitmap_region(bitmap, x1, y1, w, h, &lr, 0);

   switch (_al_get_pixel_size(bitmap->format)) {
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

   x2 -= x1;
   y2 -= y1;
   x1 = y1 = 0;

   switch (_al_get_pixel_size(bitmap->format)) {
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

   al_unlock_bitmap(bitmap);

   #undef DO_LINE
}

