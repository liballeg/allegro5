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
 *      RLE sprites.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_RLE_H
#define ALLEGRO_RLE_H

#ifdef __cplusplus
   extern "C" {
#endif

#include "base.h"
#include "gfx.h"

typedef struct RLE_SPRITE           /* a RLE compressed sprite */
{
   int w, h;                        /* width and height in pixels */
   int color_depth;                 /* color depth of the image */
   int size;                        /* size of sprite data in bytes */
   signed char dat[ZERO_SIZE];
} RLE_SPRITE;


AL_FUNC(RLE_SPRITE *, get_rle_sprite, (struct BITMAP *bitmap));
AL_FUNC(void, destroy_rle_sprite, (RLE_SPRITE *sprite));

#include "inline/rle.inl"

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_RLE_H */


