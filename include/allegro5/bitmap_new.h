/* Title: Bitmap types and constants
 */

#ifndef ALLEGRO_BITMAP_NEW_H
#define ALLEGRO_BITMAP_NEW_H

#include "allegro5/color_new.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_BITMAP
 */
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



/* Enum: ALLEGRO_PIXEL_FORMAT
 */
typedef enum ALLEGRO_PIXEL_FORMAT
{
   ALLEGRO_PIXEL_FORMAT_ANY = 0,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ARGB_8888,
   ALLEGRO_PIXEL_FORMAT_RGBA_8888,
   ALLEGRO_PIXEL_FORMAT_ARGB_4444,
   ALLEGRO_PIXEL_FORMAT_RGB_888,	/* 24 bit format */
   ALLEGRO_PIXEL_FORMAT_RGB_565,
   ALLEGRO_PIXEL_FORMAT_RGB_555,
   ALLEGRO_PIXEL_FORMAT_RGBA_5551,
   ALLEGRO_PIXEL_FORMAT_ARGB_1555,
   ALLEGRO_PIXEL_FORMAT_ABGR_8888,
   ALLEGRO_PIXEL_FORMAT_XBGR_8888,
   ALLEGRO_PIXEL_FORMAT_BGR_888,	/* 24 bit format */
   ALLEGRO_PIXEL_FORMAT_BGR_565,
   ALLEGRO_PIXEL_FORMAT_BGR_555,
   ALLEGRO_PIXEL_FORMAT_RGBX_8888,
   ALLEGRO_PIXEL_FORMAT_XRGB_8888,
   ALLEGRO_PIXEL_FORMAT_ABGR_F32,
   ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
   ALLEGRO_PIXEL_FORMAT_RGBA_4444,
   ALLEGRO_NUM_PIXEL_FORMATS
} ALLEGRO_PIXEL_FORMAT;


/*
 * Bitmap flags
 */
#define ALLEGRO_MEMORY_BITMAP         0x0001
#define ALLEGRO_KEEP_BITMAP_FORMAT    0x0002
#define ALLEGRO_FORCE_LOCKING         0x0004
#define ALLEGRO_NO_PRESERVE_TEXTURE   0x0008
#define ALLEGRO_ALPHA_TEST            0x0010


/* Flags for the blitting functions */
#define ALLEGRO_FLIP_HORIZONTAL       0x00001
#define ALLEGRO_FLIP_VERTICAL         0x00002


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
 */
typedef struct ALLEGRO_LOCKED_REGION ALLEGRO_LOCKED_REGION;
struct ALLEGRO_LOCKED_REGION {
   void *data;
   int format;
   int pitch;
};


AL_FUNC(void, al_set_new_bitmap_format, (int format));
AL_FUNC(void, al_set_new_bitmap_flags, (int flags));
AL_FUNC(int, al_get_new_bitmap_format, (void));
AL_FUNC(int, al_get_new_bitmap_flags, (void));

AL_FUNC(int, al_get_bitmap_width, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_height, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_format, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(int, al_get_bitmap_flags, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(ALLEGRO_BITMAP*, al_create_bitmap, (int w, int h));
AL_FUNC(void, al_destroy_bitmap, (ALLEGRO_BITMAP *bitmap));

/* Blitting */
AL_FUNC(void, al_draw_bitmap, (ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags));
AL_FUNC(void, al_draw_bitmap_region, (ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, int flags));
AL_FUNC(void, al_draw_scaled_bitmap, (ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags));
AL_FUNC(void, al_draw_rotated_bitmap, (ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float angle, int flags));
AL_FUNC(void, al_draw_rotated_scaled_bitmap, (ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags));

AL_FUNC(ALLEGRO_LOCKED_REGION*, al_lock_bitmap, (ALLEGRO_BITMAP *bitmap, int format, int flags));
AL_FUNC(ALLEGRO_LOCKED_REGION*, al_lock_bitmap_region, (ALLEGRO_BITMAP *bitmap, int x, int y, int width, int height, int format, int flags));
AL_FUNC(void, al_unlock_bitmap, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(void, al_put_pixel, (int x, int y, ALLEGRO_COLOR color));
AL_FUNC(ALLEGRO_COLOR, al_get_pixel, (ALLEGRO_BITMAP *bitmap, int x, int y));
AL_FUNC(int, al_get_pixel_size, (int format));

/* Pixel mapping */
AL_FUNC(ALLEGRO_COLOR, al_map_rgb, (unsigned char r, unsigned char g, unsigned char b));
AL_FUNC(ALLEGRO_COLOR, al_map_rgba, (unsigned char r, unsigned char g, unsigned char b, unsigned char a));
AL_FUNC(ALLEGRO_COLOR, al_map_rgb_f, (float r, float g, float b));
AL_FUNC(ALLEGRO_COLOR, al_map_rgba_f, (float r, float g, float b, float a));


/* Pixel unmapping */
AL_FUNC(void, al_unmap_rgb, (ALLEGRO_COLOR color, unsigned char *r, unsigned char *g, unsigned char *b));
AL_FUNC(void, al_unmap_rgba, (ALLEGRO_COLOR color, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a));
AL_FUNC(void, al_unmap_rgb_f, (ALLEGRO_COLOR color, float *r, float *g, float *b));
AL_FUNC(void, al_unmap_rgba_f, (ALLEGRO_COLOR color, float *r, float *g, float *b, float *a));
AL_FUNC(int, al_get_pixel_format_bits, (int format));


/* Masking */
AL_FUNC(void, al_convert_mask_to_alpha, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR mask_color));

/* Clipping */
AL_FUNC(void, al_set_clipping_rectangle, (int x, int y, int width, int height));
AL_FUNC(void, al_get_clipping_rectangle, (int *x, int *y, int *w, int *h));

/* Sub bitmaps */
AL_FUNC(ALLEGRO_BITMAP *, al_create_sub_bitmap, (ALLEGRO_BITMAP *parent, int x, int y, int w, int h));
AL_FUNC(bool, al_is_sub_bitmap, (ALLEGRO_BITMAP *bitmap));

/* Miscellaneous */
AL_FUNC(ALLEGRO_BITMAP *, al_clone_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(bool, al_is_bitmap_locked, (ALLEGRO_BITMAP *bitmap));

/* Blending */
AL_FUNC(void, al_set_blender, (int source, int dest, ALLEGRO_COLOR color));
AL_FUNC(void, al_get_blender, (int *source, int *dest, ALLEGRO_COLOR *color));
AL_FUNC(void, al_set_separate_blender, (int source, int dest, int alpha_source, int alpha_dest, ALLEGRO_COLOR color));
AL_FUNC(void, al_get_separate_blender, (int *source, int *dest, int *alpha_src, int *alpha_dest, ALLEGRO_COLOR *color));
AL_FUNC(ALLEGRO_COLOR *, _al_get_blend_color, (void));

AL_FUNC(void, _al_put_pixel, (ALLEGRO_BITMAP *bitmap, int x, int y, ALLEGRO_COLOR color));

#ifdef __cplusplus
   }
#endif

#endif
