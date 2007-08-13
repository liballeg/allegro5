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
 *      Bitmap conversion routines
 *
 *      By Trent Gamblin.
 *
 */

#include "allegro.h"
#include "allegro/bitmap_new.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/convert.h"

/* Copy with conversion */
#define DO_CONVERT(convert, src, stype, ssize, spitch, get, \
			dst, dtype, dsize, dpitch, set, \
			sx, sy, dx, dy, w, h) \
	stype *sstart = (stype *)(src + sy*spitch + sx*ssize); \
	stype *sptr; \
	dtype *dstart = (dtype *)(dst + dy*dpitch + dx*dsize); \
	dtype *dptr; \
	stype *send; \
	int yy; \
	for (yy = 0; yy < h; yy++) { \
		send = (stype *)((unsigned char *)sstart + w*ssize); \
		for (sptr = sstart, dptr = dstart; sptr < send; \
				sptr = (stype *)((unsigned char *)sptr + ssize), \
				dptr = (dtype *)((unsigned char *)dptr + dsize)) { \
			set(dptr, convert(get(sptr))); \
		} \
		sstart = (stype *)((unsigned char *)sstart + spitch); \
		dstart = (dtype *)((unsigned char *)dstart + dpitch); \
	}

/* Conversion functions */

#define REAL_DEFINE_CONVERSION(type, size, get, \
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
static void func1 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro1, \
		src, type, size, src_pitch, get, \
		dst, uint32_t, 4, dst_pitch, bmp_write32, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func2 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro2, \
		src, type, size, src_pitch, get, \
		dst, uint32_t, 4, dst_pitch, bmp_write32, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func3 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro3, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func4 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro4, \
		src, type, size, src_pitch, get, \
		dst, unsigned char, 3, dst_pitch, WRITE3BYTES, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func5 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro5, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func6 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro6, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func7 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro7, \
		src, type, size, src_pitch, get, \
		dst, unsigned char, 4, dst_pitch, bmp_write8, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func8 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro8, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func9 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro9, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func10 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro10, \
		src, type, size, src_pitch, get, \
		dst, uint32_t, 4, dst_pitch, bmp_write32, \
		sx, sy, dx, dy, width, height) \
} \
 \
static void func11 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro11, \
		src, type, size, src_pitch, get, \
		dst, uint32_t, 4, dst_pitch, bmp_write32, \
		sx, sy, dx, dy, width, height) \
} \
static void func12 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro12, \
		src, type, size, src_pitch, get, \
		dst, unsigned char, 3, dst_pitch, WRITE3BYTES, \
		sx, sy, dx, dy, width, height) \
} \
static void func13 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro13, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
static void func14 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro14, \
		src, type, size, src_pitch, get, \
		dst, uint16_t, 2, dst_pitch, bmp_write16, \
		sx, sy, dx, dy, width, height) \
} \
static void func15 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro15, \
		src, type, size, src_pitch, get, \
		dst, uint32_t, 4, dst_pitch, bmp_write32, \
		sx, sy, dx, dy, width, height) \
} \
\
static void func16 ( \
	void *src, int src_format, int src_pitch, \
	void *dst, int dst_format, int dst_pitch, \
	int sx, int sy, int dx, int dy, \
	int width, int height) \
{ \
	DO_CONVERT(macro16, \
		src, type, size, src_pitch, get, \
		dst, uint32_t, 4, dst_pitch, bmp_write32, \
		sx, sy, dx, dy, width, height) \
} \


#define DEFINE_CONVERSION(type, size, get, fprefix, mprefix) \
	REAL_DEFINE_CONVERSION(type, size, get, \
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

DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _argb_8888, ALLEGRO_CONVERT_ARGB_8888)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _rgba_8888, ALLEGRO_CONVERT_RGBA_8888)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _argb_4444, ALLEGRO_CONVERT_ARGB_4444)
DEFINE_CONVERSION(unsigned char, 3, READ3BYTES, _rgb_888, ALLEGRO_CONVERT_RGB_888)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _rgb_565, ALLEGRO_CONVERT_RGB_565)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _rgb_555, ALLEGRO_CONVERT_RGB_555)
DEFINE_CONVERSION(unsigned char, 1, bmp_read8, _palette_8, ALLEGRO_CONVERT_PALETTE_8)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _rgba_5551, ALLEGRO_CONVERT_RGBA_5551)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _argb_1555, ALLEGRO_CONVERT_ARGB_1555)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _abgr_8888, ALLEGRO_CONVERT_ABGR_8888)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _xbgr_8888, ALLEGRO_CONVERT_XBGR_8888)
DEFINE_CONVERSION(unsigned char, 3, READ3BYTES, _bgr_888, ALLEGRO_CONVERT_BGR_888)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _bgr_565, ALLEGRO_CONVERT_BGR_565)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _bgr_555, ALLEGRO_CONVERT_BGR_555)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _rgbx_8888, ALLEGRO_CONVERT_RGBX_8888)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _xrgb_8888, ALLEGRO_CONVERT_XRGB_8888)

/* Conversion map */

typedef void (*p_convert_func)(void *, int, int,
	void *, int, int,
	int, int, int, int, int, int);

#define DECLARE_FUNCS(prefix) \
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

static p_convert_func convert_funcs[NUM_PIXEL_FORMATS][NUM_PIXEL_FORMATS] = {
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
	DECLARE_FUNCS(_argb_8888)
	DECLARE_FUNCS(_rgba_8888)
	DECLARE_FUNCS(_argb_4444)
	DECLARE_FUNCS(_rgb_888)
	DECLARE_FUNCS(_rgb_565)
	DECLARE_FUNCS(_rgb_555)
	DECLARE_FUNCS(_palette_8)
	DECLARE_FUNCS(_rgba_5551)
	DECLARE_FUNCS(_argb_1555)
	DECLARE_FUNCS(_abgr_8888)
	DECLARE_FUNCS(_xbgr_8888)
	DECLARE_FUNCS(_bgr_888)
	DECLARE_FUNCS(_bgr_565)
	DECLARE_FUNCS(_bgr_555)
	DECLARE_FUNCS(_rgbx_8888)
	DECLARE_FUNCS(_xrgb_8888)
};


void _al_convert_bitmap_data(
	void *src, int src_format, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height)
{
	ASSERT(src_format != ALLEGRO_PIXEL_FORMAT_ANY);
	ASSERT(dst_format != ALLEGRO_PIXEL_FORMAT_ANY);

	(*convert_funcs[src_format][dst_format])(src, src_format, src_pitch,
		dst, dst_format, dst_pitch, sx, sy, dx, dy, width, height);
}


void _al_convert_compat_bitmap(
	BITMAP *src,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height)
{
	int src_format;

	ASSERT(dst_format != ALLEGRO_PIXEL_FORMAT_ANY);

	switch (bitmap_color_depth(src)) {
		case 8:
			src_format = ALLEGRO_PIXEL_FORMAT_PALETTE_8;
			break;
		case 15:
			if (_rgb_r_shift_15 > _rgb_b_shift_15)
				src_format = ALLEGRO_PIXEL_FORMAT_RGB_555;
			else
				src_format = ALLEGRO_PIXEL_FORMAT_BGR_555;
			break;
		case 16:
			if (_rgb_r_shift_16 > _rgb_b_shift_16)
				src_format = ALLEGRO_PIXEL_FORMAT_RGB_565;
			else
				src_format = ALLEGRO_PIXEL_FORMAT_BGR_565;
			break;
		case 24:
			if (_rgb_r_shift_24 > _rgb_b_shift_24)
				src_format = ALLEGRO_PIXEL_FORMAT_RGB_888;
			else
				src_format = ALLEGRO_PIXEL_FORMAT_BGR_888;
			break;
		case 32:
			if (_rgb_r_shift_32 > _rgb_b_shift_32) {
				if (_bitmap_has_alpha(src))
					src_format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
				else
					src_format = ALLEGRO_PIXEL_FORMAT_RGBX_8888;
			}
			else {
				if (_bitmap_has_alpha(src))
					src_format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
				else
					src_format = ALLEGRO_PIXEL_FORMAT_XBGR_8888;
			}
			break;
		default:
			TRACE("src has unsupported pixel format in _al_convert_compat_bitmap.\n");
			return;
	}

	(*convert_funcs[src_format][dst_format])(src->dat,
		src_format, al_get_pixel_size(src_format)*src->w,
		dst, dst_format, dst_pitch, sx, sy, dx, dy, width, height);
}

