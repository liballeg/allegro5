/* Title: Bitmap routines
 */

#ifndef ALLEGRO_BITMAP_NEW_H
#define ALLEGRO_BITMAP_NEW_H

#include "allegro5/color_new.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_BITMAP ALLEGRO_BITMAP;

/* This is just a proof-of-concept implementation. The basic idea is to leave 
 * things like pixel formats to Allegro usually. The user of course should be
 * able to specify things, as specific as they want. (E.g.
 * "i want a bitmap with 16-bit resolution per color channel"
 * "i want the best bitmap format which is HW accelerated"
 * "i want an RGBA bitmap which does not need to be HW accelerated"
 * )
 * For now, there is only a single flags parameter, which is passed on to the
 * underlying driver, but ignored otherwise.
 */


#define ALLEGRO_NUM_PIXEL_FORMATS      26

/* Enum: ALLEGRO_PIXEL_FORMAT
 * 
 * Pixel formats. Each pixel format specifies the exact size and bit
 * layout of a pixel in memory. Components are specified from high bits
 * to low, so for example a fully opaque red pixel in ARGB_8888 format
 * is 0xFFFF0000.
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA - Let the driver choose a format without
 * alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA - Let the driver choose a format with
 * alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA - Let the driver choose a 15 bit format
 * without alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA - Let the driver choose a 15 bit
 * format with alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA - Let the driver choose a 16 bit format
 * without alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA - Let the driver choose a 16 bit
 * format with alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA - Let the driver choose a 24 bit format
 * without alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA - Let the driver choose a 24 bit
 * format with alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA - Let the driver choose a 32 bit format
 * without alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA - Let the driver choose a 32 bit
 * format with alpha
 *
 * ALLEGRO_PIXEL_FORMAT_ARGB_8888 - 32 bit
 *
 * ALLEGRO_PIXEL_FORMAT_RGBA_8888 - 32 bit
 *
 * ALLEGRO_PIXEL_FORMAT_ARGB_4444 - 16 bit
 *
 * ALLEGRO_PIXEL_FORMAT_RGB_888 - 24 bit
 *
 * ALLEGRO_PIXEL_FORMAT_RGB_565 - 16 bit
 *
 * ALLEGRO_PIXEL_FORMAT_RGB_555 - 15 bit
 *
 * ALLEGRO_PIXEL_FORMAT_PALETTE_8 - 8 bit
 *
 * ALLEGRO_PIXEL_FORMAT_RGBA_5551 - 16 bit
 *
 * ALLEGRO_PIXEL_FORMAT_ARGB_1555 - 16 bit
 *
 * ALLEGRO_PIXEL_FORMAT_ABGR_8888 - 32 bit
 *
 * ALLEGRO_PIXEL_FORMAT_XBGR_8888 - 32 bit
 *
 * ALLEGRO_PIXEL_FORMAT_BGR_888 - 24 bit
 *
 * ALLEGRO_PIXEL_FORMAT_BGR_565 - 16 bit 
 *
 * ALLEGRO_PIXEL_FORMAT_BGR_555 - 15 bit
 *
 * ALLEGRO_PIXEL_FORMAT_RGBX_8888 - 32 bit
 *
 * ALLEGRO_PIXEL_FORMAT_XRGB_8888 - 32 bit
 */
enum ALLEGRO_PIXEL_FORMAT {
        ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA = 0,
	ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
        ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
	ALLEGRO_PIXEL_FORMAT_ARGB_8888,
	ALLEGRO_PIXEL_FORMAT_RGBA_8888,
	ALLEGRO_PIXEL_FORMAT_ARGB_4444,
	ALLEGRO_PIXEL_FORMAT_RGB_888,	/* 24 bit format */
	ALLEGRO_PIXEL_FORMAT_RGB_565,
	ALLEGRO_PIXEL_FORMAT_RGB_555,
        ALLEGRO_PIXEL_FORMAT_PALETTE_8,
	ALLEGRO_PIXEL_FORMAT_RGBA_5551,
	ALLEGRO_PIXEL_FORMAT_ARGB_1555,
	ALLEGRO_PIXEL_FORMAT_ABGR_8888,
	ALLEGRO_PIXEL_FORMAT_XBGR_8888,
	ALLEGRO_PIXEL_FORMAT_BGR_888,	/* 24 bit format */
	ALLEGRO_PIXEL_FORMAT_BGR_565,
	ALLEGRO_PIXEL_FORMAT_BGR_555,
	ALLEGRO_PIXEL_FORMAT_RGBX_8888,
	ALLEGRO_PIXEL_FORMAT_XRGB_8888,
};


/*
 * Bitmap flags
 */
#define ALLEGRO_MEMORY_BITMAP      0x0001
#define ALLEGRO_SYNC_MEMORY_COPY   0x0002
#define ALLEGRO_USE_ALPHA          0x0004
#define ALLEGRO_KEEP_BITMAP_FORMAT 0x0008


/* Flags for the blitting functions */
#define ALLEGRO_FLIP_HORIZONTAL       0x00001
#define ALLEGRO_FLIP_VERTICAL         0x00002

/* Flags for primitives */
#define ALLEGRO_OUTLINED              0x00000
#define ALLEGRO_FILLED                0x00001


/*
 * Locking flags
 */
#define ALLEGRO_LOCK_READONLY 1
#define ALLEGRO_LOCK_WRITEONLY 2


/*
 * Blending modes
 */
enum ALLEGRO_BLEND_MODE {
   ALLEGRO_ZERO = 0,
   ALLEGRO_ONE = 1,
   ALLEGRO_ALPHA = 2,
   ALLEGRO_INVERSE_ALPHA = 3
};


/* Type: ALLEGRO_LOCKED_REGION
 * Users who wish to manually edit or read from a bitmap
 * are required to lock it first. The ALLEGRO_LOCKED_REGION
 * structure represents the locked region of the bitmap. This
 * call will work with any bitmap, including memory bitmaps.
 *
 * > typedef struct ALLEGRO_LOCKED_REGION {
 * >         void *data; // the bitmap data
 * >         int format; // the pixel format of the data
 * >         int pitch;  // the size in bytes of a single line
 * >                     // pitch may be greater than pixel_size*bitmap->w
 * >                     // i.e. padded with extra bytes
 * > }
 */
typedef struct ALLEGRO_LOCKED_REGION {
	void *data;
	int format;
	int pitch;
} ALLEGRO_LOCKED_REGION;


AL_FUNC(void, al_set_new_bitmap_format, (int format));
AL_FUNC(void, al_set_new_bitmap_flags, (int flags));
AL_FUNC(int, al_get_new_bitmap_format, (void));
AL_FUNC(int, al_get_new_bitmap_flags, (void));

AL_FUNC(int, al_get_bitmap_width, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_height, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_format, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_flags, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(ALLEGRO_BITMAP, *al_create_bitmap, (int w, int h));
AL_FUNC(ALLEGRO_BITMAP, *al_load_bitmap, (char const *filename));
AL_FUNC(void, al_destroy_bitmap, (ALLEGRO_BITMAP *bitmap));

/* Blitting */
AL_FUNC(void, al_draw_bitmap, (ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags));
AL_FUNC(void, al_draw_bitmap_region, (ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, int flags));
AL_FUNC(void, al_draw_scaled_bitmap, (ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags));
AL_FUNC(void, al_draw_rotated_bitmap, (ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float angle, int flags));
AL_FUNC(void, al_draw_rotated_scaled_bitmap, (ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags));

AL_FUNC(ALLEGRO_LOCKED_REGION, *al_lock_bitmap, (ALLEGRO_BITMAP *bitmap, ALLEGRO_LOCKED_REGION *locked_region, int flags));
AL_FUNC(ALLEGRO_LOCKED_REGION, *al_lock_bitmap_region, (ALLEGRO_BITMAP *bitmap, int x, int y, int width, int height, ALLEGRO_LOCKED_REGION *locked_region, int flags));
AL_FUNC(void, al_unlock_bitmap, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(void, al_put_pixel, (int x, int y, ALLEGRO_COLOR *color));
AL_FUNC(ALLEGRO_COLOR, *al_get_pixel, (ALLEGRO_BITMAP *bitmap, int x, int y, ALLEGRO_COLOR *color));
AL_FUNC(int, al_get_pixel_size, (int format));

/* Pixel mapping */
AL_FUNC(ALLEGRO_COLOR, *al_map_rgb, (ALLEGRO_COLOR *color,	unsigned char r, unsigned char g, unsigned char b));
AL_FUNC(ALLEGRO_COLOR, *al_map_rgba, (ALLEGRO_COLOR *color, unsigned char r, unsigned char g, unsigned char b, unsigned char a));
AL_FUNC(ALLEGRO_COLOR, *al_map_rgb_f, (ALLEGRO_COLOR *color, float r, float g, float b));
AL_FUNC(ALLEGRO_COLOR, *al_map_rgba_f, (ALLEGRO_COLOR *color, float r, float g, float b, float a));
AL_FUNC(ALLEGRO_COLOR, *al_map_rgb_i, (ALLEGRO_COLOR *color, int r, int g, int b));
AL_FUNC(ALLEGRO_COLOR, *al_map_rgba_i, (ALLEGRO_COLOR *color, int r, int g, int b, int a));
AL_FUNC(ALLEGRO_COLOR*, al_map_rgba_ex, (int format, ALLEGRO_COLOR *color, unsigned char r, unsigned char g, unsigned char b, unsigned char a));
AL_FUNC(ALLEGRO_COLOR*, al_map_rgba_f_ex, (int format, ALLEGRO_COLOR *color, float r, float g, float b, float a));
AL_FUNC(ALLEGRO_COLOR*, al_map_rgba_i_ex, (int format, ALLEGRO_COLOR *color, int r, int g, int b, int a));
AL_FUNC(ALLEGRO_COLOR*, al_map_rgb_ex, (int format, ALLEGRO_COLOR *color, unsigned char r, unsigned char g, unsigned char b));
AL_FUNC(ALLEGRO_COLOR*, al_map_rgb_f_ex, (int format, ALLEGRO_COLOR *color, float r, float g, float b));
AL_FUNC(ALLEGRO_COLOR*, al_map_rgb_i_ex, (int format, ALLEGRO_COLOR *color, int r, int g, int b));


/* Pixel unmapping */
AL_FUNC(void, al_unmap_rgb, (ALLEGRO_COLOR *color, unsigned char *r, unsigned char *g, unsigned char *b));
AL_FUNC(void, al_unmap_rgba, (ALLEGRO_COLOR *color, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a));
AL_FUNC(void, al_unmap_rgb_f, (ALLEGRO_COLOR *color, float *r, float *g, float *b));
AL_FUNC(void, al_unmap_rgba_f, (ALLEGRO_COLOR *color, float *r, float *g, float *b, float *a));
AL_FUNC(void, al_unmap_rgb_i, (ALLEGRO_COLOR *color, int *r, int *g, int *b));
AL_FUNC(void, al_unmap_rgba_i, (ALLEGRO_COLOR *color, int *r, int *g, int *b, int *a));
AL_FUNC(void, al_unmap_rgba_ex, (int format, ALLEGRO_COLOR *color, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a));
AL_FUNC(void, al_unmap_rgba_f_ex, (int format, ALLEGRO_COLOR *color, float *r, float *g, float *b, float *a));
AL_FUNC(void, al_unmap_rgba_i_ex, (int format, ALLEGRO_COLOR *color, int *r, int *g, int *b, int *a));
AL_FUNC(void, al_unmap_rgb_ex, (int format, ALLEGRO_COLOR *color, unsigned char *r, unsigned char *g, unsigned char *b));
AL_FUNC(void, al_unmap_rgb_f_ex, (int format, ALLEGRO_COLOR *color, float *r, float *g, float *b));
AL_FUNC(void, al_unmap_rgb_i_ex, (int format, ALLEGRO_COLOR *color, int *r, int *g, int *b));


/* Masking */
AL_FUNC(void, al_convert_mask_to_alpha, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *mask_color));

/* Clipping */
AL_FUNC(void, al_set_clipping_rectangle, (int x, int y, int width, int height));
AL_FUNC(void, al_get_clipping_rectangle, (int *x, int *y, int *w, int *h));

/* Sub bitmaps */
AL_FUNC(ALLEGRO_BITMAP, *al_create_sub_bitmap, (ALLEGRO_BITMAP *parent, int x, int y, int w, int h));

/* Miscellaneous */
AL_FUNC(ALLEGRO_BITMAP, *al_clone_bitmap, (ALLEGRO_BITMAP *bitmap));

/* Blending */
AL_FUNC(void, al_set_blender, (int source, int dest, ALLEGRO_COLOR *color));
AL_FUNC(void, al_get_blender, (int *source, int *dest, ALLEGRO_COLOR *color));

#ifdef __cplusplus
   }
#endif

#endif
