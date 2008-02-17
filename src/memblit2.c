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
 *      Original scaling code by Michael Bukin.
 *
 */

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/convert.h"
#include <math.h>


#ifndef DEBUGMODE


/* draw_scaled_memory workhorse */
#define DO_DRAW_SCALED_FAST(bitmap, \
	sx, sy, sw, sh, ssize, \
	dx, dy, dw, dh, dsize, \
	flags, get, set, convert) \
{ \
   int x; /* current dst x */ \
   int y; /* current dst y */ \
   int _sx; /* current source x */ \
   int yc; /* y counter */ \
   int syinc; /* amount to increment src y each time */ \
   int ycdec; /* amount to decrement counter by, increase sy when this reaches 0 */ \
   int ycinc; /* amount to increment counter by when it reaches 0 */ \
   int dxbeg, dxend; /* clipping information */ \
   int xcstart; /* x counter start */ \
   int dybeg, dyend; \
   int xcinc, xcdec; \
   int sxinc; \
   int sxdir, sydir; \
   int i, j; \
   ALLEGRO_BITMAP *dest = al_get_target_bitmap(); \
   int pixel; \
   ALLEGRO_LOCKED_REGION src_region; \
   ALLEGRO_LOCKED_REGION dst_region; \
 \
   if ((sw <= 0) || (sh <= 0) || (dw <= 0) || (dh <= 0)) \
      return; \
 \
   /* Do clipping */ \
   dybeg = ((dy > dest->ct) ? dy : dest->ct); \
   dyend = (((dy + dh) < dest->cb) ? (dy + dh) : dest->cb); \
   if (dybeg >= dyend) \
      return; \
 \
   dxbeg = ((dx > dest->cl) ? dx : dest->cl); \
   dxend = (((dx + dw) < dest->cr) ? (dx + dw) : dest->cr); \
   if (dxbeg >= dxend) \
      return; \
 \
   syinc = sh / dh; \
   ycdec = sh - (syinc*dh); \
   ycinc = dh - ycdec; \
   yc = ycinc; \
 \
   sxinc = sw / dw; \
   xcdec = sw - ((sw/dw)*dw); \
   xcinc = dw - xcdec; \
   xcstart = xcinc; \
 \
   /* get start state (clip) */ \
   for (i = 0, j = 0; i < dxbeg-dx; i++/*, j += sxinc*/) { \
      if (xcstart <= 0) { \
         xcstart += xcinc; \
         /*j++;*/ \
      } \
      else \
         xcstart -= xcdec; \
   } \
 \
   /* skip clipped lines */ \
   for (y = dy/*, i = 0*/; y < dybeg; y++/*, i += syinc*/) { \
      if (yc <= 0) { \
	 /*i++;*/ \
	 yc += ycinc; \
      } \
      else \
         yc -= ycdec; \
   } \
 \
   i = (dxbeg-dx) * sw / dw; \
   sx += i; \
   sw = (dxend-dxbeg) * sw / dw; \
   i = (dybeg-dy) * sh / dh; \
   sy += i; \
   sh = (dyend-dybeg) * sh / dh; \
 \
   dx = dxbeg; \
   dy = dybeg; \
   dw = dxend - dxbeg; \
   dh = dyend - dybeg; \
 \
   if (!al_lock_bitmap_region(bitmap, sx, sy, sw, sh, &src_region, ALLEGRO_LOCK_READONLY)) \
      return; \
 \
   if (!al_lock_bitmap_region(dest, dx, dy, dw, dh, &dst_region, 0)) { \
      al_unlock_bitmap(bitmap); \
      return; \
   } \
 \
   y = 0; \
   dyend = dyend - dy; \
 \
   if (flags & ALLEGRO_FLIP_HORIZONTAL) { \
      sx = sw - 1; \
      sxdir = -1; \
   } \
   else { \
      sx = 0; \
      sxdir = 1; \
   } \
   if (flags & ALLEGRO_FLIP_VERTICAL) { \
      sy = sh - 1; \
      syinc = -syinc; \
      sydir = -1; \
   } \
   else { \
      sy = 0; \
      sydir = 1; \
   } \
 \
   /* Stretch it */ \
 \
   for (; y < dyend; y++, sy += syinc) { \
      int xc = xcstart; \
      _sx = sx; \
      for (x = 0; x < (dxend-dxbeg); x++) { \
         pixel = get((void*)(((char*)src_region.data)+src_region.pitch*sy+ssize*_sx)); \
	 pixel = convert(pixel); \
	 set((void*)(((char*)dst_region.data)+dst_region.pitch*y+dsize*x), pixel); \
         if (xc <= 0) { \
	    _sx += sxdir; \
	    xc += xcinc; \
         } \
         else \
	    xc -= xcdec; \
      } \
 \
      if (yc <= 0) { \
	 sy += sydir; \
	 yc += ycinc; \
      } \
      else \
	 yc -= ycdec; \
   } \
 \
   al_unlock_bitmap(bitmap); \
   al_unlock_bitmap(dest); \
}

/* draw_region_memory functions */
#define REAL_DEFINE_DRAW_SCALED(get, \
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
static void func1(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro1) \
} \
\
static void func2(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro2) \
} \
\
static void func3(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro3) \
} \
\
static void func4(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      WRITE3BYTES, \
      macro4) \
} \
\
static void func5(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro5) \
} \
\
static void func6(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro6) \
} \
\
static void func7(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write8, \
      macro7) \
} \
\
static void func8(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro8) \
} \
\
static void func9(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro9) \
} \
\
static void func10 (ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro10) \
} \
\
static void func11 (ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro11) \
} \
\
static void func12 (ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      WRITE3BYTES, \
      macro12) \
} \
\
static void func13 (ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro13) \
} \
\
static void func14(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro14) \
} \
\
static void func15(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro15) \
} \
\
static void func16(ALLEGRO_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED_FAST(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro16) \
}

#define DEFINE_DRAW_SCALED(get, fprefix, mprefix) \
	REAL_DEFINE_DRAW_SCALED(get, \
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

DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_argb_8888, ALLEGRO_CONVERT_ARGB_8888)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_rgba_8888, ALLEGRO_CONVERT_RGBA_8888)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_argb_4444, ALLEGRO_CONVERT_ARGB_4444)
DEFINE_DRAW_SCALED(READ3BYTES, _draw_scaled_memory_rgb_888, ALLEGRO_CONVERT_RGB_888)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_rgb_565, ALLEGRO_CONVERT_RGB_565)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_rgb_555, ALLEGRO_CONVERT_RGB_555)
DEFINE_DRAW_SCALED(bmp_read8, _draw_scaled_memory_palette_8, ALLEGRO_CONVERT_PALETTE_8)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_rgba_5551, ALLEGRO_CONVERT_RGBA_5551)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_argb_1555, ALLEGRO_CONVERT_ARGB_1555)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_abgr_8888, ALLEGRO_CONVERT_ABGR_8888)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_xbgr_8888, ALLEGRO_CONVERT_XBGR_8888)
DEFINE_DRAW_SCALED(READ3BYTES, _draw_scaled_memory_bgr_888, ALLEGRO_CONVERT_BGR_888)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_bgr_565, ALLEGRO_CONVERT_BGR_565)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_bgr_555, ALLEGRO_CONVERT_BGR_555)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_rgbx_8888, ALLEGRO_CONVERT_RGBX_8888)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_xrgb_8888, ALLEGRO_CONVERT_XRGB_8888)

typedef void (*_draw_scaled_func)(ALLEGRO_BITMAP *,
	int sx, int sy, int sw, int sh, int ssize,
	int dx, int dy, int dw, int dh, int dsize,
	int flags);

#define DECLARE_DRAW_SCALED_FUNCS(prefix) \
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

static _draw_scaled_func _draw_scaled_funcs[ALLEGRO_NUM_PIXEL_FORMATS][ALLEGRO_NUM_PIXEL_FORMATS] = {
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
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_argb_8888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_rgba_8888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_argb_4444)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_rgb_888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_rgb_565)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_rgb_555)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_palette_8)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_rgba_5551)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_argb_1555)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_abgr_8888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_xbgr_8888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_bgr_888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_bgr_565)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_bgr_555)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_rgbx_8888)
      DECLARE_DRAW_SCALED_FUNCS(_draw_scaled_memory_xrgb_8888)
   };

void _al_draw_scaled_bitmap_memory_fast(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
{
   int ssize = al_get_pixel_size(bitmap->format);
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   int dsize = al_get_pixel_size(dest->format);

   ASSERT(_al_pixel_format_is_real(bitmap->format));
   ASSERT(_al_pixel_format_is_real(dest->format));

   (*_draw_scaled_funcs[bitmap->format][dest->format])(
   	bitmap, sx, sy, sw, sh, ssize,
	dx, dy, dw, dh, dsize,
	flags);
}

#endif
