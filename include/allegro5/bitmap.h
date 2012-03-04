#ifndef __al_included_allegro5_bitmap_h
#define __al_included_allegro5_bitmap_h

#include "allegro5/color.h"

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_BITMAP
 */
typedef struct ALLEGRO_BITMAP ALLEGRO_BITMAP;


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
enum {
   ALLEGRO_MEMORY_BITMAP            = 0x0001,
   ALLEGRO_KEEP_BITMAP_FORMAT       = 0x0002,
   ALLEGRO_FORCE_LOCKING            = 0x0004,
   ALLEGRO_NO_PRESERVE_TEXTURE      = 0x0008,
   ALLEGRO_ALPHA_TEST               = 0x0010,
   _ALLEGRO_INTERNAL_OPENGL         = 0x0020,
   ALLEGRO_MIN_LINEAR               = 0x0040,
   ALLEGRO_MAG_LINEAR               = 0x0080,
   ALLEGRO_MIPMAP                   = 0x0100,
   ALLEGRO_NO_PREMULTIPLIED_ALPHA   = 0x0200,
   ALLEGRO_VIDEO_BITMAP             = 0x0400
};


/* Flags for the blitting functions */
enum {
   ALLEGRO_FLIP_HORIZONTAL = 0x00001,
   ALLEGRO_FLIP_VERTICAL   = 0x00002
};


/*
 * Locking flags
 */
enum {
   ALLEGRO_LOCK_READWRITE  = 0,
   ALLEGRO_LOCK_READONLY   = 1,
   ALLEGRO_LOCK_WRITEONLY  = 2
};


/*
 * Blending modes
 */
enum ALLEGRO_BLEND_MODE {
   ALLEGRO_ZERO = 0,
   ALLEGRO_ONE = 1,
   ALLEGRO_ALPHA = 2,
   ALLEGRO_INVERSE_ALPHA = 3
};

enum ALLEGRO_BLEND_OPERATIONS {
   ALLEGRO_ADD = 0,
   ALLEGRO_SRC_MINUS_DEST = 1,
   ALLEGRO_DEST_MINUS_SRC = 2,
   ALLEGRO_NUM_BLEND_OPERATIONS
};


/* Type: ALLEGRO_LOCKED_REGION
 */
typedef struct ALLEGRO_LOCKED_REGION ALLEGRO_LOCKED_REGION;
struct ALLEGRO_LOCKED_REGION {
   void *data;
   int format;
   int pitch;
   int pixel_size;
};


AL_FUNC(void, al_set_new_bitmap_format, (int format));
AL_FUNC(void, al_set_new_bitmap_flags, (int flags));
AL_FUNC(int, al_get_new_bitmap_format, (void));
AL_FUNC(int, al_get_new_bitmap_flags, (void));
AL_FUNC(void, al_add_new_bitmap_flag, (int flag));

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
AL_FUNC(void, al_draw_scaled_rotated_bitmap, (ALLEGRO_BITMAP *bitmap, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags));

/* Tinted blitting */
AL_FUNC(void, al_draw_tinted_bitmap, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float dx, float dy, int flags));
AL_FUNC(void, al_draw_tinted_bitmap_region, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, int flags));
AL_FUNC(void, al_draw_tinted_scaled_bitmap, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags));
AL_FUNC(void, al_draw_tinted_rotated_bitmap, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float cx, float cy, float dx, float dy, float angle, int flags));
AL_FUNC(void, al_draw_tinted_scaled_rotated_bitmap, (ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags));
AL_FUNC(void, al_draw_tinted_scaled_rotated_bitmap_region, (
   ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags));

/* Locking */
AL_FUNC(ALLEGRO_LOCKED_REGION*, al_lock_bitmap, (ALLEGRO_BITMAP *bitmap, int format, int flags));
AL_FUNC(ALLEGRO_LOCKED_REGION*, al_lock_bitmap_region, (ALLEGRO_BITMAP *bitmap, int x, int y, int width, int height, int format, int flags));
AL_FUNC(void, al_unlock_bitmap, (ALLEGRO_BITMAP *bitmap));

AL_FUNC(void, al_put_pixel, (int x, int y, ALLEGRO_COLOR color));
AL_FUNC(void, al_put_blended_pixel, (int x, int y, ALLEGRO_COLOR color));
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
AL_FUNC(void, al_reset_clipping_rectangle, (void));
AL_FUNC(void, al_get_clipping_rectangle, (int *x, int *y, int *w, int *h));

/* Sub bitmaps */
AL_FUNC(ALLEGRO_BITMAP *, al_create_sub_bitmap, (ALLEGRO_BITMAP *parent, int x, int y, int w, int h));
AL_FUNC(bool, al_is_sub_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(ALLEGRO_BITMAP *, al_get_parent_bitmap, (ALLEGRO_BITMAP *bitmap));

/* Miscellaneous */
AL_FUNC(ALLEGRO_BITMAP *, al_clone_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(bool, al_is_bitmap_locked, (ALLEGRO_BITMAP *bitmap));

/* Blending */
AL_FUNC(void, al_set_blender, (int op, int source, int dest));
AL_FUNC(void, al_get_blender, (int *op, int *source, int *dest));
AL_FUNC(void, al_set_separate_blender, (int op, int source, int dest,
   int alpha_op, int alpha_source, int alpha_dest));
AL_FUNC(void, al_get_separate_blender, (int *op, int *source, int *dest,
   int *alpha_op, int *alpha_src, int *alpha_dest));

AL_FUNC(void, _al_put_pixel, (ALLEGRO_BITMAP *bitmap, int x, int y, ALLEGRO_COLOR color));

#ifdef __cplusplus
   }
#endif

#endif
