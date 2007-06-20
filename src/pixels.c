#include "allegro.h"
#include "allegro/bitmap_new.h"
#include "internal/aintern.h"
#include "internal/aintern_bitmap.h"


static int pixel_sizes[] = {
	0,
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
	4,
	3,
	2,
	2,
	4
};


int _al_pixel_size(int format)
{
	return pixel_sizes[format];
}

/* map_rgba functions */

static AL_COLOR *_map_rgba_argb_8888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = a;
	p->raw[1] = r;
	p->raw[2] = g;
	p->raw[3] = b;
	return p;
}

static AL_COLOR *_map_rgba_rgba_8888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = r;
	p->raw[1] = g;
	p->raw[2] = b;
	p->raw[3] = a;
	return p;
}

static AL_COLOR *_map_rgba_argb_4444(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = a >> 4;
	p->raw[1] = r >> 4;
	p->raw[2] = g >> 4;
	p->raw[3] = b >> 4;
	return p;
}

static AL_COLOR *_map_rgba_rgb_888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = r;
	p->raw[1] = g;
	p->raw[2] = b;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_rgb_565(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = r >> 3;
	p->raw[1] = g >> 2;
	p->raw[2] = b >> 3;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_rgb_555(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = r >> 3;
	p->raw[1] = g >> 3;
	p->raw[2] = b >> 3;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_palette_8(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = makecol8(r, g, b);
	p->raw[1] = p->raw[2] = p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_rgba_5551(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = r >> 3;
	p->raw[1] = g >> 3;
	p->raw[2] = b >> 3;
	p->raw[3] = a >> 7;
	return p;
}

static AL_COLOR *_map_rgba_argb_1555(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = a >> 7;
	p->raw[1] = r >> 3;
	p->raw[2] = g >> 3;
	p->raw[3] = b >> 3;
	return p;
}

static AL_COLOR *_map_rgba_abgr_8888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = a;
	p->raw[1] = b;
	p->raw[2] = g;
	p->raw[3] = r;
	return p;
}

static AL_COLOR *_map_rgba_xbgr_8888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = 0;
	p->raw[1] = b;
	p->raw[2] = g;
	p->raw[3] = r;
	return p;
}

static AL_COLOR *_map_rgba_bgr_888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = b;
	p->raw[1] = g;
	p->raw[2] = r;
	p->raw[0] = 0;
	return p;
}

static AL_COLOR *_map_rgba_bgr_565(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = b >> 3;
	p->raw[1] = g >> 2;
	p->raw[2] = r >> 3;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_bgr_555(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = b >> 3;
	p->raw[1] = g >> 3;
	p->raw[2] = r >> 3;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_rgbx_8888(AL_COLOR *p,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	p->raw[0] = r;
	p->raw[1] = g;
	p->raw[2] = b;
	p->raw[3] = 0;
	return p;
}

/* map_rgba map */

typedef AL_COLOR *(*_map_rgba_func)(AL_COLOR *,
	unsigned char, unsigned char, unsigned char, unsigned char);

static _map_rgba_func _map_rgba_funcs[] = {
	NULL,
	_map_rgba_argb_8888,
	_map_rgba_rgba_8888,
	_map_rgba_argb_4444,
	_map_rgba_rgb_888,
	_map_rgba_rgb_565,
	_map_rgba_rgb_555,
	_map_rgba_palette_8,
	_map_rgba_rgba_5551,
	_map_rgba_argb_1555,
	_map_rgba_abgr_8888,
	_map_rgba_xbgr_8888,
	_map_rgba_bgr_888,
	_map_rgba_bgr_565,
	_map_rgba_bgr_555,
	_map_rgba_rgbx_8888
};

AL_COLOR* _al_map_rgba(int format, AL_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return (*_map_rgba_funcs[format])(color, r, g, b, a);
}

AL_COLOR* al_map_rgba(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return _al_map_rgba(bitmap->format, color, r, g, b, a);
}

AL_COLOR* al_map_rgb(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b)
{
	return _al_map_rgba(bitmap->format, color, r, g, b, 255);
}

/* map_rgba_f functions */

static AL_COLOR *_map_rgba_f_argb_8888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(a * 0xFF);
	p->raw[1] = (uint64_t)(r * 0xFF);
	p->raw[2] = (uint64_t)(g * 0xFF);
	p->raw[3] = (uint64_t)(b * 0xFF);
	return p;
}

static AL_COLOR *_map_rgba_f_rgba_8888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(r * 0xFF);
	p->raw[1] = (uint64_t)(g * 0xFF);
	p->raw[2] = (uint64_t)(b * 0xFF);
	p->raw[3] = (uint64_t)(a * 0xFF);
	return p;
}

static AL_COLOR *_map_rgba_f_argb_4444(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(a * 0xF);
	p->raw[1] = (uint64_t)(r * 0xF);
	p->raw[2] = (uint64_t)(g * 0xF);
	p->raw[3] = (uint64_t)(b * 0xF);
	return p;
}

static AL_COLOR *_map_rgba_f_rgb_888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(r * 0xFF);
	p->raw[1] = (uint64_t)(g * 0xFF);
	p->raw[2] = (uint64_t)(b * 0xFF);
	p->raw[3] = (uint64_t)(0);
	return p;
}

static AL_COLOR *_map_rgba_f_rgb_565(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(r * 0x1F);
	p->raw[1] = (uint64_t)(g * 0x3F);
	p->raw[2] = (uint64_t)(b * 0x1F);
	p->raw[3] = (uint64_t)(0);
	return p;
}

static AL_COLOR *_map_rgba_f_rgb_555(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(r * 0x1F);
	p->raw[1] = (uint64_t)(g * 0x1F);
	p->raw[2] = (uint64_t)(b * 0x1F);
	p->raw[3] = (uint64_t)(0);
	return p;
}

static AL_COLOR *_map_rgba_f_palette_8(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(makecol8(r*255, g*255, b*255));
	p->raw[1] = (uint64_t)(p->raw[2] = (uint64_t)p->raw[3] = (uint64_t)0);
	return p;
}

static AL_COLOR *_map_rgba_f_rgba_5551(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(r * 0x1F);
	p->raw[1] = (uint64_t)(g * 0x1F);
	p->raw[2] = (uint64_t)(b * 0x1F);
	p->raw[3] = (uint64_t)(a);
	return p;
}

static AL_COLOR *_map_rgba_f_argb_1555(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(a);
	p->raw[1] = (uint64_t)(r * 0x1F);
	p->raw[2] = (uint64_t)(g * 0x1F);
	p->raw[3] = (uint64_t)(b * 0x1F);
	return p;
}

static AL_COLOR *_map_rgba_f_abgr_8888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(a * 0xFF);
	p->raw[1] = (uint64_t)(b * 0xFF);
	p->raw[2] = (uint64_t)(g * 0xFF);
	p->raw[3] = (uint64_t)(r * 0xFF);
	return p;
}

static AL_COLOR *_map_rgba_f_xbgr_8888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(0);
	p->raw[1] = (uint64_t)(b * 0xFF);
	p->raw[2] = (uint64_t)(g * 0xFF);
	p->raw[3] = (uint64_t)(r * 0xFF);
	return p;
}

static AL_COLOR *_map_rgba_f_bgr_888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(b * 0xFF);
	p->raw[1] = (uint64_t)(g * 0xFF);
	p->raw[2] = (uint64_t)(r * 0xFF);
	p->raw[3] = (uint64_t)(0);
	return p;
}

static AL_COLOR *_map_rgba_f_bgr_565(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(b * 0x1F);
	p->raw[1] = (uint64_t)(g * 0x3F);
	p->raw[2] = (uint64_t)(r * 0x1F);
	p->raw[3] = (uint64_t)(0);
	return p;
}

static AL_COLOR *_map_rgba_f_bgr_555(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(b * 0x1F);
	p->raw[1] = (uint64_t)(g * 0x1F);
	p->raw[2] = (uint64_t)(r * 0x1F);
	p->raw[3] = (uint64_t)(0);
	return p;
}

static AL_COLOR *_map_rgba_f_rgbx_8888(AL_COLOR *p,
	float r, float g, float b, float a)
{
	p->raw[0] = (uint64_t)(r * 0xFF);
	p->raw[1] = (uint64_t)(g * 0xFF);
	p->raw[2] = (uint64_t)(b * 0xFF);
	p->raw[3] = (uint64_t)(0);
	return p;
}

/* map_rgba_f map */

typedef AL_COLOR *(*_map_rgba_f_func)(AL_COLOR *, float, float, float, float);

static _map_rgba_f_func _map_rgba_f_funcs[] = {
	NULL,
	_map_rgba_f_argb_8888,
	_map_rgba_f_rgba_8888,
	_map_rgba_f_argb_4444,
	_map_rgba_f_rgb_888,
	_map_rgba_f_rgb_565,
	_map_rgba_f_rgb_555,
	_map_rgba_f_palette_8,
	_map_rgba_f_rgba_5551,
	_map_rgba_f_argb_1555,
	_map_rgba_f_abgr_8888,
	_map_rgba_f_xbgr_8888,
	_map_rgba_f_bgr_888,
	_map_rgba_f_bgr_565,
	_map_rgba_f_bgr_555,
	_map_rgba_f_rgbx_8888
};

AL_COLOR *_al_map_rgba_f(int format, AL_COLOR *color,
	float r, float g, float b, float a)
{
	return (*_map_rgba_f_funcs[format])(color, r, g, b, a);
}

AL_COLOR *al_map_rgba_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float r, float g, float b, float a)
{
	return _al_map_rgba_f(bitmap->format, color, r, g, b, a);
}

AL_COLOR *al_map_rgb_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float r, float g, float b)
{
	return _al_map_rgba_f(bitmap->format, color, r, g, b, 1.0f);
}

/* map_rgba_i functions */

const int int_to_8_bit_shift = (sizeof(int) * 8) - 8 - 1;
const int int_to_6_bit_shift = (sizeof(int) * 8) - 6 - 1;
const int int_to_5_bit_shift = (sizeof(int) * 8) - 5 - 1;
const int int_to_4_bit_shift = (sizeof(int) * 8) - 4 - 1;
const int int_to_1_bit_shift = (sizeof(int) * 8) - 1 - 1;

static AL_COLOR *_map_rgba_i_argb_8888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = a >> int_to_8_bit_shift;
	p->raw[1] = r >> int_to_8_bit_shift;
	p->raw[2] = g >> int_to_8_bit_shift;
	p->raw[3] = b >> int_to_8_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_rgba_8888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = r >> int_to_8_bit_shift;
	p->raw[1] = g >> int_to_8_bit_shift;
	p->raw[2] = b >> int_to_8_bit_shift;
	p->raw[3] = a >> int_to_8_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_argb_4444(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = a >> int_to_4_bit_shift;
	p->raw[1] = r >> int_to_4_bit_shift;
	p->raw[2] = g >> int_to_4_bit_shift;
	p->raw[3] = b >> int_to_4_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_rgb_888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = r >> int_to_8_bit_shift;
	p->raw[1] = g >> int_to_8_bit_shift;
	p->raw[2] = b >> int_to_8_bit_shift;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_rgb_565(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = r >> int_to_5_bit_shift;
	p->raw[1] = g >> int_to_6_bit_shift;
	p->raw[2] = b >> int_to_5_bit_shift;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_rgb_555(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = r >> int_to_5_bit_shift;
	p->raw[1] = g >> int_to_5_bit_shift;
	p->raw[2] = b >> int_to_5_bit_shift;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_palette_8(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = makecol8(
		r >> int_to_8_bit_shift,
		g >> int_to_8_bit_shift,
		b >> int_to_8_bit_shift);
	p->raw[1] = p->raw[2] = p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_rgba_5551(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = r >> int_to_5_bit_shift;
	p->raw[1] = g >> int_to_5_bit_shift;
	p->raw[2] = b >> int_to_5_bit_shift;
	p->raw[3] = a >> int_to_1_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_argb_1555(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = a >> int_to_1_bit_shift;
	p->raw[1] = r >> int_to_5_bit_shift;
	p->raw[2] = g >> int_to_5_bit_shift;
	p->raw[3] = b >> int_to_5_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_abgr_8888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = a >> int_to_8_bit_shift;
	p->raw[1] = b >> int_to_8_bit_shift;
	p->raw[2] = g >> int_to_8_bit_shift;
	p->raw[3] = r >> int_to_8_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_xbgr_8888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = 0;
	p->raw[1] = b >> int_to_8_bit_shift;
	p->raw[2] = g >> int_to_8_bit_shift;
	p->raw[3] = r >> int_to_8_bit_shift;
	return p;
}

static AL_COLOR *_map_rgba_i_bgr_888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = b >> int_to_8_bit_shift;
	p->raw[1] = g >> int_to_8_bit_shift;
	p->raw[2] = r >> int_to_8_bit_shift;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_bgr_565(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = b >> int_to_5_bit_shift;
	p->raw[1] = g >> int_to_6_bit_shift;
	p->raw[2] = r >> int_to_5_bit_shift;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_bgr_555(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = b >> int_to_5_bit_shift;
	p->raw[1] = g >> int_to_5_bit_shift;
	p->raw[2] = r >> int_to_5_bit_shift;
	p->raw[3] = 0;
	return p;
}

static AL_COLOR *_map_rgba_i_rgbx_8888(AL_COLOR *p,
	int r, int g, int b, int a)
{
	p->raw[0] = r >> int_to_8_bit_shift;
	p->raw[1] = g >> int_to_8_bit_shift;
	p->raw[2] = b >> int_to_8_bit_shift;
	p->raw[3] = 0;
	return p;
}

/* map_rgba_i map */

typedef AL_COLOR *(*_map_rgba_i_func)(AL_COLOR *, int, int, int, int);

static _map_rgba_i_func _map_rgba_i_funcs[] = {
	NULL,
	_map_rgba_i_argb_8888,
	_map_rgba_i_rgba_8888,
	_map_rgba_i_argb_4444,
	_map_rgba_i_rgb_888,
	_map_rgba_i_rgb_565,
	_map_rgba_i_rgb_555,
	_map_rgba_i_palette_8,
	_map_rgba_i_rgba_5551,
	_map_rgba_i_argb_1555,
	_map_rgba_i_abgr_8888,
	_map_rgba_i_xbgr_8888,
	_map_rgba_i_bgr_888,
	_map_rgba_i_bgr_565,
	_map_rgba_i_bgr_555,
	_map_rgba_i_rgbx_8888
};

AL_COLOR *_al_map_rgba_i(int format, AL_COLOR *color,
	int r, int g, int b, int a)
{
	return (*_map_rgba_i_funcs[format])(color, r, g, b, a);
}

AL_COLOR *al_map_rgba_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int r, int g, int b, int a)
{
	return _al_map_rgba_i(bitmap->format, color, r, g, b, a);
}

AL_COLOR *al_map_rgb_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int r, int g, int b)
{
	return _al_map_rgba_i(bitmap->format, color, r, g, b, 1.0f);
}

/* Get pixel functions */

void _get_pixel_argb_8888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	al_map_rgba(bitmap, color,
		(pixel & 0x00FF0000) >> 16,
		(pixel & 0x0000FF00) >>  8,
		(pixel & 0x000000FF) >>  0,
		(pixel & 0xFF000000) >> 24);
}

void _get_pixel_rgba_8888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	al_map_rgba(bitmap, color,
		(pixel & 0xFF000000) >> 24,
		(pixel & 0x00FF0000) >> 16,
		(pixel & 0x0000FF00) >>  8,
		(pixel & 0x000000FF) >>  0);
}

void _get_pixel_argb_4444(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_4[(pixel & 0x0F00) >> 8],
		_rgb_scale_4[(pixel & 0x00F0) >> 4],
		_rgb_scale_4[(pixel & 0x000F)],
		(pixel & 0xF000) >>  8);
}

void _get_pixel_rgb_888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = READ3BYTES(data);	
	al_map_rgba(bitmap, color,
		(pixel & 0xFF0000) >> 16,
		(pixel & 0x00FF00) >>  8,
		(pixel & 0x0000FF) >>  0,
		0.0f);
}

void _get_pixel_rgb_565(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_5[(pixel & 0xF800) >> 11],
		_rgb_scale_6[(pixel & 0x07E0) >> 5],
		_rgb_scale_5[(pixel & 0x001F)],
		0.0f);
}

void _get_pixel_rgb_555(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_5[(pixel & 0x7C00) >> 10],
		_rgb_scale_5[(pixel & 0x03E0) >> 5],
		_rgb_scale_5[(pixel & 0x001F)],
		0.0f);
}

void _get_pixel_palette_8(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	int pixel = *(unsigned char *)(data);
	al_map_rgba(bitmap, color,
		getr8(pixel),
		getg8(pixel),
		getb8(pixel),
		0.0f);
}

void _get_pixel_rgba_5551(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_5[(pixel & 0xF800) >> 11],
		_rgb_scale_5[(pixel & 0x07C0) >> 6],
		_rgb_scale_5[(pixel & 0x003E) >> 1],
		0.0f);
}

void _get_pixel_argb_1555(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_5[(pixel & 0x7C00) >> 10],
		_rgb_scale_5[(pixel & 0x03E0) >> 5],
		_rgb_scale_5[(pixel & 0x001F)],
		0.0f);
}

void _get_pixel_abgr_8888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	al_map_rgba(bitmap, color,
		(pixel & 0x000000FF) >>  0,
		(pixel & 0x0000FF00) >>  8,
		(pixel & 0x00FF0000) >> 16,
		(pixel & 0xFF000000) >> 24);
}

void _get_pixel_xbgr_8888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	al_map_rgba(bitmap, color,
		(pixel & 0x000000FF) >>  0,
		(pixel & 0x0000FF00) >>  8,
		(pixel & 0x00FF0000) >> 16,
		0.0f);
}

void _get_pixel_bgr_888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = READ3BYTES(data);
	al_map_rgba(bitmap, color,
		(pixel & 0x000000FF) >>  0,
		(pixel & 0x0000FF00) >>  8,
		(pixel & 0x00FF0000) >> 16,
		0.0f);
}

void _get_pixel_bgr_565(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_5[(pixel & 0x001F)],
		_rgb_scale_5[(pixel & 0x07E0) >> 5],
		_rgb_scale_5[(pixel & 0xF800) >> 11],
		0.0f);
}

void _get_pixel_bgr_555(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint16_t pixel = *(uint16_t *)(data);
	al_map_rgba(bitmap, color,
		_rgb_scale_5[(pixel & 0x001F)],
		_rgb_scale_5[(pixel & 0x03E0) >> 5],
		_rgb_scale_5[(pixel & 0x7C00) >> 10],
		0.0f);
}

void _get_pixel_rgbx_8888(AL_BITMAP *bitmap, void *data, AL_COLOR *color)
{
	uint32_t pixel = *(uint32_t *)(data);
	al_map_rgba(bitmap, color,
		(pixel & 0xFF000000) >> 24,
		(pixel & 0x00FF0000) >> 16,
		(pixel & 0x0000FF00) >>  8,
		0.0f);
}

/* get pixel lookup table */

typedef void (*p_get_pixel_func)(AL_BITMAP *, void *, AL_COLOR *);

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
	_get_pixel_xbgr_8888,
	_get_pixel_bgr_888,
	_get_pixel_bgr_565,
	_get_pixel_bgr_555,
	_get_pixel_rgbx_8888
};

AL_COLOR *_al_get_pixel(AL_BITMAP *bitmap, void *data, AL_COLOR *color) {
	(*get_pixel_funcs[bitmap->format])(bitmap, data, color);
	return color;
}

AL_COLOR *al_get_pixel(int x, int y, AL_COLOR *color)
{
	AL_LOCKED_RECTANGLE lr;
	AL_BITMAP *bitmap = al_get_target_bitmap();

	if (bitmap->locked) {
		x -= bitmap->lock_x;
		y -= bitmap->lock_y;
		if (x < 0 || y < 0 || x > bitmap->lock_w || y > bitmap->lock_h) {
			TRACE("al_get_pixel out of bounds\n");
			memset(color, 0, sizeof(AL_COLOR));
			return color;
		}

		_al_get_pixel(bitmap,
			bitmap->locked_rectangle.data+y*bitmap->locked_rectangle.pitch+x*_al_pixel_size(bitmap->format),
			color);
	}
	else {
		/* FIXME: must use clip not full bitmap */
		if (x < 0 || y < 0 || x >= bitmap->w || y >= bitmap->h) {
			memset(color, 0, sizeof(AL_COLOR));
			return color;
		}

		if (!al_lock_bitmap_region(bitmap, x, y, 1, 1, &lr, AL_LOCK_READONLY)) {
			memset(color, 0, sizeof(AL_COLOR));
			return color;
		}
		
		/* FIXME: check for valid pixel format */

		_al_get_pixel(bitmap, lr.data, color);

		al_unlock_bitmap(bitmap);
	}

	return color;
}

void _put_pixel_argb_8888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 24;
	pixel |= p->raw[1] << 16;
	pixel |= p->raw[2] <<  8;
	pixel |= p->raw[3] <<  0;

	*(uint32_t *)(data) = pixel;
}

void _put_pixel_rgba_8888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 24;
	pixel |= p->raw[1] << 16;
	pixel |= p->raw[2] <<  8;
	pixel |= p->raw[3] <<  0;

	*(uint32_t *)(data) = pixel;
}

void _put_pixel_argb_4444(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] << 12;
	pixel |= p->raw[1] <<  8;
	pixel |= p->raw[2] <<  4;
	pixel |= p->raw[3] <<  0;

	*(uint16_t *)(data) = pixel;
}

void _put_pixel_rgb_888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 16;
	pixel |= p->raw[1] <<  8;
	pixel |= p->raw[2] <<  0;

	WRITE3BYTES(data, pixel);
}

void _put_pixel_rgb_565(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] <<  11;
	pixel |= p->raw[1] <<   5;
	pixel |= p->raw[2] <<   0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_rgb_555(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] << 10;
	pixel |= p->raw[1] <<  5;
	pixel |= p->raw[2] <<  0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_palette_8(void *data, AL_COLOR *p)
{
	*(unsigned char *)(data) = p->raw[0];
}

void _put_pixel_rgba_5551(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] <<  0;
	pixel |= p->raw[1] << 11;
	pixel |= p->raw[2] <<  6;
	pixel |= p->raw[3] <<  1;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_argb_1555(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] << 15;
	pixel |= p->raw[1] << 10;
	pixel |= p->raw[2] <<  5;
	pixel |= p->raw[3] <<  0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_abgr_8888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 24;
	pixel |= p->raw[1] << 16;
	pixel |= p->raw[2] <<  8;
	pixel |= p->raw[3] <<  0;
	*(uint32_t *)(data) = pixel;
}

void _put_pixel_xbgr_8888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 16;
	pixel |= p->raw[1] <<  8;
	pixel |= p->raw[2] <<  0;
	*(uint32_t *)(data) = pixel;
}

void _put_pixel_bgr_888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 16;
	pixel |= p->raw[1] <<  8;
	pixel |= p->raw[2] <<  0;
	WRITE3BYTES(data, pixel);
}

void _put_pixel_bgr_565(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] <<  11;
	pixel |= p->raw[1] <<   5;
	pixel |= p->raw[2] <<   0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_bgr_555(void *data, AL_COLOR *p)
{
	uint16_t pixel = 0;
	pixel |= p->raw[0] <<  10;
	pixel |= p->raw[1] <<   5;
	pixel |= p->raw[2] <<   0;
	*(uint16_t *)(data) = pixel;
}

void _put_pixel_rgbx_8888(void *data, AL_COLOR *p)
{
	uint32_t pixel = 0;
	pixel |= p->raw[0] << 24;
	pixel |= p->raw[1] << 16;
	pixel |= p->raw[2] <<  8;

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
	_put_pixel_xbgr_8888,
	_put_pixel_bgr_888,
	_put_pixel_bgr_565,
	_put_pixel_bgr_555,
	_put_pixel_rgbx_8888
};

void _al_put_pixel(void *data, int format, AL_COLOR *color)
{
	(*put_pixel_funcs[format])(data, color);
}

void al_put_pixel(AL_COLOR *color, int x, int y)
{
	AL_LOCKED_RECTANGLE lr;
	AL_BITMAP *bitmap = al_get_target_bitmap();

	/* FIXME: must use clip not full bitmap */
	if (bitmap->locked) {
		x -= bitmap->lock_x;
		y -= bitmap->lock_y;
		if (x < 0 || y < 0 || x > bitmap->lock_w || y > bitmap->lock_h) {
			return;
		}

		_al_put_pixel(bitmap->locked_rectangle.data+y*bitmap->locked_rectangle.pitch+x*_al_pixel_size(bitmap->format),
			bitmap->format, color);
	}
	else {
		if (x < 0 || y < 0 || x >= bitmap->w || y >= bitmap->h) {
			return;
		}

		if (!al_lock_bitmap_region(bitmap, x, y, 1, 1, &lr, 0))
			return;

		/* FIXME: check for valid pixel format */

		_al_put_pixel(lr.data, bitmap->format, color);

		al_unlock_bitmap(bitmap);
	}
}


/* unmap_rgba functions */

static void _unmap_rgba_argb_8888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = p->raw[0];
	*r = p->raw[1];
	*g = p->raw[2];
	*b = p->raw[3];
}

static void _unmap_rgba_rgba_8888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = p->raw[0];
	*g = p->raw[1];
	*b = p->raw[2];
	*a = p->raw[3];
}

static void _unmap_rgba_argb_4444(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = _rgb_scale_4[p->raw[0]];
	*r = _rgb_scale_4[p->raw[1]];
	*g = _rgb_scale_4[p->raw[2]];
	*b = _rgb_scale_4[p->raw[3]];
}

static void _unmap_rgba_rgb_888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = p->raw[0];
	*g = p->raw[1];
	*b = p->raw[2];
	*a = 0;
}

static void _unmap_rgba_rgb_565(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = _rgb_scale_5[p->raw[0]];
	*g = _rgb_scale_6[p->raw[1]];
	*b = _rgb_scale_5[p->raw[2]];
	*a = 0;
}

static void _unmap_rgba_rgb_555(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = _rgb_scale_5[p->raw[0]];
	*g = _rgb_scale_5[p->raw[1]];
	*b = _rgb_scale_5[p->raw[2]];
	*a = 0;
}

static void _unmap_rgba_palette_8(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = getr8(p->raw[0]);
	*g = getg8(p->raw[0]);
	*b = getb8(p->raw[0]);
	*a = 0;
}

static void _unmap_rgba_rgba_5551(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = _rgb_scale_5[p->raw[0]];
	*g = _rgb_scale_5[p->raw[1]];
	*b = _rgb_scale_5[p->raw[2]];
	*a = _rgb_scale_1[p->raw[3]];
}

static void _unmap_rgba_argb_1555(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = _rgb_scale_1[p->raw[0]];
	*r = _rgb_scale_5[p->raw[1]];
	*g = _rgb_scale_5[p->raw[2]];
	*b = _rgb_scale_5[p->raw[3]];
}

static void _unmap_rgba_abgr_8888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = p->raw[0];
	*b = p->raw[1];
	*g = p->raw[2];
	*r = p->raw[3];
}

static void _unmap_rgba_xbgr_8888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = 0;
	*b = p->raw[1];
	*g = p->raw[2];
	*r = p->raw[3];
}

static void _unmap_rgba_bgr_888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = 0;
	*b = p->raw[0];
	*g = p->raw[1];
	*r = p->raw[2];
}

static void _unmap_rgba_bgr_565(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = 0;
	*b = _rgb_scale_5[p->raw[0]];
	*g = _rgb_scale_6[p->raw[1]];
	*r = _rgb_scale_5[p->raw[2]];
}

static void _unmap_rgba_bgr_555(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*a = 0;
	*b = _rgb_scale_5[p->raw[0]];
	*g = _rgb_scale_5[p->raw[1]];
	*r = _rgb_scale_5[p->raw[2]];
}

static void _unmap_rgba_rgbx_8888(AL_COLOR *p,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	*r = p->raw[0];
	*g = p->raw[1];
	*b = p->raw[2];
	*a = 0;
}

/* unmap_rgba map */

typedef void (*_unmap_rgba_func)(AL_COLOR *,
	unsigned char *, unsigned char *, unsigned char *, unsigned char *);

static _unmap_rgba_func _unmap_rgba_funcs[] = {
	NULL,
	_unmap_rgba_argb_8888,
	_unmap_rgba_rgba_8888,
	_unmap_rgba_argb_4444,
	_unmap_rgba_rgb_888,
	_unmap_rgba_rgb_565,
	_unmap_rgba_rgb_555,
	_unmap_rgba_palette_8,
	_unmap_rgba_rgba_5551,
	_unmap_rgba_argb_1555,
	_unmap_rgba_abgr_8888,
	_unmap_rgba_xbgr_8888,
	_unmap_rgba_bgr_888,
	_unmap_rgba_bgr_565,
	_unmap_rgba_bgr_555,
	_unmap_rgba_rgbx_8888
};

void _al_unmap_rgba(int format, AL_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	(*_unmap_rgba_funcs[format])(color, r, g, b, a);
}

void al_unmap_rgba(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a)
{
	_al_unmap_rgba(bitmap->format, color, r, g, b, a);
}

void al_unmap_rgb(AL_BITMAP *bitmap, AL_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b)
{
	unsigned char tmp_a;
	_al_unmap_rgba(bitmap->format, color, r, g, b, &tmp_a);
}

/* unmap_rgba_f functions */

static void _unmap_rgba_f_argb_8888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = (float)p->raw[0] / 0xFF;
	*r = (float)p->raw[1] / 0xFF;
	*g = (float)p->raw[2] / 0xFF;
	*b = (float)p->raw[3] / 0xFF;
}

static void _unmap_rgba_f_rgba_8888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*r = (float)p->raw[0] / 0xFF;
	*g = (float)p->raw[1] / 0xFF;
	*b = (float)p->raw[2] / 0xFF;
	*a = (float)p->raw[3] / 0xFF;
}

static void _unmap_rgba_f_argb_4444(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = (float)p->raw[0] / 0xF;
	*r = (float)p->raw[1] / 0xF;
	*g = (float)p->raw[2] / 0xF;
	*b = (float)p->raw[3] / 0xF;
}

static void _unmap_rgba_f_rgb_888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*r = (float)p->raw[0] / 0xFF;
	*g = (float)p->raw[1] / 0xFF;
	*b = (float)p->raw[2] / 0xFF;
}

static void _unmap_rgba_f_rgb_565(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*r = (float)p->raw[0] / 0x1F;
	*g = (float)p->raw[1] / 0x3F;
	*b = (float)p->raw[2] / 0x1F;
}

static void _unmap_rgba_f_rgb_555(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*r = (float)p->raw[0] / 0x1F;
	*g = (float)p->raw[1] / 0x1F;
	*b = (float)p->raw[2] / 0x1F;
}

static void _unmap_rgba_f_palette_8(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*r = (float)getr8(p->raw[0]);
	*g = (float)getg8(p->raw[0]);
	*b = (float)getb8(p->raw[0]);
}

static void _unmap_rgba_f_rgba_5551(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*r = (float)p->raw[0] / 0x1F;
	*g = (float)p->raw[1] / 0x1F;
	*b = (float)p->raw[2] / 0x1F;
	*a = (float)p->raw[3];
}

static void _unmap_rgba_f_argb_1555(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = (float)p->raw[0];
	*r = (float)p->raw[1] / 0x1F;
	*g = (float)p->raw[2] / 0x1F;
	*b = (float)p->raw[3] / 0x1F;
}

static void _unmap_rgba_f_abgr_8888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = p->raw[0] / 0xFF;
	*b = p->raw[1] / 0xFF;
	*g = p->raw[2] / 0xFF;
	*r = p->raw[3] / 0xFF;
}

static void _unmap_rgba_f_xbgr_8888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*b = (float)p->raw[1] / 0xFF;
	*g = (float)p->raw[2] / 0xFF;
	*r = (float)p->raw[3] / 0xFF;
}

static void _unmap_rgba_f_bgr_888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*b = (float)p->raw[0] / 0xFF;
	*g = (float)p->raw[1] / 0xFF;
	*r = (float)p->raw[2] / 0xFF;
}

static void _unmap_rgba_f_bgr_565(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*b = p->raw[0] / 0x1F;
	*g = p->raw[1] / 0x3F;
	*r = p->raw[2] / 0x1F;
}

static void _unmap_rgba_f_bgr_555(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*a = 0.0f;
	*b = p->raw[0] / 0x1F;
	*g = p->raw[1] / 0x1F;
	*r = p->raw[2] / 0x1F;
}

static void _unmap_rgba_f_rgbx_8888(AL_COLOR *p,
	float *r, float *g, float *b, float *a)
{
	*r = p->raw[0] / 0xFF;
	*g = p->raw[1] / 0xFF;
	*b = p->raw[2] / 0xFF;
	*a = 0.0f;
}

/* unmap_rgba_f map */

typedef void (*_unmap_rgba_f_func)(AL_COLOR *,
	float *, float *, float *, float *);

static _unmap_rgba_f_func _unmap_rgba_f_funcs[] = {
	NULL,
	_unmap_rgba_f_argb_8888,
	_unmap_rgba_f_rgba_8888,
	_unmap_rgba_f_argb_4444,
	_unmap_rgba_f_rgb_888,
	_unmap_rgba_f_rgb_565,
	_unmap_rgba_f_rgb_555,
	_unmap_rgba_f_palette_8,
	_unmap_rgba_f_rgba_5551,
	_unmap_rgba_f_argb_1555,
	_unmap_rgba_f_abgr_8888,
	_unmap_rgba_f_xbgr_8888,
	_unmap_rgba_f_bgr_888,
	_unmap_rgba_f_bgr_565,
	_unmap_rgba_f_bgr_555,
	_unmap_rgba_f_rgbx_8888
};

void _al_unmap_rgba_f(int format, AL_COLOR *color,
	float *r, float *g, float *b, float *a)
{
	(*_unmap_rgba_f_funcs[format])(color, r, g, b, a);
}

void al_unmap_rgba_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float *r, float *g, float *b, float *a)
{
	_al_unmap_rgba_f(bitmap->format, color, r, g, b, a);
}

void al_unmap_rgb_f(AL_BITMAP *bitmap, AL_COLOR *color,
	float *r, float *g, float *b)
{
	float tmp_a;
	_al_unmap_rgba_f(bitmap->format, color, r, g, b, &tmp_a);
}

/* unmap_rgba_i */

void _al_unmap_rgba_i(int format, AL_COLOR *color,
	int *r, int *g, int *b, int *a)
{
	float fr, fg, fb, fa;
	(*_unmap_rgba_f_funcs[format])(color, &fr, &fg, &fb, &fa);
	*r = fr * INT_MAX;
	*g = fg * INT_MAX;
	*b = fb * INT_MAX;
	*a = fa * INT_MAX;
}

void al_unmap_rgba_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int *r, int *g, int *b, int *a)
{
	_al_unmap_rgba_i(bitmap->format, color, r, g, b, a);
}

void al_unmap_rgb_i(AL_BITMAP *bitmap, AL_COLOR *color,
	int *r, int *g, int *b)
{
	int tmp_a;
	_al_unmap_rgba_i(bitmap->format, color, r, g, b, &tmp_a);
}
