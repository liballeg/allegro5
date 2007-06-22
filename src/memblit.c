#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_bitmap.h"
#include "allegro/convert.h"

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
   /* FIXME: do clipping */ \
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
	func15, macro15) \
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
	fprefix ## _to_rgbx_8888, mprefix ## _TO_RGBX_8888)

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
		prefix ## _to_rgbx_8888 \
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
};


void _al_draw_bitmap_region_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags)
{
   AL_LOCKED_REGION src_region;
   AL_LOCKED_REGION dst_region;
   AL_BITMAP *dest = al_get_target_bitmap();

   /* Lock the bitmaps */
   if (!al_lock_bitmap_region(bitmap, sx, sy, sw, sh, &src_region, AL_LOCK_READONLY)) {
      return;
   }
   if (!al_lock_bitmap_region(dest, sx, sy, sw, sh, &dst_region, 0)) {
      al_unlock_bitmap(bitmap);
      return;
   }

   (*_draw_region_funcs[bitmap->format][dest->format])(
      src_region.data, sx, sy, sw, sh,
      src_region.pitch, _al_pixel_size(bitmap->format), bitmap->format,
      dst_region.data, dx, dy,
      dst_region.pitch, _al_pixel_size(dest->format), dest->format,
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
   int dybeg, dyend; \
   int xcinc, xcdec; \
   int xcstart; \
   int sxinc; \
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
   /* \
   if (dst->clip) { \
      dybeg = ((dy > dst->ct) ? dy : dst->ct); \
      dyend = (((dy + dh) < dst->cb) ? (dy + dh) : dst->cb); \
      if (dybeg >= dyend) \
	 return; \
 \
      dxbeg = ((dx > dst->cl) ? dx : dst->cl); \
      dxend = (((dx + dw) < dst->cr) ? (dx + dw) : dst->cr); \
      if (dxbeg >= dxend) \
	 return; \
   } \
   else { \
      dxbeg = dx; \
      dxend = dx + dw; \
      dybeg = dy; \
      dyend = dy + dh; \
   } \
   */ \
    \
   /* FIXME: use clipping rectangle */ \
 \
   dybeg = ((dy > 0) ? dy : 0); \
   dyend = (((dy + dh) < dest->h) ? (dy + dh) : dest->h); \
   if (dybeg >= dyend) \
      return; \
 \
   dxbeg = ((dx > 0) ? dx : 0); \
   dxend = (((dx + dw) < dest->w) ? (dx + dw) : dest->w); \
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
 \
   /* get start state (clip) */ \
   xcstart = xcinc; \
   for (i = 0, j = 0; i < dxbeg-dx; i++, j += sxinc) { \
      if (xcstart <= 0) { \
	 xcstart += xcinc; \
	 j++; \
      } \
      else \
	 xcstart -= xcdec; \
   } \
 \
   sx += j; \
   sw -= j; \
   dxbeg += i; \
 \
   /* skip clipped lines */ \
   for (y = dy, i = 0; y < dybeg; y++, i += syinc) { \
      if (yc <= 0) { \
	 i++; \
	 yc += ycinc; \
      } \
      else \
         yc -= ycdec; \
   } \
 \
   sy += i; \
   sh -= i; \
 \
   al_get_mask_color(&mask_color); \
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
   dxbeg = 0; \
   dyend = dyend - dy; \
 \
   sx = 0; \
   sy = 0; \
 \
   /* Stretch it */ \
 \
   for (; y < dyend; y++, sy += syinc) { \
      int xc = xcstart; \
      _sx = sx; \
      for (x = dxbeg; x < dxend; x++) { \
         pixel = get(src_region.data+src_region.pitch*sy+ssize*_sx); \
	 if (!(flags & AL_MASK_SOURCE) || pixel != mask_pixel) { \
	    pixel = convert(pixel); \
	    set(dst_region.data+dst_region.pitch*y+dsize*x, pixel); \
	 } \
         /* \
         al_get_pixel(bitmap, _sx, sy, &src_pixel); \
	 if (!(flags & AL_MASK_SOURCE) || \
	       memcmp(&src_pixel, &mask_color, sizeof(AL_COLOR)) != 0) { \
	    al_unmap_rgba_i(bitmap, &src_pixel, &r, &g, &b, &a); \
            al_map_rgba_i(dest, &dst_pixel, r, g, b, a); \
	    al_put_pixel(x, y, &dst_pixel); \
	 } \
	 */ \
         if (xc <= 0) { \
	    _sx++; \
	    xc += xcinc; \
         } \
         else \
	    xc -= xcdec; \
      } \
 \
      if (yc <= 0) { \
	 sy++; \
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
	func15, macro15) \
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
	fprefix ## _to_rgbx_8888, mprefix ## _TO_RGBX_8888)

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
		prefix ## _to_rgbx_8888 \
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
};

void _al_draw_scaled_bitmap_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int dw, int dh, int flags)
{
   int ssize = _al_pixel_size(bitmap->format);
   AL_BITMAP *dest = al_get_target_bitmap();
   int dsize = _al_pixel_size(dest->format);

   (*_draw_scaled_funcs[bitmap->format][dest->format])(
   	bitmap, sx, sy, sw, sh, ssize,
	dx, dy, dw, dh, dsize,
	flags);
}

