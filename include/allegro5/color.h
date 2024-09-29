#ifndef __al_included_allegro5_color_h
#define __al_included_allegro5_color_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_COLOR
 */
typedef struct A5O_COLOR A5O_COLOR;

struct A5O_COLOR
{
   float r, g, b, a;
};


/* Enum: A5O_PIXEL_FORMAT
 */
typedef enum A5O_PIXEL_FORMAT
{
   A5O_PIXEL_FORMAT_ANY                   = 0,
   A5O_PIXEL_FORMAT_ANY_NO_ALPHA          = 1,
   A5O_PIXEL_FORMAT_ANY_WITH_ALPHA        = 2,
   A5O_PIXEL_FORMAT_ANY_15_NO_ALPHA       = 3,
   A5O_PIXEL_FORMAT_ANY_16_NO_ALPHA       = 4,
   A5O_PIXEL_FORMAT_ANY_16_WITH_ALPHA     = 5,
   A5O_PIXEL_FORMAT_ANY_24_NO_ALPHA       = 6,
   A5O_PIXEL_FORMAT_ANY_32_NO_ALPHA       = 7,
   A5O_PIXEL_FORMAT_ANY_32_WITH_ALPHA     = 8,
   A5O_PIXEL_FORMAT_ARGB_8888             = 9,
   A5O_PIXEL_FORMAT_RGBA_8888             = 10,
   A5O_PIXEL_FORMAT_ARGB_4444             = 11,
   A5O_PIXEL_FORMAT_RGB_888               = 12, /* 24 bit format */
   A5O_PIXEL_FORMAT_RGB_565               = 13,
   A5O_PIXEL_FORMAT_RGB_555               = 14,
   A5O_PIXEL_FORMAT_RGBA_5551             = 15,
   A5O_PIXEL_FORMAT_ARGB_1555             = 16,
   A5O_PIXEL_FORMAT_ABGR_8888             = 17,
   A5O_PIXEL_FORMAT_XBGR_8888             = 18,
   A5O_PIXEL_FORMAT_BGR_888               = 19, /* 24 bit format */
   A5O_PIXEL_FORMAT_BGR_565               = 20,
   A5O_PIXEL_FORMAT_BGR_555               = 21,
   A5O_PIXEL_FORMAT_RGBX_8888             = 22,
   A5O_PIXEL_FORMAT_XRGB_8888             = 23,
   A5O_PIXEL_FORMAT_ABGR_F32              = 24,
   A5O_PIXEL_FORMAT_ABGR_8888_LE          = 25,
   A5O_PIXEL_FORMAT_RGBA_4444             = 26,
   A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8      = 27,
   A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1  = 28,
   A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3  = 29,
   A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5  = 30,
   A5O_NUM_PIXEL_FORMATS
} A5O_PIXEL_FORMAT;


/* Pixel mapping */
AL_FUNC(A5O_COLOR, al_map_rgb, (unsigned char r, unsigned char g, unsigned char b));
AL_FUNC(A5O_COLOR, al_map_rgba, (unsigned char r, unsigned char g, unsigned char b, unsigned char a));
AL_FUNC(A5O_COLOR, al_map_rgb_f, (float r, float g, float b));
AL_FUNC(A5O_COLOR, al_map_rgba_f, (float r, float g, float b, float a));
AL_FUNC(A5O_COLOR, al_premul_rgba,
   (unsigned char r, unsigned char g, unsigned char b, unsigned char a));
AL_FUNC(A5O_COLOR, al_premul_rgba_f,
   (float r, float g, float b, float a));

/* Pixel unmapping */
AL_FUNC(void, al_unmap_rgb, (A5O_COLOR color, unsigned char *r, unsigned char *g, unsigned char *b));
AL_FUNC(void, al_unmap_rgba, (A5O_COLOR color, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a));
AL_FUNC(void, al_unmap_rgb_f, (A5O_COLOR color, float *r, float *g, float *b));
AL_FUNC(void, al_unmap_rgba_f, (A5O_COLOR color, float *r, float *g, float *b, float *a));

/* Pixel formats */
AL_FUNC(int, al_get_pixel_size, (int format));
AL_FUNC(int, al_get_pixel_format_bits, (int format));
AL_FUNC(int, al_get_pixel_block_size, (int format));
AL_FUNC(int, al_get_pixel_block_width, (int format));
AL_FUNC(int, al_get_pixel_block_height, (int format));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
