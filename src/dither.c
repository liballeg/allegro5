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
 *      Hicolor dithering routines, by Ivan Baldo.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"



static unsigned char dither_table[7] = { 16, 68, 146, 170, 109, 187, 239 };
static unsigned char dither_ytable[8] = { 1, 5, 2, 7, 4, 0, 6, 3 };



/* makecol15_dither:
 *  Calculates a dithered 15 bit pixel value.
 */
int makecol15_dither(int r, int g, int b, int x, int y)
{
   int returned_r, returned_g, returned_b;
   int bpos;

   returned_r = r/8;
   returned_b = b/8;
   returned_g = g/8;

   y = dither_ytable[y&7];

   if (r&7) {
      bpos = (x+y)&7; 
      returned_r += ((dither_table[((r&7)-1)]) & (1<<bpos)) >> bpos;
   }

   if (b&7) {
      bpos = (x+y+3)&7; 
      returned_b += ((dither_table[((b&7)-1)]) & (1<<bpos)) >> bpos;
   }

   if (g&7) {
      bpos = (x+y+2)&7;
      returned_g += ((dither_table[(g&7)-1]) & (1<<bpos)) >> bpos;
   }

   if (returned_r > 31)
      returned_r = 31;

   if (returned_b > 31) 
      returned_b = 31;

   if (returned_g > 31) 
      returned_g = 31;

   return (returned_r<<_rgb_r_shift_15) | (returned_g<<_rgb_g_shift_15) | (returned_b<<_rgb_b_shift_15);
}



/* makecol16_dither:
 *  Calculates a dithered 16 bit pixel value.
 */
int makecol16_dither(int r, int g, int b, int x, int y)
{
   int returned_r, returned_g, returned_b;
   int bpos;

   returned_r = r/8;
   returned_b = b/8;
   returned_g = g/4;

   y = dither_ytable[y&7];

   if (r&7) {
      bpos = (x+y)&7; 
      returned_r += ((dither_table[((r&7)-1)]) & (1<<bpos)) >> bpos;
   }

   if (b&7) {
      bpos = (x+y+3)&7; 
      returned_b += ((dither_table[((b&7)-1)]) & (1<<bpos)) >> bpos;
   }

   if (g&3) {
      bpos = (x+y+2)&7;
      returned_g += ((dither_table[(g&3)*2-1]) & (1<<bpos)) >> bpos;
   }

   if (returned_r > 31)
      returned_r = 31;

   if (returned_b > 31) 
      returned_b = 31;

   if (returned_g > 63) 
      returned_g = 63;

   return (returned_r<<_rgb_r_shift_16) | (returned_g<<_rgb_g_shift_16) | (returned_b<<_rgb_b_shift_16);
}


