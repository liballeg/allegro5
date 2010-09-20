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
 *      Based on Michael Bukin's C drawing functions.
 *
 *      Conversion to the new API by Trent Gamblin.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"


typedef struct {
   float x[4];
} float4;


void _al_draw_pixel_memory(ALLEGRO_BITMAP *bitmap, float x, float y,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_COLOR result;
   int ix, iy;
   /*
    * Probably not worth it to check for identity
    */
   al_transform_coordinates(al_get_current_transform(), &x, &y);
   ix = (int)x;
   iy = (int)y;
   _al_blend_memory(color, bitmap, ix, iy, &result);
   _al_put_pixel(bitmap, ix, iy, result);
}


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
   int x, y;
   unsigned char *line_ptr;

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

   w = (x2 - x1) + 1;
   h = (y2 - y1) + 1;

   if (w <= 0 || h <= 0)
      return;

   /* XXX what about pre-locked bitmaps? */
   lr = al_lock_bitmap_region(bitmap, x1, y1, w, h, ALLEGRO_PIXEL_FORMAT_ANY, 0);
   if (!lr)
      return;

   /* Write a single pixel so we can get the raw value. */
   _al_put_pixel(bitmap, x1, y1, *color);

   /* Fill in the region. */
   line_ptr = lr->data;
   switch (lr->pixel_size) {
      case 2: {
         int pixel_value = bmp_read16(line_ptr);
         for (y = y1; y < y1 + h; y++) {
            if (pixel_value == 0) {    /* fast path */
               memset(line_ptr, 0, 2 * w);
            }
            else {
               uint16_t *data = (uint16_t *)line_ptr;
               for (x = 0; x < w; x++) {
                  bmp_write16(data, pixel_value);
                  data++;
               }
            }
            line_ptr += lr->pitch;
         }
         break;
      }

      case 3: {
         int pixel_value = READ3BYTES(line_ptr);
         for (y = y1; y < y1 + h; y++) {
            unsigned char *data = (unsigned char *)line_ptr;
            if (pixel_value == 0) {    /* fast path */
               memset(data, 0, 3 * w);
            }
            else {
               for (x = 0; x < w; x++) {
                  WRITE3BYTES(data, pixel_value);
                  data += 3;
               }
            }
            line_ptr += lr->pitch;
         }
         break;
      }

      case 4: {
         int pixel_value = bmp_read32(line_ptr);
         for (y = y1; y < y1 + h; y++) {
            uint32_t *data = (uint32_t *)line_ptr;
            /* Special casing pixel_value == 0 doesn't seem to make any
             * difference to speed, so don't bother.
             */
            for (x = 0; x < w; x++) {
               bmp_write32(data, pixel_value);
               data++;
            }
            line_ptr += lr->pitch;
         }
         break;
      }

      case sizeof(float4): {
         float4 *data = (float4 *)line_ptr;
         float4 pixel_value = *data;

         for (y = y1; y < y1 + h; y++) {
            data = (float4 *)line_ptr;
            for (x = 0; x < w; x++) {
               *data = pixel_value;
               data++;
            }
            line_ptr += lr->pitch;
         }
         break;
      }

      default:
        ASSERT(false);
        break;
   }

   al_unlock_bitmap(bitmap);
}


void _al_clear_memory(ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   _al_draw_filled_rectangle_memory_fast(0, 0, bitmap->w-1, bitmap->h-1,
      color);
}


/* vim: set sts=3 sw=3 et: */
