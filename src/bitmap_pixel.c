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
 *      Bitmap pixel manipulation.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <string.h> /* for memset */
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_pixels.h"

ALLEGRO_DEBUG_CHANNEL("bitmap")


/* Function: al_get_pixel
 */
ALLEGRO_COLOR al_get_pixel(ALLEGRO_BITMAP *bitmap, int x, int y)
{
   ALLEGRO_LOCKED_REGION *lr;
   char *data;
   ALLEGRO_COLOR color;

   if (bitmap->parent) {
      x += bitmap->xofs;
      y += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked) {
      x -= bitmap->lock_x;
      y -= bitmap->lock_y;
      if (x < 0 || y < 0 || x >= bitmap->lock_w || y >= bitmap->lock_h) {
         ALLEGRO_ERROR("Out of bounds.");
         memset(&color, 0, sizeof(ALLEGRO_COLOR));
         return color;
      }

      data = bitmap->locked_region.data;
      data += y * bitmap->locked_region.pitch;
      data += x * al_get_pixel_size(bitmap->locked_region.format);

      _AL_INLINE_GET_PIXEL(bitmap->locked_region.format, data, color, false);
   }
   else {
      /* FIXME: must use clip not full bitmap */
      if (x < 0 || y < 0 || x >= bitmap->w || y >= bitmap->h) {
         memset(&color, 0, sizeof(ALLEGRO_COLOR));
         return color;
      }

      if (!(lr = al_lock_bitmap_region(bitmap, x, y, 1, 1, bitmap->format,
            ALLEGRO_LOCK_READONLY)))
      {
         memset(&color, 0, sizeof(ALLEGRO_COLOR));
         return color;
      }

      /* FIXME: check for valid pixel format */

      data = lr->data;
      _AL_INLINE_GET_PIXEL(bitmap->format, data, color, false);

      al_unlock_bitmap(bitmap);
   }

   return color;
}


void _al_put_pixel(ALLEGRO_BITMAP *bitmap, int x, int y, ALLEGRO_COLOR color)
{
   ALLEGRO_LOCKED_REGION *lr;
   char *data;

   if (bitmap->parent) {
       x += bitmap->xofs;
       y += bitmap->yofs;
       bitmap = bitmap->parent;
   }

   if (x < bitmap->cl || y < bitmap->ct ||
       x >= bitmap->cr_excl || y >= bitmap->cb_excl)
   {
      return;
   }

   if (bitmap->locked) {
      x -= bitmap->lock_x;
      y -= bitmap->lock_y;
      if (x < 0 || y < 0 || x >= bitmap->lock_w || y >= bitmap->lock_h) {
         return;
      }

      data = bitmap->locked_region.data;
      data += y * bitmap->locked_region.pitch;
      data += x * al_get_pixel_size(bitmap->locked_region.format);

      _AL_INLINE_PUT_PIXEL(bitmap->locked_region.format, data, color, false);
   }
   else {
      lr = al_lock_bitmap_region(bitmap, x, y, 1, 1, bitmap->format,
         ALLEGRO_LOCK_WRITEONLY);
      if (!lr)
         return;

      /* FIXME: check for valid pixel format */

      data = lr->data;
      _AL_INLINE_PUT_PIXEL(bitmap->format, data, color, false);

      al_unlock_bitmap(bitmap);
   }
}


/* Function: al_put_pixel
 */
void al_put_pixel(int x, int y, ALLEGRO_COLOR color)
{
   _al_put_pixel(al_get_target_bitmap(), x, y, color);
}


/* Function: al_put_blended_pixel
 */
void al_put_blended_pixel(int x, int y, ALLEGRO_COLOR color)
{
   ALLEGRO_COLOR result;
   ALLEGRO_BITMAP* bitmap = al_get_target_bitmap();
   _al_blend_memory(&color, bitmap, x, y, &result);
   _al_put_pixel(bitmap, x, y, result);
}


/* vim: set sts=3 sw=3 et: */
