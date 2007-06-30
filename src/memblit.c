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
 *      Original rotation code by Shawn Hargreaves.
 *      Original scaling code by Michael Bukin.
 *      Conversion draw_region by Trent Gamblin.
 *
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/convert.h"
#include <math.h>

/* Memory bitmap blitting functions */


/* draw_region_memory workhorse */
#define DO_DRAW_REGION(src, \
   sx, sy, sw, sh, spitch, ssize, sformat, get, \
   dst, dx, dy, dpitch, dsize, dformat, set, \
   convert, flags) \
{ \
   int x; \
   int y; \
   AL_LOCKED_REGION lr; \
   int cdx, cdy;         /* current dest */ \
   int dxi, dyi;         /* dest increments */ \
   AL_COLOR mask_color; \
   int pixel; \
   int mask_pixel; \
\
   al_get_mask_color(&mask_color); \
   mask_pixel = _al_get_pixel_value(sformat, &mask_color); \
\
   /* Adjust for flipping */ \
\
   if (flags & AL_FLIP_HORIZONTAL) { \
      cdx = sw - 1; \
      dxi = -1; \
   } \
   else { \
      cdx = 0; \
      dxi = 1; \
   } \
\
   if (flags & AL_FLIP_VERTICAL) { \
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
	 /* Skip masked pixels if flag set */ \
	 if ((flags & AL_MASK_SOURCE) && pixel == mask_pixel) { \
	    /* skip masked pixels */ \
	 } \
	 else { \
  	    pixel = convert(pixel); \
            set(dst+cdy*dpitch+cdx*dsize, pixel); \
	 } \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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
   DO_DRAW_REGION(src, \
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

DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_argb_8888, AL_CONVERT_ARGB_8888)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_rgba_8888, AL_CONVERT_RGBA_8888)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_argb_4444, AL_CONVERT_ARGB_4444)
DEFINE_DRAW_REGION(READ3BYTES, _draw_region_memory_rgb_888, AL_CONVERT_RGB_888)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_rgb_565, AL_CONVERT_RGB_565)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_rgb_555, AL_CONVERT_RGB_555)
DEFINE_DRAW_REGION(bmp_read8, _draw_region_memory_palette_8, AL_CONVERT_PALETTE_8)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_rgba_5551, AL_CONVERT_RGBA_5551)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_argb_1555, AL_CONVERT_ARGB_1555)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_abgr_8888, AL_CONVERT_ABGR_8888)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_xbgr_8888, AL_CONVERT_XBGR_8888)
DEFINE_DRAW_REGION(READ3BYTES, _draw_region_memory_bgr_888, AL_CONVERT_BGR_888)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_bgr_565, AL_CONVERT_BGR_565)
DEFINE_DRAW_REGION(bmp_read16, _draw_region_memory_bgr_555, AL_CONVERT_BGR_555)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_rgbx_8888, AL_CONVERT_RGBX_8888)
DEFINE_DRAW_REGION(bmp_read32, _draw_region_memory_xrgb_8888, AL_CONVERT_XRGB_8888)

typedef void (*_draw_region_func)(void *src,
   int sx, int sy, int sw, int sh, int spitch, int ssize, int sformat,
   void *dst,
   int dx, int dy, int dpitch, int dsize, int dformat,
   int flags);

#define DECLARE_DRAW_REGION_FUNCS(prefix) \
	{ \
		NULL, /* ALLEGRO_PIXEL_FORMAT_ANY */ \
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

static _draw_region_func _draw_region_funcs[NUM_PIXEL_FORMATS][NUM_PIXEL_FORMATS] = {
	/* ALLEGRO_PIXEL_FORMAT_ANY */
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


void _al_draw_bitmap_region_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   AL_LOCKED_REGION src_region;
   AL_LOCKED_REGION dst_region;
   AL_BITMAP *dest = al_get_target_bitmap();
   
   /* Do clipping */
   if (dest->clip) {
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
   }

   /* Lock the bitmaps */
   if (!al_lock_bitmap_region(bitmap, sx, sy, sw, sh, &src_region, AL_LOCK_READONLY)) {
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

void _al_draw_bitmap_memory(AL_BITMAP *bitmap,
  int dx, int dy, int flags)
{
   _al_draw_bitmap_region_memory(bitmap, 0, 0, bitmap->w, bitmap->h,
      dx, dy, flags);
}


/* draw_scaled_memory workhorse */
#define DO_DRAW_SCALED(bitmap, \
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
   AL_COLOR mask_color; \
   int mask_pixel; \
   AL_BITMAP *dest = al_get_target_bitmap(); \
   int pixel; \
   AL_LOCKED_REGION src_region; \
   AL_LOCKED_REGION dst_region; \
 \
   if ((sw <= 0) || (sh <= 0) || (dw <= 0) || (dh <= 0)) \
      return; \
 \
   /* Do clipping */ \
   if (dest->clip) { \
      dybeg = ((dy > dest->ct) ? dy : dest->ct); \
      dyend = (((dy + dh) < dest->cb) ? (dy + dh) : dest->cb); \
      if (dybeg >= dyend) \
         return; \
    \
      dxbeg = ((dx > dest->cl) ? dx : dest->cl); \
      dxend = (((dx + dw) < dest->cr) ? (dx + dw) : dest->cr); \
      if (dxbeg >= dxend) \
         return; \
   } \
   else { \
      dxbeg = dx; \
      dybeg = dy; \
      dxend = dx+dw; \
      dyend = dy+dh; \
   } \
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
   al_get_mask_color(&mask_color); \
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
   if (!al_lock_bitmap_region(bitmap, sx, sy, sw, sh, &src_region, AL_LOCK_READONLY)) \
      return; \
 \
   if (!al_lock_bitmap_region(dest, dx, dy, dw, dh, &dst_region, 0)) { \
      al_unlock_bitmap(bitmap); \
      return; \
   } \
 \
   al_get_mask_color(&mask_color); \
   mask_pixel = _al_get_pixel_value(bitmap->format, &mask_color); \
 \
   y = 0; \
   dyend = dyend - dy; \
 \
   if (flags & AL_FLIP_HORIZONTAL) { \
      sx = sw - 1; \
      sxdir = -1; \
   } \
   else { \
      sx = 0; \
      sxdir = 1; \
   } \
   if (flags & AL_FLIP_VERTICAL) { \
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
         pixel = get(src_region.data+src_region.pitch*sy+ssize*_sx); \
	 if (!(flags & AL_MASK_SOURCE) || pixel != mask_pixel) { \
	    pixel = convert(pixel); \
	    set(dst_region.data+dst_region.pitch*y+dsize*x, pixel); \
	 } \
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
static void func1(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro1) \
} \
\
static void func2(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro2) \
} \
\
static void func3(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro3) \
} \
\
static void func4(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      WRITE3BYTES, \
      macro4) \
} \
\
static void func5(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro5) \
} \
\
static void func6(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro6) \
} \
\
static void func7(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write8, \
      macro7) \
} \
\
static void func8(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro8) \
} \
\
static void func9(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro9) \
} \
\
static void func10 (AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro10) \
} \
\
static void func11 (AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro11) \
} \
\
static void func12 (AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      WRITE3BYTES, \
      macro12) \
} \
\
static void func13 (AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro13) \
} \
\
static void func14(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write16, \
      macro14) \
} \
\
static void func15(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
      sx, sy, sw, sh, ssize, \
      dx, dy, dw, dh, dsize, \
      flags, get, \
      bmp_write32, \
      macro15) \
} \
\
static void func16(AL_BITMAP *bitmap, \
	int sx, int sy, int sw, int sh, int ssize, \
	int dx, int dy, int dw, int dh, int dsize, \
	int flags) \
{ \
   DO_DRAW_SCALED(bitmap, \
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

DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_argb_8888, AL_CONVERT_ARGB_8888)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_rgba_8888, AL_CONVERT_RGBA_8888)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_argb_4444, AL_CONVERT_ARGB_4444)
DEFINE_DRAW_SCALED(READ3BYTES, _draw_scaled_memory_rgb_888, AL_CONVERT_RGB_888)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_rgb_565, AL_CONVERT_RGB_565)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_rgb_555, AL_CONVERT_RGB_555)
DEFINE_DRAW_SCALED(bmp_read8, _draw_scaled_memory_palette_8, AL_CONVERT_PALETTE_8)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_rgba_5551, AL_CONVERT_RGBA_5551)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_argb_1555, AL_CONVERT_ARGB_1555)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_abgr_8888, AL_CONVERT_ABGR_8888)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_xbgr_8888, AL_CONVERT_XBGR_8888)
DEFINE_DRAW_SCALED(READ3BYTES, _draw_scaled_memory_bgr_888, AL_CONVERT_BGR_888)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_bgr_565, AL_CONVERT_BGR_565)
DEFINE_DRAW_SCALED(bmp_read16, _draw_scaled_memory_bgr_555, AL_CONVERT_BGR_555)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_rgbx_8888, AL_CONVERT_RGBX_8888)
DEFINE_DRAW_SCALED(bmp_read32, _draw_scaled_memory_xrgb_8888, AL_CONVERT_XRGB_8888)

typedef void (*_draw_scaled_func)(AL_BITMAP *,
	int sx, int sy, int sw, int sh, int ssize,
	int dx, int dy, int dw, int dh, int dsize,
	int flags);

#define DECLARE_DRAW_SCALED_FUNCS(prefix) \
	{ \
		NULL, /* ALLEGRO_PIXEL_FORMAT_ANY */ \
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

static _draw_scaled_func _draw_scaled_funcs[NUM_PIXEL_FORMATS][NUM_PIXEL_FORMATS] = {
	/* ALLEGRO_PIXEL_FORMAT_ANY */
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

void _al_draw_scaled_bitmap_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
{
   int ssize = al_get_pixel_size(bitmap->format);
   AL_BITMAP *dest = al_get_target_bitmap();
   int dsize = al_get_pixel_size(dest->format);

   (*_draw_scaled_funcs[bitmap->format][dest->format])(
   	bitmap, sx, sy, sw, sh, ssize,
	dx, dy, dw, dh, dsize,
	flags);
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
 *  If sub_pixel_accuracy=FALSE, then the scanline drawer will be called with
 *  *_bmp_x being a fixed point representation of the integers representing
 *  the x coordinate of the first and last point in bmp whose centre is
 *  covered by the sprite. If sub_pixel_accuracy=TRUE, then the scanline
 *  drawer will be called with the exact fixed point position of the first
 *  and last point in which the horizontal line passing through the centre is
 *  at least partly covered by the sprite. This is useful for doing
 *  anti-aliased blending.
 */
#define DO_PARALLELOGRAM_MAP(sub_pixel_accuracy, get, set, convert, flags) \
{ \
   /* Index in xs[] and ys[] to topmost point. */ \
   int top_index; \
   /* Rightmost point has index (top_index+right_index) int xs[] and ys[]. */ \
   int right_index; \
   /* Loop variables. */ \
   int index, i; \
   /* Coordinates in bmp ordered as top-right-bottom-left. */ \
   fixed corner_bmp_x[4], corner_bmp_y[4]; \
   /* Coordinates in spr ordered as top-right-bottom-left. */ \
   fixed corner_spr_x[4], corner_spr_y[4]; \
   /* y coordinate of bottom point, left point and right point. */ \
   int clip_bottom_i, l_bmp_y_bottom_i, r_bmp_y_bottom_i; \
   /* Left and right clipping. */ \
   fixed clip_left, clip_right; \
   /* Temporary variable. */ \
   fixed extra_scanline_fraction; \
   /* Top scanline of destination */ \
   int clip_top_i; \
 \
   AL_LOCKED_REGION src_region; \
   AL_LOCKED_REGION dst_region; \
   int ssize; \
   int dsize; \
   AL_COLOR mask_color; \
   int mask_pixel; \
 \
   /* \
    * Variables used in the loop \
    */ \
   /* Coordinates of sprite and bmp points in beginning of scanline. */ \
   fixed l_spr_x, l_spr_y, l_bmp_x, l_bmp_dx; \
   /* Increment of left sprite point as we move a scanline down. */ \
   fixed l_spr_dx, l_spr_dy; \
   /* Coordinates of sprite and bmp points in end of scanline. */ \
   fixed r_bmp_x, r_bmp_dx; \
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/ \
   fixed r_spr_x, r_spr_y; \
   /* Increment of right sprite point as we move a scanline down. */ \
   fixed r_spr_dx, r_spr_dy; \
   /*#endif*/ \
   /* Increment of sprite point as we move right inside a scanline. */ \
   fixed spr_dx, spr_dy; \
   /* Positions of beginning of scanline after rounding to integer coordinate \
      in bmp. */ \
   fixed l_spr_x_rounded, l_spr_y_rounded, l_bmp_x_rounded; \
   fixed r_bmp_x_rounded; \
   /* Current scanline. */ \
   int bmp_y_i; \
   /* Right edge of scanline. */ \
   int right_edge_test; \
 \
   /* Get index of topmost point. */ \
   top_index = 0; \
   if (ys[1] < ys[0]) \
      top_index = 1; \
   if (ys[2] < ys[top_index]) \
      top_index = 2; \
   if (ys[3] < ys[top_index]) \
      top_index = 3; \
 \
   /* Get direction of points: clockwise or anti-clockwise. */ \
   right_index = (double)(xs[(top_index+1) & 3] - xs[top_index]) * \
      (double)(ys[(top_index-1) & 3] - ys[top_index]) > \
      (double)(xs[(top_index-1) & 3] - xs[top_index]) * \
      (double)(ys[(top_index+1) & 3] - ys[top_index]) ? 1 : -1; \
   /*FIXME: why does fixmul overflow below?*/ \
   /*if (fixmul(xs[(top_index+1) & 3] - xs[top_index], \
      ys[(top_index-1) & 3] - ys[top_index]) > \
         fixmul(xs[(top_index-1) & 3] - xs[top_index], \
            ys[(top_index+1) & 3] - ys[top_index])) \
      right_index = 1; \
   else \
      right_index = -1;*/ \
 \
   /* \
    * Get coordinates of the corners. \
    */ \
 \
   /* corner_*[0] is top, [1] is right, [2] is bottom, [3] is left. */ \
   index = top_index; \
   for (i = 0; i < 4; i++) { \
      corner_bmp_x[i] = xs[index]; \
      corner_bmp_y[i] = ys[index]; \
      if (index < 2) \
	 corner_spr_y[i] = 0; \
      else \
	 /* Need `- 1' since otherwise it would be outside sprite. */ \
	 corner_spr_y[i] = (src->h << 16) - 1; \
      if ((index == 0) || (index == 3)) \
	 corner_spr_x[i] = 0; \
      else \
	 corner_spr_x[i] = (src->w << 16) - 1; \
      index = (index + right_index) & 3; \
   } \
 \
   /* Calculate left and right clipping. */ \
   if (dst->clip) { \
      clip_left = dst->cl << 16; \
      clip_right = (dst->cr << 16) - 1; \
   } \
   else { \
      ASSERT(left_bmp_x >= 0 && top_bmp_x >= 0 && bottom_bmp_x >= 0 \
	     && right_bmp_x < (dst->w << 16) \
	     && top_bmp_x < (dst->w << 16) \
	     && bottom_bmp_x < (dst->w << 16)); \
      clip_left = 0; \
      clip_right = (dst->w << 16) - 1; \
   } \
   \
   /* Quit if we're totally outside. */ \
   if ((left_bmp_x > clip_right) && \
       (top_bmp_x > clip_right) && \
       (bottom_bmp_x > clip_right)) \
      return; \
   if ((right_bmp_x < clip_left) && \
       (top_bmp_x < clip_left) && \
       (bottom_bmp_x < clip_left)) \
      return; \
 \
   /* Bottom clipping. */ \
   if (sub_pixel_accuracy) \
      clip_bottom_i = (bottom_bmp_y + 0xffff) >> 16; \
   else \
      clip_bottom_i = (bottom_bmp_y + 0x8000) >> 16; \
 \
   /* Bottom clipping */ \
   if (dst->clip) { \
      if (clip_bottom_i > dst->cb) \
         clip_bottom_i = dst->cb; \
   } \
   else { \
      ASSERT(clip_bottom_i <= dst->h); \
   } \
   \
 \
   /* Calculate y coordinate of first scanline. */ \
   if (sub_pixel_accuracy) \
      bmp_y_i = top_bmp_y >> 16; \
   else \
      bmp_y_i = (top_bmp_y + 0x8000) >> 16; \
    \
   /* Top clipping */ \
   if (dst->clip) { \
      if (bmp_y_i < dst->ct) \
         bmp_y_i = dst->ct; \
   } \
   else { \
      ASSERT(bmp_y_i >= 0); \
   } \
   \
   if (bmp_y_i < 0) \
      bmp_y_i = 0; \
 \
   /* Sprite is above or below bottom clipping area. */ \
   if (bmp_y_i >= clip_bottom_i) \
      return; \
 \
   /* Vertical gap between top corner and centre of topmost scanline. */ \
   extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - top_bmp_y; \
   /* Calculate x coordinate of beginning of scanline in bmp. */ \
   l_bmp_dx = fixdiv(left_bmp_x - top_bmp_x, \
		   left_bmp_y - top_bmp_y); \
   l_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx); \
   /* Calculate x coordinate of beginning of scanline in spr. */ \
   /* note: all these are rounded down which is probably a Good Thing (tm) */ \
   l_spr_dx = fixdiv(left_spr_x - top_spr_x, \
		   left_bmp_y - top_bmp_y); \
   l_spr_x = top_spr_x + fixmul(extra_scanline_fraction, l_spr_dx); \
   /* Calculate y coordinate of beginning of scanline in spr. */ \
   l_spr_dy = fixdiv(left_spr_y - top_spr_y, \
		   left_bmp_y - top_bmp_y); \
   l_spr_y = top_spr_y + fixmul(extra_scanline_fraction, l_spr_dy); \
 \
   /* Calculate left loop bound. */ \
   l_bmp_y_bottom_i = (left_bmp_y + 0x8000) >> 16; \
   if (l_bmp_y_bottom_i > clip_bottom_i) \
      l_bmp_y_bottom_i = clip_bottom_i; \
 \
   /* Calculate x coordinate of end of scanline in bmp. */ \
   r_bmp_dx = fixdiv(right_bmp_x - top_bmp_x, \
		   right_bmp_y - top_bmp_y); \
   r_bmp_x = top_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx); \
   /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/ \
   /* Calculate x coordinate of end of scanline in spr. */ \
   r_spr_dx = fixdiv(right_spr_x - top_spr_x, \
		   right_bmp_y - top_bmp_y); \
   r_spr_x = top_spr_x + fixmul(extra_scanline_fraction, r_spr_dx); \
   /* Calculate y coordinate of end of scanline in spr. */ \
   r_spr_dy = fixdiv(right_spr_y - top_spr_y, \
		   right_bmp_y - top_bmp_y); \
   r_spr_y = top_spr_y + fixmul(extra_scanline_fraction, r_spr_dy); \
   /*#endif*/ \
 \
   /* Calculate right loop bound. */ \
   r_bmp_y_bottom_i = (right_bmp_y + 0x8000) >> 16; \
 \
   /* Get dx and dy, the offsets to add to the source coordinates as we move \
      one pixel rightwards along a scanline. This formula can be derived by \
      considering the 2x2 matrix that transforms the sprite to the \
      parallelogram. \
      We'd better use double to get this as exact as possible, since any \
      errors will be accumulated along the scanline. \
   */ \
   spr_dx = (fixed)((ys[3] - ys[0]) * 65536.0 * (65536.0 * src->w) / \
		    ((xs[1] - xs[0]) * (double)(ys[3] - ys[0]) - \
		     (xs[3] - xs[0]) * (double)(ys[1] - ys[0]))); \
   spr_dy = (fixed)((ys[1] - ys[0]) * 65536.0 * (65536.0 * src->h) / \
		    ((xs[3] - xs[0]) * (double)(ys[1] - ys[0]) - \
		     (xs[1] - xs[0]) * (double)(ys[3] - ys[0]))); \
 \
   /* Lock the bitmaps */ \
   al_lock_bitmap(src, &src_region, AL_LOCK_READONLY); \
   \
   clip_top_i = bmp_y_i; \
   \
   if (!al_lock_bitmap_region(dst, \
         clip_left>>16, \
         clip_top_i, \
         (clip_right>>16)-(clip_left>>16), \
         clip_bottom_i-clip_top_i, \
         &dst_region, 0)) \
      return; \
   \
   al_get_mask_color(&mask_color); \
   mask_pixel = _al_get_pixel_value(src->format, &mask_color); \
 \
   ssize = al_get_pixel_size(src->format); \
   dsize = al_get_pixel_size(dst->format); \
 \
   /* \
    * Loop through scanlines. \
    */ \
 \
   while (1) { \
      /* Has beginning of scanline passed a corner? */ \
      if (bmp_y_i >= l_bmp_y_bottom_i) { \
	 /* Are we done? */ \
	 if (bmp_y_i >= clip_bottom_i) \
	    break; \
 \
	 /* Vertical gap between left corner and centre of scanline. */ \
	 extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - left_bmp_y; \
	 /* Update x coordinate of beginning of scanline in bmp. */ \
	 l_bmp_dx = fixdiv(bottom_bmp_x - left_bmp_x, \
			 bottom_bmp_y - left_bmp_y); \
	 l_bmp_x = left_bmp_x + fixmul(extra_scanline_fraction, l_bmp_dx); \
	 /* Update x coordinate of beginning of scanline in spr. */ \
	 l_spr_dx = fixdiv(bottom_spr_x - left_spr_x, \
			 bottom_bmp_y - left_bmp_y); \
	 l_spr_x = left_spr_x + fixmul(extra_scanline_fraction, l_spr_dx); \
	 /* Update y coordinate of beginning of scanline in spr. */ \
	 l_spr_dy = fixdiv(bottom_spr_y - left_spr_y, \
			 bottom_bmp_y - left_bmp_y); \
	 l_spr_y = left_spr_y + fixmul(extra_scanline_fraction, l_spr_dy); \
 \
	 /* Update loop bound. */ \
	 if (sub_pixel_accuracy) \
	    l_bmp_y_bottom_i = (bottom_bmp_y + 0xffff) >> 16; \
	 else \
	    l_bmp_y_bottom_i = (bottom_bmp_y + 0x8000) >> 16; \
	 if (l_bmp_y_bottom_i > clip_bottom_i) \
	    l_bmp_y_bottom_i = clip_bottom_i; \
      } \
 \
      /* Has end of scanline passed a corner? */ \
      if (bmp_y_i >= r_bmp_y_bottom_i) { \
	 /* Vertical gap between right corner and centre of scanline. */ \
	 extra_scanline_fraction = (bmp_y_i << 16) + 0x8000 - right_bmp_y; \
	 /* Update x coordinate of end of scanline in bmp. */ \
	 r_bmp_dx = fixdiv(bottom_bmp_x - right_bmp_x, \
			 bottom_bmp_y - right_bmp_y); \
	 r_bmp_x = right_bmp_x + fixmul(extra_scanline_fraction, r_bmp_dx); \
	 /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/ \
	 /* Update x coordinate of beginning of scanline in spr. */ \
	 r_spr_dx = fixdiv(bottom_spr_x - right_spr_x, \
			 bottom_bmp_y - right_bmp_y); \
	 r_spr_x = right_spr_x + fixmul(extra_scanline_fraction, r_spr_dx); \
	 /* Update y coordinate of beginning of scanline in spr. */ \
	 r_spr_dy = fixdiv(bottom_spr_y - right_spr_y, \
			 bottom_bmp_y - right_bmp_y); \
	 r_spr_y = right_spr_y + fixmul(extra_scanline_fraction, r_spr_dy); \
	 /*#endif*/ \
 \
	 /* Update loop bound: We aren't supposed to use this any more, so \
	    just set it to some big enough value. */ \
	 r_bmp_y_bottom_i = clip_bottom_i; \
      } \
 \
      /* Make left bmp coordinate be an integer and clip it. */ \
      if (sub_pixel_accuracy) \
	 l_bmp_x_rounded = l_bmp_x; \
      else \
	 l_bmp_x_rounded = (l_bmp_x + 0x8000) & ~0xffff; \
      if (l_bmp_x_rounded < clip_left) \
	 l_bmp_x_rounded = clip_left; \
 \
      /* ... and move starting point in sprite accordingly. */ \
      if (sub_pixel_accuracy) { \
	 l_spr_x_rounded = l_spr_x + \
			   fixmul((l_bmp_x_rounded - l_bmp_x), spr_dx); \
	 l_spr_y_rounded = l_spr_y + \
			   fixmul((l_bmp_x_rounded - l_bmp_x), spr_dy); \
      } \
      else { \
	 l_spr_x_rounded = l_spr_x + \
			   fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dx); \
	 l_spr_y_rounded = l_spr_y + \
			   fixmul(l_bmp_x_rounded + 0x7fff - l_bmp_x, spr_dy); \
      } \
 \
      /* Make right bmp coordinate be an integer and clip it. */ \
      if (sub_pixel_accuracy) \
	 r_bmp_x_rounded = r_bmp_x; \
      else \
	 r_bmp_x_rounded = (r_bmp_x - 0x8000) & ~0xffff; \
      if (r_bmp_x_rounded > clip_right) \
	 r_bmp_x_rounded = clip_right; \
 \
      /* Draw! */ \
      if (l_bmp_x_rounded <= r_bmp_x_rounded) { \
	 if (!sub_pixel_accuracy) { \
	    /* The bodies of these ifs are only reached extremely seldom, \
	       it's an ugly hack to avoid reading outside the sprite when \
	       the rounding errors are accumulated the wrong way. It would \
	       be nicer if we could ensure that this never happens by making \
	       all multiplications and divisions be rounded up or down at \
	       the correct places. \
	       I did try another approach: recalculate the edges of the \
	       scanline from scratch each scanline rather than incrementally. \
	       Drawing a sprite with that routine took about 25% longer time \
	       though. \
	    */ \
	    if ((unsigned)(l_spr_x_rounded >> 16) >= (unsigned)src->w) { \
	       if (((l_spr_x_rounded < 0) && (spr_dx <= 0)) || \
		   ((l_spr_x_rounded > 0) && (spr_dx >= 0))) { \
		  /* This can happen. */ \
		  goto skip_draw; \
	       } \
	       else { \
		  /* I don't think this can happen, but I can't prove it. */ \
		  do { \
		     l_spr_x_rounded += spr_dx; \
		     l_bmp_x_rounded += 65536; \
		     if (l_bmp_x_rounded > r_bmp_x_rounded) \
			goto skip_draw; \
		  } while ((unsigned)(l_spr_x_rounded >> 16) >= \
			   (unsigned)src->w); \
 \
	       } \
	    } \
	    right_edge_test = l_spr_x_rounded + \
			      ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) * \
			      spr_dx; \
	    if ((unsigned)(right_edge_test >> 16) >= (unsigned)src->w) { \
	       if (((right_edge_test < 0) && (spr_dx <= 0)) || \
		   ((right_edge_test > 0) && (spr_dx >= 0))) { \
		  /* This can happen. */ \
		  do { \
		     r_bmp_x_rounded -= 65536; \
		     right_edge_test -= spr_dx; \
		     if (l_bmp_x_rounded > r_bmp_x_rounded) \
			goto skip_draw; \
		  } while ((unsigned)(right_edge_test >> 16) >= \
			   (unsigned)src->w); \
	       } \
	       else { \
		  /* I don't think this can happen, but I can't prove it. */ \
		  goto skip_draw; \
	       } \
	    } \
	    if ((unsigned)(l_spr_y_rounded >> 16) >= (unsigned)src->h) { \
	       if (((l_spr_y_rounded < 0) && (spr_dy <= 0)) || \
		   ((l_spr_y_rounded > 0) && (spr_dy >= 0))) { \
		  /* This can happen. */ \
		  goto skip_draw; \
	       } \
	       else { \
		  /* I don't think this can happen, but I can't prove it. */ \
		  do { \
		     l_spr_y_rounded += spr_dy; \
		     l_bmp_x_rounded += 65536; \
		     if (l_bmp_x_rounded > r_bmp_x_rounded) \
			goto skip_draw; \
		  } while (((unsigned)l_spr_y_rounded >> 16) >= \
			   (unsigned)src->h); \
	       } \
	    } \
	    right_edge_test = l_spr_y_rounded + \
			      ((r_bmp_x_rounded - l_bmp_x_rounded) >> 16) * \
			      spr_dy; \
	    if ((unsigned)(right_edge_test >> 16) >= (unsigned)src->h) { \
	       if (((right_edge_test < 0) && (spr_dy <= 0)) || \
		   ((right_edge_test > 0) && (spr_dy >= 0))) { \
		  /* This can happen. */ \
		  do { \
		     r_bmp_x_rounded -= 65536; \
		     right_edge_test -= spr_dy; \
		     if (l_bmp_x_rounded > r_bmp_x_rounded) \
			goto skip_draw; \
		  } while ((unsigned)(right_edge_test >> 16) >= \
			   (unsigned)src->h); \
	       } \
	       else { \
		  /* I don't think this can happen, but I can't prove it. */ \
		  goto skip_draw; \
	       } \
	    } \
	 } \
 \
   {                                                                  \
      int c;                                                          \
      int x;                                                          \
      unsigned char *addr;                                            \
      unsigned char *end_addr;                                        \
      int my_r_bmp_x_i = r_bmp_x_rounded >> 16;                       \
      int my_l_bmp_x_i = l_bmp_x_rounded >> 16;                       \
      fixed my_l_spr_x = l_spr_x_rounded;                             \
      fixed my_l_spr_y = l_spr_y_rounded;                             \
      addr = dst_region.data+(bmp_y_i-clip_top_i)*dst_region.pitch;   \
      /* adjust for locking offset */                                 \
      addr -= (clip_left >> 16) * dsize;                              \
      end_addr = addr + my_r_bmp_x_i * dsize;                         \
      addr += my_l_bmp_x_i * dsize;                                   \
      for (; addr < end_addr; addr += dsize) {                        \
         c = get(src_region.data+(my_l_spr_y>>16)*src_region.pitch+ssize*(my_l_spr_x>>16)); \
         if (!(flags & AL_MASK_SOURCE) || c != mask_pixel) {          \
	    c = convert(c);                                           \
	    set(addr, c);                                             \
	 } \
	 my_l_spr_x += spr_dx;                                        \
	 my_l_spr_y += spr_dy;                                        \
      }                                                               \
   } \
 \
      } \
      /* I'm not going to apoligize for this label and its gotos: to get \
         rid of it would just make the code look worse. */ \
      skip_draw: \
 \
      /* Jump to next scanline. */ \
      bmp_y_i++; \
      /* Update beginning of scanline. */ \
      l_bmp_x += l_bmp_dx; \
      l_spr_x += l_spr_dx; \
      l_spr_y += l_spr_dy; \
      /* Update end of scanline. */ \
      r_bmp_x += r_bmp_dx; \
      /*#ifdef KEEP_TRACK_OF_RIGHT_SPRITE_SCANLINE*/ \
      r_spr_x += r_spr_dx; \
      r_spr_y += r_spr_dy; \
      /*#endif*/ \
   } \
 \
   al_unlock_bitmap(src); \
   al_unlock_bitmap(dst); \
}


/* _pivot_scaled_sprite_flip:
 *  The most generic routine to which you specify the position with angles,
 *  scales, etc.
 */
#define DO_DRAW_ROTATED_SCALED(src, dst, \
       fl_cx, fl_cy, \
       fl_dx, fl_dy, \
       fl_xscale, fl_yscale, \
       fl_angle, \
       get, set, convert, \
       flags) \
{ \
   fixed xs[4], ys[4]; \
   fixed fix_dx = ftofix(fl_dx); \
   fixed fix_dy = ftofix(fl_dy); \
   fixed fix_cx = ftofix(fl_cx); \
   fixed fix_cy = ftofix(fl_cy); \
   fixed fix_angle = ftofix(fl_angle*256/(M_PI*2)); \
   fixed fix_xscale = ftofix(fl_xscale); \
   fixed fix_yscale = ftofix(fl_yscale); \
 \
   _rotate_scale_flip_coordinates(src->w << 16, src->h << 16, \
		  fix_dx, fix_dy, fix_cx, fix_cy, fix_angle, fix_xscale, fix_yscale, \
		  flags & AL_FLIP_HORIZONTAL, flags & AL_FLIP_VERTICAL, xs, ys); \
 \
   DO_PARALLELOGRAM_MAP(false, get, set, convert, flags) \
}

/* draw_region_memory functions */
#define REAL_DEFINE_DRAW_ROTATED_SCALED(get, \
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
static void func1 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write32, \
      macro1, \
      flags) \
} \
\
static void func2 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write32, \
      macro2, \
      flags) \
} \
\
static void func3 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro3, \
      flags) \
} \
\
static void func4 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      WRITE3BYTES, \
      macro4, \
      flags) \
} \
\
static void func5 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro5, \
      flags) \
} \
\
static void func6 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro6, \
      flags) \
} \
\
static void func7 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write8, \
      macro7, \
      flags) \
} \
\
static void func8 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro8, \
      flags) \
} \
\
static void func9 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro9, \
      flags) \
} \
\
static void func10 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write32, \
      macro10, \
      flags) \
} \
\
static void func11 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write32, \
      macro11, \
      flags) \
} \
\
static void func12 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      WRITE3BYTES, \
      macro12, \
      flags) \
} \
\
static void func13 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro13, \
      flags) \
} \
\
static void func14 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write16, \
      macro14, \
      flags) \
} \
\
static void func15 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write32, \
      macro15, \
      flags) \
} \
\
static void func16 (AL_BITMAP *src, AL_BITMAP *dst, \
   float cx, float cy, float dx, float dy,  float xscale, float yscale, \
   float angle, int flags) \
{ \
   DO_DRAW_ROTATED_SCALED(src, dst, \
      cx, cy, \
      dx, dy, \
      xscale, yscale, \
      angle, \
      get, \
      bmp_write32, \
      macro16, \
      flags) \
}

#define DEFINE_DRAW_ROTATED_SCALED(get, fprefix, mprefix) \
	REAL_DEFINE_DRAW_ROTATED_SCALED(get, \
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

DEFINE_DRAW_ROTATED_SCALED(bmp_read32, _draw_rotated_scaled_memory_argb_8888, AL_CONVERT_ARGB_8888)
DEFINE_DRAW_ROTATED_SCALED(bmp_read32, _draw_rotated_scaled_memory_rgba_8888, AL_CONVERT_RGBA_8888)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_argb_4444, AL_CONVERT_ARGB_4444)
DEFINE_DRAW_ROTATED_SCALED(READ3BYTES, _draw_rotated_scaled_memory_rgb_888, AL_CONVERT_RGB_888)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_rgb_565, AL_CONVERT_RGB_565)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_rgb_555, AL_CONVERT_RGB_555)
DEFINE_DRAW_ROTATED_SCALED(bmp_read8, _draw_rotated_scaled_memory_palette_8, AL_CONVERT_PALETTE_8)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_rgba_5551, AL_CONVERT_RGBA_5551)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_argb_1555, AL_CONVERT_ARGB_1555)
DEFINE_DRAW_ROTATED_SCALED(bmp_read32, _draw_rotated_scaled_memory_abgr_8888, AL_CONVERT_ABGR_8888)
DEFINE_DRAW_ROTATED_SCALED(bmp_read32, _draw_rotated_scaled_memory_xbgr_8888, AL_CONVERT_XBGR_8888)
DEFINE_DRAW_ROTATED_SCALED(READ3BYTES, _draw_rotated_scaled_memory_bgr_888, AL_CONVERT_BGR_888)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_bgr_565, AL_CONVERT_BGR_565)
DEFINE_DRAW_ROTATED_SCALED(bmp_read16, _draw_rotated_scaled_memory_bgr_555, AL_CONVERT_BGR_555)
DEFINE_DRAW_ROTATED_SCALED(bmp_read32, _draw_rotated_scaled_memory_rgbx_8888, AL_CONVERT_RGBX_8888)
DEFINE_DRAW_ROTATED_SCALED(bmp_read32, _draw_rotated_scaled_memory_xrgb_8888, AL_CONVERT_XRGB_8888)

#define DECLARE_DRAW_ROTATED_SCALED_FUNCS(prefix) \
	{ \
		NULL, /* ALLEGRO_PIXEL_FORMAT_ANY */ \
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

typedef void (*_draw_rotated_scaled_func)(AL_BITMAP *src, AL_BITMAP *dst,
   float cx, float cy, float dx, float dy,  float xscale, float yscale,
   float angle, int flags);

static _draw_rotated_scaled_func _draw_rotated_scaled_funcs[NUM_PIXEL_FORMATS][NUM_PIXEL_FORMATS] = {
	/* ALLEGRO_PIXEL_FORMAT_ANY */
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
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_argb_8888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_rgba_8888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_argb_4444)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_rgb_888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_rgb_565)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_rgb_555)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_palette_8)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_rgba_5551)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_argb_1555)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_abgr_8888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_xbgr_8888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_bgr_888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_bgr_565)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_bgr_555)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_rgbx_8888)
	DECLARE_DRAW_ROTATED_SCALED_FUNCS(_draw_rotated_scaled_memory_xrgb_8888)
};


void _al_draw_rotated_scaled_bitmap_memory(AL_BITMAP *bitmap,
	int cx, int cy, int dx, int dy, float xscale, float yscale,
	float angle, int flags)
{
   AL_BITMAP *dest = al_get_target_bitmap();

   (*_draw_rotated_scaled_funcs[bitmap->format][dest->format])(
      bitmap, dest, cx, cy, dx, dy, xscale, yscale, -angle, flags);
}

void _al_draw_rotated_bitmap_memory(AL_BITMAP *bitmap,
	int cx, int cy, int dx, int dy, float angle, int flags)
{
	_al_draw_rotated_scaled_bitmap_memory(bitmap,
		cx, cy, dx, dy, 1.0f, 1.0f, angle, flags);
}
