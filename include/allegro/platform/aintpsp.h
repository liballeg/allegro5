/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Internal header file for the PSP Allegro library port.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTPSP_H
#define AINTPSP_H


#define DEFAULT_SCREEN_WIDTH       480
#define DEFAULT_SCREEN_HEIGHT      272
#define DEFAULT_COLOR_DEPTH         16

#define BMP_EXTRA(bmp)		    ((BMP_EXTRA_INFO *)((bmp)->extra))

typedef struct BMP_EXTRA_INFO
{
   int pitch;
   BITMAP *parent;
} BMP_EXTRA_INFO;


#endif

