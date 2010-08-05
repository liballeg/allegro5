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
 *      Memory bitmap blending routines
 *
 *      by Trent Gamblin.
 *
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include <string.h>

#define MIN _ALLEGRO_MIN
#define MAX _ALLEGRO_MAX


static float get_factor(int operation, float alpha)
{
   switch(operation) {
       case ALLEGRO_ZERO: return 0;
       case ALLEGRO_ONE: return 1;
       case ALLEGRO_ALPHA: return alpha;
       case ALLEGRO_INVERSE_ALPHA: return 1 - alpha;
   }
   ASSERT(false);
   return 0; /* silence warning in release build */
}


// FIXME: un-optimized reference implementation
void _al_blend_memory(ALLEGRO_COLOR *scol,
   ALLEGRO_BITMAP *dest,
   int dx, int dy, ALLEGRO_COLOR *result)
{
   float src, dst, asrc, adst;
   int op, src_, dst_, aop, asrc_, adst_;
   ALLEGRO_COLOR dcol;

   dcol = al_get_pixel(dest, dx, dy);
   al_get_separate_blender(&op, &src_, &dst_, &aop, &asrc_, &adst_);
   result->r = scol->r;
   result->g = scol->g;
   result->b = scol->b;
   result->a = scol->a;
   src = get_factor(src_, result->a);
   dst = get_factor(dst_, result->a);
   asrc = get_factor(asrc_, result->a);
   adst = get_factor(adst_, result->a);

   // FIXME: Better not do all those checks for each pixel but already
   // at the caller.
   // Note: Multiplying NaN or Inf with 0 doesn't give 0.
   if (dst == 0) {
      if (src == 0 || op == ALLEGRO_DEST_MINUS_SRC) {
         result->r = 0;
         result->g = 0;
         result->b = 0;
      }
      else {
         result->r = result->r * src;
         result->g = result->g * src;
         result->b = result->b * src;
      }
   } else if (src == 0) {
      if (op == ALLEGRO_SRC_MINUS_DEST) {
         result->r = 0;
         result->g = 0;
         result->b = 0;
      }
      else {
         result->r = dcol.r * dst;
         result->g = dcol.g * dst;
         result->b = dcol.b * dst;
      }
   } else if (op == ALLEGRO_ADD) {
      result->r = MIN(1, result->r * src + dcol.r * dst);
      result->g = MIN(1, result->g * src + dcol.g * dst);
      result->b = MIN(1, result->b * src + dcol.b * dst);
   }
   else if (op == ALLEGRO_SRC_MINUS_DEST) {
      result->r = MAX(0, result->r * src - dcol.r * dst);
      result->g = MAX(0, result->g * src - dcol.g * dst);
      result->b = MAX(0, result->b * src - dcol.b * dst);
   }
   else if (op == ALLEGRO_DEST_MINUS_SRC) {
      result->r = MAX(0, dcol.r * dst - result->r * src);
      result->g = MAX(0, dcol.g * dst - result->g * src);
      result->b = MAX(0, dcol.b * dst - result->b * src);
   }

   if (adst == 0)
      if (asrc == 0 || aop == ALLEGRO_DEST_MINUS_SRC)
         result->a = 0;
      else
         result->a = result->a * asrc;
   else if (asrc == 0)
      if (aop == ALLEGRO_SRC_MINUS_DEST)
         result->a = 0;
      else
         result->a = dcol.a * adst;
   else if (aop == ALLEGRO_ADD)
      result->a = MIN(1, result->a * asrc + dcol.a * adst);
   else if (aop == ALLEGRO_SRC_MINUS_DEST)
      result->a = MAX(0, result->a * asrc - dcol.a * adst);
   else if (aop == ALLEGRO_DEST_MINUS_SRC)
      result->a = MAX(0, dcol.a * adst - result->a * asrc);
}
