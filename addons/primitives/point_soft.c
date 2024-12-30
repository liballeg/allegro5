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
 *      Software point implementation functions.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */


#define _AL_NO_BLEND_INLINE_FUNC

#define ALLEGRO_INTERNAL_UNSTABLE
#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include <math.h>

int _al_fix_texcoord(float var, int max_var, ALLEGRO_BITMAP_WRAP wrap)
{
   int ivar = (int)floorf(var);
   float ret;
   switch (wrap) {
      case ALLEGRO_BITMAP_WRAP_CLAMP:
         if (ivar < 0)
            ret = 0;
         else if (ivar > max_var - 1)
            ret = max_var - 1;
         else
            ret = ivar;
         break;
      case ALLEGRO_BITMAP_WRAP_MIRROR: {
         ret = ivar % max_var;
         if(ret < 0)
            ret += max_var;
         if ((int)floorf((float)ivar / max_var) % 2)
            ret = max_var - 1 - ret;
         break;
      }
      default: {
         ret = ivar % max_var;
         if(ret < 0)
            ret += max_var;
      }
   }
   return ret;
}

void _al_point_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v)
{
   int shade = 1;
   int op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha;
   ALLEGRO_COLOR vc;
   int clip_min_x, clip_min_y, clip_max_x, clip_max_y;
   int x = (int)floorf(v->x);
   int y = (int)floorf(v->x);

   al_get_clipping_rectangle(&clip_min_x, &clip_min_y, &clip_max_x, &clip_max_y);
   clip_max_x += clip_min_x;
   clip_max_y += clip_min_y;

   if(x < clip_min_x || x >= clip_max_x || y < clip_min_y || y >= clip_max_y)
      return;

   vc = v->color;

   al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);
   if (_AL_DEST_IS_ZERO && _AL_SRC_NOT_MODIFIED) {
      shade = 0;
   }

   if (texture) {
      ALLEGRO_BITMAP_WRAP wrap_u, wrap_v;
      _al_get_bitmap_wrap(texture, &wrap_u, &wrap_v);
      int iu = _al_fix_texcoord(v->u, al_get_bitmap_width(texture), wrap_u);
      int iv = _al_fix_texcoord(v->v, al_get_bitmap_height(texture), wrap_v);
      ALLEGRO_COLOR color = al_get_pixel(texture, iu, iv);

      if(vc.r != 1 || vc.g != 1 || vc.b != 1 || vc.a != 1) {
         color.r *= vc.r;
         color.g *= vc.g;
         color.b *= vc.b;
         color.a *= vc.a;
      }

      if (shade) {
         al_put_blended_pixel(v->x, v->y, color);
      } else {
         al_put_pixel(v->x, v->y, color);
      }
   } else {
      ALLEGRO_COLOR color = al_map_rgba_f(vc.r, vc.g, vc.b, vc.a);
      if (shade) {
         al_put_blended_pixel(v->x, v->y, color);
      } else {
         al_put_pixel(v->x, v->y, color);
      }
   }
}
