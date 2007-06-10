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
 *      Blitting functions compatibility layer.
 *
 *      Draft version.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"

/*
void blit(BITMAP *source, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   //al_blit_region(0, source, s_x, s_y, w, h, dest, d_x, d_y);
}
*/

void masked_blit(BITMAP *source, BITMAP *dest, int s_x, int s_y, int d_x, int d_y, int w, int h)
{
   //al_blit_region(AL_MASK_SOURCE, source, s_x, s_y, w, h, dest, d_x, d_y);
}

void stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h)
{
   al_compat_blit_scaled(0, s, s_x, s_y, s_w, s_h,  d, d_x, d_y, d_w, d_h);

   if (d->needs_upload) {
      d->al_bitmap->vt->upload_compat_bitmap(d, d_x, d_y, d_w, d_h);
   }
}

void masked_stretch_blit(BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h)
{
 //  al_blit_scaled(AL_MASK_SOURCE, s, s_x, s_y, s_w, s_h,  d, d_x, d_y, d_w, d_h);
}

void stretch_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int w, int h)
{
 //  ASSERT(sprite);

 //  al_blit_scaled(AL_MASK_SOURCE, sprite, 0, 0, sprite->w, sprite->h, bmp, x, y, w, h);
}
