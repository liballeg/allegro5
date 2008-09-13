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
 *      By Trent Gamblin
 *
 */

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/convert.h"
#include <math.h>


void _al_draw_scaled_bitmap_memory_fast(ALLEGRO_BITMAP *src,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_LOCKED_REGION src_region;
   ALLEGRO_LOCKED_REGION dst_region;

   float sxinc;
   float syinc;
   float _sx;
   float _sy;
   float dxinc;
   float dyinc;
   float _dx;
   float _dy;
   int x, y;
   int xend;
   int yend;
   int size;

   if ((sw <= 0) || (sh <= 0))
      return;

   /* This must be calculated before clipping dw, dh. */
   sxinc = fabs((float)sw / dw);
   syinc = fabs((float)sh / dh);

   /* Do clipping */
   dy = ((dy > dest->ct) ? dy : dest->ct);
   dh = (((dy + dh) < dest->cb + 1) ? (dy + dh) : dest->cb + 1) - dy;

   dx = ((dx > dest->cl) ? dx : dest->cl);
   dw = (((dx + dw) < dest->cr + 1) ? (dx + dw) : dest->cr + 1) - dx;

   if (dw == 0 || dh == 0)
   	return;

   al_lock_bitmap(src, &src_region, ALLEGRO_LOCK_READONLY);
   /* XXX we should be able to lock less of the destination and use
    * ALLEGRO_LOCK_WRITEONLY
    */
   al_lock_bitmap(dest, &dst_region, 0);

   dxinc = dw < 0 ? -1 : 1;
   dyinc = dh < 0 ? -1 : 1;
   _dy = dy;
   xend = abs(dw);
   yend = abs(dh);

   if (flags & ALLEGRO_FLIP_HORIZONTAL) {
      sxinc = -sxinc;
      sx = sx + sw - 1;
   }

   if (flags & ALLEGRO_FLIP_VERTICAL) {
      syinc = -syinc;
      _sy = sy + sh - 1;
   }
   else
      _sy = sy;


   size = al_get_pixel_size(src->format);

   for (y = 0; y < yend; y++) {
      _sx = sx;
      _dx = dx;
      switch (size) {
         case 2:
               for (x = 0; x < xend; x++) {
                  uint16_t pix = bmp_read16((char *)src_region.data+(int)_sy*src_region.pitch+(int)_sx*2);
                  bmp_write16((char *)dst_region.data+(int)_dy*dst_region.pitch+(int)_dx*2, pix);
                  _sx += sxinc;
                  _dx += dxinc;
               }
            break;
         case 3:
               for (x = 0; x < xend; x++) {
                  int pix = READ3BYTES((char *)src_region.data+(int)_sy*src_region.pitch+(int)_sx*3);
                  WRITE3BYTES((char *)dst_region.data+(int)_dy*dst_region.pitch+(int)_dx*3, pix);
                  _sx += sxinc;
                  _dx += dxinc;
               }
            break;
         case 4:
               for (x = 0; x < xend; x++) {
                  uint32_t pix = bmp_read32((char *)src_region.data+(int)_sy*src_region.pitch+(int)_sx*4);
                  bmp_write32((char *)dst_region.data+(int)_dy*dst_region.pitch+(int)_dx*4, pix);
                  _sx += sxinc;
                  _dx += dxinc;
               }
            break;
      }
      _sy += syinc;
      _dy += dyinc;
   }

   al_unlock_bitmap(src);
   al_unlock_bitmap(dest);
}

