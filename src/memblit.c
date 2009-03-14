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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/convert.h"
#include <math.h>


#define MIN _ALLEGRO_MIN


/* Inline copy of al_map_rgba().  We may want this to be a header instead. */
static inline void _al_map_rgba(ALLEGRO_COLOR *color,
   unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   color->r = _al_u8_to_float[r];
   color->g = _al_u8_to_float[g];
   color->b = _al_u8_to_float[b];
   color->a = _al_u8_to_float[a];
}


static inline float get_factor(enum ALLEGRO_BLEND_MODE operation, float alpha)
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


static void _al_blend_inline(
   const ALLEGRO_COLOR *scol, const ALLEGRO_COLOR *dcol,
   int src_, int dst_, int asrc_, int adst_, const ALLEGRO_COLOR *bc,
   ALLEGRO_COLOR *result)
{
   float src, dst, asrc, adst;

   result->r = scol->r * bc->r;
   result->g = scol->g * bc->g;
   result->b = scol->b * bc->b;
   result->a = scol->a * bc->a;
   src = get_factor(src_, result->a);
   dst = get_factor(dst_, result->a);
   asrc = get_factor(asrc_, result->a);
   adst = get_factor(adst_, result->a);

   // FIXME: Better not do the check for each pixel but already at the
   // caller.
   // The check is necessary though because the target may be
   // uninitialized (so e.g. all NaN or Inf) and so multiplying with 0
   // doesn't mean the value is ignored.
   if (dst == 0) {
      result->r = result->r * src;
      result->g = result->g * src;
      result->b = result->b * src;
   }
   else {
      result->r = MIN(1, result->r * src + dcol->r * dst);
      result->g = MIN(1, result->g * src + dcol->g * dst);
      result->b = MIN(1, result->b * src + dcol->b * dst);
   }
   if (adst == 0)
      result->a = result->a * asrc;
   else
      result->a = MIN(1, result->a * asrc + dcol->a * adst);
}


#define INLINE_GET(format, data, color, advance)                              \
   do {                                                                       \
      switch (format) {                                                       \
         case ALLEGRO_PIXEL_FORMAT_ARGB_8888: {                               \
            uint32_t pixel = *(uint32_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               (pixel & 0x00FF0000) >> 16,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               (pixel & 0x000000FF) >>  0,                                    \
               (pixel & 0xFF000000) >> 24);                                   \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGBA_8888: {                               \
            uint32_t pixel = *(uint32_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               (pixel & 0xFF000000) >> 24,                                    \
               (pixel & 0x00FF0000) >> 16,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               (pixel & 0x000000FF) >>  0);                                   \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ARGB_4444: {                               \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_4[(pixel & 0x0F00) >> 8],                           \
               _rgb_scale_4[(pixel & 0x00F0) >> 4],                           \
               _rgb_scale_4[(pixel & 0x000F)],                                \
               _rgb_scale_4[(pixel & 0xF000) >>  12]);                        \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGB_888: {                                 \
            uint32_t pixel = READ3BYTES(data);                                \
            _al_map_rgba(&color,                                              \
               (pixel & 0xFF0000) >> 16,                                      \
               (pixel & 0x00FF00) >>  8,                                      \
               (pixel & 0x0000FF) >>  0,                                      \
               255);                                                          \
            if (advance)                                                      \
               data += 3;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGB_565: {                                 \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_5[(pixel & 0xF800) >> 11],                          \
               _rgb_scale_6[(pixel & 0x07E0) >> 5],                           \
               _rgb_scale_5[(pixel & 0x001F)],                                \
               255);                                                          \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGB_555: {                                 \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_5[(pixel & 0x7C00) >> 10],                          \
               _rgb_scale_5[(pixel & 0x03E0) >> 5],                           \
               _rgb_scale_5[(pixel & 0x001F)],                                \
               255);                                                          \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGBA_5551: {                               \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_5[(pixel & 0xF800) >> 11],                          \
               _rgb_scale_5[(pixel & 0x07C0) >> 6],                           \
               _rgb_scale_5[(pixel & 0x003E) >> 1],                           \
               255);                                                          \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ARGB_1555: {                               \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_5[(pixel & 0x7C00) >> 10],                          \
               _rgb_scale_5[(pixel & 0x03E0) >> 5],                           \
               _rgb_scale_5[(pixel & 0x001F)],                                \
               255);                                                          \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ABGR_8888: {                               \
            uint32_t pixel = *(uint32_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               (pixel & 0x000000FF) >>  0,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               (pixel & 0x00FF0000) >> 16,                                    \
               (pixel & 0xFF000000) >> 24);                                   \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_XBGR_8888: {                               \
            uint32_t pixel = *(uint32_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               (pixel & 0x000000FF) >>  0,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               (pixel & 0x00FF0000) >> 16,                                    \
               255);                                                          \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_BGR_888: {                                 \
            uint32_t pixel = READ3BYTES(data);                                \
            _al_map_rgba(&color,                                              \
               (pixel & 0x000000FF) >>  0,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               (pixel & 0x00FF0000) >> 16,                                    \
               255);                                                          \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_BGR_565: {                                 \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_5[(pixel & 0x001F)],                                \
               _rgb_scale_6[(pixel & 0x07E0) >> 5],                           \
               _rgb_scale_5[(pixel & 0xF800) >> 11],                          \
               255);                                                          \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_BGR_555: {                                 \
            uint16_t pixel = *(uint16_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               _rgb_scale_5[(pixel & 0x001F)],                                \
               _rgb_scale_5[(pixel & 0x03E0) >> 5],                           \
               _rgb_scale_5[(pixel & 0x7C00) >> 10],                          \
               255);                                                          \
            if (advance)                                                      \
               data += 2;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGBX_8888: {                               \
            uint32_t pixel = *(uint32_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               (pixel & 0xFF000000) >> 24,                                    \
               (pixel & 0x00FF0000) >> 16,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               255);                                                          \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_XRGB_8888: {                               \
            uint32_t pixel = *(uint32_t *)(data);                             \
            _al_map_rgba(&color,                                              \
               (pixel & 0x00FF0000) >> 16,                                    \
               (pixel & 0x0000FF00) >>  8,                                    \
               (pixel & 0x000000FF),                                          \
               255);                                                          \
            if (advance)                                                      \
               data += 4;                                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ABGR_F32: {                                \
            float *f = (float *)data;                                         \
            color.r = f[0];                                                   \
            color.g = f[1];                                                   \
            color.b = f[2];                                                   \
            color.a = f[3];                                                   \
            if (advance)                                                      \
               data += 4 * sizeof(float);                                     \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ANY:                                       \
         case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:                              \
         case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:                            \
         case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:                         \
         case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:                         \
            TRACE("INLINE_GET got fake pixel format: %d\n", format);          \
            abort();                                                          \
            break;                                                            \
                                                                              \
         case ALLEGRO_NUM_PIXEL_FORMATS:                                      \
            TRACE("INLINE_GET got non pixel format: %d\n", format);           \
            abort();                                                          \
            break;                                                            \
      }                                                                       \
   } while (0)


#define INLINE_PUT(format, data, color)                                       \
   do {                                                                       \
      uint32_t pixel;                                                         \
      switch (format) {                                                       \
         case ALLEGRO_PIXEL_FORMAT_ARGB_8888:                                 \
            pixel  = (int)(color.a * 255) << 24;                              \
            pixel |= (int)(color.r * 255) << 16;                              \
            pixel |= (int)(color.g * 255) <<  8;                              \
            pixel |= (int)(color.b * 255);                                    \
            *(uint32_t *)(data) = pixel;                                      \
            data += 4;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGBA_8888:                                 \
            pixel  = (int)(color.r * 255) << 24;                              \
            pixel |= (int)(color.g * 255) << 16;                              \
            pixel |= (int)(color.b * 255) <<  8;                              \
            pixel |= (int)(color.a * 255);                                    \
            *(uint32_t *)(data) = pixel;                                      \
            data += 4;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ARGB_4444:                                 \
            pixel  = (int)(color.a * 15) << 12;                               \
            pixel |= (int)(color.r * 15) <<  8;                               \
            pixel |= (int)(color.g * 15) <<  4;                               \
            pixel |= (int)(color.b * 15);                                     \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGB_888:                                   \
            pixel  = (int)(color.r * 255) << 16;                              \
            pixel |= (int)(color.g * 255) << 8;                               \
            pixel |= (int)(color.b * 255);                                    \
            WRITE3BYTES(data, pixel);                                         \
            data += 3;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGB_565:                                   \
            pixel  = (int)(color.r * 0x1f) << 11;                             \
            pixel |= (int)(color.g * 0x3f) << 5;                              \
            pixel |= (int)(color.b * 0x1f);                                   \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGB_555:                                   \
            pixel  = (int)(color.r * 0x1f) << 10;                             \
            pixel |= (int)(color.g * 0x1f) << 5;                              \
            pixel |= (int)(color.b * 0x1f);                                   \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGBA_5551:                                 \
            pixel  = (int)(color.r * 0x1f) << 11;                             \
            pixel |= (int)(color.g * 0x1f) << 6;                              \
            pixel |= (int)(color.b * 0x1f) << 1;                              \
            pixel |= (int)color.a;                                            \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ARGB_1555:                                 \
            pixel  = (int)(color.a) << 15;                                    \
            pixel |= (int)(color.r * 0x1f) << 10;                             \
            pixel |= (int)(color.g * 0x1f) <<  5;                             \
            pixel |= (int)(color.b * 0x1f);                                   \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ABGR_8888:                                 \
            pixel  = (int)(color.a * 0xff) << 24;                             \
            pixel |= (int)(color.b * 0xff) << 16;                             \
            pixel |= (int)(color.g * 0xff) << 8;                              \
            pixel |= (int)(color.r * 0xff);                                   \
            *(uint32_t *)(data) = pixel;                                      \
            data += 4;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_XBGR_8888:                                 \
            pixel  = 0xff000000;                                              \
            pixel |= (int)(color.b * 0xff) << 16;                             \
            pixel |= (int)(color.g * 0xff) << 8;                              \
            pixel |= (int)(color.r * 0xff);                                   \
            *(uint32_t *)(data) = pixel;                                      \
            data += 4;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_BGR_888:                                   \
            pixel  = (int)(color.b * 0xff) << 16;                             \
            pixel |= (int)(color.g * 0xff) << 8;                              \
            pixel |= (int)(color.r * 0xff);                                   \
            WRITE3BYTES(data, pixel);                                         \
            data += 3;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_BGR_565:                                   \
            pixel  = (int)(color.b * 0x1f) << 11;                             \
            pixel |= (int)(color.g * 0x3f) << 5;                              \
            pixel |= (int)(color.r * 0x1f);                                   \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_BGR_555:                                   \
            pixel  = (int)(color.b * 0x1f) << 10;                             \
            pixel |= (int)(color.g * 0x1f) << 5;                              \
            pixel |= (int)(color.r * 0x1f);                                   \
            *(uint16_t *)(data) = pixel;                                      \
            data += 2;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_RGBX_8888:                                 \
            pixel  = 0xff;                                                    \
            pixel |= (int)(color.r * 0xff) << 24;                             \
            pixel |= (int)(color.g * 0xff) << 16;                             \
            pixel |= (int)(color.b * 0xff) << 8;                              \
            *(uint32_t *)(data) = pixel;                                      \
            data += 4;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_XRGB_8888:                                 \
            pixel  = 0xff000000;                                              \
            pixel |= (int)(color.r * 0xff) << 16;                             \
            pixel |= (int)(color.g * 0xff) << 8;                              \
            pixel |= (int)(color.b * 0xff);                                   \
            *(uint32_t *)(data) = pixel;                                      \
            data += 4;                                                        \
            break;                                                            \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ABGR_F32: {                                \
            float *f = (float *)data;                                         \
            f[0] = color.r;                                                   \
            f[1] = color.g;                                                   \
            f[2] = color.b;                                                   \
            f[3] = color.a;                                                   \
            data += 4 * sizeof(float);                                        \
            break;                                                            \
         }                                                                    \
                                                                              \
         case ALLEGRO_PIXEL_FORMAT_ANY:                                       \
         case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:                              \
         case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:                            \
         case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:                         \
         case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:                           \
         case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:                         \
            TRACE("INLINE_PUT got fake pixel format: %d\n", format);          \
            abort();                                                          \
            break;                                                            \
                                                                              \
         case ALLEGRO_NUM_PIXEL_FORMATS:                                      \
            TRACE("INLINE_PUT got non pixel format: %d\n", format);           \
            abort();                                                          \
            break;                                                            \
      }                                                                       \
   } while (0)


void _al_draw_bitmap_region_memory(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   int x, y;
   int src_mode, dst_mode;
   ALLEGRO_COLOR *ic;
   bool unlock_dest = false;
   int xinc, yinc;
   int yd;
   int sxd;

   al_get_blender(&src_mode, &dst_mode, NULL);
   ic = _al_get_blend_color();

   if (src_mode == ALLEGRO_ONE && dst_mode == ALLEGRO_ZERO &&
      ic->r == 1.0f && ic->g == 1.0f && ic->b == 1.0f && ic->a == 1.0f &&
      // FIXME: Those should not have to be special cased - but see
      // comments in memblit1.c.
      bitmap->format != ALLEGRO_PIXEL_FORMAT_ABGR_F32 &&
      dest->format != ALLEGRO_PIXEL_FORMAT_ABGR_F32)
   {
      _al_draw_bitmap_region_memory_fast(bitmap, sx, sy, sw, sh, dx, dy, flags);
      return;
   }

   ASSERT(_al_pixel_format_is_real(bitmap->format));
   ASSERT(_al_pixel_format_is_real(dest->format));

   /* Do clipping */
   if (dx < dest->cl) {
      int inc = dest->cl - dx;
      sx += inc;
      dx = dest->cl;
      sw -= inc;
   }
   if (dx+sw > dest->cr) {
      int inc = (dx+sw) - dest->cr;
      sw -= inc;
   }

   if (dy < dest->ct) {
      int inc = dest->ct - dy;
      sy += inc;
      dy = dest->ct;
      sh -= inc;
   }
   if (dy+sh > dest->cb) {
      int inc = (dy+sh) - dest->cb;
      sh -= inc;
   }

   if (sw <= 0 || sh <= 0) {
      return;
   }

   /* Handle sub bitmaps */
   if (dest->parent) {
      dx += dest->xofs;
      dy += dest->yofs;
      dest = dest->parent;
   }

   if (bitmap->parent) {
      sx += bitmap->xofs;
      sy += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   /* Lock the bitmaps */
   if (!(src_region = al_lock_bitmap_region(bitmap, sx, sy, sw, sh, ALLEGRO_PIXEL_FORMAT_ANY,
      ALLEGRO_LOCK_READONLY)))
   {
      return;
   }

   if (!al_is_bitmap_locked(dest)) {
      if (!(dst_region = al_lock_bitmap_region(dest, dx, dy, sw, sh, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
         al_unlock_bitmap(bitmap);
         return;
      }
      unlock_dest = true;
   }

   if (flags & ALLEGRO_FLIP_VERTICAL) {
      yd = sh-1;
      yinc = -1;
   }
   else {
      yd = 0;
      yinc = 1;
   }
   if (flags & ALLEGRO_FLIP_HORIZONTAL) {
      xinc = -1;
      sxd = sw-1;
   }
   else {
      xinc = 1;
      sxd = 0;
   }

   ASSERT(!bitmap->parent);
   ASSERT(!dest->parent);

   {
      ALLEGRO_COLOR src_color = {0, 0, 0, 0};   /* avoid bogus warnings */
      ALLEGRO_COLOR dst_color = {0, 0, 0, 0};
      int src_, dst_, asrc_, adst_;
      ALLEGRO_COLOR bc;

      al_get_separate_blender(&src_, &dst_, &asrc_, &adst_, &bc);

      for (y = 0; y < sh; y++, yd += yinc) {
         char *src_data =
            (((char *) bitmap->locked_region.data)
             + y * bitmap->locked_region.pitch);

         char *dest_data =
            (((char *) dest->locked_region.data)
             + yd * dest->locked_region.pitch);

         for (x = 0; x < sw; x++) {
            ALLEGRO_COLOR result;

            INLINE_GET(bitmap->format, src_data, src_color, true);
            INLINE_GET(dest->format, dest_data, dst_color, false);
            _al_blend_inline(&src_color, &dst_color,
               src_, dst_, asrc_, adst_, &bc, &result);
            INLINE_PUT(dest->format, dest_data, result);
         }
      }
   }

   al_unlock_bitmap(bitmap);
   if (unlock_dest)
      al_unlock_bitmap(dest);
}



void _al_draw_bitmap_memory(ALLEGRO_BITMAP *bitmap,
  int dx, int dy, int flags)
{
   _al_draw_bitmap_region_memory(bitmap, 0, 0, bitmap->w, bitmap->h,
      dx, dy, flags);
}



void _al_draw_scaled_bitmap_memory(ALLEGRO_BITMAP *src,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   int src_mode, dst_mode;
   ALLEGRO_COLOR *bc;

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
   ALLEGRO_COLOR src_color, result;

   al_get_blender(&src_mode, &dst_mode, NULL);
   bc = _al_get_blend_color();

   if (src_mode == ALLEGRO_ONE && dst_mode == ALLEGRO_ZERO &&
      bc->r == 1.0f && bc->g == 1.0f && bc->b == 1.0f && bc->a == 1.0f &&
      (src->format == dest->format)) {
      _al_draw_scaled_bitmap_memory_fast(src,
            sx, sy, sw, sh, dx, dy, dw, dh, flags);
      return;
   }

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

   /* Handle sub bitmaps */
   if (dest->parent) {
      dx += dest->xofs;
      dy += dest->yofs;
      dest = dest->parent;
   }

   if (src->parent) {
      sx += src->xofs;
      sy += src->yofs;
      src = src->parent;
   }

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
   else {
      _sy = sy;
   }

   /* XXX optimise this */
   for (y = 0; y < yend; y++) {
      _sx = sx;
      _dx = dx;
      for (x = 0; x < xend; x++) {
         src_color = al_get_pixel(src, _sx, _sy);
         _al_blend(&src_color, dest, _dx, _dy, &result);
         _al_put_pixel(dest, _dx, _dy, result);
         _sx += sxinc;
         _dx += dxinc;
      }
      _sy += syinc;
      _dy += dyinc;
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
   fixed corner_bmp_x[4], corner_bmp_y[4];                                   \
   /* Coordinates in spr ordered as top-right-bottom-left. */                \
   fixed corner_spr_x[4], corner_spr_y[4];                                   \
   /* y coordinate of bottom point, left point and right point. */           \
   int clip_bottom_i, l_bmp_y_bottom_i, r_bmp_y_bottom_i;                    \
   /* Left and right clipping. */                                            \
   fixed clip_left, clip_right;                                              \
   /* Temporary variable. */                                                 \
   fixed extra_scanline_fraction;                                            \
   /* Top scanline of destination */                                         \
   int clip_top_i;                                                           \
                                                                             \
   ALLEGRO_LOCKED_REGION *src_region;                                         \
   ALLEGRO_LOCKED_REGION *dst_region;                                         \
                                                                             \
   /*                                                                        \
    * Variables used in the loop                                             \
    */                                                                       \
   /* Coordinates of sprite and bmp points in beginning of scanline. */      \
   fixed l_spr_x, l_spr_y, l_bmp_x, l_bmp_dx;                                \
   /* Increment of left sprite point as we move a scanline down. */          \
   fixed l_spr_dx, l_spr_dy;                                                 \
   /* Coordinates of sprite and bmp points in end of scanline. */            \
   fixed r_bmp_x, r_bmp_dx;                                                  \
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                            \
   fixed r_spr_x, r_spr_y;                                                   \
   /* Increment of right sprite point as we move a scanline down. */         \
   fixed r_spr_dx, r_spr_dy;                                                 \
   /*#endif*/                                                                \
   /* Increment of sprite point as we move right inside a scanline. */       \
   fixed spr_dx, spr_dy;                                                     \
   /* Positions of beginning of scanline after rounding to integer coordinate\
      in bmp. */                                                             \
   fixed l_spr_x_rounded, l_spr_y_rounded, l_bmp_x_rounded;                  \
   fixed r_bmp_x_rounded;                                                    \
   /* Current scanline. */                                                   \
   int bmp_y_i;                                                              \
   /* Right edge of scanline. */                                             \
   int right_edge_test;                                                      \
   ALLEGRO_COLOR src_color, result;                                          \
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
   /*FIXME: why does fixmul overflow below?*/                                \
   /*if (fixmul(xs[(top_index+1) & 3] - xs[top_index],                       \
      ys[(top_index-1) & 3] - ys[top_index]) >                               \
         fixmul(xs[(top_index-1) & 3] - xs[top_index],                       \
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
      if (index < 2)                                                         \
         corner_spr_y[i] = 0;                                                \
      else                                                                   \
         /* Need `- 1' since otherwise it would be outside sprite. */        \
         corner_spr_y[i] = (src->h << 16) - 1;                               \
      if ((index == 0) || (index == 3))                                      \
         corner_spr_x[i] = 0;                                                \
      else                                                                   \
         corner_spr_x[i] = (src->w << 16) - 1;                               \
      index = (index + right_index) & 3;                                     \
   }                                                                         \
                                                                             \
   /* Calculate left and right clipping. */                                  \
   clip_left = dst->cl << 16;                                                \
   clip_right = (dst->cr << 16) - 1;                                         \
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
   if (clip_bottom_i > dst->cb)                                              \
      clip_bottom_i = dst->cb;                                               \
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
   l_bmp_dx = fixdiv(left_bmp_x - top_bmp_x,                                 \
                   left_bmp_y - top_bmp_y);                                  \
   l_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx);          \
   /* Calculate x coordinate of beginning of scanline in spr. */             \
   /* note: all these are rounded down which is probably a Good Thing (tm) */\
   l_spr_dx = fixdiv(left_spr_x - top_spr_x,                                 \
                   left_bmp_y - top_bmp_y);                                  \
   l_spr_x = top_spr_x + fixmul(extra_scanline_fraction, l_spr_dx);          \
   /* Calculate y coordinate of beginning of scanline in spr. */             \
   l_spr_dy = fixdiv(left_spr_y - top_spr_y,                                 \
                   left_bmp_y - top_bmp_y);                                  \
   l_spr_y = top_spr_y + fixmul(extra_scanline_fraction, l_spr_dy);          \
                                                                             \
   /* Calculate left loop bound. */                                          \
   l_bmp_y_bottom_i = (left_bmp_y + 0x8000) >> 16;                           \
   if (l_bmp_y_bottom_i > clip_bottom_i)                                     \
      l_bmp_y_bottom_i = clip_bottom_i;                                      \
                                                                             \
   /* Calculate x coordinate of end of scanline in bmp. */                   \
   r_bmp_dx = fixdiv(right_bmp_x - top_bmp_x,                                \
                   right_bmp_y - top_bmp_y);                                 \
   r_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx);          \
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                            \
   /* Calculate x coordinate of end of scanline in spr. */                   \
   r_spr_dx = fixdiv(right_spr_x - top_spr_x,                                \
                   right_bmp_y - top_bmp_y);                                 \
   r_spr_x = top_spr_x + fixmul(extra_scanline_fraction, r_spr_dx);          \
   /* Calculate y coordinate of end of scanline in spr. */                   \
   r_spr_dy = fixdiv(right_spr_y - top_spr_y,                                \
                   right_bmp_y - top_bmp_y);                                 \
   r_spr_y = top_spr_y + fixmul(extra_scanline_fraction, r_spr_dy);          \
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
   spr_dx = (fixed)((ys[3] - ys[0]) * 65536.0 * (65536.0 * src->w) /         \
                    ((xs[1] - xs[0]) * (double)(ys[3] - ys[0]) -             \
                     (xs[3] - xs[0]) * (double)(ys[1] - ys[0])));            \
   spr_dy = (fixed)((ys[1] - ys[0]) * 65536.0 * (65536.0 * src->h) /         \
                    ((xs[3] - xs[0]) * (double)(ys[1] - ys[0]) -             \
                     (xs[1] - xs[0]) * (double)(ys[3] - ys[0])));            \
                                                                             \
   /* Lock the bitmaps */                                                    \
   src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);                  \
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
         l_bmp_dx = fixdiv(bottom_bmp_x - left_bmp_x,                        \
                         bottom_bmp_y - left_bmp_y);                         \
         l_bmp_x = left_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx);   \
         /* Update x coordinate of beginning of scanline in spr. */          \
         l_spr_dx = fixdiv(bottom_spr_x - left_spr_x,                        \
                         bottom_bmp_y - left_bmp_y);                         \
         l_spr_x = left_spr_x + fixmul(extra_scanline_fraction, l_spr_dx);   \
         /* Update y coordinate of beginning of scanline in spr. */          \
         l_spr_dy = fixdiv(bottom_spr_y - left_spr_y,                        \
                         bottom_bmp_y - left_bmp_y);                         \
         l_spr_y = left_spr_y + fixmul(extra_scanline_fraction, l_spr_dy);   \
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
         r_bmp_dx = fixdiv(bottom_bmp_x - right_bmp_x,                       \
                         bottom_bmp_y - right_bmp_y);                        \
         r_bmp_x = right_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx);  \
         /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/                      \
         /* Update x coordinate of beginning of scanline in spr. */          \
         r_spr_dx = fixdiv(bottom_spr_x - right_spr_x,                       \
                         bottom_bmp_y - right_bmp_y);                        \
         r_spr_x = right_spr_x + fixmul(extra_scanline_fraction, r_spr_dx);  \
         /* Update y coordinate of beginning of scanline in spr. */          \
         r_spr_dy = fixdiv(bottom_spr_y - right_spr_y,                       \
                         bottom_bmp_y - right_bmp_y);                        \
         r_spr_y = right_spr_y + fixmul(extra_scanline_fraction, r_spr_dy);  \
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
                           fixmul((l_bmp_x_rounded - l_bmp_x), spr_dx);      \
         l_spr_y_rounded = l_spr_y +                                         \
                           fixmul((l_bmp_x_rounded - l_bmp_x), spr_dy);      \
      }                                                                      \
      else {                                                                 \
         l_spr_x_rounded = l_spr_x +                                         \
                           fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dx);\
         l_spr_y_rounded = l_spr_y +                                         \
                           fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dy);\
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
            if ((unsigned)(l_spr_x_rounded >> 16) >= (unsigned)src->w) {     \
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
                           (unsigned)src->w);                                \
                                                                             \
               }                                                             \
            }                                                                \
            right_edge_test = l_spr_x_rounded +                              \
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *  \
                              spr_dx;                                        \
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)src->w) {     \
               if (((right_edge_test < 0) && (spr_dx <= 0)) ||               \
                   ((right_edge_test > 0) && (spr_dx >= 0))) {               \
                  /* This can happen. */                                     \
                  do {                                                       \
                     r_bmp_x_rounded -= 65536;                               \
                     right_edge_test -= spr_dx;                              \
                     if (l_bmp_x_rounded > r_bmp_x_rounded)                  \
                        goto skip_draw;                                      \
                  } while ((unsigned)(right_edge_test >> 16) >=              \
                           (unsigned)src->w);                                \
               }                                                             \
               else {                                                        \
                  /* I don't think this can happen, but I can't prove it. */ \
                  goto skip_draw;                                            \
               }                                                             \
            }                                                                \
            if ((unsigned)(l_spr_y_rounded >> 16) >= (unsigned)src->h) {     \
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
                           (unsigned)src->h);                                \
               }                                                             \
            }                                                                \
            right_edge_test = l_spr_y_rounded +                              \
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *  \
                              spr_dy;                                        \
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)src->h) {     \
               if (((right_edge_test < 0) && (spr_dy <= 0)) ||               \
                   ((right_edge_test > 0) && (spr_dy >= 0))) {               \
                  /* This can happen. */                                     \
                  do {                                                       \
                     r_bmp_x_rounded -= 65536;                               \
                     right_edge_test -= spr_dy;                              \
                     if (l_bmp_x_rounded > r_bmp_x_rounded)                  \
                        goto skip_draw;                                      \
                  } while ((unsigned)(right_edge_test >> 16) >=              \
                           (unsigned)src->h);                                \
               }                                                             \
               else {                                                        \
                  /* I don't think this can happen, but I can't prove it. */ \
                  goto skip_draw;                                            \
               }                                                             \
            }                                                                \
         }                                                                   \
                                                                             \
   {                                                                         \
      int c;                                                                 \
      int x;                                                                 \
      /*unsigned char *addr;                                                 \
      unsigned char *end_addr;*/                                             \
      int my_r_bmp_x_i = r_bmp_x_rounded >> 16;                              \
      int my_l_bmp_x_i = l_bmp_x_rounded >> 16;                              \
      fixed my_l_spr_x = l_spr_x_rounded;                                    \
      fixed my_l_spr_y = l_spr_y_rounded;                                    \
      int startx = my_l_bmp_x_i;                                             \
      int endx = my_r_bmp_x_i;                                               \
      /*addr = dst_region.data+(bmp_y_i-clip_top_i)*dst_region.pitch;*/      \
      /* adjust for locking offset */                                        \
      /*addr -= (clip_left >> 16) * dsize;                                   \
      end_addr = addr + my_r_bmp_x_i * dsize;                                \
      addr += my_l_bmp_x_i * dsize;*/                                        \
      /*for (; addr < end_addr; addr += dsize) {*/                           \
      /* XXX optimise this */                                                \
      for (x = 0; x < endx-startx; x++) {                                    \
         src_color = al_get_pixel(src, my_l_spr_x>>16, my_l_spr_y>>16);      \
         _al_blend(&src_color, dst, x+my_l_bmp_x_i, bmp_y_i, &result);       \
         _al_put_pixel(dst, x+my_l_bmp_x_i, bmp_y_i, result);                \
         (void) c;                                                           \
         /*c = get(src_region.data+(my_l_spr_y>>16)*src_region.pitch+ssize*(my_l_spr_x>>16));\
         c = convert(c);                                                     \
         set(addr, c);*/                                                     \
         my_l_spr_x += spr_dx;                                               \
         my_l_spr_y += spr_dy;                                               \
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


/* _pivot_scaled_sprite_flip:
 *  The most generic routine to which you specify the position with angles,
 *  scales, etc.
 */
#define DO_DRAW_ROTATED_SCALED(src, dst,                                     \
       fl_cx, fl_cy,                                                         \
       fl_dx, fl_dy,                                                         \
       fl_xscale, fl_yscale,                                                 \
       fl_angle,                                                             \
       flags)                                                                \
do {                                                                         \
   fixed xs[4], ys[4];                                                       \
   fixed fix_dx = ftofix(fl_dx);                                             \
   fixed fix_dy = ftofix(fl_dy);                                             \
   fixed fix_cx = ftofix(fl_cx);                                             \
   fixed fix_cy = ftofix(fl_cy);                                             \
   fixed fix_angle = ftofix(fl_angle*256/(ALLEGRO_PI*2));                    \
   fixed fix_xscale = ftofix(fl_xscale);                                     \
   fixed fix_yscale = ftofix(fl_yscale);                                     \
                                                                             \
   _rotate_scale_flip_coordinates(src->w << 16, src->h << 16,                \
      fix_dx, fix_dy, fix_cx, fix_cy, fix_angle, fix_xscale, fix_yscale,     \
      flags & ALLEGRO_FLIP_HORIZONTAL, flags & ALLEGRO_FLIP_VERTICAL,        \
      xs, ys);                                                               \
                                                                             \
   DO_PARALLELOGRAM_MAP(false, flags);                                       \
} while (0)


void _al_draw_rotated_scaled_bitmap_memory(ALLEGRO_BITMAP *src,
   int cx, int cy, int dx, int dy, float xscale, float yscale,
   float angle, int flags)
{
   ALLEGRO_BITMAP *dst = al_get_target_bitmap();
   int src_mode, dst_mode;
   ALLEGRO_COLOR *ic;

   al_get_blender(&src_mode, &dst_mode, NULL);
   ic = _al_get_blend_color();

   if (src_mode == ALLEGRO_ONE && dst_mode == ALLEGRO_ZERO &&
      ic->r == 1.0f && ic->g == 1.0f && ic->b == 1.0f && ic->a == 1.0f &&
      (src->format == dst->format)) {
      _al_draw_rotated_scaled_bitmap_memory_fast(src,
         cx, cy, dx, dy, xscale, yscale, angle, flags);
      return;
   }

   ASSERT(_al_pixel_format_is_real(src->format));
   ASSERT(_al_pixel_format_is_real(dst->format));

   DO_DRAW_ROTATED_SCALED(src, dst,
      cx, cy, dx, dy, xscale, yscale, -angle, flags);
}


void _al_draw_rotated_bitmap_memory(ALLEGRO_BITMAP *src,
   int cx, int cy, int dx, int dy, float angle, int flags)
{
   _al_draw_rotated_scaled_bitmap_memory(src,
      cx, cy, dx, dy, 1.0f, 1.0f, angle, flags);
}

void _al_draw_bitmap_region_memory_fast(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   ASSERT(_al_pixel_format_is_real(bitmap->format));
   ASSERT(_al_pixel_format_is_real(dest->format));

   /* Do clipping */
   if (dx < dest->cl) {
      int inc = dest->cl - dx;
      sx += inc;
      dx = dest->cl;
      sw -= inc;
   }
   if (dx+sw-1 > dest->cr) {
      int inc = (dx+sw-1) - dest->cr;
      sw -= inc;
   }

   if (dy < dest->ct) {
      int inc = dest->ct - dy;
      sy += inc;
      dy = dest->ct;
      sh -= inc;
   }
   if (dy+sh-1 > dest->cb) {
      int inc = (dy+sh-1) - dest->cb;
      sh -= inc;
   }

   /* Handle sub bitmaps */
   if (dest->parent) {
      dx += dest->xofs;
      dy += dest->yofs;
      dest = dest->parent;
   }

   if (bitmap->parent) {
      sx += bitmap->xofs;
      sy += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   /* Lock the bitmaps */
   if (!(src_region = al_lock_bitmap_region(bitmap, sx, sy, sw, sh,
      ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA, ALLEGRO_LOCK_READONLY))) {
      return;
   }

   if (!(dst_region = al_lock_bitmap_region(dest, dx, dy, sw, sh,
      src_region->format, ALLEGRO_LOCK_WRITEONLY))) {
      al_unlock_bitmap(bitmap);
      return;
   }

   do {
      int x;
      int y;
      int cdx, cdy;         /* current dest */
      int dxi, dyi;         /* dest increments */
      int pixel;

      (void)sx;
      (void)sy;
      (void)dx;
      (void)dy;

      /* Adjust for flipping */

      if (flags & ALLEGRO_FLIP_HORIZONTAL) {
         cdx = sw - 1;
         dxi = -1;
      }
      else {
         cdx = 0;
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
         cdx = 0;
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


void _al_draw_scaled_bitmap_memory_fast(ALLEGRO_BITMAP *src,
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

   /* Handle sub bitmaps */
   if (dest->parent) {
      dx += dest->xofs;
      dy += dest->yofs;
      dest = dest->parent;
   }

   if (src->parent) {
      sx += src->xofs;
      sy += src->yofs;
      src = src->parent;
   }

   if (!(src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ARGB_8888, ALLEGRO_LOCK_READONLY))) {
      return;
   }
   /* XXX we should be able to lock less of the destination and use
    * ALLEGRO_LOCK_WRITEONLY
    */
   if (!(dst_region = al_lock_bitmap(dest, ALLEGRO_PIXEL_FORMAT_ARGB_8888, 0))) {
      al_unlock_bitmap(src);
      return;
   }

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
   else {
      _sy = sy;
   }

   for (y = 0; y < yend; y++) {
      _sx = sx;
      _dx = dx;
      for (x = 0; x < xend; x++) {
         uint32_t pix = bmp_read32((char *)src_region->data+(int)_sy*src_region->pitch+(int)_sx*4);
         bmp_write32((char *)dst_region->data+(int)_dy*dst_region->pitch+(int)_dy*4, pix);
      }
      _sy += syinc;
      _dy += dyinc;
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
void _al_draw_rotated_scaled_bitmap_memory_fast(ALLEGRO_BITMAP *src,
   int cx, int cy, int dx, int dy, float xscale, float yscale,
   float angle, int flags)
{
   /* Index in xs[] and ys[] to topmost point. */
   int top_index;
   /* Rightmost point has index (top_index+right_index) int xs[] and ys[]. */
   int right_index;
   /* Loop variables. */
   int index, i;
   /* Coordinates in bmp ordered as top-right-bottom-left. */
   fixed corner_bmp_x[4], corner_bmp_y[4];
   /* Coordinates in spr ordered as top-right-bottom-left. */
   fixed corner_spr_x[4], corner_spr_y[4];
   /* y coordinate of bottom point, left point and right point. */
   int clip_bottom_i, l_bmp_y_bottom_i, r_bmp_y_bottom_i;
   /* Left and right clipping. */
   fixed clip_left, clip_right;
   /* Temporary variable. */
   fixed extra_scanline_fraction;
   /* Top scanline of destination */
   int clip_top_i;

   ALLEGRO_LOCKED_REGION *src_region;
   ALLEGRO_LOCKED_REGION *dst_region;
   int ssize;
   int dsize;

   ALLEGRO_BITMAP *dst = al_get_target_bitmap();

   bool sub_pixel_accuracy = false;
   
   fixed xs[4], ys[4];
   fixed fix_dx = ftofix(dx);
   fixed fix_dy = ftofix(dy);
   fixed fix_cx = ftofix(cx);
   fixed fix_cy = ftofix(cy);
   fixed fix_angle = ftofix(angle*256/(ALLEGRO_PI*2));
   fixed fix_xscale = ftofix(xscale);
   fixed fix_yscale = ftofix(yscale);

   /*
    * Variables used in the loop
    */
   /* Coordinates of sprite and bmp points in beginning of scanline. */
   fixed l_spr_x, l_spr_y, l_bmp_x, l_bmp_dx;
   /* Increment of left sprite point as we move a scanline down. */
   fixed l_spr_dx, l_spr_dy;
   /* Coordinates of sprite and bmp points in end of scanline. */
   fixed r_bmp_x, r_bmp_dx;
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
   fixed r_spr_x, r_spr_y;
   /* Increment of right sprite point as we move a scanline down. */
   fixed r_spr_dx, r_spr_dy;
   /*#endif*/
   /* Increment of sprite point as we move right inside a scanline. */
   fixed spr_dx, spr_dy;
   /* Positions of beginning of scanline after rounding to integer coordinate
      in bmp. */
   fixed l_spr_x_rounded, l_spr_y_rounded, l_bmp_x_rounded;
   fixed r_bmp_x_rounded;
   /* Current scanline. */
   int bmp_y_i;
   /* Right edge of scanline. */
   int right_edge_test;

   _rotate_scale_flip_coordinates(src->w << 16, src->h << 16,
      fix_dx, fix_dy, fix_cx, fix_cy, fix_angle, fix_xscale, fix_yscale,
      flags & ALLEGRO_FLIP_HORIZONTAL, flags & ALLEGRO_FLIP_VERTICAL, xs, ys);

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
   /*FIXME: why does fixmul overflow below?*/
   /*if (fixmul(xs[(top_index+1) & 3] - xs[top_index],
      ys[(top_index-1) & 3] - ys[top_index]) >
         fixmul(xs[(top_index-1) & 3] - xs[top_index],
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
      if (index < 2)
         corner_spr_y[i] = 0;
      else
         /* Need `- 1' since otherwise it would be outside sprite. */
         corner_spr_y[i] = (src->h << 16) - 1;
      if ((index == 0) || (index == 3))
         corner_spr_x[i] = 0;
      else
         corner_spr_x[i] = (src->w << 16) - 1;
      index = (index + right_index) & 3;
   }

   /* Calculate left and right clipping. */
   clip_left = dst->cl << 16;
   clip_right = (dst->cr << 16) - 1;

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
   if (clip_bottom_i > dst->cb)
      clip_bottom_i = dst->cb;

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
   l_bmp_dx = fixdiv(left_bmp_x - top_bmp_x,
                   left_bmp_y - top_bmp_y);
   l_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx);
   /* Calculate x coordinate of beginning of scanline in spr. */
   /* note: all these are rounded down which is probably a Good Thing (tm) */
   l_spr_dx = fixdiv(left_spr_x - top_spr_x,
                   left_bmp_y - top_bmp_y);
   l_spr_x = top_spr_x + fixmul(extra_scanline_fraction, l_spr_dx);
   /* Calculate y coordinate of beginning of scanline in spr. */
   l_spr_dy = fixdiv(left_spr_y - top_spr_y,
                   left_bmp_y - top_bmp_y);
   l_spr_y = top_spr_y + fixmul(extra_scanline_fraction, l_spr_dy);

   /* Calculate left loop bound. */
   l_bmp_y_bottom_i = (left_bmp_y + 0x8000) >> 16;
   if (l_bmp_y_bottom_i > clip_bottom_i)
      l_bmp_y_bottom_i = clip_bottom_i;

   /* Calculate x coordinate of end of scanline in bmp. */
   r_bmp_dx = fixdiv(right_bmp_x - top_bmp_x,
                   right_bmp_y - top_bmp_y);
   r_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx);
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
   /* Calculate x coordinate of end of scanline in spr. */
   r_spr_dx = fixdiv(right_spr_x - top_spr_x,
                   right_bmp_y - top_bmp_y);
   r_spr_x = top_spr_x + fixmul(extra_scanline_fraction, r_spr_dx);
   /* Calculate y coordinate of end of scanline in spr. */
   r_spr_dy = fixdiv(right_spr_y - top_spr_y,
                   right_bmp_y - top_bmp_y);
   r_spr_y = top_spr_y + fixmul(extra_scanline_fraction, r_spr_dy);
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
   spr_dx = (fixed)((ys[3] - ys[0]) * 65536.0 * (65536.0 * src->w) /
                    ((xs[1] - xs[0]) * (double)(ys[3] - ys[0]) -
                     (xs[3] - xs[0]) * (double)(ys[1] - ys[0])));
   spr_dy = (fixed)((ys[1] - ys[0]) * 65536.0 * (65536.0 * src->h) /
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
         l_bmp_dx = fixdiv(bottom_bmp_x - left_bmp_x,
                         bottom_bmp_y - left_bmp_y);
         l_bmp_x = left_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx);
         /* Update x coordinate of beginning of scanline in spr. */
         l_spr_dx = fixdiv(bottom_spr_x - left_spr_x,
                         bottom_bmp_y - left_bmp_y);
         l_spr_x = left_spr_x + fixmul(extra_scanline_fraction, l_spr_dx);
         /* Update y coordinate of beginning of scanline in spr. */
         l_spr_dy = fixdiv(bottom_spr_y - left_spr_y,
                         bottom_bmp_y - left_bmp_y);
         l_spr_y = left_spr_y + fixmul(extra_scanline_fraction, l_spr_dy);

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
         r_bmp_dx = fixdiv(bottom_bmp_x - right_bmp_x,
                         bottom_bmp_y - right_bmp_y);
         r_bmp_x = right_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx);
         /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/
         /* Update x coordinate of beginning of scanline in spr. */
         r_spr_dx = fixdiv(bottom_spr_x - right_spr_x,
                         bottom_bmp_y - right_bmp_y);
         r_spr_x = right_spr_x + fixmul(extra_scanline_fraction, r_spr_dx);
         /* Update y coordinate of beginning of scanline in spr. */
         r_spr_dy = fixdiv(bottom_spr_y - right_spr_y,
                         bottom_bmp_y - right_bmp_y);
         r_spr_y = right_spr_y + fixmul(extra_scanline_fraction, r_spr_dy);
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
                           fixmul((l_bmp_x_rounded - l_bmp_x), spr_dx);
         l_spr_y_rounded = l_spr_y +
                           fixmul((l_bmp_x_rounded - l_bmp_x), spr_dy);
      }
      else {
         l_spr_x_rounded = l_spr_x +
            fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dx);
         l_spr_y_rounded = l_spr_y +
            fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dy);
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
            if ((unsigned)(l_spr_x_rounded >> 16) >= (unsigned)src->w) {
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
                           (unsigned)src->w);

               }
            }
            right_edge_test = l_spr_x_rounded +
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *
                              spr_dx;
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)src->w) {
               if (((right_edge_test < 0) && (spr_dx <= 0)) ||
                   ((right_edge_test > 0) && (spr_dx >= 0))) {
                  /* This can happen. */
                  do {
                     r_bmp_x_rounded -= 65536;
                     right_edge_test -= spr_dx;
                     if (l_bmp_x_rounded > r_bmp_x_rounded)
                        goto skip_draw;
                  } while ((unsigned)(right_edge_test >> 16) >=
                           (unsigned)src->w);
               }
               else {
                  /* I don't think this can happen, but I can't prove it. */
                  goto skip_draw;
               }
            }
            if ((unsigned)(l_spr_y_rounded >> 16) >= (unsigned)src->h) {
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
                           (unsigned)src->h);
               }
            }
            right_edge_test = l_spr_y_rounded +
                              ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) *
                              spr_dy;
            if ((unsigned)(right_edge_test >> 16) >= (unsigned)src->h) {
               if (((right_edge_test < 0) && (spr_dy <= 0)) ||
                   ((right_edge_test > 0) && (spr_dy >= 0))) {
                  /* This can happen. */
                  do {
                     r_bmp_x_rounded -= 65536;
                     right_edge_test -= spr_dy;
                     if (l_bmp_x_rounded > r_bmp_x_rounded)
                        goto skip_draw;
                  } while ((unsigned)(right_edge_test >> 16) >=
                           (unsigned)src->h);
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
      fixed my_l_spr_x = l_spr_x_rounded;
      fixed my_l_spr_y = l_spr_y_rounded;
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
