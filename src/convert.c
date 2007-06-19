#include "allegro.h"
#include "allegro/bitmap_new.h"
#include "internal/aintern.h"
#include "internal/aintern_bitmap.h"


/* Conversionf for most pixels */
#define SHIFT_CONVERT(p, \
		a_mask, a_lshift, a_rshift, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((((p & a_mask) >> a_rshift) << a_lshift) | \
	 (((p & r_mask) >> r_rshift) << r_lshift) | \
	 (((p & g_mask) >> g_rshift) << g_lshift) | \
	 (((p & b_mask) >> b_rshift) << b_lshift))


/* Left shift conversion */
#define LS_CONVERT(p, \
		a_mask, a_lshift, \
		r_mask, r_lshift, \
		g_mask, g_lshift, \
		b_mask, b_lshift) \
	SHIFT_CONVERT(p, \
		a_mask, a_lshift, 0, \
		r_mask, r_lshift, 0, \
		g_mask, g_lshift, 0, \
		b_mask, b_lshift, 0)


/* Right shift conversion */
#define RS_CONVERT(p, \
		a_mask, a_rshift, \
		r_mask, r_rshift, \
		g_mask, g_rshift, \
		b_mask, b_rshift) \
	SHIFT_CONVERT(p, \
		a_mask, 0, a_rshift, \
		r_mask, 0, r_rshift, \
		g_mask, 0, g_rshift, \
		b_mask, 0, b_rshift)


/* ARGB_8888 */

#define AL_CONVERT_ARGB_8888_TO_ARGB_8888(p) (p)

#define AL_CONVERT_ARGB_8888_TO_RGBA_8888(p) \
	(((p & 0x00FFFFFF) << 8) | \
	 ((p & 0xFF000000) >> 24))

#define AL_CONVERT_ARGB_8888_TO_ARGB_4444(p) \
	RS_CONVERT(p, \
		0xF0000000, 16, \
		0x00F00000, 12, \
		0x0000F000, 8, \
		0x000000F0, 4)

#define AL_CONVERT_ARGB_8888_TO_RGB_888(p) \
	(p & 0xFFFFFF)

#define AL_CONVERT_ARGB_8888_TO_RGB_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 8, \
		0x0000FC00, 5, \
		0x000000F8, 3)

#define AL_CONVERT_ARGB_8888_TO_RGB_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 9, \
		0x0000F800, 6, \
		0x000000F8, 3)

#define AL_CONVERT_ARGB_8888_TO_PALETTE_8(p) \
	makecol8(((p & 0x00FF0000) >> 16) & 0xFF, \
		 ((p & 0x0000FF00) >> 8) & 0xFF, \
		  (p & 0x000000FF))

#define AL_CONVERT_ARGB_8888_TO_RGBA_5551(p) \
	RS_CONVERT(p, \
		0x80000000, 31, \
		0x00F80000, 8, \
		0x0000F800, 5, \
		0x000000F8, 2)

#define AL_CONVERT_ARGB_8888_TO_ARGB_1555(p) \
	RS_CONVERT(p, \
		0x80000000, 16, \
		0x00F80000, 9, \
		0x0000F800, 6, \
		0x000000F8, 3)

#define AL_CONVERT_ARGB_8888_TO_ABGR_8888(p) \
	SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0, \
		0x000000FF, 24, 0)

#define AL_CONVERT_ARGB_8888_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0, \
		0x000000FF, 24, 0)

#define AL_CONVERT_ARGB_8888_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 0, 0)

#define AL_CONVERT_ARGB_8888_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F80000, 0, 19, \
		0x0000FC00, 0, 5, \
		0x000000F8, 8, 0)

#define AL_CONVERT_ARGB_8888_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F80000, 0, 19, \
		0x0000F800, 0, 6, \
		0x000000F8, 7, 0)

#define AL_CONVERT_ARGB_8888_TO_RGBX_8888(p) \
	((p & 0x00FFFFFF) << 8)

/* RGBA_8888 */

#define AL_CONVERT_RGBA_8888_TO_RGBA_8888(p) (p)

#define AL_CONVERT_RGBA_8888_TO_ARGB_8888(p) \
	(((p & 0xFFFFFF00) >> 8) | \
	 ((p & 0xFF) << 24))

#define AL_CONVERT_RGBA_8888_TO_ARGB_4444(p) \
	SHIFT_CONVERT(p, \
		0x000000F0, 8, 0, \
		0xF0000000, 0, 20, \
		0x00F00000, 0, 16, \
		0x0000F000, 0, 12)

#define AL_CONVERT_RGBA_8888_TO_RGB_888(p) \
	(p >> 8)

#define AL_CONVERT_RGBA_8888_TO_RGB_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 16, \
		0x00FC0000, 13, \
		0x0000F800, 11)

#define AL_CONVERT_RGBA_8888_TO_RGB_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 17, \
		0x00F80000, 14, \
		0x0000F800, 11)

#define AL_CONVERT_RGBA_8888_TO_PALETTE_8(p) \
	makecol8((p & 0xFF000000) >> 24, \
		 (p & 0x00FF0000) >> 16, \
		 (p & 0x0000FF00) >> 8)

#define AL_CONVERT_RGBA_8888_TO_RGBA_5551(p) \
	RS_CONVERT(p, \
		0xFF, 7, \
		0xF8000000, 16, \
		0x00F80000, 13, \
		0x0000F800, 10)

#define AL_CONVERT_RGBA_8888_TO_ARGB_1555(p) \
	SHIFT_CONVERT(p, \
		0x80, 12, 0, \
		0xF8000000, 0, 17, \
		0x00F80000, 0, 14, \
		0x0000F800, 0, 11)

#define AL_CONVERT_RGBA_8888_TO_ABGR_8888(p) \
	SHIFT_CONVERT(p, \
		0xFF, 24, 0, \
		0xFF000000, 0, 16, \
		0x00FF0000, 0, 0, \
		0x0000FF00, 16, 0)

#define AL_CONVERT_RGBA_8888_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 16, \
		0x00FF0000, 0, 0, \
		0x0000FF00, 16, 0)

#define AL_CONVERT_RGBA_8888_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0)

#define AL_CONVERT_RGBA_8888_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 27, \
		0x00FF0000, 0, 13, \
		0x0000FF00, 0, 0)

#define AL_CONVERT_RGBA_8888_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 27, \
		0x00FF0000, 0, 14, \
		0x0000FF00, 0, 1)

#define AL_CONVERT_RGBA_8888_TO_RGBX_8888(p) \
	(p & 0xFFFFFF00)

/* ARGB_4444 */

#define AL_CONVERT_ARGB_4444_TO_ARGB_4444(p) (p)

#define AL_CONVERT_ARGB_4444_TO_ARGB_8888(p) \
	LS_CONVERT(p, \
		0xF000, 16, \
		0x0F00, 12, \
		0x00F0, 8, \
		0x000F, 4)

#define AL_CONVERT_ARGB_4444_TO_RGBA_8888(p) \
	SHIFT_CONVERT(p, \
		0xF000, 0, 8, \
		0x0F00, 20, 0, \
		0x00F0, 16, 0, \
		0x000F, 12, 0)

#define AL_CONVERT_ARGB_4444_TO_RGB_888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x0F00, 12, \
		0x00F0, 8, \
		0x000F, 4)

#define AL_CONVERT_ARGB_4444_TO_RGB_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x0F00, 4, \
		0x00F0, 3, \
		0x000F, 1)

#define AL_CONVERT_ARGB_4444_TO_RGB_555(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x0F00, 3, \
		0x00F0, 2, \
		0x000F, 1)

#define AL_CONVERT_ARGB_4444_TO_PALETTE_8(p) \
	makecol8((p & 0x0F00) >> 4, \
		 (p & 0x00F0), \
		 (p & 0x000F) << 4)

#define AL_CONVERT_ARGB_4444_TO_RGBA_5551(p) \
	SHIFT_CONVERT(p, \
		0x8000, 0, 15, \
		0x0F00, 4, 0, \
		0x00F0, 3, 0, \
		0x000F, 2, 0)

#define AL_CONVERT_ARGB_4444_TO_ARGB_1555(p) \
	LS_CONVERT(p, \
		0x8000, 0, /* alpha can stay */ \
		0x0F00, 3, \
		0x00F0, 2, \
		0x000F, 1)

#define AL_CONVERT_ARGB_4444_TO_ABGR_8888(p) \
	SHIFT_CONVERT(p, \
		0xF000, 16, 0, \
		0x0F00, 4, 0, \
		0x00F0, 16, 0, \
		0x000F, 28, 0)

#define AL_CONVERT_ARGB_4444_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 4, 0, \
		0x00F0, 16, 0, \
		0x000F, 28, 0)


#define AL_CONVERT_ARGB_4444_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 0, 4, \
		0x00F0, 8, 0, \
		0x000F, 20, 0)

#define AL_CONVERT_ARGB_4444_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 0, 7, \
		0x00F0, 3, 0, \
		0x000F, 12, 0)

#define AL_CONVERT_ARGB_4444_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 0, 7, \
		0x00F0, 2, 0, \
		0x000F, 11, 0)

#define AL_CONVERT_ARGB_4444_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 20, 0, \
		0x00F0, 16, 0, \
		0x000F, 12, 0)

/* RGB_888 */

#define AL_CONVERT_RGB_888_TO_RGB_888(p) (p)

#define AL_CONVERT_RGB_888_TO_ARGB_8888(p) \
	(p | 0xFF000000)

#define AL_CONVERT_RGB_888_TO_RGBA_8888(p) \
	((p << 8) | 0xFF)

#define AL_CONVERT_RGB_888_TO_ARGB_4444(p) \
	(0xF000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF00000, 12, \
		0x00F000, 8, \
		0x0000F0, 4))

#define AL_CONVERT_RGB_888_TO_RGB_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF80000, 8, \
		0x00FC00, 5, \
		0x0000F8, 3)

#define AL_CONVERT_RGB_888_TO_RGB_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF80000, 9, \
		0x00F800, 6, \
		0x0000F8, 3)

#define AL_CONVERT_RGB_888_TO_PALETTE_8(p) \
	makecol8((p & 0xFF0000) >> 16, \
		 (p & 0x00FF00) >> 8, \
		 (p & 0x0000FF))

#define AL_CONVERT_RGB_888_TO_RGBA_5551(p) \
	(1 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF80000, 8, \
		0x00F800, 5, \
		0x0000F8, 2))

#define AL_CONVERT_RGB_888_TO_ARGB_1555(p) \
	(0x8000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF80000, 9, \
		0x00F800, 6, \
		0x0000F8, 3))

#define AL_CONVERT_RGB_888_TO_ABGR_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 8, \
		0x00FF00, 8, 0, \
		0x0000FF, 24, 0))

#define AL_CONVERT_RGB_888_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 8, \
		0x00FF00, 8, 0, \
		0x0000FF, 24, 0)

#define AL_CONVERT_RGB_888_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 16, \
		0x00FF00, 0, 0, \
		0x0000FF, 16, 0)

#define AL_CONVERT_RGB_888_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 19, \
		0x00FF00, 0, 5, \
		0x0000FF, 8, 0)

#define AL_CONVERT_RGB_888_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 19, \
		0x00FF00, 0, 6, \
		0x0000FF, 7, 0)

#define AL_CONVERT_RGB_888_TO_RGBX_8888(p) \
	(p << 8)

/* RGB_565 */

#define AL_CONVERT_RGB_565_TO_RGB_565(p) (p)

#define AL_CONVERT_RGB_565_TO_ARGB_8888(p) \
	(0xFF000000 | \
		LS_CONVERT(p, \
		0, 0, \
		0xF800, 8, \
		0x07E0, 5, \
		0x001F, 3))

#define AL_CONVERT_RGB_565_TO_RGBA_8888(p) \
	(0xFF | \
		LS_CONVERT(p, \
		0, 0, \
		0xF800, 16, \
		0x07E0, 13, \
		0x001F, 11))

#define AL_CONVERT_RGB_565_TO_ARGB_4444(p) \
	(0xF000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF000, 4, \
		0x0780, 3, \
		0x001E, 1))

#define AL_CONVERT_RGB_565_TO_RGB_888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0xF800, 8, \
		0x07E0, 5, \
		0x001F, 3)

#define AL_CONVERT_RGB_565_TO_RGB_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF800, 1, \
		0x07E0, 1, \
		0x001F, 0)

#define AL_CONVERT_RGB_565_TO_PALETTE_8(p) \
	makecol8((p & 0xF800) >> 8, \
		 (p & 0x07E0) >> 3, \
		 (p & 0x001F) << 3)

#define AL_CONVERT_RGB_565_TO_RGBA_5551(p) \
	(1 | \
		LS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07E0, 0, \
		0x001F, 1))

#define AL_CONVERT_RGB_565_TO_ARGB_1555(p) \
	(0x8000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF800, 1, \
		0x07E0, 1, \
		0x001F, 0))

#define AL_CONVERT_RGB_565_TO_ABGR_8888(p) \
	(0xFF000000 | \
		LS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07E0, 13, \
		0x001F, 27))

#define AL_CONVERT_RGB_565_TO_XBGR_8888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07E0, 13, \
		0x001F, 27)

#define AL_CONVERT_RGB_565_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 8, \
		0x07E0, 5, 0, \
		0x001F, 19, 0)

#define AL_CONVERT_RGB_565_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07E0, 0, 0, \
		0x001F, 11, 0)

#define AL_CONVERT_RGB_565_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07E0, 0, 1, \
		0x001F, 10, 0)

#define AL_CONVERT_RGB_565_TO_RGBX_8888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0xF800, 16, \
		0x07E0, 13, \
		0x001F, 11)

/* RGB_555 */

#define AL_CONVERT_RGB_555_TO_RGB_555(p) (p)

#define AL_CONVERT_RGB_555_TO_ARGB_8888(p) \
	(0xFF000000 | \
		LS_CONVERT(p, \
		0, 0, \
		0x7C00, 9, \
		0x03E0, 6, \
		0x001F, 3))

#define AL_CONVERT_RGB_555_TO_RGBA_8888(p) \
	(0xFF | \
		LS_CONVERT(p, \
		0, 0, \
		0x7C00, 17, \
		0x03E0, 14, \
		0x001F, 11))

#define AL_CONVERT_RGB_555_TO_ARGB_4444(p) \
	(0xF000 | \
		RS_CONVERT(p, \
		0, 0, \
		0x7C00, 3, \
		0x03E0, 2, \
		0x001F, 1))

#define AL_CONVERT_RGB_555_TO_RGB_888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 9, \
		0x03E0, 6, \
		0x001F, 3)

#define AL_CONVERT_RGB_555_TO_RGB_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 1, \
		0x001F, 0)

#define AL_CONVERT_RGB_555_TO_PALETTE_8(p) \
	makecol8((p & 0x7C00) >> 7, \
		 (p & 0x03E0) >> 2, \
		 (p & 0x001F) << 3)

#define AL_CONVERT_RGB_555_TO_RGBA_5551(p) \
	((p << 1) | 1)

#define AL_CONVERT_RGB_555_TO_ARGB_1555(p) \
	(p | 0x8000)

#define AL_CONVERT_RGB_555_TO_ABGR_8888(p) \
	(0xFF000000 | \
		LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 14, \
		0x001F, 27))

#define AL_CONVERT_RGB_555_TO_XBGR_8888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 14, \
		0x001F, 27)

#define AL_CONVERT_RGB_555_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 7, \
		0x03E0, 6, 0, \
		0x001F, 19, 0)

#define AL_CONVERT_RGB_555_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 1, 0, \
		0x001F, 11, 0)

#define AL_CONVERT_RGB_555_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 0, 0, \
		0x001F, 10, 0)

#define AL_CONVERT_RGB_555_TO_RGBX_8888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 17, \
		0x03E0, 14, \
		0x001F, 11)

/* PALETTE_8 */

#define AL_CONVERT_PALETTE_8_TO_PALETTE_8(p) (p)

#define AL_CONVERT_PALETTE_8_TO_ARGB_8888(p) \
	((getr8(p) << 16) | \
	 (getg8(p) << 8) | \
	 getb8(p) | 0xFF000000)

#define AL_CONVERT_PALETTE_8_TO_RGBA_8888(p) \
	((getr8(p) << 24) | \
	 (getg8(p) << 16) | \
	 (getb8(p) << 8) | 0xFF)

#define AL_CONVERT_PALETTE_8_TO_ARGB_4444(p) \
	(0xF000 | \
		((getr8(p) & 0xF0) << 12) | \
		(getg8(p) & 0xF0) | \
		((getb8(p) & 0xF0) >> 4))

#define AL_CONVERT_PALETTE_8_TO_RGB_888(p) \
	((getr8(p) << 16) | \
	 (getg8(p) << 8) | \
	 getb8(p))

#define AL_CONVERT_PALETTE_8_TO_RGB_565(p) \
	(((getr8(p) & 0xF8) << 8) | \
	 ((getg8(p) & 0xFC) << 3) | \
	 ((getb8(p) & 0xF8) >> 3))

#define AL_CONVERT_PALETTE_8_TO_RGB_555(p) \
	(((getr8(p) & 0xF8) << 7) | \
	 ((getg8(p) & 0xF8) << 2) | \
	 ((getb8(p) & 0xF8) >> 3))

#define AL_CONVERT_PALETTE_8_TO_RGBA_5551(p) \
	(1 | \
	((getr8(p) & 0xF8) << 8) | \
	((getg8(p) & 0xF8) << 3) | \
	((getb8(p) & 0xF8) >> 2))

#define AL_CONVERT_PALETTE_8_TO_ARGB_1555(p) \
	(0x8000 | \
	((getr8(p) & 0xF8) << 7) | \
	((getg8(p) & 0xF8) << 2) | \
	((getb8(p) & 0xF8) >> 3))

#define AL_CONVERT_PALETTE_8_TO_ABGR_8888(p) \
	(0xFF000000 | \
	(getr8(p) << 8) | \
	(getg8(p) << 16) | \
	(getb8(p) << 24))

#define AL_CONVERT_PALETTE_8_TO_XBGR_8888(p) \
	(getr8(p) | \
	(getg8(p) << 8) | \
	(getb8(p) << 16))

#define AL_CONVERT_PALETTE_8_TO_BGR_888(p) \
	(getr(p) | \
		(getg(p) << 8) | \
		(getb(p) << 16))

#define AL_CONVERT_PALETTE_8_TO_BGR_565(p) \
	(((getr(p) & 0xF8) >> 3) | \
		((getg(p) & 0xFC) << 3) | \
		((getb(p) & 0xF8) << 8))

#define AL_CONVERT_PALETTE_8_TO_BGR_555(p) \
	(((getr(p) & 0xF8) >> 3) | \
		((getg(p) & 0xF8) << 2) | \
		((getb(p) & 0xF8) << 7))

#define AL_CONVERT_PALETTE_8_TO_RGBX_8888(p) \
	((getr8(p) << 24) | \
	 (getg8(p) << 16) | \
	 (getb8(p) << 8))

/* RGBA_5551 */

#define AL_CONVERT_RGBA_5551_TO_RGBA_5551(p) (p)

#define AL_CONVERT_RGBA_5551_TO_ARGB_8888(p) \
	LS_CONVERT(p, \
		0x1, 31, \
		0xF800, 8, \
		0x07C0, 5, \
		0x003E, 2)

#define AL_CONVERT_RGBA_5551_TO_RGBA_8888(p) \
	LS_CONVERT(p, \
		0x1, 7, \
		0xF800, 16, \
		0x07C0, 13, \
		0x003E, 10)

#define AL_CONVERT_RGBA_5551_TO_ARGB_4444(p) \
	SHIFT_CONVERT(p, \
		0x1, 15, 0, \
		0xF000, 0, 4, \
		0x0780, 0, 3, \
		0x003C, 0, 2)

#define AL_CONVERT_RGBA_5551_TO_RGB_888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0xF800, 8, \
		0x07C0, 5, \
		0x003E, 2)

#define AL_CONVERT_RGBA_5551_TO_RGB_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07C0, 0, \
		0x003E, 1)

#define AL_CONVERT_RGBA_5551_TO_RGB_555(p) \
	(p >> 1)

#define AL_CONVERT_RGBA_5551_TO_PALETTE_8(p) \
	makecol8((p & 0xF800) >> 8, \
		 (p & 0x07C0) >> 3, \
		 (p & 0x003E) << 2)

#define AL_CONVERT_RGBA_5551_TO_ARGB_1555(p) \
	(((p & 1) << 15) | (p >> 1))

#define AL_CONVERT_RGBA_5551_TO_ABGR_8888(p) \
	LS_CONVERT(p, \
		0x1, 31, \
		0xF800, 0, \
		0x07C0, 8, \
		0x003E, 26)

#define AL_CONVERT_RGBA_5551_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 8, \
		0x07C0, 5, 0, \
		0x003E, 18, 0)

#define AL_CONVERT_RGBA_5551_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 8, \
		0x07C0, 5, 0, \
		0x003E, 18, 0)

#define AL_CONVERT_RGBA_5551_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 10, \
		0x07C0, 0, 0, \
		0x003E, 0, 1)

#define AL_CONVERT_RGBA_5551_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07C0, 0, 1, \
		0x003E, 9, 0)

#define AL_CONVERT_RGBA_5551_TO_RGBX_8888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0xF800, 16, \
		0x07C0, 13, \
		0x003E, 10)

/* ARGB_1555 */

#define AL_CONVERT_ARGB_1555_TO_ARGB_1555(p) (p)

#define AL_CONVERT_ARGB_1555_TO_ARGB_8888(p) \
	LS_CONVERT(p, \
		0x8000, 16, \
		0x7C00, 9, \
		0x03E0, 6, \
		0x001F, 3)

#define AL_CONVERT_ARGB_1555_TO_RGBA_8888(p) \
	SHIFT_CONVERT(p, \
		0x8000, 0, 8, \
		0x7C00, 17, 0, \
		0x03E0, 14, 0, \
		0x001F, 11, 0)

#define AL_CONVERT_ARGB_1555_TO_ARGB_4444(p) \
	RS_CONVERT(p, \
		0x8000, 0, \
		0x7C00, 3, \
		0x03E0, 2, \
		0x001F, 1)

#define AL_CONVERT_ARGB_1555_TO_RGB_888(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 9, \
		0x03E0, 6, \
		0x001F, 3)

#define AL_CONVERT_ARGB_1555_TO_RGB_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 1, \
		0x001F, 0)

#define AL_CONVERT_ARGB_1555_TO_RGB_555(p) \
	(p & 0x7FFF)

#define AL_CONVERT_ARGB_1555_TO_PALETTE_8(p) \
	makecol8((p & 0x7C00) >> 7, \
		 (p & 0x03E0) >> 2, \
		 (p & 0x001F) << 3)

#define AL_CONVERT_ARGB_1555_TO_RGBA_5551(p) \
	((p << 1) | (p >> 15))

#define AL_CONVERT_ARGB_1555_TO_ABGR_8888(p) \
	SHIFT_CONVERT(p, \
		0x8000, 16, 0, \
		0x7C00, 1, 0, \
		0x03E0, 14, 0, \
		0x001F, 11, 0)

#define AL_CONVERT_ARGB_1555_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 7, \
		0x03E0, 6, 0, \
		0x001F, 19, 0)

#define AL_CONVERT_ARGB_1555_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 3, \
		0x03E0, 6, 0, \
		0x001F, 19, 0)

#define AL_CONVERT_ARGB_1555_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 1, 0, \
		0x001F, 11, 0)

#define AL_CONVERT_ARGB_1555_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 0, 0, \
		0x001F, 10, 0)

#define AL_CONVERT_ARGB_1555_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 17, 0, \
		0x03E0, 14, 0, \
		0x001F, 11, 0)

/* ABGR_8888 */

#define AL_CONVERT_ABGR_8888_TO_ABGR_8888(p) (p)

#define AL_CONVERT_ABGR_8888_TO_ARGB_8888(p) \
	SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define AL_CONVERT_ABGR_8888_TO_RGBA_8888(p) \
	SHIFT_CONVERT(p, \
		0xFF000000, 0, 24, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8)

#define AL_CONVERT_ABGR_8888_TO_ARGB_4444(p) \
	SHIFT_CONVERT(p, \
		0xF0000000, 0, 16, \
		0x000000F0, 4, 0, \
		0x0000F000, 0, 8, \
		0x00F00000, 0, 20)

#define AL_CONVERT_ABGR_8888_TO_RGB_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define AL_CONVERT_ABGR_8888_TO_RGB_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 8, 0, \
		0x0000FC00, 0, 5, \
		0x00F80000, 0, 19)

#define AL_CONVERT_ABGR_8888_TO_RGB_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 7, 0, \
		0x0000F800, 0, 6, \
		0x00F80000, 0, 19)

#define AL_CONVERT_ABGR_8888_TO_PALETTE_8(p) \
	makecol8((p & 0x000000FF), \
		 (p & 0x0000FF00) >> 8, \
		 (p & 0x00FF0000) >> 16)

#define AL_CONVERT_ABGR_8888_TO_RGBA_5551(p) \
	SHIFT_CONVERT(p, \
		0x80000000, 0, 31, \
		0x000000F8, 8, 0, \
		0x0000F800, 0, 5, \
		0x00F80000, 0, 18)

#define AL_CONVERT_ABGR_8888_TO_ARGB_1555(p) \
	SHIFT_CONVERT(p, \
		0x80000000, 0, 16, \
		0x000000F8, 7, 0, \
		0x0000F800, 0, 6, \
		0x00F80000, 0, 19)

#define AL_CONVERT_ABGR_8888_TO_XBGR_8888(p) \
	(p & 0x00FFFFFF)

#define AL_CONVERT_ABGR_8888_TO_BGR_888(p) \
	(p & 0xFFFFFF)

#define AL_CONVERT_ABGR_8888_TO_BGR_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000FC00, 5, \
		0x00F80000, 8)

#define AL_CONVERT_ABGR_8888_TO_BGR_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000F800, 5, \
		0x00F80000, 9)

#define AL_CONVERT_ABGR_8888_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8)

/* XBGR_8888 */

#define AL_CONVERT_XBGR_8888_TO_XBGR_8888(p) (p)

#define AL_CONVERT_XBGR_8888_TO_ARGB_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16))
		

#define AL_CONVERT_XBGR_8888_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8))

#define AL_CONVERT_XBGR_8888_TO_ARGB_4444(p) \
	(0xF000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F0, 16, 0, \
		0x0000F000, 0, 0, \
		0x00F00000, 0, 16))

#define AL_CONVERT_XBGR_8888_TO_RGB_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define AL_CONVERT_XBGR_8888_TO_RGB_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 8, 0, \
		0x0000FC00, 0, 5, \
		0x00F80000, 0, 19)

#define AL_CONVERT_XBGR_8888_TO_RGB_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 7, 0, \
		0x0000F800, 0, 6, \
		0x00F80000, 0, 19)

#define AL_CONVERT_XBGR_8888_TO_PALETTE_8(p) \
	makecol8((p & 0x0000FF), \
		 (p & 0x00FF00) >> 8, \
		 (p & 0xFF0000) >> 16)

#define AL_CONVERT_XBGR_8888_TO_RGBA_5551(p) \
	(1 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 8, 0, \
		0x00F800, 0, 5, \
		0xF80000, 0, 18))
		

#define AL_CONVERT_XBGR_8888_TO_ARGB_1555(p) \
	(0x8000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 7, 0, \
		0x00F800, 0, 6, \
		0xF80000, 0, 19))

#define AL_CONVERT_XBGR_8888_TO_ABGR_8888(p) \
	(0xFF000000 | p)

#define AL_CONVERT_XBGR_8888_TO_BGR_888(p) \
	(p & 0xFFFFFF)

#define AL_CONVERT_XBGR_8888_TO_BGR_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000FC00, 5, \
		0x00F80000, 8)

#define AL_CONVERT_XBGR_8888_TO_BGR_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000F800, 5, \
		0x00F80000, 9)

#define AL_CONVERT_XBGR_8888_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8)

/* BGR_888 */

#define AL_CONVERT_BGR_888_TO_ARGB_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 16, 0, \
		0x00FF00, 0, 0, \
		0xFF0000, 0, 16))
		

#define AL_CONVERT_BGR_888_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 24, 0, \
		0x00FF00, 8, 0, \
		0xFF0000, 0, 8))

#define AL_CONVERT_BGR_888_TO_ARGB_4444(p) \
	(0xF000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F0, 4, 0, \
		0x00F000, 0, 8, \
		0xF00000, 0, 20))

#define AL_CONVERT_BGR_888_TO_RGB_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 16, 0, \
		0x00FF00, 0, 0, \
		0xFF0000, 0, 16)

#define AL_CONVERT_BGR_888_TO_RGB_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 8, 0, \
		0x00FC00, 0, 5, \
		0xF80000, 0, 19)

#define AL_CONVERT_BGR_888_TO_RGB_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 7, 0, \
		0x00F800, 0, 6, \
		0xF80000, 0, 18)

#define AL_CONVERT_BGR_888_TO_PALETTE_8(p) \
	makecol8(p & 0xFF, \
		(p >> 8) & 0xFF, \
		(p >> 16) & 0xFF)

#define AL_CONVERT_BGR_888_TO_RGBA_5551(p) \
	(1 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 8, 0, \
		0x00F800, 0, 5, \
		0xF80000, 0, 18))

#define AL_CONVERT_BGR_888_TO_ARGB_1555(p) \
	(0x8000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 7, 0, \
		0x00F800, 0, 6, \
		0xF80000, 0, 19))

#define AL_CONVERT_BGR_888_TO_ABGR_8888(p) \
	(0xFF000000 | p)

#define AL_CONVERT_BGR_888_TO_XBGR_8888(p) \
	(p)

#define AL_CONVERT_BGR_888_TO_BGR_888(p) \
	(p)

#define AL_CONVERT_BGR_888_TO_BGR_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x0000F8, 3, \
		0x00FC00, 5, \
		0xF80000, 8)

#define AL_CONVERT_BGR_888_TO_BGR_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0x0000F8, 3, \
		0x00F800, 6, \
		0xF80000, 9)

#define AL_CONVERT_BGR_888_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 24, 0, \
		0x00FF00, 8, 0, \
		0xFF0000, 0, 8)

/* BGR_565 */

#define AL_CONVERT_BGR_565_TO_ARGB_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 19, 0, \
		0x07E0, 5, 0, \
		0xF800, 0, 8))

#define AL_CONVERT_BGR_565_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 27, 0, \
		0x07E0, 13, 0, \
		0xF800, 0, 0))


#define AL_CONVERT_BGR_565_TO_ARGB_4444(p) \
	(0xFF000000, \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 7, 0, \
		0x0780, 0, 3, \
		0xF000, 0, 8))

#define AL_CONVERT_BGR_565_TO_RGB_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 19, 0, \
		0x07E0, 5, 0, \
		0xF800, 0, 8)

#define AL_CONVERT_BGR_565_TO_RGB_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x07E0, 0, 0, \
		0xF800, 0, 11)

#define AL_CONVERT_BGR_565_TO_RGB_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 11)

#define AL_CONVERT_BGR_565_TO_PALETTE_8(p) \
	makecol8((p & 0x001F) << 3, \
		 (p & 0x07E0) >> 3, \
		 (p & 0xF800) >> 8)

#define AL_CONVERT_BGR_565_TO_RGBA_5551(p) \
	(1 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x07C0, 0, 0, \
		0xF800, 0, 10))

#define AL_CONVERT_BGR_565_TO_ARGB_1555(p) \
	(0x8000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 11))

#define AL_CONVERT_BGR_565_TO_ABGR_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x07E0, 5, 0, \
		0xF800, 8, 0))

#define AL_CONVERT_BGR_565_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x07E0, 5, 0, \
		0xF800, 8, 0)

#define AL_CONVERT_BGR_565_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x07E0, 5, 0, \
		0xF800, 8, 0)

#define AL_CONVERT_BGR_565_TO_BGR_565(p) \
	(p)

#define AL_CONVERT_BGR_565_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 0, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 1)

#define AL_CONVERT_BGR_565_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 27, 0, \
		0x07E0, 13, 0, \
		0xF800, 0, 0)

/* BGR_555 */

#define AL_CONVERT_BGR_555_TO_ARGB_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 19, 0, \
		0x03E0, 6, 0, \
		0x7C00, 0, 7))

#define AL_CONVERT_BGR_555_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 27, 0, \
		0x03E0, 14, 0, \
		0x7C00, 1, 0))

#define AL_CONVERT_BGR_555_TO_ARGB_4444(p) \
	(0xF000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 7, 0, \
		0x03C0, 0, 2, \
		0x7800, 0, 11))

#define AL_CONVERT_BGR_555_TO_RGB_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 19, 0, \
		0x03E0, 6, 0, \
		0x7C00, 0, 7)

#define AL_CONVERT_BGR_555_TO_RGB_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x03E0, 1, 0, \
		0x7C00, 0, 10)

#define AL_CONVERT_BGR_555_TO_RGB_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x03E0, 0, 0, \
		0x7C00, 0, 10)

#define AL_CONVERT_BGR_555_TO_PALETTE_8(p) \
	makecol8((p & 0x001F) << 3, \
		 (p & 0x03E0) >> 2, \
		 (p & 0x7C00) >> 7)

#define AL_CONVERT_BGR_555_TO_RGBA_5551(p) \
	(1 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x03E0, 1, 0, \
		0x7C00, 0, 9))

#define AL_CONVERT_BGR_555_TO_ARGB_1555(p) \
	(0x8000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x03E0, 0, 0, \
		0x7C00, 0, 10))

#define AL_CONVERT_BGR_555_TO_ABGR_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x03E0, 6, 0, \
		0x7C00, 9, 0))

#define AL_CONVERT_BGR_555_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x03E0, 6, 0, \
		0x7C00, 9, 0)

#define AL_CONVERT_BGR_555_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x03E0, 6, 0, \
		0x7C00, 9, 0)

#define AL_CONVERT_BGR_555_TO_BGR_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x001F, 0, \
		0x03E0, 1, \
		0x7C00, 1)

#define AL_CONVERT_BGR_555_TO_BGR_555(p) \
	(p)

#define AL_CONVERT_BGR_555_TO_RGBX_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 27, 0, \
		0x03E0, 14, 0, \
		0x7C00, 1, 0)

/* RGBX_8888 */

#define AL_CONVERT_RGBX_8888_TO_ARGB_8888(p) \
	(0xFF000000 | (p >> 8))

#define AL_CONVERT_RGBX_8888_TO_RGBA_8888(p) \
	(0xFF | p)

#define AL_CONVERT_RGBX_8888_TO_ARGB_4444(p) \
	(0xF000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF0000000, 20, \
		0x00F00000, 16, \
		0x0000F000, 12))

#define AL_CONVERT_RGBX_8888_TO_RGB_888(p) \
	(p >> 8)

#define AL_CONVERT_RGBX_8888_TO_RGB_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 16, \
		0x00FC0000, 13, \
		0x0000F800, 11)

#define AL_CONVERT_RGBX_8888_TO_RGB_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 17, \
		0x00F80000, 14, \
		0x0000F800, 11)

#define AL_CONVERT_RGBX_8888_TO_PALETTE_8(p) \
	makecol8((p & 0xFF000000) >> 24, \
		 (p & 0x00FF0000) >> 16, \
		 (p & 0x0000FF00) >> 8)

#define AL_CONVERT_RGBX_8888_TO_RGBA_5551(p) \
	(1 | \
		RS_CONVERT(p, \
			0, 0, \
			0xF8000000, 16, \
			0x00F80000, 13, \
			0x0000F800, 10))

#define AL_CONVERT_RGBX_8888_TO_ARGB_1555(p) \
	(0x8000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 17, \
		0x00F80000, 14, \
		0x0000F800, 11))

#define AL_CONVERT_RGBX_8888_TO_ABGR_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0))

#define AL_CONVERT_RGBX_8888_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0)

#define AL_CONVERT_RGBX_8888_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0)

#define AL_CONVERT_RGBX_8888_TO_BGR_565(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF8000000, 0, 27, \
		0x00FC0000, 0, 13, \
		0x0000F800, 0, 0)

#define AL_CONVERT_RGBX_8888_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF8000000, 0, 27, \
		0x00F80000, 0, 14, \
		0x0000F800, 0, 1)

#define AL_CONVERT_RGBX_8888_TO_RGBX_8888(p) \
	(p)

#if 0
/* Fast copy for when pixel formats match */
#define FAST_CONVERT(src, spitch, dst, dpitch, size, x, y, w, h) \
	void *sptr = (void *)(src + y*spitch + x*size); \
	void *dptr = (void *)(dst + y*dpitch + x*size); \
	int numbytes = w * size; \
	unsigned int y; \
	for (y = 0; y < h; y++) { \
		memcpy(dptr, sptr, numbytes); \
		sptr += spitch; \
		sptr += dpitch; \
	}
#endif

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
	func15, macro15) \
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
	fprefix ## _to_rgbx_8888, mprefix ## _TO_RGBX_8888)

DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _argb_8888, AL_CONVERT_ARGB_8888)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _rgba_8888, AL_CONVERT_RGBA_8888)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _argb_4444, AL_CONVERT_ARGB_4444)
DEFINE_CONVERSION(unsigned char, 3, READ3BYTES, _rgb_888, AL_CONVERT_RGB_888)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _rgb_565, AL_CONVERT_RGB_565)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _rgb_555, AL_CONVERT_RGB_555)
DEFINE_CONVERSION(unsigned char, 1, bmp_read8, _palette_8, AL_CONVERT_PALETTE_8)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _rgba_5551, AL_CONVERT_RGBA_5551)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _argb_1555, AL_CONVERT_ARGB_1555)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _abgr_8888, AL_CONVERT_ABGR_8888)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _xbgr_8888, AL_CONVERT_XBGR_8888)
DEFINE_CONVERSION(unsigned char, 3, READ3BYTES, _bgr_888, AL_CONVERT_BGR_888)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _bgr_565, AL_CONVERT_BGR_565)
DEFINE_CONVERSION(uint16_t, 2, bmp_read16, _bgr_555, AL_CONVERT_BGR_555)
DEFINE_CONVERSION(uint32_t, 4, bmp_read32, _rgbx_8888, AL_CONVERT_RGBX_8888)

/* Conversion map */

#define NUM_PIXEL_FORMATS 16

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
		prefix ## _to_rgbx_8888 \
	}, \

p_convert_func convert_funcs[NUM_PIXEL_FORMATS][NUM_PIXEL_FORMATS] = {
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
		src_format, _al_pixel_size(src_format)*src->w,
		dst, dst_format, dst_pitch, sx, sy, dx, dy, width, height);
}

