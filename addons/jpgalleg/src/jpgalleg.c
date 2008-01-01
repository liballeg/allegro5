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
 *      Common library data and functions.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 *
 */


#include <internal.h>


HUFFMAN_TABLE _jpeg_huffman_ac_table[4];
HUFFMAN_TABLE _jpeg_huffman_dc_table[4];
IO_BUFFER _jpeg_io;

const unsigned char _jpeg_zigzag_scan[64] = {
	 0, 1, 5, 6,14,15,27,28,
	 2, 4, 7,13,16,26,29,42,
	 3, 8,12,17,25,30,41,43,
	 9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
};

const char *_jpeg_component_name[] = { "Y", "Cb", "Cr" };

int jpgalleg_error = JPG_ERROR_NONE;


/* _jpeg_trace:
 *  Internal debugging routine: prints error to stderr if in debug mode.
 */
void
_jpeg_trace(const char *msg, ...)
{
#ifdef DEBUG
	va_list ap;
	
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
#else
	(void)msg;
#endif
}
