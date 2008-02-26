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
#include "allegro5/internal/aintern_bitmap.h"
#include <string.h>



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
   *result = al_map_rgba_f(
      dcol->r*scol->a,
      dcol->g*scol->a,
      dcol->b*scol->a,
      dcol->a*scol->a);
}



void _al_blender_zero_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   *result = al_map_rgba_f(
      dcol->r*(1.0f-scol->a),
      dcol->g*(1.0f-scol->a),
      dcol->b*(1.0f-scol->a),
      dcol->a*(1.0f-scol->a));
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

   r = MIN(1.0f, dcol->r*scol->a + scol->r * bc->r);
   g = MIN(1.0f, dcol->g*scol->a + scol->g * bc->g);
   b = MIN(1.0f, dcol->b*scol->a + scol->b * bc->b);
   a = MIN(1.0f, dcol->a*scol->a + scol->a * bc->a);

   *result = al_map_rgba_f(r, g, b, a);
}



void _al_blender_one_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   float r, g, b, a;
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   r = MIN(1.0f, dcol->r*(1.0f-scol->a) + scol->r * bc->r);
   g = MIN(1.0f, dcol->g*(1.0f-scol->a) + scol->g * bc->g);
   b = MIN(1.0f, dcol->b*(1.0f-scol->a) + scol->b * bc->b);
   a = MIN(1.0f, dcol->a*(1.0f-scol->a) + scol->a * bc->a);

   *result = al_map_rgba_f(r, g, b, a);
}



void _al_blender_alpha_zero(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      scol->r*scol->a*bc->r,
      scol->g*scol->a*bc->g,
      scol->b*scol->a*bc->b,
      scol->a*scol->a*bc->a);
}



void _al_blender_alpha_one(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*scol->a*bc->r + dcol->r),
      MIN(1.0f, scol->g*scol->a*bc->g + dcol->g),
      MIN(1.0f, scol->b*scol->a*bc->b + dcol->b),
      MIN(1.0f, scol->a*scol->a*bc->a + dcol->a));
}



void _al_blender_alpha_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*scol->a*bc->r + dcol->r*scol->a),
      MIN(1.0f, scol->g*scol->a*bc->g + dcol->g*scol->a),
      MIN(1.0f, scol->b*scol->a*bc->b + dcol->b*scol->a),
      MIN(1.0f, scol->a*scol->a*bc->a + dcol->a*scol->a));
}



void _al_blender_alpha_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*scol->a*bc->r + dcol->r*(1.0f-scol->a)),
      MIN(1.0f, scol->g*scol->a*bc->g + dcol->g*(1.0f-scol->a)),
      MIN(1.0f, scol->b*scol->a*bc->b + dcol->b*(1.0f-scol->a)),
      MIN(1.0f, scol->a*scol->a*bc->a + dcol->a*(1.0f-scol->a)));
}



void _al_blender_inverse_alpha_zero(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      scol->r*(1.0f-scol->a)*bc->r,
      scol->g*(1.0f-scol->a)*bc->g,
      scol->b*(1.0f-scol->a)*bc->b,
      scol->a*(1.0f-scol->a)*bc->a);
}



void _al_blender_inverse_alpha_one(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*(1.0f-scol->a)*bc->r + dcol->r),
      MIN(1.0f, scol->g*(1.0f-scol->a)*bc->g + dcol->g),
      MIN(1.0f, scol->b*(1.0f-scol->a)*bc->b + dcol->b),
      MIN(1.0f, scol->a*(1.0f-scol->a)*bc->a + dcol->a));
}



void _al_blender_inverse_alpha_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*(1.0f-scol->a)*bc->r + dcol->r*scol->a),
      MIN(1.0f, scol->g*(1.0f-scol->a)*bc->g + dcol->g*scol->a),
      MIN(1.0f, scol->b*(1.0f-scol->a)*bc->b + dcol->b*scol->a),
      MIN(1.0f, scol->a*(1.0f-scol->a)*bc->a + dcol->a*scol->a));
}



void _al_blender_inverse_alpha_inverse_alpha(ALLEGRO_COLOR *scol,
   ALLEGRO_COLOR *dcol, ALLEGRO_COLOR *result)
{
   ALLEGRO_COLOR *bc = _al_get_blend_color();

   *result = al_map_rgba_f(
      MIN(1.0f, scol->r*(1.0f-scol->a)*bc->r + dcol->r*(1.0f-scol->a)),
      MIN(1.0f, scol->g*(1.0f-scol->a)*bc->g + dcol->g*(1.0f-scol->a)),
      MIN(1.0f, scol->b*(1.0f-scol->a)*bc->b + dcol->b*(1.0f-scol->a)),
      MIN(1.0f, scol->a*(1.0f-scol->a)*bc->a + dcol->a*(1.0f-scol->a)));
}


void _al_blend(ALLEGRO_COLOR *scol,
   int dx, int dy, ALLEGRO_COLOR *result)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_MEMORY_BLENDER blender = _al_get_memory_blender();
   ALLEGRO_COLOR dcol;

   dcol = al_get_pixel(dest, dx, dy);

   blender(scol, &dcol, result);
}

