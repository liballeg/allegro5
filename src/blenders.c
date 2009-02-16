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


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include <string.h>

#define MIN _ALLEGRO_MIN
#define MAX _ALLEGRO_MAX

/* TODO: Commented out optimized version without alpha separation.
 * Note the optimization is per-pixel only - so whenever these functions
 * get used things are going to be slow anyway (as the function is
 * called for each single pixel). Things like drawing an alpha
 * blended bitmap or a fully opaque filled rectangle have much more
 * potential for optimization on a higher level.
 */
#if 0
void _al_blender_zero_zero(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   memset(result, 0, sizeof(ALLEGRO_COLOR));
}



void _al_blender_zero_one(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   memcpy(result, dcol, sizeof(ALLEGRO_COLOR));
}



void _al_blender_zero_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      dcol->r*scol->a*bc->a,
      dcol->g*scol->a*bc->a,
      dcol->b*scol->a*bc->a,
      dcol->a*scol->a*bc->a);
}



void _al_blender_zero_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      dcol->r*(1.0f-(scol->a*bc->a)),
      dcol->g*(1.0f-(scol->a*bc->a)),
      dcol->b*(1.0f-(scol->a*bc->a)),
      dcol->a*(1.0f-(scol->a*bc->a)));
}



void _al_blender_one_zero(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      scol->r*bc->r,
      scol->g*bc->g,
      scol->b*bc->b,
      scol->a*bc->a);
}



void _al_blender_one_one(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   float r, g, b, a;
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   r = MIN(1.0f, dcol->r + scol->r * bc->r);
   g = MIN(1.0f, dcol->g + scol->g * bc->g);
   b = MIN(1.0f, dcol->b + scol->b * bc->b);
   a = MIN(1.0f, dcol->a + scol->a * bc->a);

   *result = al_map_rgba_f(r, g, b, a);
}



void _al_blender_one_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   float r, g, b, a;
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   r = MIN(1.0f, dcol->r*scol->a*bc->a + scol->r * bc->r);
   g = MIN(1.0f, dcol->g*scol->a*bc->a + scol->g * bc->g);
   b = MIN(1.0f, dcol->b*scol->a*bc->a + scol->b * bc->b);
   a = MIN(1.0f, dcol->a*scol->a*bc->a + scol->a * bc->a);

   *result = al_map_rgba_f(r, g, b, a);
}



void _al_blender_one_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   float r, g, b, a;
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   r = MIN(1.0f, dcol->r*(1.0f-(scol->a*bc->a)) + scol->r * bc->r);
   g = MIN(1.0f, dcol->g*(1.0f-(scol->a*bc->a)) + scol->g * bc->g);
   b = MIN(1.0f, dcol->b*(1.0f-(scol->a*bc->a)) + scol->b * bc->b);
   a = MIN(1.0f, dcol->a*(1.0f-(scol->a*bc->a)) + scol->a * bc->a);

   *result = al_map_rgba_f(r, g, b, a);
}



void _al_blender_alpha_zero(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      scol->r*scol->a*bc->r*bc->a,
      scol->g*scol->a*bc->g*bc->a,
      scol->b*scol->a*bc->b*bc->a,
      scol->a*scol->a*bc->a*bc->a);
}



void _al_blender_alpha_one(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*scol->a*bc->r*bc->a + dcol->r),
      MIN(1.0f, scol->g*scol->a*bc->g*bc->a + dcol->g),
      MIN(1.0f, scol->b*scol->a*bc->b*bc->a + dcol->b),
      MIN(1.0f, scol->a*scol->a*bc->a*bc->a + dcol->a));
}



void _al_blender_alpha_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*scol->a*bc->r*bc->a + dcol->r*scol->a*bc->a),
      MIN(1.0f, scol->g*scol->a*bc->g*bc->a + dcol->g*scol->a*bc->a),
      MIN(1.0f, scol->b*scol->a*bc->b*bc->a + dcol->b*scol->a*bc->a),
      MIN(1.0f, scol->a*scol->a*bc->a*bc->a + dcol->a*scol->a*bc->a));
}



void _al_blender_alpha_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*scol->a*bc->a*bc->r + dcol->r*(1.0f-(bc->a*scol->a))),
      MIN(1.0f, scol->g*scol->a*bc->a*bc->g + dcol->g*(1.0f-(bc->a*scol->a))),
      MIN(1.0f, scol->b*scol->a*bc->a*bc->b + dcol->b*(1.0f-(bc->a*scol->a))),
      MIN(1.0f, scol->a*scol->a*bc->a*bc->a + dcol->a*(1.0f-(bc->a*scol->a))));
}



void _al_blender_inverse_alpha_zero(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      scol->r*(1.0f-(scol->a*bc->a))*bc->r,
      scol->g*(1.0f-(scol->a*bc->a))*bc->g,
      scol->b*(1.0f-(scol->a*bc->a))*bc->b,
      scol->a*(1.0f-(scol->a*bc->a))*bc->a);
}



void _al_blender_inverse_alpha_one(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*(1.0f-(scol->a*bc->a))*bc->r + dcol->r),
      MIN(1.0f, scol->g*(1.0f-(scol->a*bc->a))*bc->g + dcol->g),
      MIN(1.0f, scol->b*(1.0f-(scol->a*bc->a))*bc->b + dcol->b),
      MIN(1.0f, scol->a*(1.0f-(scol->a*bc->a))*bc->a + dcol->a));
}



void _al_blender_inverse_alpha_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*(1.0f-(scol->a*bc->a))*bc->r + dcol->r*scol->a*bc->a),
      MIN(1.0f, scol->g*(1.0f-(scol->a*bc->a))*bc->g + dcol->g*scol->a*bc->a),
      MIN(1.0f, scol->b*(1.0f-(scol->a*bc->a))*bc->b + dcol->b*scol->a*bc->a),
      MIN(1.0f, scol->a*(1.0f-(scol->a*bc->a))*bc->a + dcol->a*scol->a*bc->a));
}



void _al_blender_inverse_alpha_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*(1.0f-(scol->a*bc->a))*bc->r + dcol->r*(1.0f-(scol->a*bc->a))),
      MIN(1.0f, scol->g*(1.0f-(scol->a*bc->a))*bc->g + dcol->g*(1.0f-(scol->a*bc->a))),
      MIN(1.0f, scol->b*(1.0f-(scol->a*bc->a))*bc->b + dcol->b*(1.0f-(scol->a*bc->a))),
      MIN(1.0f, scol->a*(1.0f-(scol->a*bc->a))*bc->a + dcol->a*(1.0f-(scol->a*bc->a))));
}


// FIXME
void _al_blend_combined_optimized(ALLEGRO_COLOR *scol,
   ALLEGRO_BITMAP *dest,
   int dx, int dy, ALLEGRO_COLOR *result)
{
   ALLEGRO_MEMORY_BLENDER blender = _al_get_memory_blender();
   ALLEGRO_COLOR dcol;

   // FIXME: some blenders won't need this and it may be slow, should
   // optimize it away for those?
   dcol = al_get_pixel(dest, dx, dy);

   blender(scol, &dcol, result);
}
#endif


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
void _al_blend(ALLEGRO_COLOR *scol,
   ALLEGRO_BITMAP *dest,
   int dx, int dy, ALLEGRO_COLOR *result)
{
   float src, dst, asrc, adst;
   int src_, dst_, asrc_, adst_;
   ALLEGRO_COLOR dcol;
   ALLEGRO_COLOR bc;

   dcol = al_get_pixel(dest, dx, dy);
   al_get_separate_blender(&src_, &dst_, &asrc_, &adst_, &bc);
   result->r = scol->r * bc.r;
   result->g = scol->g * bc.g;
   result->b = scol->b * bc.b;
   result->a = scol->a * bc.a;
   src = get_factor(src_, result->a);
   dst = get_factor(dst_, result->a);
   asrc = get_factor(asrc_, result->a);
   adst = get_factor(adst_, result->a);

   result->r = MIN(1, result->r * src + dcol.r * dst);
   result->g = MIN(1, result->g * src + dcol.g * dst);
   result->b = MIN(1, result->b * src + dcol.b * dst);
   result->a = MIN(1, result->a * asrc + dcol.a * adst);
}
