#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_bitmap.h"

/* Memory bitmap blitting functions */

void _al_draw_bitmap_region_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   AL_BITMAP *dest = al_get_target_bitmap();
   int x;
   int y;
   AL_LOCKED_REGION lr;
   AL_COLOR src_pixel;
   AL_COLOR dst_pixel;
   AL_COLOR mask_color;
   int r, g, b, a;
   int csx, csy;         /* current source x/y */
   int cdx;              /* current dest x */
   int dxi, dyi;         /* dest increments */

   al_get_mask_color(&mask_color);

   /* Adjust for flipping */

   if (flags & AL_FLIP_HORIZONTAL) {
      dx = dx + sw - 1;
      dxi = -1;
   }
   else {
      dxi = 1;
   }

   if (flags & AL_FLIP_VERTICAL) {
      dy = dy + sh - 1;
      dyi = -1;
   }
   else {
      dyi = 1;
   }

   /* FIXME: do clipping */

   al_lock_bitmap_region(bitmap, sx, sy, sw, sh, &lr, AL_LOCK_READONLY);
   al_lock_bitmap_region(dest, dx, dy, sw, sh, &lr, 0);

   _al_push_target_bitmap();
   al_set_target_bitmap(dest);
   
   csy = sy;

   for (y = 0; y < sh; y++) {
      csx = sx;
      cdx = dx;
      for (x = 0; x < sw; x++) {
         al_get_pixel(bitmap, csx, csy, &src_pixel);
	 csx++;
	 /* Skip masked pixels if flag set */
	 if (flags & AL_MASK_SOURCE) {
	    if (memcmp(&src_pixel, &mask_color, sizeof(AL_COLOR)) == 0) {
	       cdx += dxi;
	       continue;
	    }
	 }
	 al_unmap_rgba_i(bitmap, &src_pixel, &r, &g, &b, &a);
	 al_map_rgba_i(dest, &dst_pixel, r, g, b, a);
	 al_put_pixel(cdx, dy, &dst_pixel);
	 cdx += dxi;
      }
      csy++;
      dy += dyi;
   }

   _al_pop_target_bitmap();

   al_unlock_bitmap(bitmap);
   al_unlock_bitmap(dest);
}

void _al_draw_bitmap_memory(AL_BITMAP *bitmap,
  int dx, int dy, int flags)
{
   _al_draw_bitmap_region_memory(bitmap, 0, 0, bitmap->w, bitmap->h,
      dx, dy, flags);
}


