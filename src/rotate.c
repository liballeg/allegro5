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
 *      Sprite rotation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Flipping routines by Andrew Geers.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/aintern.h"



/* helper macro for rotating sprites in different color depths */
#define DO_ROTATE(bits, scale, getpix, checkpix)                             \
{                                                                            \
   addr = bmp_write_line(bmp, y+hgap-dy-1) + x*scale;                        \
									     \
   for (dx=wgap-1; dx>=0; dx--) {                                            \
      sx = fixtoi(f2x);                                                      \
									     \
      if ((unsigned)sx < (unsigned)sprite->w) {                              \
	 sy = fixtoi(f2y);                                                   \
									     \
	 if ((unsigned)sy < (unsigned)sprite->h) {                           \
	    getpix;                                                          \
	    if (checkpix)                                                    \
	       bmp_write##bits(addr, pixel);                                 \
	 }                                                                   \
      }                                                                      \
									     \
      addr += scale;                                                         \
      f2x += f2xd;                                                           \
      f2y += f2yd;                                                           \
   }                                                                         \
}



/* pivot_sprite:
 *  Rotates a sprite around the specified pivot centre point.
 */
void pivot_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle)
{
   pivot_scaled_sprite(bmp, sprite, x, y, cx, cy, angle, itofix(1));
}



/* pivot_sprite_v_flip:
 *  Similar to pivot_sprite, except flips the sprite vertically first.
 */
void pivot_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle)
{
   pivot_scaled_sprite_v_flip(bmp, sprite, x, y, cx, cy, angle, itofix(1));
}



/* pivot_scaled_sprite:
 *  Rotates a sprite around the specified pivot centre point.
 */
void pivot_scaled_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale)
{
   x -= fixtoi(sprite->w*scale/2);
   y -= fixtoi(sprite->h*scale/2);

   cx -= sprite->w/2;
   cy -= sprite->h/2;

   x -= fixtoi(fmul(cx*fcos(angle) - cy*fsin(angle), scale));
   y -= fixtoi(fmul(cx*fsin(angle) + cy*fcos(angle), scale));

   rotate_scaled_sprite(bmp, sprite, x, y, angle, scale);
}



/* pivot_scaled_sprite_v_flip:
 *  Similar to pivot_scaled_sprite, except flips the sprite vertically first.
 */
void pivot_scaled_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale)
{
   x -= fixtoi(sprite->w*scale/2);
   y -= fixtoi(sprite->h*scale/2);

   cx -= sprite->w/2;
   cy -= sprite->h/2;

   x -= fixtoi(fmul(cx*fcos(angle) - cy*fsin(angle), scale));
   y -= fixtoi(fmul(cx*fsin(angle) + cy*fcos(angle), scale));

   rotate_scaled_sprite_v_flip(bmp, sprite, x, y, angle, scale);
}



/* rotate_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
void rotate_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle)
{
   rotate_scaled_sprite(bmp, sprite, x, y, angle, itofix(1));
}



/* rotate_sprite_v_flip:
 *  Similar to rotate_sprite, except flips the sprite vertically first.
 */
void rotate_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle)
{
   rotate_scaled_sprite_v_flip(bmp, sprite, x, y, angle, itofix(1));
}



/* rotate_scaled_sprite_flip:
 *  Rotates and scales a sprite, optionally flipping it about either axis.
 */
void rotate_scaled_sprite_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale, int h_flip, int v_flip)
{
   fixed f1x, f1y, f1xd, f1yd;
   fixed f2x, f2y, f2xd, f2yd;
   fixed w, h;
   fixed dir, dir2, dist;
   int x1, y1, x2, y2;
   int dx, dy;
   int sx, sy;
   unsigned long addr;
   int wgap = sprite->w;
   int hgap = sprite->h;
   int pixel;

   /* rotate the top left pixel of the sprite */
   w = itofix(wgap/2);
   h = itofix(hgap/2);
   dir = fatan2(h, w);
   dir2 = fatan2(h, -w);
   dist = fsqrt((wgap*wgap + hgap*hgap) << 6) << 4;
   f1x = w - fmul(dist, fcos(dir - angle));
   f1y = h - fmul(dist, fsin(dir - angle));

   /* map the destination y axis onto the sprite */
   f1xd = fcos(itofix(64) - angle);
   f1yd = fsin(itofix(64) - angle);

   /* map the destination x axis onto the sprite */
   f2xd = fcos(-angle);
   f2yd = fsin(-angle);

   /* scale the sprite? */
   if (scale != itofix(1)) {
      dist = fmul(dist, scale);
      wgap = fixtoi(fmul(itofix(wgap), scale));
      hgap = fixtoi(fmul(itofix(hgap), scale));
      scale = fdiv(itofix(1), scale);
      f1xd = fmul(f1xd, scale);
      f1yd = fmul(f1yd, scale);
      f2xd = fmul(f2xd, scale);
      f2yd = fmul(f2yd, scale);
   }

   /* adjust the size of the region to be scanned */
   x1 = fixtoi(fmul(dist, fcos(dir + angle)));
   y1 = fixtoi(fmul(dist, fsin(dir + angle)));

   x2 = fixtoi(fmul(dist, fcos(dir2 + angle)));
   y2 = fixtoi(fmul(dist, fsin(dir2 + angle)));

   x1 = MAX(ABS(x1), ABS(x2)) - wgap/2;
   y1 = MAX(ABS(y1), ABS(y2)) - hgap/2;

   x -= x1;
   wgap += x1 * 2;
   f1x -= f2xd * x1;
   f1y -= f2yd * x1;

   y -= y1;
   hgap += y1 * 2;
   f1x -= f1xd * y1;
   f1y -= f1yd * y1;

   /* clip the output rectangle */
   if (bmp->clip) {
      while (x < bmp->cl) {
	 x++;
	 wgap--;
	 f1x += f2xd;
	 f1y += f2yd;
      }

      while (y < bmp->ct) {
	 y++;
	 hgap--;
	 f1x += f1xd;
	 f1y += f1yd;
      }

      while (x+wgap > bmp->cr)
	 wgap--;

      while (y+hgap > bmp->cb)
	 hgap--;

      if ((wgap <= 0) || (hgap <= 0))
	 return;
   }

   bmp_select(bmp);

   if (h_flip)
      f2xd = -f2xd;
   if (v_flip)
      f2yd = -f2yd;

   /* and trace a bunch of lines through the bitmaps */
   for (dy=hgap-1; dy>=0; dy--) {
      f2x = f1x;
      f2y = f1y;

      if (h_flip)
         f2x = itofix(sprite->w) - f2x - itofix(1);
      if (v_flip)
         f2y = itofix(sprite->h) - f2y - itofix(1);

      if (is_linear_bitmap(bmp)) {           /* draw onto a linear bitmap */
	 switch (bitmap_color_depth(bmp)) {

	    #ifdef ALLEGRO_COLOR8

	       case 8:
		  DO_ROTATE(8, 1, 
			    pixel = sprite->line[sy][sx],
			    (pixel != 0));
	       break;

	    #endif

	    #ifdef ALLEGRO_COLOR16

	       case 15:
	       case 16:
		  DO_ROTATE(16, sizeof(short),
			    pixel = ((unsigned short *)sprite->line[sy])[sx],
			    (pixel != bmp->vtable->mask_color));
		  break;

	    #endif

	    #ifdef ALLEGRO_COLOR24

	       case 24:
		  DO_ROTATE(24, 3,
			    bmp_select(sprite); pixel = bmp_read24((unsigned long)(sprite->line[sy]+sx*3)); bmp_select(bmp),
			    (pixel != bmp->vtable->mask_color));
		  break;

	    #endif

	    #ifdef ALLEGRO_COLOR32

	       case 32:
		  DO_ROTATE(32, sizeof(long),
			    pixel = ((unsigned long *)sprite->line[sy])[sx],
			    (pixel != bmp->vtable->mask_color));
		  break;

	    #endif
	 }
      }

   #ifdef GFX_MODEX

      else {                                 /* draw onto a mode-X bitmap */
	 fixed f3x, f3y;
	 int plane;

	 for (plane=0; plane<4; plane++) {
	    f3x = f2x;
	    f3y = f2y;
	    addr = (unsigned long)bmp->line[y+hgap-dy-1] + ((x+plane)>>2);
	    outportw(0x3C4, (0x100<<((x+plane)&3))|2);

	    for (dx=plane; dx<wgap; dx+=4) {
	       sx = fixtoi(f3x);

	       if ((unsigned)sx < (unsigned)sprite->w) {
		  sy = fixtoi(f3y);

		  if ((unsigned)sy < (unsigned)sprite->h) {
		     if (sprite->line[sy][sx])
			_farnspokeb(addr, sprite->line[sy][sx]);
		  }
	       }

	       addr++;
	       f3x += (f2xd<<2);
	       f3y += (f2yd<<2);
	    }

	    f2x += f2xd;
	    f2y += f2yd;
	 }
      }

   #endif   /* GFX_MODEX */

      f1x += f1xd;
      f1y += f1yd;
   }

   bmp_unwrite_line(bmp);
}



/* rotate_scaled_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
void rotate_scaled_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale)
{
   rotate_scaled_sprite_flip(bmp, sprite, x, y, angle, scale, 0, 0);
}



/* rotate_scaled_sprite_v_flip:
 *  Similar to rotate_scaled_sprite, except flips the sprite vertically first.
 */
void rotate_scaled_sprite_v_flip(BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale)
{
   rotate_scaled_sprite_flip(bmp, sprite, x, y, angle, scale, 0, 1);
}
