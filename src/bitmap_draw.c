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
 *      Bitmap drawing routines.
 *
 *      See LICENSE.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memblit.h"
#include "allegro5/internal/aintern_pixels.h"


static ALLEGRO_COLOR solid_white = {1, 1, 1, 1};


static void _bitmap_drawer(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_DISPLAY *display = _al_get_bitmap_display(dest);
   ASSERT(bitmap->parent == NULL);
   ASSERT(!(flags & (ALLEGRO_FLIP_HORIZONTAL | ALLEGRO_FLIP_VERTICAL)));
   ASSERT(bitmap != dest && bitmap != dest->parent);

   /* If destination is memory, do a memory blit */
   if (al_get_bitmap_flags(dest) & ALLEGRO_MEMORY_BITMAP ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(dest))) {
      _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh, 0, 0, flags);
   }
   else {
      /* if source is memory or incompatible */
      if ((al_get_bitmap_flags(bitmap) & ALLEGRO_MEMORY_BITMAP) ||
          (!al_is_compatible_bitmap(bitmap)))
      {
         if (display && display->vt->draw_memory_bitmap_region) {
            display->vt->draw_memory_bitmap_region(display, bitmap,
               sx, sy, sw, sh, flags);
         }
         else {
            _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh, 0, 0, flags);
         }
      }
      else {
         /* Compatible display bitmap, use full acceleration */
         bitmap->vt->draw_bitmap_region(bitmap, tint, sx, sy, sw, sh, flags);
      }
   }
}


static void _draw_tinted_rotated_scaled_bitmap_region(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint, float cx, float cy, float angle,
   float xscale, float yscale,
   float sx, float sy, float sw, float sh, float dx, float dy,
   int flags)
{
   ALLEGRO_TRANSFORM backup;
   ALLEGRO_TRANSFORM t;
   ALLEGRO_BITMAP *parent = bitmap;
   float const orig_sw = sw;
   float const orig_sh = sh;
   ASSERT(bitmap);

   al_copy_transform(&backup, al_get_current_transform());
   al_identity_transform(&t);

   if (bitmap->parent) {
      parent = bitmap->parent;
      sx += bitmap->xofs;
      sy += bitmap->yofs;
   }

   if (sx < 0) {
      sw += sx;
      al_translate_transform(&t, -sx, 0);
      sx = 0;
   }
   if (sy < 0) {
      sh += sy;
      al_translate_transform(&t, 0, -sy);
      sy = 0;
   }
   if (sx + sw > parent->w)
      sw = parent->w - sx;
   if (sy + sh > parent->h)
      sh = parent->h - sy;

   if (flags & ALLEGRO_FLIP_HORIZONTAL) {
      al_scale_transform(&t, -1, 1);
      al_translate_transform(&t, orig_sw, 0);
      flags &= ~ALLEGRO_FLIP_HORIZONTAL;
   }

   if (flags & ALLEGRO_FLIP_VERTICAL) {
      al_scale_transform(&t, 1, -1);
      al_translate_transform(&t, 0, orig_sh);
      flags &= ~ALLEGRO_FLIP_VERTICAL;
   }

   al_translate_transform(&t, -cx, -cy);
   al_scale_transform(&t, xscale, yscale);
   al_rotate_transform(&t, angle);
   al_translate_transform(&t, dx, dy);
   al_compose_transform(&t, &backup);

   al_use_transform(&t);
   _bitmap_drawer(parent, tint, sx, sy, sw, sh, flags);
   al_use_transform(&backup);
}


/* Function: al_draw_tinted_bitmap_region
 */
void al_draw_tinted_bitmap_region(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, float dx, float dy,
   int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      0, 0, 0, 1, 1, sx, sy, sw, sh, dx, dy, flags);
}


/* Function: al_draw_tinted_bitmap
 */
void al_draw_tinted_bitmap(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint,
   float dx, float dy, int flags)
{
   ASSERT(bitmap);
   al_draw_tinted_bitmap_region(bitmap, tint, 0, 0,
      bitmap->w, bitmap->h, dx, dy, flags);
}


/* Function: al_draw_bitmap
 */
void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
   al_draw_tinted_bitmap(bitmap, solid_white, dx, dy, flags);
}


/* Function: al_draw_bitmap_region
 */
void al_draw_bitmap_region(ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh, float dx, float dy, int flags)
{
   al_draw_tinted_bitmap_region(bitmap, solid_white, sx, sy, sw, sh,
      dx, dy, flags);
}


/* Function: al_draw_tinted_scaled_bitmap
 */
void al_draw_tinted_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh, int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      0, 0, 0,
      dw / sw, dh / sh,
      sx, sy, sw, sh, dx, dy, flags);
}


/* Function: al_draw_scaled_bitmap
 */
void al_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh, int flags)
{
   al_draw_tinted_scaled_bitmap(bitmap, solid_white, sx, sy, sw, sh,
      dx, dy, dw, dh, flags);
}


/* Function: al_draw_tinted_rotated_bitmap
 *
 * angle is specified in radians and moves clockwise
 * on the screen.
 */
void al_draw_tinted_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float angle, int flags)
{
   al_draw_tinted_scaled_rotated_bitmap(bitmap, tint, cx, cy, dx, dy,
      1, 1, angle, flags);
}


/* Function: al_draw_rotated_bitmap
 */
void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float angle, int flags)
{
   al_draw_tinted_rotated_bitmap(bitmap, solid_white, cx, cy, dx, dy,
      angle, flags);
}


/* Function: al_draw_tinted_scaled_rotated_bitmap
 */
void al_draw_tinted_scaled_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      cx, cy, angle,
      xscale, yscale,
      0, 0, bitmap->w, bitmap->h, dx, dy, flags);
}


/* Function: al_draw_tinted_scaled_rotated_bitmap_region
 */
void al_draw_tinted_scaled_rotated_bitmap_region(ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      cx, cy, angle,
      xscale, yscale,
      sx, sy, sw, sh, dx, dy, flags);
}


/* Function: al_draw_scaled_rotated_bitmap
 */
void al_draw_scaled_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   al_draw_tinted_scaled_rotated_bitmap(bitmap, solid_white,
      cx, cy, dx, dy, xscale, yscale, angle, flags);
}


/* vim: set ts=8 sts=3 sw=3 et: */
