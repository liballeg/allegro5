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
 *      Internal library definitions.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#ifndef _JPGALLEG_INTERNAL_H_
#define _JPGALLEG_INTERNAL_H_

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jpgalleg.h>
#include <dct.h>
#include <mmx.h>

#ifdef TRACE
	#undef TRACE
#endif
#ifdef DEBUG
	#define TRACE fprintf(stderr, "[JPGalleg: %s]", __FUNCTION__); _jpeg_trace
#else
	#define TRACE 1 ? (void)0 : _jpeg_trace
#endif


#define CHUNK_SOI		0xffd8
#define CHUNK_EOI		0xffd9

#define CHUNK_JPG0		0xf0
#define CHUNK_JPG13		0xfd
#define CHUNK_DHP		0xde
#define CHUNK_EXP		0xdf
#define CHUNK_DNL		0xdc
#define CHUNK_DAC		0xcc
#define CHUNK_SOF3		0xc3
#define CHUNK_SOF5		0xc5
#define CHUNK_SOF6		0xc6
#define CHUNK_SOF7		0xc7
#define CHUNK_SOF9		0xc9
#define CHUNK_SOF10		0xca
#define CHUNK_SOF11		0xcb
#define CHUNK_SOF13		0xcd
#define CHUNK_SOF14		0xce
#define CHUNK_SOF15		0xcf
#define CHUNK_APP0		0xe0
#define CHUNK_APP1		0xe1
#define CHUNK_APP2		0xe2
#define CHUNK_APP3		0xe3
#define CHUNK_APP4		0xe4
#define CHUNK_APP5		0xe5
#define CHUNK_APP6		0xe6
#define CHUNK_APP7		0xe7
#define CHUNK_APP8		0xe8
#define CHUNK_APP9		0xe9
#define CHUNK_APP10		0xea
#define CHUNK_APP11		0xeb
#define CHUNK_APP12		0xec
#define CHUNK_APP13		0xed
#define CHUNK_APP14		0xee
#define CHUNK_APP15		0xef

#define CHUNK_TEM		0x01
#define CHUNK_RST0		0xffd0
#define CHUNK_RST1		0xffd1
#define CHUNK_RST2		0xffd2
#define CHUNK_RST3		0xffd3
#define CHUNK_RST4		0xffd4
#define CHUNK_RST5		0xffd5
#define CHUNK_RST6		0xffd6
#define CHUNK_RST7		0xffd7

#define CHUNK_JPG		0xc8

#define CHUNK_SOF0		0xc0
#define CHUNK_SOF1		0xc1
#define CHUNK_SOF2		0xc2
#define CHUNK_DHT		0xc4
#define CHUNK_SOS		0xda
#define CHUNK_DQT		0xdb
#define CHUNK_DRI		0xdd
#define CHUNK_COM		0xfe

#define SOF0_DEFINED		0x01
#define DHT_DEFINED		0x02
#define SOS_DEFINED		0x04
#define DQT_DEFINED		0x08
#define APP0_DEFINED		0x10
#define APP1_DEFINED		0x20
#define DRI_DEFINED		0x40
#define IS_PROGRESSIVE		0x80

#define JFIF_OK			(SOF0_DEFINED | DHT_DEFINED | SOS_DEFINED | DQT_DEFINED | APP0_DEFINED)
#define EXIF_OK			(SOF0_DEFINED | DHT_DEFINED | SOS_DEFINED | DQT_DEFINED | APP1_DEFINED)

#define LUMINANCE		0
#define CHROMINANCE		1

#define PASS_WRITE		0
#define PASS_COMPUTE_HUFFMAN    1


/* This expression is made to return:
 *   Quality = 100 -> Factor = 10
 *   Quality = 1   -> Factor = 0.1
 * Other values follow a logarithmic curve
 */
#define QUALITY_FACTOR(q)       (pow(10.0, ((q) - 50.5) / 49.5))

/* Default quality/flags values for save_jpg() and save_memory_jpg() */
#define DEFAULT_QUALITY		75
#define DEFAULT_FLAGS		JPG_SAMPLING_444



typedef struct HUFFMAN_ENTRY
{
	int encoded_value;
	int value;
	int bits_length;
	int frequency;
} HUFFMAN_ENTRY;


typedef struct HUFFMAN_TABLE
{
	HUFFMAN_ENTRY entry[257];
	HUFFMAN_ENTRY *entry_of_length[16];
	HUFFMAN_ENTRY *code[256];
} HUFFMAN_TABLE;


typedef struct HUFFMAN_NODE
{
	HUFFMAN_ENTRY *entry;
	int frequency;
	struct HUFFMAN_NODE *parent;
	struct HUFFMAN_NODE *left, *right;
	struct HUFFMAN_NODE *prev, *next;
} HUFFMAN_NODE;


typedef struct HUFFMAN_TREE
{
	HUFFMAN_NODE *node;
	HUFFMAN_NODE *head, *tail;
	int depth;
} HUFFMAN_TREE;


typedef struct DATA_BUFFER
{
	short data[64];
} DATA_BUFFER;


typedef struct IO_BUFFER
{
	unsigned char *buffer;
	unsigned char *buffer_start, *buffer_end;
	int current_bit;
} IO_BUFFER;


extern int _jpeg_getc(void);
extern int _jpeg_putc(int);
extern int _jpeg_getw(void);
extern int _jpeg_putw(int);
extern INLINE int _jpeg_get_bit(void);
extern int _jpeg_put_bit(int);
extern void _jpeg_flush_bits(void);
extern void _jpeg_open_chunk(void);
extern void _jpeg_close_chunk(void);
extern int _jpeg_eoc(void);
extern void _jpeg_new_chunk(int);
extern void _jpeg_write_chunk(void);
extern void _jpeg_chunk_putc(int);
extern void _jpeg_chunk_putw(int);
extern void _jpeg_chunk_puts(unsigned char *, int);

extern void _jpeg_mmx_idct(short *, short *, short *, short *);
extern void _jpeg_mmx_ycbcr2rgb(int, int, int, int, int, int, int, int, int, int, int, int, int);
extern void _jpeg_mmx_ycbcr2bgr(int, int, int, int, int, int, int, int, int, int, int, int, int);

extern void _jpeg_mmx_rgb2ycbcr(int, short *, short *, short *, short *, short *, short *);
extern void _jpeg_mmx_bgr2ycbcr(int, short *, short *, short *, short *, short *, short *);

extern void _jpeg_trace(const char *, ...);

extern HUFFMAN_TABLE _jpeg_huffman_ac_table[];
extern HUFFMAN_TABLE _jpeg_huffman_dc_table[];
extern IO_BUFFER _jpeg_io;
extern const unsigned char _jpeg_zigzag_scan[];
extern const char *_jpeg_component_name[];


#endif
