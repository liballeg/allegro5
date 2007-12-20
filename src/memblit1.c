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

#include "allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/convert.h"
#include <math.h>


#ifndef DEBUGMODE

/* draw_region_memory workhorse */
#define DO_DRAW_REGION_FAST(src, \
   sx, sy, sw, sh, spitch, ssize, sformat, get, \
   dst, dx, dy, dpitch, dsize, dformat, set, \
   convert, flags) \
{ \
   int x; \
   int y; \
   ALLEGRO_LOCKED_REGION lr; \
   int cdx, cdy;         /* current dest */ \
   int dxi, dyi;         /* dest increments */ \
   int pixel; \
\
   /* Adjust for flipping */ \
\
   if (flags & ALLEGRO_FLIP_HORIZONTAL) { \
      cdx = sw - 1; \
      dxi = -1; \
   } \
   else { \
      cdx = 0; \
      dxi = 1; \
   } \
\
   if (flags & ALLEGRO_FLIP_VERTICAL) { \
      cdy = sh - 1; \
      dyi = -1; \
   } \
   else { \
      cdy = 0; \
      dyi = 1; \
   } \
\
   for (y = 0; y < sh; y++) { \
      cdx = 0; \
      for (x = 0; x < sw; x++) { \
         pixel = get(src+y*spitch+x*ssize); \
  	 pixel = convert(pixel); \
         set(dst+cdy*dpitch+cdx*dsize, pixel); \
	 cdx += dxi; \
      } \
      cdy += dyi; \
   } \
}

/* draw_region_memory functions */
#define REAL_DEFINE_DRAW_REGION(get, \
	func1, macro1, \
	func2, macro2, \
	func3, macro3, \
	func4, macro4, \
	func5, macro5, \
	func6, macro6, \
	func7, macro7, \
	func8, macro8, \
	func9, macro9, \
	func10, macro10, \
	func11, macro11, \
	func12, macro12, \
	func13, macro13, \
	func14, macro14, \
	func15, macro15, \
	func16, macro16) \
\
static void func1 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 4, dformat, bmp_write32, \
      macro1, flags) \
} \
\
static void func2 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 4, dformat, bmp_write32, \
      macro2, flags) \
} \
\
static void func3 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro3, flags) \
} \
\
static void func4 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 3, dformat, WRITE3BYTES, \
      macro4, flags) \
} \
\
static void func5 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro5, flags) \
} \
\
static void func6 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro6, flags) \
} \
\
static void func7 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 1, dformat, bmp_write8, \
      macro7, flags) \
} \
\
static void func8 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro8, flags) \
} \
\
static void func9 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro9, flags) \
} \
\
static void func10 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 4, dformat, bmp_write32, \
      macro10, flags) \
} \
\
static void func11 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 4, dformat, bmp_write32, \
      macro11, flags) \
} \
\
static void func12 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 3, dformat, WRITE3BYTES, \
      macro1, flags) \
} \
\
static void func13 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro13, flags) \
} \
\
static void func14 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 2, dformat, bmp_write16, \
      macro14, flags) \
} \
\
static void func15 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 4, dformat, bmp_write32, \
      macro15, flags) \
} \
\
static void func16 (void *src, \
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat, \
   void *dst, \
   int dx, int dy, int dpitch, int dsize, int dformat, \
   int flags) \
{ \
   DO_DRAW_REGION_FAST(src, \
      sx, sy, sw, sh, spitch, ssize, sformat, get, \
      dst, dx, dy, dpitch, 4, dformat, bmp_write32, \
      macro16, flags) \
}

#define DEFINE_DRAW_REGION(get, fprefix, mprefix) \
	REAL_DEFINE_DRAW_REGION(get, \
	fprefix ## _to_argb_8888, mprefix ## _TO_ARGB_8888, \
	fprefix ## _to_rgba_8888, mprefix ## _TO_RGBA_8888, \
	fprefix ## _to_argb_4444, mprefix ## _TO_ARGB_4444, \
	fprefix ## _to_rgb_888, mprefix ## _TO_RGB_888, \
	fprefix ## _to_rgb_565, mprefix ## _TO_RGB_565, \
	fprefix ## _to_rgb_555, mprefix ## _TO_RGB_555, \
	fprefix ## _to_palette_8, mprefix ## _TO_PALETTE_8, \
	fprefix ## _to_rgba_5551, mprefix ## _TO_RGBA_5551, \
	fprefix ## _to_argb_1555, mprefix ## _TO_ARGB_1555, \
	fprefix ## _to_abgr_8888, mprefix ## _TO_ABGR_8888, \
	fprefix ## _to_xbgr_8888, mprefix ## _TO_XBGR_8888, \
	fprefix ## _to_bgr_888, mprefix ## _TO_BGR_888, \
	fprefix ## _to_bgr_565, mprefix ## _TO_BGR_565, \
	fprefix ## _to_bgr_555, mprefix ## _TO_BGR_555, \
	fprefix ## _to_rgbx_8888, mprefix ## _TO_RGBX_8888, \
	fprefix ## _to_xrgb_8888, mprefix ## _TO_XRGB_8888)

DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_argb_8888, ALLEGRO_CONVERT_ARGB_8888)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_rgba_8888, ALLEGRO_CONVERT_RGBA_8888)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_argb_4444, ALLEGRO_CONVERT_ARGB_4444)
DEFINE_DRAW_REGION(READ3BYTES, _draw_region_memory_rgb_888, ALLEGRO_CONVERT_RGB_888)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_rgb_565, ALLEGRO_CONVERT_RGB_565)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_rgb_555, ALLEGRO_CONVERT_RGB_555)
DEFINE_DRAW_REGION(bmp_read8, _draw_region_memory_palette_8, ALLEGRO_CONVERT_PALETTE_8)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_rgba_5551, ALLEGRO_CONVERT_RGBA_5551)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_argb_1555, ALLEGRO_CONVERT_ARGB_1555)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_abgr_8888, ALLEGRO_CONVERT_ABGR_8888)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_xbgr_8888, ALLEGRO_CONVERT_XBGR_8888)
DEFINE_DRAW_REGION(READ3BYTES, _draw_region_memory_bgr_888, ALLEGRO_CONVERT_BGR_888)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_bgr_565, ALLEGRO_CONVERT_BGR_565)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_bgr_555, ALLEGRO_CONVERT_BGR_555)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_rgbx_8888, ALLEGRO_CONVERT_RGBX_8888)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_xrgb_8888, ALLEGRO_CONVERT_XRGB_8888)

typedef void (*_draw_region_func)(void *src,
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat,
   void *dst,
   int dx, int dy, int dpitch, int dsize, int dformat,
   int flags);

#define DECLARE_DRAW_REGION_FUNCS(prefix) \
   { \
      /* Fake formats */ \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      NULL, \
      /* End fake formats */ \
      prefix ## _to_argb_8888, \
      prefix ## _to_rgba_8888, \
      prefix ## _to_argb_4444, \
      prefix ## _to_rgb_888, \
      prefix ## _to_rgb_565, \
      prefix ## _to_rgb_555, \
      prefix ## _to_palette_8, \
      prefix ## _to_rgba_5551, \
      prefix ## _to_argb_1555, \
      prefix ## _to_abgr_8888, \
      prefix ## _to_xbgr_8888, \
      prefix ## _to_bgr_888, \
      prefix ## _to_bgr_565, \
      prefix ## _to_bgr_555, \
      prefix ## _to_rgbx_8888, \
      prefix ## _to_xrgb_8888 \
   }, \

static _draw_region_func _draw_region_funcs[ALLEGRO_NUM_PIXEL_FORMATS][ALLEGRO_NUM_PIXEL_FORMATS] = {
      /* Fake formats */
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      {
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL,
         NULL
      },
      /* End fake formats */
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_argb_8888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_rgba_8888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_argb_4444)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_rgb_888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_rgb_565)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_rgb_555)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_palette_8)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_rgba_5551)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_argb_1555)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_abgr_8888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_xbgr_8888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_bgr_888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_bgr_565)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_bgr_555)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_rgbx_8888)
      DECLARE_DRAW_REGION_FUNCS(_draw_region_memory_xrgb_8888)
   };


void _al_draw_bitmap_region_memory_fast(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   ALLEGRO_LOCKED_REGION src_region;
   ALLEGRO_LOCKED_REGION dst_region;
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

   /* Lock the bitmaps */
   if (!al_lock_bitmap_region(bitmap, sx, sy, sw, sh, &src_region, ALLEGRO_LOCK_READONLY)) {
      return;
   }
   if (!al_lock_bitmap_region(dest, dx, dy, sw, sh, &dst_region, 0)) {
      al_unlock_bitmap(bitmap);
      return;
   }

   (*_draw_region_funcs[bitmap->format][dest->format])(
      src_region.data, sx, sy, sw, sh,
      src_region.pitch, al_get_pixel_size(bitmap->format), bitmap->format,
      dst_region.data, dx, dy,
      dst_region.pitch, al_get_pixel_size(dest->format), dest->format,
      flags);

   al_unlock_bitmap(bitmap);
   al_unlock_bitmap(dest);
}

void _al_draw_bitmap_memory_fast(ALLEGRO_BITMAP *bitmap,
  int dx, int dy, int flags)
{
   _al_draw_bitmap_region_memory_fast(bitmap, 0, 0, bitmap->w, bitmap->h,
      dx, dy, flags);
}

#endif

