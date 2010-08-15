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
#include "allegro5/internal/aintern_float.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_convert.h"
#include "allegro5/transformations.h"
#include <math.h>

// TODO: Re-write this file with *clean* code.

#define MIN _ALLEGRO_MIN
#define MAX _ALLEGRO_MAX

static void _al_draw_transformed_scaled_bitmap_memory(
   ALLEGRO_BITMAP *src, ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh,
   int flags);

static void _al_parallelogram_map_fast(ALLEGRO_BITMAP *src,
   al_fixed xs[4],
   al_fixed ys[4], int sx, int sy, int sw, int sh);

static bool is_identity(const ALLEGRO_TRANSFORM* trans)
{
   return trans->m[0][0] == 1 &&
          trans->m[1][0] == 0 &&
          trans->m[2][0] == 0 &&
          trans->m[3][0] == 0 &&
          trans->m[0][1] == 0 &&
          trans->m[1][1] == 1 &&
          trans->m[2][1] == 0 &&
          trans->m[3][1] == 0 &&
          trans->m[0][2] == 0 &&
          trans->m[1][2] == 0 &&
          trans->m[2][2] == 1 &&
          trans->m[3][2] == 0 &&
          trans->m[0][3] == 0 &&
          trans->m[1][3] == 0 &&
          trans->m[2][3] == 0 &&
          trans->m[3][3] == 1;
}


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


static INLINE float get_factor(enum ALLEGRO_BLEND_MODE operation, float alpha)
{
   switch (operation) {
       case ALLEGRO_ZERO: return 0;
       case ALLEGRO_ONE: return 1;
       case ALLEGRO_ALPHA: return alpha;
       case ALLEGRO_INVERSE_ALPHA: return 1 - alpha;
   }
   ASSERT(false);
   return 0; /* silence warning in release build */
}


static INLINE void _al_blend_inline(
   const ALLEGRO_COLOR *scol, const ALLEGRO_COLOR *dcol,
   int op, int src_, int dst_, int aop, int asrc_, int adst_,
   ALLEGRO_COLOR *result)
{
   float src, dst, asrc, adst;

   result->r = scol->r;
   result->g = scol->g;
   result->b = scol->b;
   result->a = scol->a;

   src = get_factor(src_, result->a);
   dst = get_factor(dst_, result->a);
   asrc = get_factor(asrc_, result->a);
   adst = get_factor(adst_, result->a);

   #define BLEND(c, src, dst) \
      result->c = OP(result->c * src, dcol->c * dst);
   switch (op) {
      case ALLEGRO_ADD:
         #define OP(x, y) MIN(1, x + y)
         BLEND(r, src, dst)
         BLEND(g, src, dst)
         BLEND(b, src, dst)
         #undef OP
         break;
      case ALLEGRO_SRC_MINUS_DEST:
         #define OP(x, y) MAX(0, x - y)
         BLEND(r, src, dst)
         BLEND(g, src, dst)
         BLEND(b, src, dst)
         #undef OP
         break;
      case ALLEGRO_DEST_MINUS_SRC:
         #define OP(x, y) MAX(0, y - x)
         BLEND(r, src, dst)
         BLEND(g, src, dst)
         BLEND(b, src, dst)
         #undef OP
         break;
   }

   switch (aop) {
      case ALLEGRO_ADD:
         #define OP(x, y) MIN(1, x + y)
         BLEND(a, asrc, adst)
         #undef OP
         break;
      case ALLEGRO_SRC_MINUS_DEST:
         #define OP(x, y) MAX(0, x - y)
         BLEND(a, asrc, adst)
         #undef OP
         break;
      case ALLEGRO_DEST_MINUS_SRC:
         #define OP(x, y) MAX(0, y - x)
         BLEND(a, asrc, adst)
         #undef OP
         break;
   }
}


static void _al_blend_inline_dest_zero_add(
   const ALLEGRO_COLOR *scol,
   int src_, int asrc_,
   ALLEGRO_COLOR *result)
{
   float src, asrc;

   result->r = scol->r;
   result->g = scol->g;
   result->b = scol->b;
   result->a = scol->a;

   src = get_factor(src_, result->a);
   asrc = get_factor(asrc_, result->a);

   result->r *= src;
   result->g *= src;
   result->b *= src;
   result->a *= asrc;
}


#define DEST_IS_ZERO \
   dst_mode == ALLEGRO_ZERO &&  dst_alpha == ALLEGRO_ZERO && \
   op != ALLEGRO_DEST_MINUS_SRC && op_alpha != ALLEGRO_DEST_MINUS_SRC

#define SRC_NOT_MODIFIED \
   src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && \
   tint.r == 1.0f && tint.g == 1.0f && tint.b == 1.0f && tint.a == 1.0f

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
  int cl = dest->cl, cr = dest->cr_excl;                                 \
  int ct = dest->ct, cb = dest->cb_excl;                                 \
  bool hflip = false, vflip = false;                                     \
  if (dw < 0) {                                                          \
     hflip = true;                                                       \
     dx += dw;                                                           \
     dw = -dw;                                                           \
     sx -= sw;                                                           \
  }                                                                      \
  if (dh < 0) {                                                          \
     vflip = true;                                                       \
     dy += dh;                                                           \
     dh = -dh;                                                           \
     sy -= sh;                                                           \
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
  if (hflip) {dx += dw; dw = -dw; sx += sw; dx--;}                       \
  if (vflip) {dy += dh; dh = -dh; sy += sh; dy--;}                       \
}


static void _al_draw_scaled_bitmap_memory_fast(ALLEGRO_BITMAP *src,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
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


void _al_draw_scaled_bitmap_memory(ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
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

   if (DEST_IS_ZERO && SRC_NOT_MODIFIED) {
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
             + dst_size * dx);

         ALLEGRO_COLOR result;

         _sx = sx;

         if (DEST_IS_ZERO) {
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
   
   ASSERT(src->parent == NULL);

   if(!is_identity(al_get_current_transform()))
   {
      float xscale, yscale, xtrans, ytrans;
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

   al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);

   if (DEST_IS_ZERO && SRC_NOT_MODIFIED)
   {
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
		  _al_draw_bitmap_region_optimized_rgba_4444_to_rgb_565(src, sx, sy, sw, sh, dest, dx, dy, flags);
		  return;
               }
	       case ALLEGRO_PIXEL_FORMAT_RGBA_4444: {
		  _al_draw_bitmap_region_optimized_rgba_4444_to_rgba_4444(src, sx, sy, sw, sh, dest, dx, dy, flags);
		  return;
	       }
	    }
	 }
         case ALLEGRO_PIXEL_FORMAT_RGB_565: {
            switch (dest->format) {
               case ALLEGRO_PIXEL_FORMAT_RGB_565: {
		  _al_draw_bitmap_region_optimized_rgb_565_to_rgb_565(src, sx, sy, sw, sh, dest, dx, dy, flags);
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

/*
* Get scanline starts, ends and deltas, and clipping coordinates.
*/
#define top_bmp_y    corner_bmp_y[0]
#define right_bmp_y  corner_bmp_y[1]
#define bottom_bmp_y corner_bmp_y[2]
#define left_bmp_y   corner_bmp_y[3]
#define top_bmp_x    corner_bmp_x[0]
#define right_bmp_x  corner_bmp_x[1]
#define bottom_bmp_x corner_bmp_x[2]
#define left_bmp_x   corner_bmp_x[3]
#define top_spr_y    corner_spr_y[0]
#define right_spr_y  corner_spr_y[1]
#define bottom_spr_y corner_spr_y[2]
#define left_spr_y   corner_spr_y[3]
#define top_spr_x    corner_spr_x[0]
#define right_spr_x  corner_spr_x[1]
#define bottom_spr_x corner_spr_x[2]
#define left_spr_x   corner_spr_x[3]

/* Copied from rotate.c */

/* _parallelogram_map:
 *  Worker routine for drawing rotated and/or scaled and/or flipped sprites:
 *  It actually maps the sprite to any parallelogram-shaped area of the
 *  bitmap. The top left corner is mapped to (xs[0], ys[0]), the top right to
 *  (xs[1], ys[1]), the bottom right to x (xs[2], ys[2]), and the bottom left
 *  to (xs[3], ys[3]). The corners are assumed to form a perfect
 *  parallelogram, i.e. xs[0]+xs[2] = xs[1]+xs[3]. The corners are given in
 *  fixed point format, so xs[] and ys[] are coordinates of the outer corners
 *  of corner pixels in clockwise order beginning with top left.
 *  All coordinates begin with 0 in top left corner of pixel (0, 0). So a
 *  rotation by 0 degrees of a sprite to the top left of a bitmap can be
 *  specified with coordinates (0, 0) for the top left pixel in source
 *  bitmap. With the default scanline drawer, a pixel in the destination
 *  bitmap is drawn if and only if its center is covered by any pixel in the
 *  sprite. The color of this covering sprite pixel is used to draw.
 *  If sub_pixel_accuracy=false, then the scanline drawer will be called with
 *  *_bmp_x being a fixed point representation of the integers representing
 *  the x coordinate of the first and last point in bmp whose centre is
 *  covered by the sprite. If sub_pixel_accuracy=true, then the scanline
 *  drawer will be called with the exact fixed point position of the first
 *  and last point in which the horizontal line passing through the centre is
 *  at least partly covered by the sprite. This is useful for doing
 *  anti-aliased blending.
 */
#define DO_PARALLELOGRAM_MAP(sub_pixel_accuracy, flags)                      \
do {                                                                         \
   /* Index in xs[] and ys[] to topmost point. */                            \
   int top_index;                                                            \
   /* Rightmost point has index (top_index+right_index) int xs[] and ys[]. */\
   int right_index;                                                          \
   /* Loop variables. */                                                     \
   int index, i;                                                             \
   /* Coordinates in bmp ordered as top-right-bottom-left. */                \
   al_fixed corner_bmp_x[4], corner_bmp_y[4];                                \
   /* Coordinates in spr ordered as top-right-bottom-left. */                \
   al_fixed corner_spr_x[4], corner_spr_y[4];                                \
   /* y coordinate of bottom point, left point and right point. */           \
   int clip_bottom_i, l_bmp_y_bottom_i, r_bmp_y_bottom_i;                    \
   /* Left and right clipping. */                                            \
   al_fixed clip_left, clip_right;                                           \
   /* Temporary variable. */                                                 \
   al_fixed extra_scanline_fraction;                                         \
   /* Top scanline of destination */                                         \
   int clip_top_i;                                                           \
                                                                             \
   ALLEGRO_LOCKED_REGION *src_region;                                        \
   ALLEGRO_LOCKED_REGION *dst_region;                                        \
                                                                             \
   /*                                                                        \
    * Variables used in the loop                                             \
    */                                                                       \
   /* Coordinates of sprite and bmp points in beginning of scanline. */      \
   al_fixed l_spr_x, l_spr_y, l_bmp_x, l_bmp_dx;                             \
   /* Increment of left sprite point as we move a scanline down. */          \
   al_fixed l_spr_dx, l_spr_dy;                                              \
   /* Coordinates of sprite and bmp points in end of scanline. */            \
   al_fixed r_bmp_x, r_bmp_dx;                                               \
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                            \
   al_fixed r_spr_x, r_spr_y;                                                \
   /* Increment of right sprite point as we move a scanline down. */         \
   al_fixed r_spr_dx, r_spr_dy;                                              \
   /*#endif*/                                                                \
   /* Increment of sprite point as we move right inside a scanline. */       \
   al_fixed spr_dx, spr_dy;                                                  \
   /* Positions of beginning of scanline after rounding to integer coordinate\
      in bmp. */                                                             \
   al_fixed l_spr_x_rounded, l_spr_y_rounded, l_bmp_x_rounded;               \
   al_fixed r_bmp_x_rounded;                                                 \
   /* Current scanline. */                                                   \
   int bmp_y_i;                                                              \
   /* Right edge of scanline. */                                             \
   int right_edge_test;                                                      \
   ALLEGRO_COLOR src_color = { 0, 0, 0, 0 }, result;                         \
                                                                             \
   /* Get index of topmost point. */                                         \
   top_index = 0;                                                            \
   if (ys[1] < ys[0])                                                        \
      top_index = 1;                                                         \
   if (ys[2] < ys[top_index])                                                \
      top_index = 2;                                                         \
   if (ys[3] < ys[top_index])                                                \
      top_index = 3;                                                         \
                                                                             \
   /* Get direction of points: clockwise or anti-clockwise. */               \
   right_index = (double)(xs[(top_index+1) & 3] - xs[top_index]) *           \
      (double)(ys[(top_index-1) & 3] - ys[top_index]) >                      \
      (double)(xs[(top_index-1) & 3] - xs[top_index]) *                      \
      (double)(ys[(top_index+1) & 3] - ys[top_index]) ? 1 : -1;              \
   /*FIXME: why does al_fixmul overflow below?*/                             \
   /*if (al_fixmul(xs[(top_index+1) & 3] - xs[top_index],                    \
      ys[(top_index-1) & 3] - ys[top_index]) >                               \
         al_fixmul(xs[(top_index-1) & 3] - xs[top_index],                    \
            ys[(top_index+1) & 3] - ys[top_index]))                          \
      right_index = 1;                                                       \
   else                                                                      \
      right_index = -1;*/                                                    \
                                                                             \
   /*                                                                        \
    * Get coordinates of the corners.                                        \
    */                                                                       \
                                                                             \
   /* corner_*[0] is top, [1] is right, [2] is bottom, [3] is left. */       \
   index = top_index;                                                        \
   for (i = 0; i < 4; i++) {                                                 \
      corner_bmp_x[i] = xs[index];                                           \
      corner_bmp_y[i] = ys[index];                                           \
      switch(index)                                                          \
      {                                                                      \
         case 0:                                                             \
            corner_spr_x[i] = sx << 16;                                      \
            corner_spr_y[i] = sy << 16;                                      \
            break;                                                           \
         case 1:                                                             \
            /* Need `- 1' since otherwise it would be outside sprite. */     \
            corner_spr_x[i] = ((sx + sw) << 16) - 1;                         \
            corner_spr_y[i] = sy << 16;                                      \
            break;                                                           \
         case 2:                                                             \
            corner_spr_x[i] = ((sx + sw) << 16) - 1;                         \
            corner_spr_y[i] = ((sy + sh) << 16) - 1;                         \
            break;                                                           \
         case 3:                                                             \
            corner_spr_x[i] = sx << 16;                                      \
            corner_spr_y[i] = ((sy + sh) << 16) - 1;                         \
            break;                                                           \
      }                                                                      \
      index = (index + right_index) & 3;                                     \
   }                                                                         \
                                                                             \
   /* Calculate left and right clipping. */                                  \
   clip_left = dst->cl << 16;                                                \
   clip_right = dst->cr_excl << 16;                                          \
                                                                             \
   /* Quit if we're totally outside. */                                      \
   if ((left_bmp_x > clip_right) &&                                          \
       (top_bmp_x > clip_right) &&                                           \
       (bottom_bmp_x > clip_right))                                          \
      return;                                                                \
   if ((right_bmp_x < clip_left) &&                                          \
       (top_bmp_x < clip_left) &&                                            \
       (bottom_bmp_x < clip_left))                                           \
      return;                                                                \
                                                                             \
   /* Bottom clipping. */                                                    \
   if (sub_pixel_accuracy)                                                   \
      clip_bottom_i = (bottom_bmp_y + 0xffff) >> 16;                         \
   else                                                                      \
      clip_bottom_i = (bottom_bmp_y + 0x8000) >> 16;                         \
                                                                             \
   /* Bottom clipping */                                                     \
   if (clip_bottom_i > dst->cb_excl)                                         \
      clip_bottom_i = dst->cb_excl;                                          \
                                                                             \
   /* Calculate y coordinate of first scanline. */                           \
   if (sub_pixel_accuracy)                                                   \
      bmp_y_i = top_bmp_y >> 16;                                             \
   else                                                                      \
      bmp_y_i = (top_bmp_y + 0x8000) >> 16;                                  \
                                                                             \
   /* Top clipping */                                                        \
   if (bmp_y_i < dst->ct)                                                    \
      bmp_y_i = dst->ct;                                                     \
                                                                             \
   if (bmp_y_i < 0)                                                          \
      bmp_y_i = 0;                                                           \
                                                                             \
   /* Sprite is above or below bottom clipping area. */                      \
   if (bmp_y_i >= clip_bottom_i)                                             \
      return;                                                                \
                                                                             \
   /* Vertical gap between top corner and centre of topmost scanline. */     \
   extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - top_bmp_y;           \
   /* Calculate x coordinate of beginning of scanline in bmp. */             \
   l_bmp_dx = al_fixdiv(left_bmp_x - top_bmp_x,                              \
                   left_bmp_y - top_bmp_y);                                  \
   l_bmp_x = top_bmp_x + al_fixmul(extra_scanline_fraction, l_bmp_dx);       \
   /* Calculate x coordinate of beginning of scanline in spr. */             \
   /* note: all these are rounded down which is probably a Good Thing (tm) */\
   l_spr_dx = al_fixdiv(left_spr_x - top_spr_x,                              \
                   left_bmp_y - top_bmp_y);                                  \
   l_spr_x = top_spr_x + al_fixmul(extra_scanline_fraction, l_spr_dx);       \
   /* Calculate y coordinate of beginning of scanline in spr. */             \
   l_spr_dy = al_fixdiv(left_spr_y - top_spr_y,                              \
                   left_bmp_y - top_bmp_y);                                  \
   l_spr_y = top_spr_y + al_fixmul(extra_scanline_fraction, l_spr_dy);       \
                                                                             \
   /* Calculate left loop bound. */                                          \
   l_bmp_y_bottom_i = (left_bmp_y + 0x8000) >> 16;                           \
   if (l_bmp_y_bottom_i > clip_bottom_i)                                     \
      l_bmp_y_bottom_i = clip_bottom_i;                                      \
                                                                             \
   /* Calculate x coordinate of end of scanline in bmp. */                   \
   r_bmp_dx = al_fixdiv(right_bmp_x - top_bmp_x,                             \
                   right_bmp_y - top_bmp_y);                                 \
   r_bmp_x = top_bmp_x + al_fixmul(extra_scanline_fraction, r_bmp_dx);       \
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                            \
   /* Calculate x coordinate of end of scanline in spr. */                   \
   r_spr_dx = al_fixdiv(right_spr_x - top_spr_x,                             \
                   right_bmp_y - top_bmp_y);                                 \
   r_spr_x = top_spr_x + al_fixmul(extra_scanline_fraction, r_spr_dx);       \
   /* Calculate y coordinate of end of scanline in spr. */                   \
   r_spr_dy = al_fixdiv(right_spr_y - top_spr_y,                             \
                   right_bmp_y - top_bmp_y);                                 \
   r_spr_y = top_spr_y + al_fixmul(extra_scanline_fraction, r_spr_dy);       \
   /*#endif*/                                                                \
                                                                             \
   /* Calculate right loop bound. */                                         \
   r_bmp_y_bottom_i = (right_bmp_y + 0x8000) >> 16;                          \
                                                                             \
   /* Get dx and dy, the offsets to add to the source coordinates as we move \
      one pixel rightwards along a scanline. This formula can be derived by  \
      considering the 2x2 matrix that transforms the sprite to the           \
      parallelogram.                                                         \
      We'd better use double to get this as exact as possible, since any     \
      errors will be accumulated along the scanline.                         \
   */                                                                        \
   spr_dx = (al_fixed)((ys[3] - ys[0]) * 65536.0 * (65536.0 * sw) /          \
                    ((xs[1] - xs[0]) * (double)(ys[3] - ys[0]) -             \
                     (xs[3] - xs[0]) * (double)(ys[1] - ys[0])));            \
   spr_dy = (al_fixed)((ys[1] - ys[0]) * 65536.0 * (65536.0 * sh) /          \
                    ((xs[3] - xs[0]) * (double)(ys[1] - ys[0]) -             \
                     (xs[1] - xs[0]) * (double)(ys[3] - ys[0])));            \
                                                                             \
   /* Lock the bitmaps */                                                    \
   src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);\
                                                                             \
   clip_top_i = bmp_y_i;                                                     \
                                                                             \
   if (!(dst_region = al_lock_bitmap_region(dst,                             \
         clip_left>>16,                                                      \
         clip_top_i,                                                         \
         (clip_right>>16)-(clip_left>>16),                                   \
         clip_bottom_i-clip_top_i,                                           \
         ALLEGRO_PIXEL_FORMAT_ANY, 0)))                                      \
      return;                                                                \
                                                                             \
                                                                             \
   /*                                                                        \
    * Loop through scanlines.                                                \
    */                                                                       \
                                                                             \
   while (1) {                                                               \
      /* Has beginning of scanline passed a corner? */                       \
      if (bmp_y_i >= l_bmp_y_bottom_i) {                                     \
         /* Are we done? */                                                  \
         if (bmp_y_i >= clip_bottom_i)                                       \
            break;                                                           \
                                                                             \
         /* Vertical gap between left corner and centre of scanline. */      \
         extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - left_bmp_y;    \
         /* Update x coordinate of beginning of scanline in bmp. */          \
         l_bmp_dx = al_fixdiv(bottom_bmp_x - left_bmp_x,                     \
                         bottom_bmp_y - left_bmp_y);                         \
         l_bmp_x = left_bmp_x + al_fixmul(extra_scanline_fraction, l_bmp_dx);\
         /* Update x coordinate of beginning of scanline in spr. */          \
         l_spr_dx = al_fixdiv(bottom_spr_x - left_spr_x,                     \
                         bottom_bmp_y - left_bmp_y);                         \
         l_spr_x = left_spr_x + al_fixmul(extra_scanline_fraction, l_spr_dx);\
         /* Update y coordinate of beginning of scanline in spr. */          \
         l_spr_dy = al_fixdiv(bottom_spr_y - left_spr_y,                     \
                         bottom_bmp_y - left_bmp_y);                         \
         l_spr_y = left_spr_y + al_fixmul(extra_scanline_fraction, l_spr_dy);\
                                                                             \
         /* Update loop bound. */                                            \
         if (sub_pixel_accuracy)                                             \
            l_bmp_y_bottom_i = (bottom_bmp_y + 0xffff) >> 16;                \
         else                                                                \
            l_bmp_y_bottom_i = (bottom_bmp_y + 0x8000) >> 16;                \
         if (l_bmp_y_bottom_i > clip_bottom_i)                               \
            l_bmp_y_bottom_i = clip_bottom_i;                                \
      }                                                                      \
                                                                             \
      /* Has end of scanline passed a corner? */                             \
      if (bmp_y_i >= r_bmp_y_bottom_i) {                                     \
         /* Vertical gap between right corner and centre of scanline. */     \
         extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - right_bmp_y;   \
         /* Update x coordinate of end of scanline in bmp. */                \
         r_bmp_dx = al_fixdiv(bottom_bmp_x - right_bmp_x,                    \
                         bottom_bmp_y - right_bmp_y);                        \
         r_bmp_x = right_bmp_x + al_fixmul(extra_scanline_fraction, r_bmp_dx);\
         /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                      \
         /* Update x coordinate of beginning of scanline in spr. */          \
         r_spr_dx = al_fixdiv(bottom_spr_x - right_spr_x,                    \
                         bottom_bmp_y - right_bmp_y);                        \
         r_spr_x = right_spr_x + al_fixmul(extra_scanline_fraction, r_spr_dx);\
         /* Update y coordinate of beginning of scanline in spr. */          \
         r_spr_dy = al_fixdiv(bottom_spr_y - right_spr_y,                    \
                         bottom_bmp_y - right_bmp_y);                        \
         r_spr_y = right_spr_y + al_fixmul(extra_scanline_fraction, r_spr_dy);\
         /*#endif*/                                                          \
                                                                             \
         /* Update loop bound: We aren't supposed to use this any more, so   \
            just set it to some big enough value. */                         \
         r_bmp_y_bottom_i = clip_bottom_i;                                   \
      }                                                                      \
                                                                             \
      /* Make left bmp coordinate be an integer and clip it. */              \
      if (sub_pixel_accuracy)                                                \
         l_bmp_x_rounded = l_bmp_x;                                          \
      else                                                                   \
         l_bmp_x_rounded = (l_bmp_x + 0x8000) & ~0xffff;                     \
      if (l_bmp_x_rounded < clip_left)                                       \
         l_bmp_x_rounded = clip_left;                                        \
                                                                             \
      /* ... and move starting point in sprite accordingly. */               \
      if (sub_pixel_accuracy) {                                              \
         l_spr_x_rounded = l_spr_x +                                         \
                           al_fixmul((l_bmp_x_rounded - l_bmp_x), spr_dx);   \
         l_spr_y_rounded = l_spr_y +                                         \
                           al_fixmul((l_bmp_x_rounded - l_bmp_x), spr_dy);   \
      }                                                                      \
      else {                                                                 \
         l_spr_x_rounded = l_spr_x +                                         \
                           al_fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dx);\
         l_spr_y_rounded = l_spr_y +                                         \
                           al_fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dy);\
      }                                                                      \
                                                                             \
      /* Make right bmp coordinate be an integer and clip it. */             \
      if (sub_pixel_accuracy)                                                \
         r_bmp_x_rounded = r_bmp_x;                                          \
      else                                                                   \
         r_bmp_x_rounded = (r_bmp_x - 0x8000) & ~0xffff;                     \
      if (r_bmp_x_rounded > clip_right)                                      \
         r_bmp_x_rounded = clip_right;                                       \
                                                                             \
      /* Draw! */                                                            \
      if (l_bmp_x_rounded <= r_bmp_x_rounded) {                              \
         if (!sub_pixel_accuracy) {                                          \
            /* The bodies of these ifs are only reached extremely seldom,    \
               it's an ugly hack to avoid reading outside the sprite when    \
               the rounding errors are accumulated the wrong way. It would   \
               be nicer if we could ensure that this never happens by making \
               all multiplications and divisions be rounded up or down at    \
               the correct places.                                           \
               I did try another approach: recalculate the edges of the      \
               scanline from scratch each scanline rather than incrementally.\
               Drawing a sprite with that routine took about 25% longer time \
               though.                                                       \
            */                                                               \
            if ((unsigned)(l_spr_x_rounded >> 16) >= (unsigned)sw) {         \
               if (((l_spr_x_rounded < 0) && (spr_dx <= 0)) ||               \
                   ((l_spr_x_rounded > 0) && (spr_dx >= 0))) {               \
                  /* This can happen. */                                     \
                  goto skip_draw;                                            \
               }                                                             \
               else {                                                        \
                  /* I don't think this can happen, but I can't prove it. */ \
                  do {                                                       \
                     l_spr_x_rounded += spr_dx;                              \
                     l_bmp_x_rounded += 65536;                               \
                     if (l_bmp_x_rounded > r_bmp_x_rounded)                  \
                        goto skip_draw;                                      \
                  } while ((unsigned)(l_spr_x_rounded >> 16) >=              \
                           (unsigned)sw);                                    \
                                                                             \
               }                                                             \
            }                                                                \
            right_edge_test = l_spr_x_rounded +                              \
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *  \
                              spr_dx;                                        \
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)sw) {         \
               if (((right_edge_test < 0) && (spr_dx <= 0)) ||               \
                   ((right_edge_test > 0) && (spr_dx >= 0))) {               \
                  /* This can happen. */                                     \
                  do {                                                       \
                     r_bmp_x_rounded -= 65536;                               \
                     right_edge_test -= spr_dx;                              \
                     if (l_bmp_x_rounded > r_bmp_x_rounded)                  \
                        goto skip_draw;                                      \
                  } while ((unsigned)(right_edge_test >> 16) >=              \
                           (unsigned)sw);                                    \
               }                                                             \
               else {                                                        \
                  /* I don't think this can happen, but I can't prove it. */ \
                  goto skip_draw;                                            \
               }                                                             \
            }                                                                \
            if ((unsigned)(l_spr_y_rounded >> 16) >= (unsigned)sh) {         \
               if (((l_spr_y_rounded < 0) && (spr_dy <= 0)) ||               \
                   ((l_spr_y_rounded > 0) && (spr_dy >= 0))) {               \
                  /* This can happen. */                                     \
                  goto skip_draw;                                            \
               }                                                             \
               else {                                                        \
                  /* I don't think this can happen, but I can't prove it. */ \
                  do {                                                       \
                     l_spr_y_rounded += spr_dy;                              \
                     l_bmp_x_rounded += 65536;                               \
                     if (l_bmp_x_rounded > r_bmp_x_rounded)                  \
                        goto skip_draw;                                      \
                  } while (((unsigned)l_spr_y_rounded >> 16) >=              \
                           (unsigned)sh);                                    \
               }                                                             \
            }                                                                \
            right_edge_test = l_spr_y_rounded +                              \
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *  \
                              spr_dy;                                        \
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)sh) {         \
               if (((right_edge_test < 0) && (spr_dy <= 0)) ||               \
                   ((right_edge_test > 0) && (spr_dy >= 0))) {               \
                  /* This can happen. */                                     \
                  do {                                                       \
                     r_bmp_x_rounded -= 65536;                               \
                     right_edge_test -= spr_dy;                              \
                     if (l_bmp_x_rounded > r_bmp_x_rounded)                  \
                        goto skip_draw;                                      \
                  } while ((unsigned)(right_edge_test >> 16) >=              \
                           (unsigned)sh);                                    \
               }                                                             \
               else {                                                        \
                  /* I don't think this can happen, but I can't prove it. */ \
                  goto skip_draw;                                            \
               }                                                             \
            }                                                                \
         }                                                                   \
                                                                             \
   {                                                                         \
      const int my_r_bmp_x_i = r_bmp_x_rounded >> 16;                        \
      const int my_l_bmp_x_i = l_bmp_x_rounded >> 16;                        \
      al_fixed my_l_spr_x = l_spr_x_rounded;                                 \
      al_fixed my_l_spr_y = l_spr_y_rounded;                                 \
      const int src_size = al_get_pixel_size(src->format);                   \
      char *dst_data;                                                        \
      ALLEGRO_COLOR dst_color = {0, 0, 0, 0};   /* avoid bogus warning */    \
      int x;                                                                 \
                                                                             \
      dst_data = (char*)dst_region->data                                     \
         + dst_region->pitch * (bmp_y_i - clip_top_i)                        \
         + al_get_pixel_size(dst->format) * (my_l_bmp_x_i - (clip_left>>16));\
                                                                             \
      if (DEST_IS_ZERO) {                                            \
         for (x = my_l_bmp_x_i; x < my_r_bmp_x_i; x++) {                     \
            const char *src_data = (char*)src_region->data                   \
                  + src_region->pitch * (my_l_spr_y>>16)                     \
                  + src_size * (my_l_spr_x>>16);                             \
            _AL_INLINE_GET_PIXEL(src->format, src_data, src_color, false);   \
            src_color.r *= tint.r;                                           \
            src_color.g *= tint.g;                                           \
            src_color.b *= tint.b;                                           \
            src_color.a *= tint.a;                                           \
            _al_blend_inline_dest_zero_add(&src_color, src_mode, src_alpha,  \
               &result);                                                     \
            _AL_INLINE_PUT_PIXEL(dst->format, dst_data, result, true);       \
                                                                             \
            my_l_spr_x += spr_dx;                                            \
            my_l_spr_y += spr_dy;                                            \
         }                                                                   \
      }                                                                      \
      else {                                                                 \
         for (x = my_l_bmp_x_i; x < my_r_bmp_x_i; x++) {                     \
            const char *src_data = (char*)src_region->data                   \
                  + src_region->pitch * (my_l_spr_y>>16)                     \
                  + src_size * (my_l_spr_x>>16);                             \
            _AL_INLINE_GET_PIXEL(src->format, src_data, src_color, false);   \
            _AL_INLINE_GET_PIXEL(dst->format, dst_data, dst_color, false);   \
            src_color.r *= tint.r;                                           \
            src_color.g *= tint.g;                                           \
            src_color.b *= tint.b;                                           \
            src_color.a *= tint.a;                                           \
            _al_blend_inline(&src_color, &dst_color,                         \
               op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha, &result);                      \
            _AL_INLINE_PUT_PIXEL(dst->format, dst_data, result, true);       \
                                                                             \
            my_l_spr_x += spr_dx;                                            \
            my_l_spr_y += spr_dy;                                            \
         }                                                                   \
      }                                                                      \
   }                                                                         \
                                                                             \
      }                                                                      \
      /* I'm not going to apoligize for this label and its gotos: to get     \
         rid of it would just make the code look worse. */                   \
      skip_draw:                                                             \
                                                                             \
      /* Jump to next scanline. */                                           \
      bmp_y_i++;                                                             \
      /* Update beginning of scanline. */                                    \
      l_bmp_x += l_bmp_dx;                                                   \
      l_spr_x += l_spr_dx;                                                   \
      l_spr_y += l_spr_dy;                                                   \
      /* Update end of scanline. */                                          \
      r_bmp_x += r_bmp_dx;                                                   \
      /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                         \
      r_spr_x += r_spr_dx;                                                   \
      r_spr_y += r_spr_dy;                                                   \
      /*#endif*/                                                             \
   }                                                                         \
                                                                             \
   al_unlock_bitmap(src);                                                    \
   al_unlock_bitmap(dst);                                                    \
} while (0)


static void _al_draw_transformed_bitmap_memory(ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   int sx, int sy, int sw, int sh, int dw, int dh,
   ALLEGRO_TRANSFORM* local_trans, int flags)
{
   ALLEGRO_BITMAP *dst = al_get_target_bitmap();
   int op, src_mode, dst_mode;
   int op_alpha, src_alpha, dst_alpha;
   al_fixed xs[4], ys[4];
   float xsf[4], ysf[4];
   int tl = 0, tr = 1, bl = 3, br = 2;
   int tmp;

   al_get_separate_blender(&op, &src_mode, &dst_mode,
      &op_alpha, &src_alpha, &dst_alpha);

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

   xs[tl] = al_ftofix(xsf[0]);
   ys[tl] = al_ftofix(ysf[0]);

   xs[tr] = al_ftofix(xsf[1]);
   ys[tr] = al_ftofix(ysf[1]);

   xs[br] = al_ftofix(xsf[2] + xsf[1] - xsf[0]);
   ys[br] = al_ftofix(ysf[2] + ysf[1] - ysf[0]);

   xs[bl] = al_ftofix(xsf[2]);
   ys[bl] = al_ftofix(ysf[2]);

   if (DEST_IS_ZERO && SRC_NOT_MODIFIED) {
      _al_parallelogram_map_fast(src, xs, ys, sx, sy, sw, sh);
      return;
   }

   DO_PARALLELOGRAM_MAP(true, flags);
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


/*
* Get scanline starts, ends and deltas, and clipping coordinates.
*/
#define top_bmp_y    corner_bmp_y[0]
#define right_bmp_y  corner_bmp_y[1]
#define bottom_bmp_y corner_bmp_y[2]
#define left_bmp_y   corner_bmp_y[3]
#define top_bmp_x    corner_bmp_x[0]
#define right_bmp_x  corner_bmp_x[1]
#define bottom_bmp_x corner_bmp_x[2]
#define left_bmp_x   corner_bmp_x[3]
#define top_spr_y    corner_spr_y[0]
#define right_spr_y  corner_spr_y[1]
#define bottom_spr_y corner_spr_y[2]
#define left_spr_y   corner_spr_y[3]
#define top_spr_x    corner_spr_x[0]
#define right_spr_x  corner_spr_x[1]
#define bottom_spr_x corner_spr_x[2]
#define left_spr_x   corner_spr_x[3]

/* Copied from rotate.c */

/* _parallelogram_map:
 *  Worker routine for drawing rotated and/or scaled and/or flipped sprites:
 *  It actually maps the sprite to any parallelogram-shaped area of the
 *  bitmap. The top left corner is mapped to (xs[0], ys[0]), the top right to
 *  (xs[1], ys[1]), the bottom right to x (xs[2], ys[2]), and the bottom left
 *  to (xs[3], ys[3]). The corners are assumed to form a perfect
 *  parallelogram, i.e. xs[0]+xs[2] = xs[1]+xs[3]. The corners are given in
 *  fixed point format, so xs[] and ys[] are coordinates of the outer corners
 *  of corner pixels in clockwise order beginning with top left.
 *  All coordinates begin with 0 in top left corner of pixel (0, 0). So a
 *  rotation by 0 degrees of a sprite to the top left of a bitmap can be
 *  specified with coordinates (0, 0) for the top left pixel in source
 *  bitmap. With the default scanline drawer, a pixel in the destination
 *  bitmap is drawn if and only if its center is covered by any pixel in the
 *  sprite. The color of this covering sprite pixel is used to draw.
 *  If sub_pixel_accuracy=false, then the scanline drawer will be called with
 *  *_bmp_x being a fixed point representation of the integers representing
 *  the x coordinate of the first and last point in bmp whose centre is
 *  covered by the sprite. If sub_pixel_accuracy=true, then the scanline
 *  drawer will be called with the exact fixed point position of the first
 *  and last point in which the horizontal line passing through the centre is
 *  at least partly covered by the sprite. This is useful for doing
 *  anti-aliased blending.
 */
static void _al_parallelogram_map_fast(ALLEGRO_BITMAP *src, al_fixed xs[4],
   al_fixed ys[4], int sx, int sy, int sw, int sh)
{
   /* Index in xs[] and ys[] to topmost point. */
   int top_index;
   /* Rightmost point has index (top_index+right_index) int xs[] and ys[]. */
   int right_index;
   /* Loop variables. */
   int index, i;
   /* Coordinates in bmp ordered as top-right-bottom-left. */
   al_fixed corner_bmp_x[4], corner_bmp_y[4];
   /* Coordinates in spr ordered as top-right-bottom-left. */
   al_fixed corner_spr_x[4], corner_spr_y[4];
   /* y coordinate of bottom point, left point and right point. */
   int clip_bottom_i, l_bmp_y_bottom_i, r_bmp_y_bottom_i;
   /* Left and right clipping. */
   al_fixed clip_left, clip_right;
   /* Temporary variable. */
   al_fixed extra_scanline_fraction;
   /* Top scanline of destination */
   int clip_top_i;

   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   int ssize;
   int dsize;

   ALLEGRO_BITMAP *dst = al_get_target_bitmap();

   bool sub_pixel_accuracy = true;

   /*
    * Variables used in the loop
    */
   /* Coordinates of sprite and bmp points in beginning of scanline. */
   al_fixed l_spr_x, l_spr_y, l_bmp_x, l_bmp_dx;
   /* Increment of left sprite point as we move a scanline down. */
   al_fixed l_spr_dx, l_spr_dy;
   /* Coordinates of sprite and bmp points in end of scanline. */
   al_fixed r_bmp_x, r_bmp_dx;
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
   al_fixed r_spr_x, r_spr_y;
   /* Increment of right sprite point as we move a scanline down. */
   al_fixed r_spr_dx, r_spr_dy;
   /*#endif*/
   /* Increment of sprite point as we move right inside a scanline. */
   al_fixed spr_dx, spr_dy;
   /* Positions of beginning of scanline after rounding to integer coordinate
      in bmp. */
   al_fixed l_spr_x_rounded, l_spr_y_rounded, l_bmp_x_rounded;
   al_fixed r_bmp_x_rounded;
   /* Current scanline. */
   int bmp_y_i;
   /* Right edge of scanline. */
   int right_edge_test;

   /* Get index of topmost point. */
   top_index = 0;
   if (ys[1] < ys[0])
      top_index = 1;
   if (ys[2] < ys[top_index])
      top_index = 2;
   if (ys[3] < ys[top_index])
      top_index = 3;

   /* Get direction of points: clockwise or anti-clockwise. */
   right_index = (double)(xs[(top_index+1) & 3] - xs[top_index]) *
      (double)(ys[(top_index-1) & 3] - ys[top_index]) >
      (double)(xs[(top_index-1) & 3] - xs[top_index]) *
      (double)(ys[(top_index+1) & 3] - ys[top_index]) ? 1 : -1;
   /*FIXME: why does al_fixmul overflow below?*/
   /*if (al_fixmul(xs[(top_index+1) & 3] - xs[top_index],
      ys[(top_index-1) & 3] - ys[top_index]) >
         al_fixmul(xs[(top_index-1) & 3] - xs[top_index],
            ys[(top_index+1) & 3] - ys[top_index]))
      right_index = 1;
   else
      right_index = -1;*/

   /*
    * Get coordinates of the corners.
    */

   /* corner_*[0] is top, [1] is right, [2] is bottom, [3] is left. */
   index = top_index;
   for (i = 0; i < 4; i++) {
      corner_bmp_x[i] = xs[index];
      corner_bmp_y[i] = ys[index];
      switch(index)
      {
         case 0:
            corner_spr_x[i] = sx << 16;
            corner_spr_y[i] = sy << 16;
            break;
         case 1:
            /* Need `- 1' since otherwise it would be outside sprite. */
            corner_spr_x[i] = ((sx + sw) << 16) - 1;
            corner_spr_y[i] = sy << 16;
            break;
         case 2:
            corner_spr_x[i] = ((sx + sw) << 16) - 1;
            corner_spr_y[i] = ((sy + sh) << 16) - 1;
            break;
         case 3:
            corner_spr_x[i] = sx << 16;
            corner_spr_y[i] = ((sy + sh) << 16) - 1;
            break;
      }
      index = (index + right_index) & 3;
   }

   /* Calculate left and right clipping. */
   clip_left = dst->cl << 16;
   clip_right = dst->cr_excl << 16;

   /* Quit if we're totally outside. */
   if ((left_bmp_x > clip_right) &&
       (top_bmp_x > clip_right) &&
       (bottom_bmp_x > clip_right))
      return;
   if ((right_bmp_x < clip_left) &&
       (top_bmp_x < clip_left) &&
       (bottom_bmp_x < clip_left))
      return;

   /* Bottom clipping. */
   if (sub_pixel_accuracy)
      clip_bottom_i = (bottom_bmp_y + 0xffff) >> 16;
   else
      clip_bottom_i = (bottom_bmp_y + 0x8000) >> 16;

   /* Bottom clipping */
   if (clip_bottom_i > dst->cb_excl)
      clip_bottom_i = dst->cb_excl;

   /* Calculate y coordinate of first scanline. */
   if (sub_pixel_accuracy)
      bmp_y_i = top_bmp_y >> 16;
   else
      bmp_y_i = (top_bmp_y + 0x8000) >> 16;

   /* Top clipping */
   if (bmp_y_i < dst->ct)
      bmp_y_i = dst->ct;

   if (bmp_y_i < 0)
      bmp_y_i = 0;

   /* Sprite is above or below bottom clipping area. */
   if (bmp_y_i >= clip_bottom_i)
      return;

   /* Vertical gap between top corner and centre of topmost scanline. */
   extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - top_bmp_y;
   /* Calculate x coordinate of beginning of scanline in bmp. */
   l_bmp_dx = al_fixdiv(left_bmp_x - top_bmp_x,
                   left_bmp_y - top_bmp_y);
   l_bmp_x = top_bmp_x + al_fixmul(extra_scanline_fraction, l_bmp_dx);
   /* Calculate x coordinate of beginning of scanline in spr. */
   /* note: all these are rounded down which is probably a Good Thing (tm) */
   l_spr_dx = al_fixdiv(left_spr_x - top_spr_x,
                   left_bmp_y - top_bmp_y);
   l_spr_x = top_spr_x + al_fixmul(extra_scanline_fraction, l_spr_dx);
   /* Calculate y coordinate of beginning of scanline in spr. */
   l_spr_dy = al_fixdiv(left_spr_y - top_spr_y,
                   left_bmp_y - top_bmp_y);
   l_spr_y = top_spr_y + al_fixmul(extra_scanline_fraction, l_spr_dy);

   /* Calculate left loop bound. */
   l_bmp_y_bottom_i = (left_bmp_y + 0x8000) >> 16;
   if (l_bmp_y_bottom_i > clip_bottom_i)
      l_bmp_y_bottom_i = clip_bottom_i;

   /* Calculate x coordinate of end of scanline in bmp. */
   r_bmp_dx = al_fixdiv(right_bmp_x - top_bmp_x,
                   right_bmp_y - top_bmp_y);
   r_bmp_x = top_bmp_x + al_fixmul(extra_scanline_fraction, r_bmp_dx);
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
   /* Calculate x coordinate of end of scanline in spr. */
   r_spr_dx = al_fixdiv(right_spr_x - top_spr_x,
                   right_bmp_y - top_bmp_y);
   r_spr_x = top_spr_x + al_fixmul(extra_scanline_fraction, r_spr_dx);
   /* Calculate y coordinate of end of scanline in spr. */
   r_spr_dy = al_fixdiv(right_spr_y - top_spr_y,
                   right_bmp_y - top_bmp_y);
   r_spr_y = top_spr_y + al_fixmul(extra_scanline_fraction, r_spr_dy);
   /*#endif*/

   /* Calculate right loop bound. */
   r_bmp_y_bottom_i = (right_bmp_y + 0x8000) >> 16;

   /* Get dx and dy, the offsets to add to the source coordinates as we move
      one pixel rightwards along a scanline. This formula can be derived by
      considering the 2x2 matrix that transforms the sprite to the
      parallelogram.
      We'd better use double to get this as exact as possible, since any
      errors will be accumulated along the scanline.
   */
   spr_dx = (al_fixed)((ys[3] - ys[0]) * 65536.0 * (65536.0 * sw) /
                    ((xs[1] - xs[0]) * (double)(ys[3] - ys[0]) -
                     (xs[3] - xs[0]) * (double)(ys[1] - ys[0])));
   spr_dy = (al_fixed)((ys[1] - ys[0]) * 65536.0 * (65536.0 * sh) /
                    ((xs[3] - xs[0]) * (double)(ys[1] - ys[0]) -
                     (xs[1] - xs[0]) * (double)(ys[3] - ys[0])));

   /* Lock the bitmaps */
   src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ARGB_8888, ALLEGRO_LOCK_READONLY);

   clip_top_i = bmp_y_i;

   if (!(dst_region = al_lock_bitmap_region(dst,
         clip_left>>16,
         clip_top_i,
         (clip_right>>16)-(clip_left>>16),
         clip_bottom_i-clip_top_i,
         ALLEGRO_PIXEL_FORMAT_ARGB_8888, 0)))
      return;

   ssize = al_get_pixel_size(src->format);
   dsize = al_get_pixel_size(dst->format);

   /*
    * Loop through scanlines.
    */

   while (1) {
      /* Has beginning of scanline passed a corner? */
      if (bmp_y_i >= l_bmp_y_bottom_i) {
         /* Are we done? */
         if (bmp_y_i >= clip_bottom_i)
            break;

         /* Vertical gap between left corner and centre of scanline. */
         extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - left_bmp_y;
         /* Update x coordinate of beginning of scanline in bmp. */
         l_bmp_dx = al_fixdiv(bottom_bmp_x - left_bmp_x,
                         bottom_bmp_y - left_bmp_y);
         l_bmp_x = left_bmp_x + al_fixmul(extra_scanline_fraction, l_bmp_dx);
         /* Update x coordinate of beginning of scanline in spr. */
         l_spr_dx = al_fixdiv(bottom_spr_x - left_spr_x,
                         bottom_bmp_y - left_bmp_y);
         l_spr_x = left_spr_x + al_fixmul(extra_scanline_fraction, l_spr_dx);
         /* Update y coordinate of beginning of scanline in spr. */
         l_spr_dy = al_fixdiv(bottom_spr_y - left_spr_y,
                         bottom_bmp_y - left_bmp_y);
         l_spr_y = left_spr_y + al_fixmul(extra_scanline_fraction, l_spr_dy);

         /* Update loop bound. */
         if (sub_pixel_accuracy)
            l_bmp_y_bottom_i = (bottom_bmp_y + 0xffff) >> 16;
         else
            l_bmp_y_bottom_i = (bottom_bmp_y + 0x8000) >> 16;
         if (l_bmp_y_bottom_i > clip_bottom_i)
            l_bmp_y_bottom_i = clip_bottom_i;
      }

      /* Has end of scanline passed a corner? */
      if (bmp_y_i >= r_bmp_y_bottom_i) {
         /* Vertical gap between right corner and centre of scanline. */
         extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - right_bmp_y;
         /* Update x coordinate of end of scanline in bmp. */
         r_bmp_dx = al_fixdiv(bottom_bmp_x - right_bmp_x,
                         bottom_bmp_y - right_bmp_y);
         r_bmp_x = right_bmp_x + al_fixmul(extra_scanline_fraction, r_bmp_dx);
         /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
         /* Update x coordinate of beginning of scanline in spr. */
         r_spr_dx = al_fixdiv(bottom_spr_x - right_spr_x,
                         bottom_bmp_y - right_bmp_y);
         r_spr_x = right_spr_x + al_fixmul(extra_scanline_fraction, r_spr_dx);
         /* Update y coordinate of beginning of scanline in spr. */
         r_spr_dy = al_fixdiv(bottom_spr_y - right_spr_y,
                         bottom_bmp_y - right_bmp_y);
         r_spr_y = right_spr_y + al_fixmul(extra_scanline_fraction, r_spr_dy);
         /*#endif*/

         /* Update loop bound: We aren't supposed to use this any more, so
            just set it to some big enough value. */
         r_bmp_y_bottom_i = clip_bottom_i;
      }

      /* Make left bmp coordinate be an integer and clip it. */
      if (sub_pixel_accuracy)
         l_bmp_x_rounded = l_bmp_x;
      else
         l_bmp_x_rounded = (l_bmp_x + 0x8000) & ~0xffff;
      if (l_bmp_x_rounded < clip_left)
         l_bmp_x_rounded = clip_left;

      /* ... and move starting point in sprite accordingly. */
      if (sub_pixel_accuracy) {
         l_spr_x_rounded = l_spr_x +
                           al_fixmul((l_bmp_x_rounded - l_bmp_x), spr_dx);
         l_spr_y_rounded = l_spr_y +
                           al_fixmul((l_bmp_x_rounded - l_bmp_x), spr_dy);
      }
      else {
         l_spr_x_rounded = l_spr_x +
            al_fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dx);
         l_spr_y_rounded = l_spr_y +
            al_fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dy);
      }

      /* Make right bmp coordinate be an integer and clip it. */
      if (sub_pixel_accuracy)
         r_bmp_x_rounded = r_bmp_x;
      else
         r_bmp_x_rounded = (r_bmp_x - 0x8000) & ~0xffff;
      if (r_bmp_x_rounded > clip_right)
         r_bmp_x_rounded = clip_right;

      /* Draw! */
      if (l_bmp_x_rounded <= r_bmp_x_rounded) {
         if (!sub_pixel_accuracy) {
            /* The bodies of these ifs are only reached extremely seldom,
               it's an ugly hack to avoid reading outside the sprite when
               the rounding errors are accumulated the wrong way. It would
               be nicer if we could ensure that this never happens by making
               all multiplications and divisions be rounded up or down at
               the correct places.
               I did try another approach: recalculate the edges of the
               scanline from scratch each scanline rather than incrementally.
               Drawing a sprite with that routine took about 25% longer time
               though.
            */
            if ((unsigned)(l_spr_x_rounded >> 16) >= (unsigned)sw) {
               if (((l_spr_x_rounded < 0) && (spr_dx <= 0)) ||
                   ((l_spr_x_rounded > 0) && (spr_dx >= 0))) {
                  /* This can happen. */
                  goto skip_draw;
               }
               else {
                  /* I don't think this can happen, but I can't prove it. */
                  do {
                     l_spr_x_rounded += spr_dx;
                     l_bmp_x_rounded += 65536;
                     if (l_bmp_x_rounded > r_bmp_x_rounded)
                        goto skip_draw;
                  } while ((unsigned)(l_spr_x_rounded >> 16) >=
                           (unsigned)sw);

               }
            }
            right_edge_test = l_spr_x_rounded +
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *
                              spr_dx;
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)sw) {
               if (((right_edge_test < 0) && (spr_dx <= 0)) ||
                   ((right_edge_test > 0) && (spr_dx >= 0))) {
                  /* This can happen. */
                  do {
                     r_bmp_x_rounded -= 65536;
                     right_edge_test -= spr_dx;
                     if (l_bmp_x_rounded > r_bmp_x_rounded)
                        goto skip_draw;
                  } while ((unsigned)(right_edge_test >> 16) >=
                           (unsigned)sw);
               }
               else {
                  /* I don't think this can happen, but I can't prove it. */
                  goto skip_draw;
               }
            }
            if ((unsigned)(l_spr_y_rounded >> 16) >= (unsigned)sh) {
               if (((l_spr_y_rounded < 0) && (spr_dy <= 0)) ||
                   ((l_spr_y_rounded > 0) && (spr_dy >= 0))) {
                  /* This can happen. */
                  goto skip_draw;
               }
               else {
                  /* I don't think this can happen, but I can't prove it. */
                  do {
                     l_spr_y_rounded += spr_dy;
                     l_bmp_x_rounded += 65536;
                     if (l_bmp_x_rounded > r_bmp_x_rounded)
                        goto skip_draw;
                  } while (((unsigned)l_spr_y_rounded >> 16) >=
                           (unsigned)sh);
               }
            }
            right_edge_test = l_spr_y_rounded +
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *
                              spr_dy;
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)sh) {
               if (((right_edge_test < 0) && (spr_dy <= 0)) ||
                   ((right_edge_test > 0) && (spr_dy >= 0))) {
                  /* This can happen. */
                  do {
                     r_bmp_x_rounded -= 65536;
                     right_edge_test -= spr_dy;
                     if (l_bmp_x_rounded > r_bmp_x_rounded)
                        goto skip_draw;
                  } while ((unsigned)(right_edge_test >> 16) >=
                           (unsigned)sh);
               }
               else {
                  /* I don't think this can happen, but I can't prove it. */
                  goto skip_draw;
               }
            }
         }

   {
      int c;
      unsigned char *addr;
      unsigned char *end_addr;
      int my_r_bmp_x_i = r_bmp_x_rounded >> 16;
      int my_l_bmp_x_i = l_bmp_x_rounded >> 16;
      al_fixed my_l_spr_x = l_spr_x_rounded;
      al_fixed my_l_spr_y = l_spr_y_rounded;
      addr = (void *)(((char *)dst_region->data) +
         (bmp_y_i - clip_top_i) * dst_region->pitch);
      /* adjust for locking offset */
      addr -= (clip_left >> 16) * dsize;
      end_addr = addr + my_r_bmp_x_i * dsize;
      addr += my_l_bmp_x_i * dsize;
      for (; addr < end_addr; addr += dsize) {
       c = bmp_read32((void *)(((char *)src_region->data) +
	  (my_l_spr_y>>16) * src_region->pitch + ssize * (my_l_spr_x>>16)));
       bmp_write32(addr, c);
       my_l_spr_x += spr_dx;
       my_l_spr_y += spr_dy;
      }
   }

      }
      /* I'm not going to apoligize for this label and its gotos: to get
         rid of it would just make the code look worse. */
      skip_draw:

      /* Jump to next scanline. */
      bmp_y_i++;
      /* Update beginning of scanline. */
      l_bmp_x += l_bmp_dx;
      l_spr_x += l_spr_dx;
      l_spr_y += l_spr_dy;
      /* Update end of scanline. */
      r_bmp_x += r_bmp_dx;
      /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
      r_spr_x += r_spr_dx;
      r_spr_y += r_spr_dy;
      /*#endif*/
   }

   al_unlock_bitmap(src);
   al_unlock_bitmap(dst);
}

/* vim: set sts=3 sw=3 et: */
