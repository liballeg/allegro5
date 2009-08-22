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
 *      Optimized by Sven Sandberg.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include <math.h>

/* _al_rotate_scale_flip_coordinates:
 *  Calculates the coordinates for the rotated, scaled and flipped sprite,
 *  and passes them on to the given function.
 */
void _al_rotate_scale_flip_coordinates(al_fixed w, al_fixed h,
				    al_fixed x, al_fixed y, al_fixed cx, al_fixed cy,
				    al_fixed angle,
				    al_fixed scale_x, al_fixed scale_y,
				    int h_flip, int v_flip,
				    al_fixed xs[4], al_fixed ys[4])
{
   al_fixed fix_cos, fix_sin;
   int tl = 0, tr = 1, bl = 3, br = 2;
   int tmp;
   double cos_angle, sin_angle;
   al_fixed xofs, yofs;

   /* Setting angle to the range -180...180 degrees makes sin & cos
      more numerically stable. (Yes, this does have an effect for big
      angles!) Note that using "real" sin() and cos() gives much better
      precision than al_fixsin() and al_fixcos(). */
   angle = angle & 0xffffff;
   if (angle >= 0x800000)
      angle -= 0x1000000;

   _AL_SINCOS(angle * (ALLEGRO_PI / (double)0x800000), sin_angle, cos_angle);

   if (cos_angle >= 0)
      fix_cos = (int)(cos_angle * 0x10000 + 0.5);
   else
      fix_cos = (int)(cos_angle * 0x10000 - 0.5);
   if (sin_angle >= 0)
      fix_sin = (int)(sin_angle * 0x10000 + 0.5);
   else
      fix_sin = (int)(sin_angle * 0x10000 - 0.5);

   /* Decide what order to take corners in. */
   if (v_flip) {
      tl = 3;
      tr = 2;
      bl = 0;
      br = 1;
   }
   else {
      tl = 0;
      tr = 1;
      bl = 3;
      br = 2;
   }
   if (h_flip) {
      tmp = tl;
      tl = tr;
      tr = tmp;
      tmp = bl;
      bl = br;
      br = tmp;
   }

   /* Calculate new coordinates of all corners. */
   w = al_fixmul(w, scale_x);
   h = al_fixmul(h, scale_y);
   cx = al_fixmul(cx, scale_x);
   cy = al_fixmul(cy, scale_y);

   xofs = x - al_fixmul(cx, fix_cos) + al_fixmul(cy, fix_sin);

   yofs = y - al_fixmul(cx, fix_sin) - al_fixmul(cy, fix_cos);

   xs[tl] = xofs;
   ys[tl] = yofs;
   xs[tr] = xofs + al_fixmul(w, fix_cos);
   ys[tr] = yofs + al_fixmul(w, fix_sin);
   xs[bl] = xofs - al_fixmul(h, fix_sin);
   ys[bl] = yofs + al_fixmul(h, fix_cos);

   xs[br] = xs[tr] + xs[bl] - xs[tl];
   ys[br] = ys[tr] + ys[bl] - ys[tl];
}


