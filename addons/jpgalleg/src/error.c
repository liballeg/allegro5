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
 *      Error reporting utility function.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 *
 */


#include <internal.h>


/*
 * jpgalleg_error_string:
 *  Returns the latest error message string.
 */
const char *jpgalleg_error_string(void)
{
	switch (jpgalleg_error) {
		case JPG_ERROR_NONE:				return "No error";
		case JPG_ERROR_READING_FILE:			return "File read error";
		case JPG_ERROR_WRITING_FILE:			return "File write error";
		case JPG_ERROR_INPUT_BUFFER_TOO_SMALL:		return "Input memory buffer too small";
		case JPG_ERROR_OUTPUT_BUFFER_TOO_SMALL:		return "Output memory buffer too small";
		case JPG_ERROR_HUFFMAN:				return "Huffman compression error";
		case JPG_ERROR_NOT_JPEG:			return "Not a valid JPEG";
		case JPG_ERROR_UNSUPPORTED_ENCODING:		return "Unsupported encoding";
		case JPG_ERROR_UNSUPPORTED_COLOR_SPACE:		return "Unsupported color space";
		case JPG_ERROR_UNSUPPORTED_DATA_PRECISION:	return "Unsupported data precision";
		case JPG_ERROR_BAD_IMAGE:			return "Image data is corrupted";
		case JPG_ERROR_OUT_OF_MEMORY:			return "Out of memory";
	}
	return "Unknown error";
}
