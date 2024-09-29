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
 */

#define _AL_NO_BLEND_INLINE_FUNC

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_convert.h"
#include "allegro5/internal/aintern_memblit.h"
#include "allegro5/internal/aintern_transform.h"
#include "allegro5/internal/aintern_tri_soft.h"
#include <math.h>

#define MIN _A5O_MIN
#define MAX _A5O_MAX

static void _al_draw_transformed_scaled_bitmap_memory(
   A5O_BITMAP *src, A5O_COLOR tint,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh,
   int flags);
static void _al_draw_bitmap_region_memory_fast(A5O_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags);


/* The CLIPPER macro takes pre-clipped coordinates for both the source
 * and destination bitmaps and clips them as necessary, taking sub-
 * bitmaps into consideration. The wr and hr parameters are the ratio of
 * source width & height to destination width & height _before_ clipping.
 *
 * First the left (top) coordinates are moved inward. Then the right
 * (bottom) coordinates are moved inward. The changes are applied
 * simultaneously to the complementary bitmap with scaling taken into
 * consideration.
 *
 * The coordinates are modified, and the sub-bitmaps are set to the
 * parent bitmaps. If nothing needs to be drawn, the macro exits the
 * function.
 */

#define CLIPPER(src, sx, sy, sw, sh, dest, dx, dy, dw, dh, wr, hr, flags)\
{                                                                        \
  float cl = dest->cl, cr = dest->cr_excl;                               \
  float ct = dest->ct, cb = dest->cb_excl;                               \
  float sx_ = 0, sy_ = 0, sw_ = 0, sh_ = 0;                              \
  bool hflip = false, vflip = false;                                     \
  if (dw < 0) {                                                          \
     hflip = true;                                                       \
     dx += dw;                                                           \
     dw = -dw;                                                           \
     sx_ = sx; sw_ = sw;                                                 \
  }                                                                      \
  if (dh < 0) {                                                          \
     vflip = true;                                                       \
     dy += dh;                                                           \
     dh = -dh;                                                           \
     sy_ = sy; sh_ = sh;                                                 \
  }                                                                      \
                                                                         \
  if (dest->parent) {                                                    \
     dx += dest->xofs;                                                   \
     dy += dest->yofs;                                                   \
                                                                         \
     cl += dest->xofs;                                                   \
     if (cl >= dest->parent->w) {                                        \
        return;                                                          \
     }                                                                   \
     else if (cl < 0) {                                                  \
        cl = 0;                                                          \
     }                                                                   \
                                                                         \
     ct += dest->yofs;                                                   \
     if (ct >= dest->parent->h) {                                        \
        return;                                                          \
     }                                                                   \
     else if (ct < 0) {                                                  \
        ct = 0;                                                          \
     }                                                                   \
                                                                         \
     cr = MIN(dest->parent->w, cr + dest->xofs);                         \
     cb = MIN(dest->parent->h, cb + dest->yofs);                         \
                                                                         \
     dest = dest->parent;                                                \
  }                                                                      \
                                                                         \
  if (dx < cl) {                                                         \
     const int d = cl - dx;                                              \
     dx = cl;                                                            \
     dw -= d;                                                            \
     sx += d * wr;                                                       \
     sw -= d * wr;                                                       \
  }                                                                      \
                                                                         \
  if (dx + dw > cr) {                                                    \
     const int d = dx + dw - cr;                                         \
     dw -= d;                                                            \
     sw -= d * wr;                                                       \
  }                                                                      \
                                                                         \
  if (dy < ct) {                                                         \
     const int d = ct - dy;                                              \
     dy = ct;                                                            \
     dh -= d;                                                            \
     sy += d * hr;                                                       \
     sh -= d * hr;                                                       \
  }                                                                      \
                                                                         \
  if (dy + dh > cb) {                                                    \
     const int d = dy + dh - cb;                                         \
     dh -= d;                                                            \
     sh -= d * hr;                                                       \
  }                                                                      \
                                                                         \
  if (sh <= 0 || sw <= 0) return;                                        \
                                                                         \
  if (hflip) {dx += dw; dw = -dw; sx = sx_ + sw_ - sw + sx_ - sx; dx--;} \
  if (vflip) {dy += dh; dh = -dh; sy = sy_ + sh_ - sh + sy_ - sy; dy--;} \
}


void _al_draw_bitmap_region_memory(A5O_BITMAP *src,
   A5O_COLOR tint,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   int op, src_mode, dst_mode;
   int op_alpha, src_alpha, dst_alpha;
   float xtrans, ytrans;
   
   ASSERT(src->parent == NULL);

   al_get_separate_bitmap_blender(&op,
      &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

   if (_AL_DEST_IS_ZERO && _AL_SRC_NOT_MODIFIED_TINT_WHITE &&
      _al_transform_is_translation(al_get_current_transform(), &xtrans, &ytrans))
   {
      _al_draw_bitmap_region_memory_fast(src, sx, sy, sw, sh,
         dx + xtrans, dy + ytrans, flags);
      return;
   }

   /* We used to have special cases for translation/scaling only, but the
    * general version received much more optimisation and ended up being
    * faster.
    */
   _al_draw_transformed_scaled_bitmap_memory(src, tint, sx, sy,
      sw, sh, dx, dy, sw, sh, flags);
}


static void _al_draw_transformed_bitmap_memory(A5O_BITMAP *src,
   A5O_COLOR tint,
   int sx, int sy, int sw, int sh, int dw, int dh,
   A5O_TRANSFORM* local_trans, int flags)
{
   float xsf[4], ysf[4];
   int tl = 0, tr = 1, bl = 3, br = 2;
   int tmp;
   A5O_VERTEX v[4];

   ASSERT(_al_pixel_format_is_real(al_get_bitmap_format(src)));

   /* Decide what order to take corners in. */
   if (flags & A5O_FLIP_VERTICAL) {
      tl = 3;
      tr = 2;
      bl = 0;
      br = 1;
   }
   else {
      tl = 0;
      tr = 1;
      bl = 3;
      br = 2;
   }
   if (flags & A5O_FLIP_HORIZONTAL) {
      tmp = tl;
      tl = tr;
      tr = tmp;
      tmp = bl;
      bl = br;
      br = tmp;
   }

   xsf[0] = 0;
   ysf[0] = 0;

   xsf[1] = dw;
   ysf[1] = 0;

   xsf[2] = 0;
   ysf[2] = dh;

   al_transform_coordinates(local_trans, &xsf[0], &ysf[0]);
   al_transform_coordinates(local_trans, &xsf[1], &ysf[1]);
   al_transform_coordinates(local_trans, &xsf[2], &ysf[2]);

   v[tl].x = xsf[0];
   v[tl].y = ysf[0];
   v[tl].z = 0;
   v[tl].u = sx;
   v[tl].v = sy;
   v[tl].color = tint;

   v[tr].x = xsf[1];
   v[tr].y = ysf[1];
   v[tr].z = 0;
   v[tr].u = sx + sw;
   v[tr].v = sy;
   v[tr].color = tint;

   v[br].x = xsf[2] + xsf[1] - xsf[0];
   v[br].y = ysf[2] + ysf[1] - ysf[0];
   v[br].z = 0;
   v[br].u = sx + sw;
   v[br].v = sy + sh;
   v[br].color = tint;

   v[bl].x = xsf[2];
   v[bl].y = ysf[2];
   v[bl].z = 0;
   v[bl].u = sx;
   v[bl].v = sy + sh;
   v[bl].color = tint;

   al_lock_bitmap(src, A5O_PIXEL_FORMAT_ANY, A5O_LOCK_READONLY);

   _al_triangle_2d(src, &v[tl], &v[tr], &v[br]);
   _al_triangle_2d(src, &v[tl], &v[br], &v[bl]);

   al_unlock_bitmap(src);
}


static void _al_draw_transformed_scaled_bitmap_memory(
   A5O_BITMAP *src, A5O_COLOR tint,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int flags)
{
   A5O_TRANSFORM local_trans;

   al_identity_transform(&local_trans);
   al_translate_transform(&local_trans, dx, dy);
   al_compose_transform(&local_trans, al_get_current_transform());

   _al_draw_transformed_bitmap_memory(src, tint, sx, sy, sw, sh, dw, dh,
      &local_trans, flags);
}


static void _al_draw_bitmap_region_memory_fast(A5O_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   A5O_LOCKED_REGION *src_region;
   A5O_LOCKED_REGION *dst_region;
   A5O_BITMAP *dest = al_get_target_bitmap();
   int dw = sw, dh = sh;

   ASSERT(_al_pixel_format_is_real(al_get_bitmap_format(bitmap)));
   ASSERT(_al_pixel_format_is_real(al_get_bitmap_format(dest)));
   ASSERT(bitmap->parent == NULL);

   /* Currently the only flags are for flipping, which is handled as negative
    * scaling.
    */
   ASSERT(flags == 0);
   (void)flags;

   CLIPPER(bitmap, sx, sy, sw, sh, dest, dx, dy, dw, dh, 1, 1, flags)

   if (!(src_region = al_lock_bitmap_region(bitmap, sx, sy, sw, sh,
         A5O_PIXEL_FORMAT_ANY, A5O_LOCK_READONLY))) {
      return;
   }

   if (!(dst_region = al_lock_bitmap_region(dest, dx, dy, sw, sh,
         A5O_PIXEL_FORMAT_ANY, A5O_LOCK_WRITEONLY))) {
      al_unlock_bitmap(bitmap);
      return;
   }

   /* will detect if no conversion is needed */
   _al_convert_bitmap_data(
      src_region->data, src_region->format, src_region->pitch,
      dst_region->data, dst_region->format, dst_region->pitch,
      0, 0, 0, 0, sw, sh);

   al_unlock_bitmap(bitmap);
   al_unlock_bitmap(dest);
}


/* vim: set sts=3 sw=3 et: */
