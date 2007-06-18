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
	/* Allegro 4.x formats */
	ALLEGRO_PIXEL_FORMAT_ABGR_8888,
	ALLEGRO_PIXEL_FORMAT_XBGR_8888
	ALLEGRO_PIXEL_FORMAT_BGR_888,
	ALLEGRO_PIXEL_FORMAT_BGR_565,
	ALLEGRO_PIXEL_FORMAT_BGR_555
	/* End Allegro 4.x formats */
	/*ALLEGRO_PIXEL_FORMAT_RGBA_32F*/
};

/*
 * Bitmap flags
 */
#define AL_MEMORY_BITMAP     0x0001
#define AL_SYNC_MEMORY_COPY  0x0002
#define AL_NO_ALPHA          0x0004
#define AL_HAS_ALPHA         0x0008

/*
 * Locking flags
 */
#define AL_LOCK_READONLY 1

typedef struct AL_LOCKED_RECTANGLE {
	void *data;
	int format;
	int pitch;
} AL_LOCKED_RECTANGLE;


void al_set_bitmap_parameters(int format, int flags);
void al_get_bitmap_parameters(int *format, int *flags);
AL_BITMAP *al_create_bitmap(int w, int h);
//AL_BITMAP *al_create_memory_bitmap(int w, int h, int flags);
//void al_destroy_memory_bitmap(AL_BITMAP *bitmap);
AL_BITMAP *al_load_bitmap(char const *filename);
void al_destroy_bitmap(AL_BITMAP *bitmap);
//void al_draw_bitmap(int flag, AL_BITMAP *bitmap, float x, float y);
void al_draw_bitmap(AL_BITMAP *bitmap, float x, float y, int flags);
/*
void al_draw_sub_bitmap(AL_BITMAP *bitmap, float x, float y,
    float sx, float sy, float sw, float sh);
void al_blit(int flag, AL_BITMAP *src, AL_BITMAP *dest,
	float dx, float dy);
void al_blit_region(int flag, AL_BITMAP *src, float sx, float sy,
	AL_BITMAP *dest, float dx, float dy, float w, float h);
void al_blit_scaled(int flag,
	AL_BITMAP *src,	float sx, float sy, float sw, float sh,
	AL_BITMAP *dest, float dx, float dy, float dw, float dh);
void al_rotate_bitmap(int flag, AL_BITMAP *source,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float angle);
void al_rotate_scaled(int flag, AL_BITMAP *source,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float xscale, float yscale,
	float angle);
void al_blit_region_3(int flag,
	AL_BITMAP *source1, int source1_x, int source1_y,
	AL_BITMAP *source2, int source2_x, int source2_y,
	AL_BITMAP *dest, int dest_x, int dest_y,
	int dest_w, int dest_h);
void al_set_light_color(AL_BITMAP *bitmap, AL_COLOR *light_color);
*/
AL_LOCKED_RECTANGLE *al_lock_bitmap(AL_BITMAP *bitmap,
	AL_LOCKED_RECTANGLE *locked_rectangle, int flags);
AL_LOCKED_RECTANGLE *al_lock_bitmap_region(AL_BITMAP *bitmap,
	int x, int y, int width, int height,
	AL_LOCKED_RECTANGLE *locked_rectangle,
	int flags);
void al_unlock_bitmap(AL_BITMAP *bitmap);

void _al_put_pixel(void *data, int format, AL_COLOR *color);
AL_COLOR *_al_get_pixel(void *data, int format, AL_COLOR *color);

void al_put_pixel(AL_BITMAP *bitmap, int x, int y, AL_COLOR *color);
AL_COLOR *al_get_pixel(AL_BITMAP *bitmap, int x, int y, AL_COLOR *color);

void _al_convert_bitmap_data(
	void *src, int src_format, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
void _al_convert_compat_bitmap(
	BITMAP *src,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
int _al_pixel_size(int format);

#endif
