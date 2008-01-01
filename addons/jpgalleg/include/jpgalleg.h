/*
 *         __   _____    ______   ______   ___    ___
 *        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
 *        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
 *      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
 *     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
 *     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
 *      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
 *                                                                  /\____/
 *                                                                  \_/__/
 *
 *      Version 2.6, by Angelo Mottola, 2000-2006
 *
 *      Public header file.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */

#ifndef _JPGALLEG_H_
#define _JPGALLEG_H_

#include <allegro.h>

/* Library version constant and string */
#define JPGALLEG_VERSION 0x0206
#define JPGALLEG_VERSION_STRING			"JPGalleg 2.6, by Angelo Mottola, 2000-2006"


/* Subsampling mode */
#define JPG_SAMPLING_444			0
#define JPG_SAMPLING_422			1
#define JPG_SAMPLING_411			2

/* Force greyscale when saving */
#define JPG_GREYSCALE				0x10

/* Use optimized encoding */
#define JPG_OPTIMIZE				0x20


/* Error codes */
#define JPG_ERROR_NONE				0
#define JPG_ERROR_READING_FILE			-1
#define JPG_ERROR_WRITING_FILE			-2
#define JPG_ERROR_INPUT_BUFFER_TOO_SMALL	-3
#define JPG_ERROR_OUTPUT_BUFFER_TOO_SMALL	-4
#define JPG_ERROR_HUFFMAN			-5
#define JPG_ERROR_NOT_JPEG			-6
#define JPG_ERROR_UNSUPPORTED_ENCODING		-7
#define JPG_ERROR_UNSUPPORTED_COLOR_SPACE	-8
#define JPG_ERROR_UNSUPPORTED_DATA_PRECISION	-9
#define JPG_ERROR_BAD_IMAGE			-10
#define JPG_ERROR_OUT_OF_MEMORY			-11


/* Datafile object type for JPG images */
#define DAT_JPEG				DAT_ID('J','P','E','G')


#ifdef __cplusplus
extern "C" {
#endif


extern int jpgalleg_init(void);

extern BITMAP *load_jpg(AL_CONST char *filename, RGB *palette);
extern BITMAP *load_jpg_ex(AL_CONST char *filename, RGB *palette, void (*callback)(int progress));
extern BITMAP *load_memory_jpg(void *buffer, int size, RGB *palette);
extern BITMAP *load_memory_jpg_ex(void *buffer, int size, RGB *palette, void (*callback)(int progress));

extern int save_jpg(AL_CONST char *filename, BITMAP *image, AL_CONST RGB *palette);
extern int save_jpg_ex(AL_CONST char *filename, BITMAP *image, AL_CONST RGB *palette, int quality, int flags, void (*callback)(int progress));
extern int save_memory_jpg(void *buffer, int *size, BITMAP *image, AL_CONST RGB *palette);
extern int save_memory_jpg_ex(void *buffer, int *size, BITMAP *image, AL_CONST RGB *palette, int quality, int flags, void (*callback)(int progress));

extern int jpgalleg_error;
extern const char *jpgalleg_error_string(void);


#ifdef __cplusplus
}
#endif

#endif
