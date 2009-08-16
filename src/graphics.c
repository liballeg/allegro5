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
 *      Graphics mode set and bitmap creation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"

#include "allegro5/internal/aintern_display.h"

extern void blit_end(void);   /* for LOCK_FUNCTION; defined in blit.c */


#define PREFIX_I                "al-gfx INFO: "
#define PREFIX_W                "al-gfx WARNING: "
#define PREFIX_E                "al-gfx ERROR: "

/* lookup table for scaling 1 bit colors up to 8 bits */
const int _rgb_scale_1[2] =
{
   0, 255
};


/* lookup table for scaling 4 bit colors up to 8 bits */
const int _rgb_scale_4[16] =
{
     0,  17,  34,  51,  68,  85, 102, 119,
   136, 153, 170, 187, 204, 221, 238, 255
};


/* lookup table for scaling 5 bit colors up to 8 bits */
const int _rgb_scale_5[32] =
{
   0,   8,   16,  24,  33,  41,  49,  57,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   198, 206, 214, 222, 231, 239, 247, 255
};


/* lookup table for scaling 6 bit colors up to 8 bits */
const int _rgb_scale_6[64] =
{
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   195, 199, 203, 207, 211, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};


