#ifndef _AL_CONVERT_H
#define _AL_CONVERT_H

#include "allegro.h"

#define NUM_PIXEL_FORMATS 16

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

/* Converting 565 to 888 */
#define SHIFT_CONVERT_565(p, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((_rgb_scale_5[((p & r_mask) >> r_rshift)] << r_lshift) | \
	 (_rgb_scale_6[((p & g_mask) >> g_rshift)] << g_lshift) | \
	 (_rgb_scale_5[((p & b_mask) >> b_rshift)] << b_lshift))

/* Converting 555(1) to 888 */
#define SHIFT_CONVERT_555(p, \
		a_mask, a_lshift, a_rshift, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((_rgb_scale_1[((p & a_mask) >> a_rshift)] << a_lshift) | \
	 (_rgb_scale_5[((p & r_mask) >> r_rshift)] << r_lshift) | \
	 (_rgb_scale_5[((p & g_mask) >> g_rshift)] << g_lshift) | \
	 (_rgb_scale_5[((p & b_mask) >> b_rshift)] << b_lshift))

/* Converting 444 to 888 */
#define SHIFT_CONVERT_4444(p, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((_rgb_scale_4[((p & r_mask) >> r_rshift)] << r_lshift) | \
	 (_rgb_scale_4[((p & g_mask) >> g_rshift)] << g_lshift) | \
	 (_rgb_scale_4[((p & b_mask) >> b_rshift)] << b_lshift))

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
		SHIFT_CONVERT_565(p, \
		0xF800, 19, 11, \
		0x07E0, 10, 5, \
		0x001F, 3, 0))

#define AL_CONVERT_RGB_565_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT_565(p, \
		0xF800, 27, 11, \
		0x07E0, 18, 5, \
		0x001F, 11, 0))

#define AL_CONVERT_RGB_565_TO_ARGB_4444(p) \
	(0xF000 | \
		RS_CONVERT(p, \
		0, 0, \
		0xF000, 4, \
		0x0780, 3, \
		0x001E, 1))

#define AL_CONVERT_RGB_565_TO_RGB_888(p) \
	SHIFT_CONVERT_565(p, \
		0xF800, 19, 11, \
		0x07E0, 10, 5, \
		0x001F, 3, 0)

#define AL_CONVERT_RGB_565_TO_RGB_555(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF800, 1, \
		0x07E0, 1, \
		0x001F, 0)

#define AL_CONVERT_RGB_565_TO_PALETTE_8(p) \
	makecol8(_rgb_scale_5[(p & 0xF800) >> 8], \
		 _rgb_scale_6[(p & 0x07E0) >> 3], \
		 _rgb_scale_5[(p & 0x001F) << 3])

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
		SHIFT_CONVERT_565(p, \
		0xF800, 11, 11, \
		0x07E0, 18, 5, \
		0x001F, 27, 0))

#define AL_CONVERT_RGB_565_TO_XBGR_8888(p) \
	SHIFT_CONVERT_565(p, \
		0xF800, 11, 11, \
		0x07E0, 18, 5, \
		0x001F, 27, 0)

#define AL_CONVERT_RGB_565_TO_BGR_888(p) \
	SHIFT_CONVERT_565(p, \
		0xF800, 3, 11, \
		0x07E0, 10, 5, \
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
	SHIFT_CONVERT_565(p, \
		0xF800, 27, 11, \
		0x07E0, 18, 5, \
		0x001F, 11, 0)

/* RGB_555 */

#define AL_CONVERT_RGB_555_TO_RGB_555(p) (p)

#define AL_CONVERT_RGB_555_TO_ARGB_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 19, 10, \
		0x03E0, 11, 5, \
		0x001F, 3, 0))

#define AL_CONVERT_RGB_555_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 27, 10, \
		0x03E0, 19, 5, \
		0x001F, 11, 0))

#define AL_CONVERT_RGB_555_TO_ARGB_4444(p) \
	(0xF000 | \
		RS_CONVERT(p, \
		0, 0, \
		0x7C00, 3, \
		0x03E0, 2, \
		0x001F, 1))

#define AL_CONVERT_RGB_555_TO_RGB_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 19, 10, \
		0x03E0, 11, 5, \
		0x001F, 3, 0)

#define AL_CONVERT_RGB_555_TO_RGB_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 1, \
		0x001F, 0)

#define AL_CONVERT_RGB_555_TO_PALETTE_8(p) \
	makecol8(_rgb_scale_5[(p & 0x7C00) >> 7], \
		 _rgb_scale_5[(p & 0x03E0) >> 2], \
		 _rgb_scale_5[(p & 0x001F) << 3])

#define AL_CONVERT_RGB_555_TO_RGBA_5551(p) \
	((p << 1) | 1)

#define AL_CONVERT_RGB_555_TO_ARGB_1555(p) \
	(p | 0x8000)

#define AL_CONVERT_RGB_555_TO_ABGR_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 11, 10, \
		0x03E0, 19, 5, \
		0x001F, 27, 0))

#define AL_CONVERT_RGB_555_TO_XBGR_8888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 11, 10, \
		0x03E0, 19, 5, \
		0x001F, 27, 0)

#define AL_CONVERT_RGB_555_TO_BGR_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 3, 10, \
		0x03E0, 11, 5, \
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
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 27, 10, \
		0x03E0, 19, 5, \
		0x001F, 11, 0)

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
		(((getr8(p) & 0xF0) << 4) | \
		  (getg8(p) & 0xF0) | \
		  (getb8(p) & 0xF0) >> 4))

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
	(getr8(p) << 0) | \
	(getg8(p) << 8) | \
	(getb8(p) << 16))

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
	SHIFT_CONVERT_555(p, \
		0x1, 31, 0, \
		0xF800, 19, 11, \
		0x07C0, 11, 6, \
		0x003E, 3, 1)

#define AL_CONVERT_RGBA_5551_TO_RGBA_8888(p) \
	SHIFT_CONVERT_555(p, \
		0x1, 7, 0, \
		0xF800, 27, 11, \
		0x07C0, 19, 6, \
		0x003E, 11, 1)

#define AL_CONVERT_RGBA_5551_TO_ARGB_4444(p) \
	SHIFT_CONVERT(p, \
		0x1, 15, 0, \
		0xF000, 0, 4, \
		0x0780, 0, 3, \
		0x003C, 0, 2)

#define AL_CONVERT_RGBA_5551_TO_RGB_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 19, 11, \
		0x07C0, 11, 6, \
		0x003E, 3, 1)

#define AL_CONVERT_RGBA_5551_TO_RGB_565(p) \
	RS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07C0, 0, \
		0x003E, 1)

#define AL_CONVERT_RGBA_5551_TO_RGB_555(p) \
	(p >> 1)

#define AL_CONVERT_RGBA_5551_TO_PALETTE_8(p) \
	makecol8(_rgb_scale_5[(p & 0xF800) >> 8], \
		 _rgb_scale_5[(p & 0x07C0) >> 3], \
		 _rgb_scale_5[(p & 0x003E) << 2])

#define AL_CONVERT_RGBA_5551_TO_ARGB_1555(p) \
	(((p & 1) << 15) | (p >> 1))

#define AL_CONVERT_RGBA_5551_TO_ABGR_8888(p) \
	SHIFT_CONVERT_555(p, \
		0x1, 31, 0, \
		0xF800, 3, 11, \
		0x07C0, 14, 6, \
		0x003E, 27, 1)

#define AL_CONVERT_RGBA_5551_TO_XBGR_8888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 3, 11, \
		0x07C0, 11, 6, \
		0x003E, 19, 1)

#define AL_CONVERT_RGBA_5551_TO_BGR_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 3, 11, \
		0x07C0, 11, 6, \
		0x003E, 19, 1)

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
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 27, 11, \
		0x07C0, 19, 6, \
		0x003E, 11, 1)

/* ARGB_1555 */

#define AL_CONVERT_ARGB_1555_TO_ARGB_1555(p) (p)

#define AL_CONVERT_ARGB_1555_TO_ARGB_8888(p) \
	SHIFT_CONVERT_555(p, \
		0x8000, 31, 15, \
		0x7C00, 19, 10, \
		0x03E0, 11, 5, \
		0x001F, 3, 0)

#define AL_CONVERT_ARGB_1555_TO_RGBA_8888(p) \
	SHIFT_CONVERT_555(p, \
		0x8000, 7, 15, \
		0x7C00, 27, 10, \
		0x03E0, 19, 5, \
		0x001F, 11, 0)

#define AL_CONVERT_ARGB_1555_TO_ARGB_4444(p) \
	RS_CONVERT(p, \
		0x8000, 0, \
		0x7C00, 3, \
		0x03E0, 2, \
		0x001F, 1)

#define AL_CONVERT_ARGB_1555_TO_RGB_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 19, 10, \
		0x03E0, 11, 5, \
		0x001F, 3, 0)

#define AL_CONVERT_ARGB_1555_TO_RGB_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 1, \
		0x001F, 0)

#define AL_CONVERT_ARGB_1555_TO_RGB_555(p) \
	(p & 0x7FFF)

#define AL_CONVERT_ARGB_1555_TO_PALETTE_8(p) \
	makecol8(_rgb_scale_5[(p & 0x7C00) >> 7], \
		 _rgb_scale_5[(p & 0x03E0) >> 2], \
		 _rgb_scale_5[(p & 0x001F) << 3])

#define AL_CONVERT_ARGB_1555_TO_RGBA_5551(p) \
	((p << 1) | (p >> 15))

#define AL_CONVERT_ARGB_1555_TO_ABGR_8888(p) \
	SHIFT_CONVERT_555(p, \
		0x8000, 31, 15, \
		0x7C00, 11, 10, \
		0x03E0, 19, 5, \
		0x001F, 11, 0)

#define AL_CONVERT_ARGB_1555_TO_XBGR_8888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 3, 10, \
		0x03E0, 11, 5, \
		0x001F, 19, 0)

#define AL_CONVERT_ARGB_1555_TO_BGR_888(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 3, 10, \
		0x03E0, 11, 5, \
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
		0x7C00, 27, 10, \
		0x03E0, 19, 5, \
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
		SHIFT_CONVERT_565(p, \
		0x001F, 19, 0, \
		0x07E0, 10, 5, \
		0xF800, 3, 11))

#define AL_CONVERT_BGR_565_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT_565(p, \
		0x001F, 27, 0, \
		0x07E0, 18, 5, \
		0xF800, 11, 11))


#define AL_CONVERT_BGR_565_TO_ARGB_4444(p) \
	(0xFF000000, \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 7, 0, \
		0x0780, 0, 3, \
		0xF000, 0, 8))

#define AL_CONVERT_BGR_565_TO_RGB_888(p) \
	SHIFT_CONVERT_565(p, \
		0x001F, 19, 0, \
		0x07E0, 10, 5, \
		0xF800, 3, 11)

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
	makecol8(_rgb_scale_5[(p & 0x001F) << 3], \
		 _rgb_scale_6[(p & 0x07E0) >> 3], \
		 _rgb_scale_5[(p & 0xF800) >> 8])

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
		SHIFT_CONVERT_565(p, \
		0x001F, 3, 0, \
		0x07E0, 10, 5, \
		0xF800, 19, 11))

#define AL_CONVERT_BGR_565_TO_XBGR_8888(p) \
	SHIFT_CONVERT_565(p, \
		0x001F, 3, 0, \
		0x07E0, 10, 5, \
		0xF800, 19, 11)

#define AL_CONVERT_BGR_565_TO_BGR_888(p) \
	SHIFT_CONVERT_565(p, \
		0x001F, 3, 0, \
		0x07E0, 10, 5, \
		0xF800, 19, 11)

#define AL_CONVERT_BGR_565_TO_BGR_565(p) \
	(p)

#define AL_CONVERT_BGR_565_TO_BGR_555(p) \
	SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 0, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 1)

#define AL_CONVERT_BGR_565_TO_RGBX_8888(p) \
	SHIFT_CONVERT_565(p, \
		0x001F, 27, 0, \
		0x07E0, 18, 5, \
		0xF800, 11, 11)

/* BGR_555 */

#define AL_CONVERT_BGR_555_TO_ARGB_8888(p) \
	(0xFF000000 | \
		SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 19, 0, \
		0x03E0, 11, 5, \
		0x7C00, 3, 10))

#define AL_CONVERT_BGR_555_TO_RGBA_8888(p) \
	(0xFF | \
		SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 27, 0, \
		0x03E0, 19, 5, \
		0x7C00, 11, 10))

#define AL_CONVERT_BGR_555_TO_ARGB_4444(p) \
	(0xF000 | \
		SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 7, 0, \
		0x03C0, 0, 2, \
		0x7800, 0, 11))

#define AL_CONVERT_BGR_555_TO_RGB_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 19, 0, \
		0x03E0, 11, 5, \
		0x7C00, 3, 10)

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
	makecol8(_rgb_scale_5[(p & 0x001F) << 3], \
		 _rgb_scale_6[(p & 0x03E0) >> 2], \
		 _rgb_scale_5[(p & 0x7C00) >> 7])

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
		SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x03E0, 11, 5, \
		0x7C00, 19, 10))

#define AL_CONVERT_BGR_555_TO_XBGR_8888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x03E0, 11, 5, \
		0x7C00, 19, 10)

#define AL_CONVERT_BGR_555_TO_BGR_888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 3, 0, \
		0x03E0, 11, 5, \
		0x7C00, 19, 10)

#define AL_CONVERT_BGR_555_TO_BGR_565(p) \
	LS_CONVERT(p, \
		0, 0, \
		0x001F, 0, \
		0x03E0, 1, \
		0x7C00, 1)

#define AL_CONVERT_BGR_555_TO_BGR_555(p) \
	(p)

#define AL_CONVERT_BGR_555_TO_RGBX_8888(p) \
	SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 27, 0, \
		0x03E0, 19, 5, \
		0x7C00, 11, 10)

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

#endif
