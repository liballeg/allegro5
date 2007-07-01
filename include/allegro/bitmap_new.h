#ifndef ALLEGRO_BITMAP_NEW_H
#define ALLEGRO_BITMAP_NEW_H

#include "allegro/color_new.h"

typedef struct AL_BITMAP AL_BITMAP;

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


#define NUM_PIXEL_FORMATS 17


/*
 * Pixel formats
 */
enum ALLEGRO_PIXEL_FORMAT {
	ALLEGRO_PIXEL_FORMAT_ANY = 0,
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
	ALLEGRO_PIXEL_FORMAT_XRGB_8888
	/*ALLEGRO_PIXEL_FORMAT_RGBA_32F*/
};


/*
 * Bitmap flags
 */
#define AL_MEMORY_BITMAP     0x0001
#define AL_SYNC_MEMORY_COPY  0x0002
#define AL_NO_ALPHA          0x0004
#define AL_USE_ALPHA         0x0008


enum ALLEGRO_PATTERN_MODE {
   AL_PATTERN_SOLID = 0,
   AL_PATTERN_XOR,
   AL_PATTERN_COPY_PATTERN,
   AL_PATTERN_SOLID_PATTERN,
   AL_PATTERN_MASKED_PATTERN
};


/*
 * Locking flags
 */
#define AL_LOCK_READONLY 1

typedef struct AL_LOCKED_REGION {
	void *data;
	int format;
	int pitch;
} AL_LOCKED_REGION;


void al_set_new_bitmap_format(int format);
void al_set_new_bitmap_flags(int flags);
int al_get_new_bitmap_format(void);
int al_get_new_bitmap_flags(void);

int al_get_bitmap_width(AL_BITMAP *bitmap);
int al_get_bitmap_height(AL_BITMAP *bitmap);
int al_get_bitmap_format(AL_BITMAP *bitmap);
int al_get_bitmap_flags(AL_BITMAP *bitmap);

AL_BITMAP *al_create_bitmap(int w, int h);
AL_BITMAP *al_load_bitmap(char const *filename);
void al_destroy_bitmap(AL_BITMAP *bitmap);

/* Blitting */
void al_draw_bitmap(AL_BITMAP *bitmap, float dx, float dy, int flags);
void al_draw_bitmap_region(AL_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, int flags);
void al_draw_scaled_bitmap(AL_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, float dw, float dh, int flags);
void al_draw_rotated_bitmap(AL_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float angle, int flags);
void al_draw_rotated_scaled_bitmap(AL_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float xscale, float yscale, float angle,
	int flags);

AL_LOCKED_REGION *al_lock_bitmap(AL_BITMAP *bitmap,
	AL_LOCKED_REGION *locked_region, int flags);
AL_LOCKED_REGION *al_lock_bitmap_region(AL_BITMAP *bitmap,
	int x, int y, int width, int height,
	AL_LOCKED_REGION *locked_region,
	int flags);
void al_unlock_bitmap(AL_BITMAP *bitmap);

void al_put_pixel(int x, int y, AL_COLOR *color, int flags);
AL_COLOR *al_get_pixel(AL_BITMAP *bitmap, int x, int y, AL_COLOR *color);
int al_get_pixel_size(int format);

/* Pixel mapping */
AL_COLOR *al_map_rgb(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b);
AL_COLOR *al_map_rgba(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a);
AL_COLOR *al_map_rgb_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float r, float g, float b);
AL_COLOR *al_map_rgba_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float r, float g, float b, float a);
AL_COLOR *al_map_rgb_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int r, int g, int b);
AL_COLOR *al_map_rgba_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int r, int g, int b, int a);

/* Pixel unmapping */
void al_unmap_rgb(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b);
void al_unmap_rgba(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);
void al_unmap_rgb_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float *r, float *g, float *b);
void al_unmap_rgba_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float *r, float *g, float *b, float *a);
void al_unmap_rgb_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int *r, int *g, int *b);
void al_unmap_rgba_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int *r, int *g, int *b, int *a);

/* Masking */
void al_set_mask_color(AL_COLOR *color);
AL_COLOR *al_get_mask_color(AL_COLOR *color);
void al_convert_mask_to_alpha(AL_BITMAP *bitmap, AL_COLOR *mask_color);

/* Clipping */
void al_set_bitmap_clip(AL_BITMAP *bitmap, int x, int y,
   int width, int height);
void al_get_bitmap_clip(AL_BITMAP *bitmap, int *x, int *y,
   int *w, int *h);
void al_enable_bitmap_clip(AL_BITMAP *bitmap, bool enable);
bool al_is_bitmap_clip_enabled(AL_BITMAP *bitmap);

/* Sub bitmaps */
AL_BITMAP *al_create_sub_bitmap(AL_BITMAP *parent,
   int x, int y, int w, int h);

/* Pattern drawing */
bool al_set_drawing_pattern(AL_BITMAP *bitmap, int mode, AL_BITMAP *pattern,
   int anchor_x, int anchor_y);
void al_get_drawing_pattern(AL_BITMAP *bitmap,
   int *mode, AL_BITMAP **pattern, int *anchor_x, int *anchor_y);

#endif
