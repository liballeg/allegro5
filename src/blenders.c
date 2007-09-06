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


#include "allegro.h"
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/internal/aintern_color.h"


#define GET_DEST() \
   al_unmap_rgba_f(dst, dst_color, &rd, &gd, &bd, &ad);

#define GET_SRC() \
   al_unmap_rgba_f(src, src_color, &rs, &gs, &bs, &as);



void _al_blender_zero_zero(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   memset(result, 0, sizeof(ALLEGRO_COLOR));
}



void _al_blender_zero_one(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   memcpy(result, dst_color, sizeof(ALLEGRO_COLOR));
}



void _al_blender_zero_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rd, gd, bd, ad;

   GET_DEST();

   al_map_rgba_f(dst, result,
      rd*ad,
      gd*ad,
      bd*ad,
      ad*ad);
}



void _al_blender_zero_inverse_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rd, gd, bd, ad;

   GET_DEST();

   al_map_rgba_f(dst, result,
      rd*(1.0f-ad),
      gd*(1.0f-ad),
      bd*(1.0f-ad),
      ad*(1.0f-ad));
}



void _al_blender_one_zero(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();

   al_map_rgba_f(dst, result,
      rs*bc->r,
      gs*bc->g,
      bs*bc->b,
      as*bc->a);
}



void _al_blender_one_one(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   /* Not exactly sure what's proper here */
   _al_blender_one_zero(src, src_color, dst, dst_color, result);
}



void _al_blender_one_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float r, g, b, a;
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   r = MIN(1.0f, rd*ad + rs * bc->r);
   g = MIN(1.0f, gd*ad + gs * bc->g);
   b = MIN(1.0f, bd*ad + bs * bc->b);
   a = MIN(1.0f, ad*ad + as * bc->a);

   al_map_rgba_f(dst, result, r, g, b, a);
}



void _al_blender_one_inverse_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float r, g, b, a;
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   r = MIN(1.0f, rd*(1.0f-ad) + rs * bc->r);
   g = MIN(1.0f, gd*(1.0f-ad) + gs * bc->g);
   b = MIN(1.0f, bd*(1.0f-ad) + bs * bc->b);
   a = MIN(1.0f, ad*(1.0f-ad) + as * bc->a);

   al_map_rgba_f(dst, result, r, g, b, a);
}



void _al_blender_alpha_zero(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();

   al_map_rgba_f(dst, result,
      rs*as*bc->r,
      gs*as*bc->g,
      bs*as*bc->b,
      as*as*bc->a);
}



void _al_blender_alpha_one(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   al_map_rgba_f(dst, result,
      MIN(1.0f, rs*as*bc->r + rd),
      MIN(1.0f, gs*as*bc->g + gd),
      MIN(1.0f, bs*as*bc->b + bd),
      MIN(1.0f, as*as*bc->a + ad));
}



void _al_blender_alpha_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   al_map_rgba_f(dst, result,
      MIN(1.0f, rs*as*bc->r + rd*ad),
      MIN(1.0f, gs*as*bc->g + gd*ad),
      MIN(1.0f, bs*as*bc->b + bd*ad),
      MIN(1.0f, as*as*bc->a + ad*ad));
}



void _al_blender_alpha_inverse_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   al_map_rgba_f(dst, result,
      MIN(1.0f, rs*as*bc->r + rd*(1.0f-ad)),
      MIN(1.0f, gs*as*bc->g + gd*(1.0f-ad)),
      MIN(1.0f, bs*as*bc->b + bd*(1.0f-ad)),
      MIN(1.0f, as*as*bc->a + ad*(1.0f-ad)));
}



void _al_blender_inverse_alpha_zero(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();

   al_map_rgba_f(dst, result,
      rs*(1.0f-as)*bc->r,
      gs*(1.0f-as)*bc->g,
      bs*(1.0f-as)*bc->b,
      as*(1.0f-as)*bc->a);
}



void _al_blender_inverse_alpha_one(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   al_map_rgba_f(dst, result,
      MIN(1.0f, rs*(1.0f-as)*bc->r + rd),
      MIN(1.0f, gs*(1.0f-as)*bc->g + gd),
      MIN(1.0f, bs*(1.0f-as)*bc->b + bd),
      MIN(1.0f, as*(1.0f-as)*bc->a + ad));
}



void _al_blender_inverse_alpha_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   al_map_rgba_f(dst, result,
      MIN(1.0f, rs*(1.0f-as)*bc->r + rd*ad),
      MIN(1.0f, gs*(1.0f-as)*bc->g + gd*ad),
      MIN(1.0f, bs*(1.0f-as)*bc->b + bd*ad),
      MIN(1.0f, as*(1.0f-as)*bc->a + ad*ad));
}



void _al_blender_inverse_alpha_inverse_alpha(ALLEGRO_BITMAP *src, ALLEGRO_COLOR *src_color,
   ALLEGRO_BITMAP *dst, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result)
{
   float rs, gs, bs, as;
   float rd, gd, bd, ad;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();

   GET_SRC();
   GET_DEST();

   al_map_rgba_f(dst, result,
      MIN(1.0f, rs*(1.0f-as)*bc->r + rd*(1.0f-ad)),
      MIN(1.0f, gs*(1.0f-as)*bc->g + gd*(1.0f-ad)),
      MIN(1.0f, bs*(1.0f-as)*bc->b + bd*(1.0f-ad)),
      MIN(1.0f, as*(1.0f-as)*bc->a + ad*(1.0f-ad)));
}



