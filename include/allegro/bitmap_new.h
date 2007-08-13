#ifndef ALLEGRO_BITMAP_NEW_H
#define ALLEGRO_BITMAP_NEW_H

#include "allegro/color_new.h"

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
#define ALLEGRO_MEMORY_BITMAP     0x0001
#define ALLEGRO_SYNC_MEMORY_COPY  0x0002
#define ALLEGRO_NO_ALPHA          0x0004
#define ALLEGRO_USE_ALPHA         0x0008


/* Flags for the blitting functions */
#define ALLEGRO_MASK_SOURCE           0x00001
#define ALLEGRO_FLIP_HORIZONTAL       0x00002
#define ALLEGRO_FLIP_VERTICAL         0x00004
/* Flags for primitives */
#define ALLEGRO_OUTLINED              0x00000
#define ALLEGRO_FILLED                0x00008


/*
 * Locking flags
 */
#define ALLEGRO_LOCK_READONLY 1
#define ALLEGRO_LOCK_WRITEONLY 2

typedef struct ALLEGRO_LOCKED_REGION {
	void *data;
	int format;
	int pitch;
} ALLEGRO_LOCKED_REGION;


void al_set_new_bitmap_format(int format);
void al_set_new_bitmap_flags(int flags);
int al_get_new_bitmap_format(void);
int al_get_new_bitmap_flags(void);

int al_get_bitmap_width(ALLEGRO_BITMAP *bitmap);
int al_get_bitmap_height(ALLEGRO_BITMAP *bitmap);
int al_get_bitmap_format(ALLEGRO_BITMAP *bitmap);
int al_get_bitmap_flags(ALLEGRO_BITMAP *bitmap);

ALLEGRO_BITMAP *al_create_bitmap(int w, int h);
ALLEGRO_BITMAP *al_load_bitmap(char const *filename);
void al_destroy_bitmap(ALLEGRO_BITMAP *bitmap);

/* Blitting */
void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags);
void al_draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, int flags);
void al_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, float dw, float dh, int flags);
void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float angle, int flags);
void al_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float xscale, float yscale, float angle,
	int flags);

ALLEGRO_LOCKED_REGION *al_lock_bitmap(ALLEGRO_BITMAP *bitmap,
	ALLEGRO_LOCKED_REGION *locked_region, int flags);
ALLEGRO_LOCKED_REGION *al_lock_bitmap_region(ALLEGRO_BITMAP *bitmap,
	int x, int y, int width, int height,
	ALLEGRO_LOCKED_REGION *locked_region,
	int flags);
void al_unlock_bitmap(ALLEGRO_BITMAP *bitmap);

void al_put_pixel(int x, int y, ALLEGRO_COLOR *color, int flags);
ALLEGRO_COLOR *al_get_pixel(ALLEGRO_BITMAP *bitmap, int x, int y, ALLEGRO_COLOR *color);
int al_get_pixel_size(int format);

/* Pixel mapping */
ALLEGRO_COLOR *al_map_rgb(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b);
ALLEGRO_COLOR *al_map_rgba(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a);
ALLEGRO_COLOR *al_map_rgb_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float r, float g, float b);
ALLEGRO_COLOR *al_map_rgba_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float r, float g, float b, float a);
ALLEGRO_COLOR *al_map_rgb_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int r, int g, int b);
ALLEGRO_COLOR *al_map_rgba_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int r, int g, int b, int a);

/* Pixel unmapping */
void al_unmap_rgb(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b);
void al_unmap_rgba(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);
void al_unmap_rgb_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float *r, float *g, float *b);
void al_unmap_rgba_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float *r, float *g, float *b, float *a);
void al_unmap_rgb_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int *r, int *g, int *b);
void al_unmap_rgba_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int *r, int *g, int *b, int *a);

/* Masking */
void al_set_mask_color(ALLEGRO_COLOR *color);
ALLEGRO_COLOR *al_get_mask_color(ALLEGRO_COLOR *color);
void al_convert_mask_to_alpha(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *mask_color);

/* Clipping */
void al_set_clipping_rectangle(int x, int y, int width, int height);
void al_get_clipping_rectangle(int *x, int *y, int *w, int *h);

/* Sub bitmaps */
ALLEGRO_BITMAP *al_create_sub_bitmap(ALLEGRO_BITMAP *parent,
   int x, int y, int w, int h);

#endif
