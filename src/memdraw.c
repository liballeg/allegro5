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
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_memdraw.h"
#include "allegro5/internal/aintern_pixels.h"

/* generic versions of the video memory access helpers */
/* FIXME: why do we need macros for this? */
#define bmp_write16(addr, c)        (*((uint16_t *)(addr)) = (c))
#define bmp_write32(addr, c)        (*((uint32_t *)(addr)) = (c))

#define bmp_read16(addr)            (*((uint16_t *)(addr)))
#define bmp_read32(addr)            (*((uint32_t *)(addr)))

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


void _al_clear_bitmap_by_locking(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color)
{
   ALLEGRO_LOCKED_REGION *lr;
   int x1, y1, w, h;
   int x, y;
   unsigned char *line_ptr;

   /* This function is not just used on memory bitmaps, but also on OpenGL
    * video bitmaps which are not the current target, or when locked.
    */
   ASSERT(bitmap);
   ASSERT((al_get_bitmap_flags(bitmap) & (ALLEGRO_MEMORY_BITMAP | _ALLEGRO_INTERNAL_OPENGL)) ||
          _al_pixel_format_is_compressed(al_get_bitmap_format(bitmap)));

   x1 = bitmap->cl;
   y1 = bitmap->ct;
   w = bitmap->cr_excl - x1;
   h = bitmap->cb_excl - y1;

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
         int pixel_value = _AL_READ3BYTES(line_ptr);
         for (y = y1; y < y1 + h; y++) {
            unsigned char *data = (unsigned char *)line_ptr;
            if (pixel_value == 0) {    /* fast path */
               memset(data, 0, 3 * w);
            }
            else {
               for (x = 0; x < w; x++) {
                  _AL_WRITE3BYTES(data, pixel_value);
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

/* vim: set sts=3 sw=3 et: */
