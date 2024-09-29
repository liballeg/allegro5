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
A5O_LOCKED_REGION *al_lock_bitmap_region(A5O_BITMAP *bitmap,
   int x, int y, int width, int height, int format, int flags)
{
   A5O_LOCKED_REGION *lr;
   int bitmap_format = al_get_bitmap_format(bitmap);
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   int block_width = al_get_pixel_block_width(bitmap_format);
   int block_height = al_get_pixel_block_height(bitmap_format);
   int xc, yc, wc, hc;
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(width >= 0);
   ASSERT(height >= 0);
   ASSERT(!_al_pixel_format_is_video_only(format));
   if (_al_pixel_format_is_real(format)) {
      ASSERT(al_get_pixel_block_width(format) == 1);
      ASSERT(al_get_pixel_block_height(format) == 1);
   }

   /* For sub-bitmaps */
   if (bitmap->parent) {
      x += bitmap->xofs;
      y += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked)
      return NULL;

   if (!(bitmap_flags & A5O_MEMORY_BITMAP) &&
         !(flags & A5O_LOCK_READONLY))
      bitmap->dirty = true;

   ASSERT(x+width <= bitmap->w);
   ASSERT(y+height <= bitmap->h);

   xc = (x / block_width) * block_width;
   yc = (y / block_height) * block_height;
   wc = _al_get_least_multiple(x + width, block_width) - xc;
   hc = _al_get_least_multiple(y + height, block_height) - yc;

   bitmap->lock_x = xc;
   bitmap->lock_y = yc;
   bitmap->lock_w = wc;
   bitmap->lock_h = hc;
   bitmap->lock_flags = flags;

   if (flags == A5O_LOCK_WRITEONLY &&
       (xc != x || yc != y || wc != width || hc != height)) {
      /* Unaligned write-only access requires that we fill in the padding
       * from the texture.
       * XXX: In principle, this could be done more efficiently. */
      flags = A5O_LOCK_READWRITE;
   }

   if (bitmap_flags & A5O_MEMORY_BITMAP) {
      int f = _al_get_real_pixel_format(al_get_current_display(), format);
      if (f < 0) {
         return NULL;
      }
      ASSERT(bitmap->memory);
      if (format == A5O_PIXEL_FORMAT_ANY || bitmap_format == format || bitmap_format == f) {
         bitmap->locked_region.data = bitmap->memory
            + bitmap->pitch * yc + xc * al_get_pixel_size(bitmap_format);
         bitmap->locked_region.format = bitmap_format;
         bitmap->locked_region.pitch = bitmap->pitch;
         bitmap->locked_region.pixel_size = al_get_pixel_size(bitmap_format);
      }
      else {
         bitmap->locked_region.pitch = al_get_pixel_size(f) * wc;
         bitmap->locked_region.data = al_malloc(bitmap->locked_region.pitch*hc);
         bitmap->locked_region.format = f;
         bitmap->locked_region.pixel_size = al_get_pixel_size(f);
         if (!(bitmap->lock_flags & A5O_LOCK_WRITEONLY)) {
            _al_convert_bitmap_data(
               bitmap->memory, bitmap_format, bitmap->pitch,
               bitmap->locked_region.data, f, bitmap->locked_region.pitch,
               xc, yc, 0, 0, wc, hc);
         }
      }
      lr = &bitmap->locked_region;
   }
   else {
      lr = bitmap->vt->lock_region(bitmap, xc, yc, wc, hc, format, flags);
      if (!lr) {
         return NULL;
      }
   }

   bitmap->lock_data = lr->data;
   /* Fixup the data pointer for unaligned access */
   lr->data = (char*)lr->data + (x - xc) * lr->pixel_size + (y - yc) * lr->pitch;

   bitmap->locked = true;

   return lr;
}


/* Function: al_lock_bitmap
 */
A5O_LOCKED_REGION *al_lock_bitmap(A5O_BITMAP *bitmap,
   int format, int flags)
{
   return al_lock_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h, format, flags);
}


/* Function: al_unlock_bitmap
 */
void al_unlock_bitmap(A5O_BITMAP *bitmap)
{
   int bitmap_format = al_get_bitmap_format(bitmap);
   /* For sub-bitmaps */
   if (bitmap->parent) {
      bitmap = bitmap->parent;
   }

   if (!(al_get_bitmap_flags(bitmap) & A5O_MEMORY_BITMAP)) {
      if (_al_pixel_format_is_compressed(bitmap->locked_region.format))
         bitmap->vt->unlock_compressed_region(bitmap);
      else
         bitmap->vt->unlock_region(bitmap);
   }
   else {
      if (bitmap->locked_region.format != 0 && bitmap->locked_region.format != bitmap_format) {
         if (!(bitmap->lock_flags & A5O_LOCK_READONLY)) {
            _al_convert_bitmap_data(
               bitmap->locked_region.data, bitmap->locked_region.format, bitmap->locked_region.pitch,
               bitmap->memory, bitmap_format, bitmap->pitch,
               0, 0, bitmap->lock_x, bitmap->lock_y, bitmap->lock_w, bitmap->lock_h);
         }
         al_free(bitmap->locked_region.data);
      }
   }

   bitmap->locked = false;
}


/* Function: al_is_bitmap_locked
 */
bool al_is_bitmap_locked(A5O_BITMAP *bitmap)
{
   return bitmap->locked;
}

/* Function: al_lock_bitmap_blocked
 */
A5O_LOCKED_REGION *al_lock_bitmap_blocked(A5O_BITMAP *bitmap,
   int flags)
{
   int bitmap_format = al_get_bitmap_format(bitmap);
   int block_width = al_get_pixel_block_width(bitmap_format);
   int block_height = al_get_pixel_block_height(bitmap_format);

   return al_lock_bitmap_region_blocked(bitmap, 0, 0,
      _al_get_least_multiple(bitmap->w, block_width) / block_width,
      _al_get_least_multiple(bitmap->h, block_height) / block_height,
      flags);
}

/* Function: al_lock_bitmap_region_blocked
 */
A5O_LOCKED_REGION *al_lock_bitmap_region_blocked(A5O_BITMAP *bitmap,
   int x_block, int y_block, int width_block, int height_block, int flags)
{
   A5O_LOCKED_REGION *lr;
   int bitmap_format = al_get_bitmap_format(bitmap);
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   int block_width = al_get_pixel_block_width(bitmap_format);
   int block_height = al_get_pixel_block_height(bitmap_format);
   ASSERT(x_block >= 0);
   ASSERT(y_block >= 0);
   ASSERT(width_block >= 0);
   ASSERT(height_block >= 0);

   if (block_width == 1 && block_height == 1 && !_al_pixel_format_is_video_only(bitmap_format)) {
      return al_lock_bitmap_region(bitmap, x_block, y_block, width_block,
         height_block, bitmap_format, flags);
   }

   /* Currently, this is the only format that gets to this point */
   ASSERT(_al_pixel_format_is_compressed(bitmap_format));
   ASSERT(!(bitmap_flags & A5O_MEMORY_BITMAP));

   /* For sub-bitmaps */
   if (bitmap->parent) {
      if (bitmap->xofs % block_width != 0 ||
          bitmap->yofs % block_height != 0) {
         return NULL;
      }
      x_block += bitmap->xofs / block_width;
      y_block += bitmap->yofs / block_height;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked)
      return NULL;

   if (!(flags & A5O_LOCK_READONLY))
      bitmap->dirty = true;

   ASSERT(x_block + width_block
      <= _al_get_least_multiple(bitmap->w, block_width) / block_width);
   ASSERT(y_block + height_block
      <= _al_get_least_multiple(bitmap->h, block_height) / block_height);

   bitmap->lock_x = x_block * block_width;
   bitmap->lock_y = y_block * block_height;
   bitmap->lock_w = width_block * block_width;
   bitmap->lock_h = height_block * block_height;
   bitmap->lock_flags = flags;

   lr = bitmap->vt->lock_compressed_region(bitmap, bitmap->lock_x,
      bitmap->lock_y, bitmap->lock_w, bitmap->lock_h, flags);
   if (!lr) {
      return NULL;
   }

   bitmap->locked = true;

   return lr;
}

/* vim: set ts=8 sts=3 sw=3 et: */
