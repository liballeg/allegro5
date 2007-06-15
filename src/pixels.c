#include "allegro.h"
#include "allegro/bitmap_new.h"
#include "internal/aintern.h"
#include "internal/aintern_bitmap.h"


static int pixel_sizes[] = {
	-1,
	4,
	4,
	2,
	3,
	2,
	2,
	1,
	2,
	2,
	4,
	4
};


int _al_pixel_size(int format)
{
	return pixel_sizes[format];
}


/* Get pixel functions */

void _get_pixel_argb_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	color->r = (float)((pixel & 0x00FF0000) >> 16) / 255.0f;
	color->g = (float)((pixel & 0x0000FF00) >>  8) / 255.0f;
	color->b = (float)((pixel & 0x000000FF) >>  0) / 255.0f;
	color->a = (float)((pixel & 0xFF000000) >> 24) / 255.0f;
}

void _get_pixel_rgba_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	color->r = (float)((pixel & 0xFF000000) >> 24) / 255.0f;
	color->g = (float)((pixel & 0x00FF0000) >> 16) / 255.0f;
	color->b = (float)((pixel & 0x0000FF00) >>  8) / 255.0f;
	color->a = (float)((pixel & 0x000000FF) >>  0) / 255.0f;
}

void _get_pixel_argb_4444(void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	color->r = (float)((pixel & 0x0F00) >>  4) / 255.0f;
	color->g = (float)((pixel & 0x00F0) >>  0) / 255.0f;
	color->b = (float)((pixel & 0x000F) <<  4) / 255.0f;
	color->a = (float)((pixel & 0xF000) >>  8) / 255.0f;
}

void _get_pixel_rgb_888(void *data, AL_COLOR *color)
{
	uint32_t pixel = READ3BYTES(data);	
	color->r = (float)((pixel & 0xFF0000) >> 16) / 255.0f;
	color->g = (float)((pixel & 0x00FF00) >>  8) / 255.0f;
	color->b = (float)((pixel & 0x0000FF) >>  0) / 255.0f;
	color->a = 0.0f;
}

void _get_pixel_rgb_565(void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	color->r = (float)((pixel & 0xF100) >>  8) / 255.0f;
	color->g = (float)((pixel & 0x07E0) >>  3) / 255.0f;
	color->b = (float)((pixel & 0x001F) <<  3) / 255.0f;
	color->a = 0.0f;
}

void _get_pixel_rgb_555(void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	color->r = (float)((pixel & 0x7C00) >>  7) / 255.0f;
	color->g = (float)((pixel & 0x03E0) >>  2) / 255.0f;
	color->b = (float)((pixel & 0x001F) <<  3) / 255.0f;
	color->a = 0.0f;
}

void _get_pixel_palette_8(void *data, AL_COLOR *color)
{
	int pixel = *(unsigned char *)(data);
	color->r = (float)getr8(pixel) / 255.0f;
	color->g = (float)getg8(pixel) / 255.0f;
	color->b = (float)getb8(pixel) / 255.0f;
	color->a = 0.0f;
}

void _get_pixel_rgba_5551(void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	color->r = (float)((pixel & 0xF800) >>  8) / 255.0f;
	color->g = (float)((pixel & 0x07C0) >>  3) / 255.0f;
	color->b = (float)((pixel & 0x003E) <<  2) / 255.0f;
	color->a = 0.0f;
}

void _get_pixel_argb_1555(void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	color->r = (float)((pixel & 0x7C00) >>  7) / 255.0f;
	color->g = (float)((pixel & 0x03E0) >>  2) / 255.0f;
	color->b = (float)((pixel & 0x001F) <<  3) / 255.0f;
	color->a = 0.0f;
}

void _get_pixel_abgr_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	color->r = (float)((pixel & 0x000000FF) >>  0) / 255.0f;
	color->g = (float)((pixel & 0x0000FF00) >>  8) / 255.0f;
	color->b = (float)((pixel & 0x00FF0000) >> 16) / 255.0f;
	color->a = (float)((pixel & 0xFF000000) >> 24) / 255.0f;
}

void _get_pixel_xbgr_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	color->r = (float)((pixel & 0x000000FF) >>  0) / 255.0f;
	color->g = (float)((pixel & 0x0000FF00) >>  8) / 255.0f;
	color->b = (float)((pixel & 0x00FF0000) >> 16) / 255.0f;
	color->a = 0.0f;
}

/* get pixel lookup table */

typedef void (*p_get_pixel_func)(void *, AL_COLOR *);

p_get_pixel_func get_pixel_funcs[] = {
	NULL,
	_get_pixel_argb_8888,
	_get_pixel_rgba_8888,
	_get_pixel_argb_4444,
	_get_pixel_rgb_888,
	_get_pixel_rgb_565,
	_get_pixel_rgb_555,
	_get_pixel_palette_8,
	_get_pixel_rgba_5551,
	_get_pixel_argb_1555,
	_get_pixel_abgr_8888,
	_get_pixel_xbgr_8888
};

AL_COLOR *_al_get_pixel(void *data, int format, AL_COLOR *color) {
	(*get_pixel_funcs[format])(data, color);
	return color;
}

AL_COLOR *al_get_pixel(AL_BITMAP *bitmap,
	int x, int y, AL_COLOR *color)
{
	AL_LOCKED_RECTANGLE lr;

	/* FIXME: must use clip not full bitmap */
	if (x < 0 || y < 0 || x >= bitmap->w || y >= bitmap->h) {
		color->r = 0.0f;
		color->g = 0.0f;
		color->b = 0.0f;
		color->a = 0.0f;
		return color;
	}

	if (!al_lock_bitmap_region(bitmap, x, y, 1, 1, &lr, AL_LOCK_READONLY))
		return color;
	
	/* FIXME: check for valid pixel format */

	_al_get_pixel(lr.data, bitmap->format, color);

	al_unlock_bitmap(bitmap);

	return color;
}

/* put pixel functions */

void _put_pixel_argb_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = 0;
	pixel |= (int)(color->r * 255) << 16;
	pixel |= (int)(color->g * 255) <<  8;
	pixel |= (int)(color->b * 255) <<  0;
	pixel |= (int)(color->a * 255) << 24;

	*(uint32_t *)(data) = pixel;
}

void _put_pixel_rgba_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = 0;
	pixel |= (int)(color->r * 255) << 24;
	pixel |= (int)(color->g * 255) << 16;
	pixel |= (int)(color->b * 255) <<  8;
	pixel |= (int)(color->a * 255) <<  0;

	*(uint32_t *)(data) = pixel;
}

void _put_pixel_argb_4444(void *data, AL_COLOR *color)
{
	uint16_t pixel = 0;
	pixel |= (int)(color->r * 16) <<  8;
	pixel |= (int)(color->g * 16) <<  4;
	pixel |= (int)(color->b * 16) <<  0;
	pixel |= (int)(color->a * 16) << 12;

	*(uint16_t *)(data) = pixel;
}

void _put_pixel_rgb_888(void *data, AL_COLOR *color)
{
	uint32_t pixel = 0;
	pixel |= (int)(color->r * 255) << 16;
	pixel |= (int)(color->g * 255) <<  8;
	pixel |= (int)(color->b * 255) <<  0;

	WRITE3BYTES(data, pixel);
}

void _put_pixel_rgb_565(void *data, AL_COLOR *color)
{
	uint16_t pixel = 0;
	pixel |= (int)(color->r * 0x1F) <<  11;
	pixel |= (int)(color->g * 0x3F) <<   5;
	pixel |= (int)(color->b * 0x1F) <<   0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_rgb_555(void *data, AL_COLOR *color)
{
	uint16_t pixel = 0;
	pixel |= (int)(color->r * 0x1F) << 10;
	pixel |= (int)(color->g * 0x1F) <<  5;
	pixel |= (int)(color->b * 0x1F) <<  0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_palette_8(void *data, AL_COLOR *color)
{
	unsigned char pixel = 0;
	pixel = makecol8((int)(color->r * 255),
		(int)(color->g * 255),
		(int)(color->b * 255));
	*(unsigned char *)(data) = pixel;
}

void _put_pixel_rgba_5551(void *data, AL_COLOR *color)
{
	uint16_t pixel = 0;
	pixel |= (int)(color->r * 0x1F) << 11;
	pixel |= (int)(color->g * 0x1F) <<  6;
	pixel |= (int)(color->b * 0x1F) <<  1;
	pixel |= (int)(color->a *    1) <<  0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_argb_1555(void *data, AL_COLOR *color)
{
	uint16_t pixel = 0;
	pixel |= (int)(color->r * 0x1F) << 10;
	pixel |= (int)(color->g * 0x1F) <<  5;
	pixel |= (int)(color->b * 0x1F) <<  0;
	pixel |= (int)(color->a *    1) << 15;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_abgr_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = 0;
	pixel |= (int)(color->r * 255) <<  0;
	pixel |= (int)(color->g * 255) <<  8;
	pixel |= (int)(color->b * 255) << 16;
	pixel |= (int)(color->a * 255) << 24;
	*(uint32_t *)(data) = pixel;
}

void _put_pixel_xbgr_8888(void *data, AL_COLOR *color)
{
	uint32_t pixel = 0;
	pixel |= (int)(color->r * 255) <<  0;
	pixel |= (int)(color->g * 255) <<  8;
	pixel |= (int)(color->b * 255) << 16;
	*(uint32_t *)(data) = pixel;
}

/* put pixel lookup table */

typedef void (*p_put_pixel_func)(void *data, AL_COLOR *);

p_put_pixel_func put_pixel_funcs[] = {
	NULL,
	_put_pixel_argb_8888,
	_put_pixel_rgba_8888,
	_put_pixel_argb_4444,
	_put_pixel_rgb_888,
	_put_pixel_rgb_565,
	_put_pixel_rgb_555,
	_put_pixel_palette_8,
	_put_pixel_rgba_5551,
	_put_pixel_argb_1555,
	_put_pixel_abgr_8888,
	_put_pixel_xbgr_8888
};

void _al_put_pixel(void *data, int format, AL_COLOR *color)
{
	(*put_pixel_funcs[format])(data, color);
}

void al_put_pixel(AL_BITMAP *bitmap, int x, int y, AL_COLOR *color)
{
	AL_LOCKED_RECTANGLE lr;

	/* FIXME: must use clip not full bitmap */
	if (x < 0 || y < 0 || x >= bitmap->w || y >= bitmap->h) {
		return;
	}

	if (!al_lock_bitmap_region(bitmap, x, y, 1, 1, &lr, 0))
		return;

	/* FIXME: check for valid pixel format */

	_al_put_pixel(lr.data, bitmap->format, color);

	al_unlock_bitmap(bitmap);
}

