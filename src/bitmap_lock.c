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
 *      Bitmap locking routines.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_pixels.h"


/* Function: al_lock_bitmap_region
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int width, int height, int format, int flags)
{
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(width >= 0);
   ASSERT(height >= 0);

   /* For sub-bitmaps */
   if (bitmap->parent) {
      x += bitmap->xofs;
      y += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked)
      return NULL;

   ASSERT(x+width <= bitmap->w);
   ASSERT(y+height <= bitmap->h);

   bitmap->lock_x = x;
   bitmap->lock_y = y;
   bitmap->lock_w = width;
   bitmap->lock_h = height;
   bitmap->lock_flags = flags;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      int f = _al_get_real_pixel_format(al_get_current_display(), format);
      if (f < 0) {
         return NULL;
      }
      ASSERT(bitmap->memory);
      if (format == ALLEGRO_PIXEL_FORMAT_ANY || bitmap->format == format || f == bitmap->format) {
         bitmap->locked_region.data = bitmap->memory
            + bitmap->pitch * y + x * al_get_pixel_size(bitmap->format);
         bitmap->locked_region.format = bitmap->format;
         bitmap->locked_region.pitch = bitmap->pitch;
         bitmap->locked_region.pixel_size = al_get_pixel_size(bitmap->format);
      }
      else {
         bitmap->locked_region.pitch = al_get_pixel_size(f) * width;
         bitmap->locked_region.data = al_malloc(bitmap->locked_region.pitch*height);
         bitmap->locked_region.format = f;
         bitmap->locked_region.pixel_size = al_get_pixel_size(f);
         if (!(bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY)) {
            _al_convert_bitmap_data(
               bitmap->memory, bitmap->format, bitmap->pitch,
               bitmap->locked_region.data, f, bitmap->locked_region.pitch,
               x, y, 0, 0, width, height);
         }
      }
   }
   else {
      if (!bitmap->vt->lock_region(bitmap, x, y, width, height, format, flags)) {
         return NULL;
      }
   }

   bitmap->locked = true;

   return &bitmap->locked_region;
}


/* Function: al_lock_bitmap
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap(ALLEGRO_BITMAP *bitmap,
   int format, int flags)
{
   return al_lock_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h, format, flags);
}


/* Function: al_unlock_bitmap
 */
void al_unlock_bitmap(ALLEGRO_BITMAP *bitmap)
{
   /* For sub-bitmaps */
   if (bitmap->parent) {
      bitmap = bitmap->parent;
   }

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
      bitmap->vt->unlock_region(bitmap);
   }
   else {
      if (bitmap->locked_region.format != 0 && bitmap->locked_region.format != bitmap->format) {
         if (!(bitmap->lock_flags & ALLEGRO_LOCK_READONLY)) {
            _al_convert_bitmap_data(
               bitmap->locked_region.data, bitmap->locked_region.format, bitmap->locked_region.pitch,
               bitmap->memory, bitmap->format, bitmap->pitch,
               0, 0, bitmap->lock_x, bitmap->lock_y, bitmap->lock_w, bitmap->lock_h);
         }
         al_free(bitmap->locked_region.data);
      }
   }

   bitmap->locked = false;
}


/* Function: al_is_bitmap_locked
 */
bool al_is_bitmap_locked(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->locked;
}


/* vim: set ts=8 sts=3 sw=3 et: */
