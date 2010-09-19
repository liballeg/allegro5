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

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_convert.h"
#include "allegro5/internal/aintern_float.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_tri_soft.h"
#include "allegro5/transformations.h"
#include <math.h>

// TODO: Re-write this file with *clean* code.

#define MIN _ALLEGRO_MIN
#define MAX _ALLEGRO_MAX

static void _al_draw_transformed_scaled_bitmap_memory(
   ALLEGRO_BITMAP *src, ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh,
   int flags);

static bool is_scale_trans(const ALLEGRO_TRANSFORM* trans,
   float *sx, float *sy, float *tx, float *ty)
{
   if (   trans->m[1][0] == 0 &&
          trans->m[2][0] == 0 &&
          trans->m[0][1] == 0 &&
          trans->m[2][1] == 0 &&
          trans->m[0][2] == 0 &&
          trans->m[1][2] == 0 &&
          trans->m[2][2] == 1 &&
          trans->m[3][2] == 0 &&
          trans->m[0][3] == 0 &&
          trans->m[1][3] == 0 &&
          trans->m[2][3] == 0 &&
          trans->m[3][3] == 1) {
      *sx = trans->m[0][0];
      *sy = trans->m[1][1];
      *tx = trans->m[3][0];
      *ty = trans->m[3][1];
      return true;
   }
   return false;
}


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
  float sx_, sy_, sw_, sh_;                                              \
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


static void _al_draw_scaled_bitmap_memory_fast(ALLEGRO_BITMAP *src,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;

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
   
   ASSERT(sx >= 0 && sy >= 0);
   ASSERT(src->parent == NULL);
   ASSERT(!(flags & (ALLEGRO_FLIP_HORIZONTAL | ALLEGRO_FLIP_VERTICAL)));

   if ((sw <= 0) || (sh <= 0))
      return;

   /* This must be calculated before clipping dw, dh. */
   sxinc = fabs((float)sw / dw);
   syinc = fabs((float)sh / dh);

   CLIPPER(src, sx, sy, sw, sh, dest, dx, dy, dw, dh, sxinc, syinc, flags)

   if (src->format == dest->format) {
      if (!(src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ANY,
            ALLEGRO_LOCK_READONLY))) {
         return;
      }
      /* XXX we should be able to lock less of the destination and use
       * ALLEGRO_LOCK_WRITEONLY
       */
      if (!(dst_region = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
         al_unlock_bitmap(src);
         return;
      }
      size = al_get_pixel_size(src->format);
   }
   else {
      if (!(src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ARGB_8888,
            ALLEGRO_LOCK_READONLY))) {
         return;
      }
      /* XXX we should be able to lock less of the destination and use
       * ALLEGRO_LOCK_WRITEONLY
       */
      if (!(dst_region = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ARGB_8888, 0))) {
         al_unlock_bitmap(src);
         return;
      }
      size = 4;
   }

   dxinc = dw < 0 ? -1 : 1;
   dyinc = dh < 0 ? -1 : 1;
   xend = abs(dw);
   yend = abs(dh);

   _sy = sy;

   #define INNER(t, s, r, w) \
      for (x = 0; x < xend; x++) { \
         t pix = r((char *)src_region->data+(int)_sy*src_region->pitch+(int)_sx*s); \
         w((char *)dst_region->data+(int)_dy*dst_region->pitch+(int)_dx*s, pix); \
         _sx += sxinc; \
         _dx += dxinc; \
      } \

   _dy = dy;
   for (y = 0; y < yend; y++) {
      _sx = sx;
      _dx = dx;
      switch (size) {
         case 2:
            INNER(uint16_t, size, bmp_read16, bmp_write16)
            break;
         case 3:
            INNER(uint32_t, size, READ3BYTES, WRITE3BYTES)
            break;
         default:
            INNER(uint32_t, size, bmp_read32, bmp_write32)
            break;
      }
      _sy += syinc;
      _dy += dyinc;
   }

   al_unlock_bitmap(src);
   al_unlock_bitmap(dest);
}


static void _al_draw_scaled_bitmap_memory(ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   int op, src_mode, dst_mode;
   int op_alpha, src_alpha, dst_alpha;

   float sxinc;
   float syinc;
   float _sx;
   float _sy;
   float dxinc;
   float dyinc;
   float _dy;
   int x, y;
   int xend;
   int yend;
   
   ASSERT(src->parent == NULL);

   al_get_separate_blender(&op, &src_mode, &dst_mode,
      &op_alpha, &src_alpha, &dst_alpha);

   if (_AL_DEST_IS_ZERO && _AL_SRC_NOT_MODIFIED_TINT_WHITE) {
      _al_draw_scaled_bitmap_memory_fast(src,
            sx, sy, sw, sh, dx, dy, dw, dh, flags);
      return;
   }

   if ((sw <= 0) || (sh <= 0))
      return;

   /* This must be calculated before clipping dw, dh. */
   sxinc = fabs((float)sw / dw);
   syinc = fabs((float)sh / dh);

   CLIPPER(src, sx, sy, sw, sh, dest, dx, dy, dw, dh, sxinc, syinc, flags)

   if (!(src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY))) {
      return;
   }
   /* XXX we should be able to lock less of the destination */
   if (!(dst_region = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
      al_unlock_bitmap(src);
      return;
   }

   dxinc = dw < 0 ? -1 : 1;
   dyinc = dh < 0 ? -1 : 1;
   xend = abs(dw);
   yend = abs(dh);

   _sy = sy;

   _dy = dy;

   {
      const int src_size = al_get_pixel_size(src->format);
      const int dst_size = al_get_pixel_size(dest->format);
      const int dst_inc = dst_size * (int)dxinc;

      ALLEGRO_COLOR src_color = {0, 0, 0, 0};   /* avoid bogus warnings */
      ALLEGRO_COLOR dst_color = {0, 0, 0, 0};

      for (y = 0; y < yend; y++) {
         const char *src_row =
            (((char *) src->locked_region.data)
             + _al_fast_float_to_int(_sy) * src->locked_region.pitch);

         char *dst_data =
            (((char *) dest->locked_region.data)
             + _al_fast_float_to_int(_dy) * dest->locked_region.pitch
             + dst_size * _al_fast_float_to_int(dx));

         ALLEGRO_COLOR result;

         _sx = sx;

         if (_AL_DEST_IS_ZERO) {
            for (x = 0; x < xend; x++) {
               const char *src_data = src_row
                  + src_size * _al_fast_float_to_int(_sx);
               _AL_INLINE_GET_PIXEL(src->format, src_data, src_color, false);
               src_color.r *= tint.r;
               src_color.g *= tint.g;
               src_color.b *= tint.b;
               src_color.a *= tint.a;
               _al_blend_inline_dest_zero_add(&src_color, src_mode, src_alpha,
                  &result);
               _AL_INLINE_PUT_PIXEL(dest->format, dst_data, result, false);

               _sx += sxinc;
               dst_data += dst_inc;
            }
         }
         else {
            for (x = 0; x < xend; x++) {
               const char * src_data = src_row
                  + src_size * _al_fast_float_to_int(_sx);
               _AL_INLINE_GET_PIXEL(src->format, src_data, src_color, false);
               _AL_INLINE_GET_PIXEL(dest->format, dst_data, dst_color, false);
               src_color.r *= tint.r;
               src_color.g *= tint.g;
               src_color.b *= tint.b;
               src_color.a *= tint.a;
               _al_blend_inline(&src_color, &dst_color,
                  op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
               _AL_INLINE_PUT_PIXEL(dest->format, dst_data, result, false);

               _sx += sxinc;
               dst_data += dst_inc;
            }
         }

         _sy += syinc;
         _dy += dyinc;
      }
   }

   al_unlock_bitmap(src);
   al_unlock_bitmap(dest);
}


void _al_draw_bitmap_region_memory(ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   int x, y;
   int op, src_mode, dst_mode;
   int op_alpha, src_alpha, dst_alpha;
   int xinc, yinc;
   int yd;
   int sxd;
   int dw = sw, dh = sh;
   float xtrans, ytrans;
   
   ASSERT(src->parent == NULL);

   if(!_al_transform_is_translation(al_get_current_transform(),
      &xtrans, &ytrans))
   {
      float xscale, yscale;
      if (is_scale_trans(al_get_current_transform(), &xscale, &yscale,
         &xtrans, &ytrans)) {
         _al_draw_scaled_bitmap_memory(src, tint, sx, sy, sw, sh,
            dx + xtrans, dy + ytrans, sw * xscale, sh * yscale, flags);
      }
      else {
         _al_draw_transformed_scaled_bitmap_memory(src, tint, sx, sy,
            sw, sh, dx, dy, sw, sh, flags);
      }
      return;
   }
   
   dx += xtrans;
   dy += ytrans;

   al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

   if (_AL_DEST_IS_ZERO && _AL_SRC_NOT_MODIFIED_TINT_WHITE) {
      _al_draw_bitmap_region_memory_fast(src, sx, sy, sw, sh, dx, dy, flags);
      return;
   }

   ASSERT(_al_pixel_format_is_real(src->format));
   ASSERT(_al_pixel_format_is_real(dest->format));

   CLIPPER(src, sx, sy, sw, sh, dest, dx, dy, dw, dh, 1, 1, flags)

#ifdef ALLEGRO_GP2XWIZ
   if (src_mode == ALLEGRO_ALPHA &&
	 dst_mode == ALLEGRO_INVERSE_ALPHA &&
	 src_alpha == ALLEGRO_ALPHA &&
	 dst_alpha == ALLEGRO_INVERSE_ALPHA) {
      switch (src->format) {
	 case ALLEGRO_PIXEL_FORMAT_RGBA_4444: {
	    switch (dest->format) {
	       case ALLEGRO_PIXEL_FORMAT_RGB_565: {
		  _al_draw_bitmap_region_optimized_rgba_4444_to_rgb_565(tint, src, sx, sy, sw, sh, dest, dx, dy, flags);
		  return;
               }
	       case ALLEGRO_PIXEL_FORMAT_RGBA_4444: {
		  _al_draw_bitmap_region_optimized_rgba_4444_to_rgba_4444(tint, src, sx, sy, sw, sh, dest, dx, dy, flags);
		  return;
	       }
	    }
	 }
         case ALLEGRO_PIXEL_FORMAT_RGB_565: {
            switch (dest->format) {
               case ALLEGRO_PIXEL_FORMAT_RGB_565: {
		  _al_draw_bitmap_region_optimized_rgb_565_to_rgb_565(tint, src, sx, sy, sw, sh, dest, dx, dy, flags);
                  return;
               }
            }
         }
      }
   }
#endif
   /* Blitting to a locked bitmap is a user mistake. */
   ASSERT(!al_is_bitmap_locked(dest));

   /* Lock the bitmaps */
   if (!(src_region = al_lock_bitmap_region(src, sx, sy, sw, sh, ALLEGRO_PIXEL_FORMAT_ANY,
      ALLEGRO_LOCK_READONLY)))
   {
      return;
   }

   if (!(dst_region = al_lock_bitmap_region(dest, dx, dy, sw, sh, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
      al_unlock_bitmap(src);
      return;
   }

   
   yd = 0;
   yinc = 1;
   
   xinc = 1;
   sxd = 0;

   ASSERT(!src->parent);
   ASSERT(!dest->parent);

   {
      const int src_data_inc = xinc * al_get_pixel_size(src_region->format);
      ALLEGRO_COLOR src_color = {0, 0, 0, 0};   /* avoid bogus warnings */
      ALLEGRO_COLOR dst_color = {0, 0, 0, 0};
      ALLEGRO_COLOR result;

      for (y = 0; y < sh; y++, yd += yinc) {
         char *dest_data =
            (((char *) dst_region->data)
             + yd * dst_region->pitch);

         char *src_data =
            (((char *) src_region->data)
             + y * src_region->pitch);

         if (src_data_inc < 0)
            src_data += (sw - 1) * -src_data_inc;

         /* Special case this for two reasons:
          * - we don't need to read and blend with the destination pixel;
          * - the destination may be uninitialised and may contain NaNs, Inf
          *   which would not be clobbered when multiplied with zero.
          */
         if (dst_mode == ALLEGRO_ZERO && dst_alpha == ALLEGRO_ZERO &&
            op != ALLEGRO_DEST_MINUS_SRC && op_alpha != ALLEGRO_DEST_MINUS_SRC) {
            for (x = 0; x < sw; x++) {
               _AL_INLINE_GET_PIXEL(src_region->format, src_data, src_color, false);
               src_color.r *= tint.r;
               src_color.g *= tint.g;
               src_color.b *= tint.b;
               src_color.a *= tint.a;
               _al_blend_inline_dest_zero_add(&src_color, src_mode, src_alpha,
                  &result);
               _AL_INLINE_PUT_PIXEL(dst_region->format, dest_data, result, true);

               src_data += src_data_inc;
            }
         }
         else {
            for (x = 0; x < sw; x++) {
               _AL_INLINE_GET_PIXEL(src_region->format, src_data, src_color, false);
               _AL_INLINE_GET_PIXEL(dst_region->format, dest_data, dst_color, false);
               src_color.r *= tint.r;
               src_color.g *= tint.g;
               src_color.b *= tint.b;
               src_color.a *= tint.a;
               _al_blend_inline(&src_color, &dst_color,
                  op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);
               _AL_INLINE_PUT_PIXEL(dst_region->format, dest_data, result, true);

               src_data += src_data_inc;
            }
         }
      }
   }

   al_unlock_bitmap(src);
   al_unlock_bitmap(dest);
}


static void _al_draw_transformed_bitmap_memory(ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh, int dw, int dh,
   ALLEGRO_TRANSFORM* local_trans, int flags)
{
   float xsf[4], ysf[4];
   int tl = 0, tr = 1, bl = 3, br = 2;
   int tmp;
   ALLEGRO_VERTEX v[4];

   ASSERT(_al_pixel_format_is_real(src->format));
   ASSERT(_al_pixel_format_is_real(dst->format));

   /* Decide what order to take corners in. */
   if (flags & ALLEGRO_FLIP_VERTICAL) {
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
   if (flags & ALLEGRO_FLIP_HORIZONTAL) {
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

   al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

   _al_triangle_2d(src, &v[tl], &v[tr], &v[br]);
   _al_triangle_2d(src, &v[tl], &v[br], &v[bl]);

   al_unlock_bitmap(src);
}


static void _al_draw_transformed_scaled_bitmap_memory(
   ALLEGRO_BITMAP *src, ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int flags)
{
   ALLEGRO_TRANSFORM local_trans;

   al_identity_transform(&local_trans);
   al_translate_transform(&local_trans, dx, dy);
   al_compose_transform(&local_trans, al_get_current_transform());

   _al_draw_transformed_bitmap_memory(src, tint, sx, sy, sw, sh, dw, dh,
      &local_trans, flags);
}


void _al_draw_bitmap_region_memory_fast(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   int x;
   int y;
   int dw = sw, dh = sh;

   ASSERT(_al_pixel_format_is_real(bitmap->format));
   ASSERT(_al_pixel_format_is_real(dest->format));
   ASSERT(bitmap->parent == NULL);

   CLIPPER(bitmap, sx, sy, sw, sh, dest, dx, dy, dw, dh, 1, 1, flags)

   /* Fast paths for no flipping */
   if (!(flags & ALLEGRO_FLIP_HORIZONTAL) &&
         !(flags & ALLEGRO_FLIP_VERTICAL)) {
      if (!(src_region = al_lock_bitmap_region(bitmap, sx, sy, sw, sh,
            bitmap->format, ALLEGRO_LOCK_READONLY))) {
         return;
      }

      if (!(dst_region = al_lock_bitmap_region(dest, dx, dy, sw, sh,
         dest->format, ALLEGRO_LOCK_WRITEONLY))) {
            al_unlock_bitmap(bitmap);
         return;
      }

      /* will detect if no conversion is needed */
      _al_convert_bitmap_data(
         src_region->data, bitmap->format, src_region->pitch,
         dst_region->data, dest->format, dst_region->pitch,
         0, 0, 0, 0, sw, sh);

      al_unlock_bitmap(bitmap);
      al_unlock_bitmap(dest);

      return;
   }

   if (!(src_region = al_lock_bitmap_region(bitmap, sx, sy, sw, sh,
         ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA, ALLEGRO_LOCK_READONLY))) {
      return;
   }

   if (!(dst_region = al_lock_bitmap_region(dest, dx, dy, sw, sh,
      ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA, ALLEGRO_LOCK_WRITEONLY))) {
         al_unlock_bitmap(bitmap);
      return;
   }

   do {
      int cdx_start, cdy;   /* current dest */
      int dxi, dyi;         /* dest increments */
      uint32_t pixel;

      (void)sx;
      (void)sy;
      (void)dx;
      (void)dy;

      /* Adjust for flipping */

      if (flags & ALLEGRO_FLIP_HORIZONTAL) {
         cdx_start = sw - 1;
         dxi = -1;
      }
      else {
         cdx_start = 0;
         dxi = 1;
      }

      if (flags & ALLEGRO_FLIP_VERTICAL) {
         cdy = sh - 1;
         dyi = -1;
      }
      else {
         cdy = 0;
         dyi = 1;
      }

      for (y = 0; y < sh; y++) {
         int cdx = cdx_start;
         for (x = 0; x < sw; x++) {
            pixel = *(uint32_t *)(((char *)src_region->data) + y * src_region->pitch + x * 4);
            *(uint32_t *)(((char *)dst_region->data) + cdy * dst_region->pitch + cdx * 4) = pixel;
            cdx += dxi;
         }
         cdy += dyi;
      }
   } while (0);

   al_unlock_bitmap(bitmap);
   al_unlock_bitmap(dest);
}


/* vim: set sts=3 sw=3 et: */
