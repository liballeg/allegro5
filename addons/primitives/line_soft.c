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
 *      Software line implementation functions.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */


#define _AL_NO_BLEND_INLINE_FUNC

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include <math.h>

/*
Nomenclature
shader_{texture}_{grad,solid}_{any,rgb888,rgba8888,etc}_{draw_{shade,opaque},step,first}
*/

typedef void (*shader_draw)(uintptr_t, int, int);
typedef void (*shader_first)(uintptr_t, int, int, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*);
typedef void (*shader_step)(uintptr_t, int);

typedef struct {
   ALLEGRO_COLOR color;
} state_solid_any_2d;

static void shader_solid_any_draw_shade(uintptr_t state, int x, int y)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   al_put_blended_pixel(x, y, s->color);
}

static void shader_solid_any_draw_opaque(uintptr_t state, int x, int y)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   al_put_pixel(x, y, s->color);
}

static void shader_solid_any_first(uintptr_t state, int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   s->color = v1->color;

   (void)start_x;
   (void)start_y;
   (void)v2;
}

static void shader_solid_any_step(uintptr_t state, int minor_step)
{
   (void)state;
   (void)minor_step;
}

/*----------------------------------------------------*/

typedef struct {
   state_solid_any_2d solid;
   ALLEGRO_COLOR minor_color;
   ALLEGRO_COLOR major_color;
} state_grad_any_2d;

static void get_interpolation_parameters(int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, float* param, float* minor_delta_param, float* major_delta_param)
{
   float dx = v2->x - v1->x;
   float dy = v2->y - v1->y;
   float lensq = dx * dx + dy * dy;
   
   if (lensq == 0) {
      lensq = 0.0001f;
      *param = 0;
   } else {
      *param = ((float)start_x - v1->x) * dx + ((float)start_y - v1->y) * dy;
      *param /= lensq;
   }
   
   dx = fabsf(dx);
   dy = fabsf(dy);
   if (dx > dy)
      *minor_delta_param = dx / lensq;
   else
      *minor_delta_param = dy / lensq;
   *major_delta_param = (dx + dy) / lensq;
}

static void shader_grad_any_first(uintptr_t state, int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   float param;
   float minor_delta_param;
   float major_delta_param;
   state_grad_any_2d* st = (state_grad_any_2d*)state;
   ALLEGRO_COLOR diff, v1c, v2c;

   v1c = v1->color;
   v2c = v2->color;

   get_interpolation_parameters(start_x, start_y, v1, v2, &param, &minor_delta_param, &major_delta_param);
  
   diff.a = v2c.a - v1c.a;
   diff.r = v2c.r - v1c.r;
   diff.g = v2c.g - v1c.g;
   diff.b = v2c.b - v1c.b;
   
   st->solid.color.a = v1c.a + diff.a * param;
   st->solid.color.r = v1c.r + diff.r * param;
   st->solid.color.g = v1c.g + diff.g * param;
   st->solid.color.b = v1c.b + diff.b * param;
   
   st->minor_color.a = diff.a * minor_delta_param;
   st->minor_color.r = diff.r * minor_delta_param;
   st->minor_color.g = diff.g * minor_delta_param;
   st->minor_color.b = diff.b * minor_delta_param;
   
   st->major_color.a = diff.a * major_delta_param;
   st->major_color.r = diff.r * major_delta_param;
   st->major_color.g = diff.g * major_delta_param;
   st->major_color.b = diff.b * major_delta_param;
}

static void shader_grad_any_step(uintptr_t state, int minor_step)
{
   state_grad_any_2d* s = (state_grad_any_2d*)state;
   if (minor_step) {
      s->solid.color.a += s->minor_color.a;
      s->solid.color.r += s->minor_color.r;
      s->solid.color.g += s->minor_color.g;
      s->solid.color.b += s->minor_color.b;
   } else {
      s->solid.color.a += s->major_color.a;
      s->solid.color.r += s->major_color.r;
      s->solid.color.g += s->major_color.g;
      s->solid.color.b += s->major_color.b;
   }
}

/*===================== Texture Shaders =======================*/

static int fix_var(float var, int max_var)
{
   const int ivar = (int)floorf(var);
   const int ret = ivar % max_var;
   if(ret >= 0)
      return ret;
   else
      return ret + max_var;
}

#define SHADE_COLORS(A, B) \
   A.r = B.r * A.r;        \
   A.g = B.g * A.g;        \
   A.b = B.b * A.b;        \
   A.a = B.a * A.a;

#define FIX_UV const int u = fix_var(s->u, s->w); const int v = fix_var(s->v, s->h);

typedef struct {
   ALLEGRO_COLOR color;
   ALLEGRO_BITMAP* texture;
   int w, h;
   float u, v;
   float minor_du;
   float minor_dv;
   float major_du;
   float major_dv;
} state_texture_solid_any_2d;

static void shader_texture_solid_any_draw_shade(uintptr_t state, int x, int y)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;
   FIX_UV

   ALLEGRO_COLOR color = al_get_pixel(s->texture, u, v);
   SHADE_COLORS(color, s->color)
   al_put_blended_pixel(x, y, color);
}

static void shader_texture_solid_any_draw_shade_white(uintptr_t state, int x, int y)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;
   FIX_UV

   al_put_blended_pixel(x, y, al_get_pixel(s->texture, u, v));
}

static void shader_texture_solid_any_draw_opaque(uintptr_t state, int x, int y)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;
   FIX_UV

   ALLEGRO_COLOR color = al_get_pixel(s->texture, u, v);
   SHADE_COLORS(color, s->color)
   al_put_pixel(x, y, color);
}

static void shader_texture_solid_any_draw_opaque_white(uintptr_t state, int x, int y)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;
   FIX_UV

   al_put_pixel(x, y, al_get_pixel(s->texture, u, v));
}

static void shader_texture_solid_any_first(uintptr_t state, int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   float param;
   float minor_delta_param;
   float major_delta_param;
   state_texture_solid_any_2d* st = (state_texture_solid_any_2d*)state;
   float du, dv;
   ALLEGRO_COLOR v1c;

   v1c = v1->color;

   get_interpolation_parameters(start_x, start_y, v1, v2, &param, &minor_delta_param, &major_delta_param);
  
   st->w = al_get_bitmap_width(st->texture);
   st->h = al_get_bitmap_height(st->texture);

   du = v2->u - v1->u;
   dv = v2->v - v1->v;
   
   st->color.r = v1c.r;
   st->color.g = v1c.g;
   st->color.b = v1c.b;
   st->color.a = v1c.a;

   st->u = v1->u + du * param;
   st->v = v1->v + dv * param;
   
   st->minor_du = du * minor_delta_param;
   st->minor_dv = dv * minor_delta_param;

   st->major_du = du * major_delta_param;
   st->major_dv = dv * major_delta_param;
}

static void shader_texture_solid_any_step(uintptr_t state, int minor_step)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;
   if (minor_step) {
      s->u += s->minor_du;
      s->v += s->minor_dv;
   } else {
      s->u += s->major_du;
      s->v += s->major_dv;
   }
}

/*----------------------------------------------------*/

typedef struct {
   state_texture_solid_any_2d solid;
   ALLEGRO_COLOR minor_color;
   ALLEGRO_COLOR major_color;
} state_texture_grad_any_2d;

static void shader_texture_grad_any_first(uintptr_t state, int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   float param;
   float minor_delta_param;
   float major_delta_param;
   state_texture_grad_any_2d* st = (state_texture_grad_any_2d*)state;
   float du, dv;
   ALLEGRO_COLOR diff, v1c, v2c;

   v1c = v1->color;
   v2c = v2->color;

   get_interpolation_parameters(start_x, start_y, v1, v2, &param, &minor_delta_param, &major_delta_param);
  
   st->solid.w = al_get_bitmap_width(st->solid.texture);
   st->solid.h = al_get_bitmap_height(st->solid.texture);

   du = v2->u - v1->u;
   dv = v2->v - v1->v;
   
   st->solid.color.r = v1c.r;
   st->solid.color.g = v1c.g;
   st->solid.color.b = v1c.b;
   st->solid.color.a = v1c.a;

   st->solid.u = v1->u + du * param;
   st->solid.v = v1->v + dv * param;
   
   st->solid.minor_du = du * minor_delta_param;
   st->solid.minor_dv = dv * minor_delta_param;

   st->solid.major_du = du * major_delta_param;
   st->solid.major_dv = dv * major_delta_param;
  
   diff.a = v2c.a - v1c.a;
   diff.r = v2c.r - v1c.r;
   diff.g = v2c.g - v1c.g;
   diff.b = v2c.b - v1c.b;
   
   st->solid.color.a = v1c.a + diff.a * param;
   st->solid.color.r = v1c.r + diff.r * param;
   st->solid.color.g = v1c.g + diff.g * param;
   st->solid.color.b = v1c.b + diff.b * param;
   
   st->minor_color.a = diff.a * minor_delta_param;
   st->minor_color.r = diff.r * minor_delta_param;
   st->minor_color.g = diff.g * minor_delta_param;
   st->minor_color.b = diff.b * minor_delta_param;
   
   st->major_color.a = diff.a * major_delta_param;
   st->major_color.r = diff.r * major_delta_param;
   st->major_color.g = diff.g * major_delta_param;
   st->major_color.b = diff.b * major_delta_param;
}

static void shader_texture_grad_any_step(uintptr_t state, int minor_step)
{
   state_texture_grad_any_2d* s = (state_texture_grad_any_2d*)state;
   shader_texture_solid_any_step(state, minor_step);
   if (minor_step) {
      s->solid.color.a += s->minor_color.a;
      s->solid.color.r += s->minor_color.r;
      s->solid.color.g += s->minor_color.g;
      s->solid.color.b += s->minor_color.b;
   } else {
      s->solid.color.a += s->major_color.a;
      s->solid.color.r += s->major_color.r;
      s->solid.color.g += s->major_color.g;
      s->solid.color.b += s->major_color.b;
   }
}

static void line_stepper(uintptr_t state, shader_first first, shader_step step, shader_draw draw, ALLEGRO_VERTEX* vtx1, ALLEGRO_VERTEX* vtx2)
{
   float x1, y1, x2, y2;
   float dx, dy;
   int end_x, end_y;

   if (vtx2->y < vtx1->y) {
      ALLEGRO_VERTEX* t;
      t = vtx1;
      vtx1 = vtx2;
      vtx2 = t;
   }
   
   vtx1->x -= 0.5001f;
   vtx1->y -= 0.5001f;
   vtx2->x -= 0.5001f;
   vtx2->y -= 0.5001f;

   x1 = vtx1->x;
   y1 = vtx1->y;
   x2 = vtx2->x;
   y2 = vtx2->y;
   
   dx = x2 - x1;
   dy = y2 - y1;
   
   end_x = floorf(x2 + 0.5f);
   end_y = floorf(y2 + 0.5f);
   
#define FIRST                                                              \
   first(state, x, y, vtx1, vtx2);                                         \
   if((x2 - x1) * ((float)x - x1) + (y2 - y1) * ((float)y - y1) >= 0)      \
      draw(state, x, y);                                                   \
   (void)minor;
   
#define STEP                                                               \
   step(state, minor);                                                     \
   draw(state, x, y);
   
#define LAST                                                               \
   step(state, minor);                                                     \
   if((x1 - x2) * ((float)x - x2) + (y1 - y2) * ((float)y - y2) > 0)       \
      draw(state, x, y);
   
   
#define WORKER(var1, var2, comp, dvar1, dvar2, derr1, derr2, func)         \
   {                                                                       \
      int minor = 1;                                                       \
      if(err comp)                                                         \
      {                                                                    \
         var1 += dvar1;                                                    \
         err += derr1;                                                     \
         minor = 0;                                                        \
      }                                                                    \
      \
      func                                                                 \
      \
      var2 += dvar2;                                                       \
      err += derr2;                                                        \
   }
   
   if (dx > 0) {
      if (dx > dy) {
         int x = floorf(x1 + 0.5f);
         int y = floorf(y1);
         
         float err = (y1 - (float)y) * dx - (x1 - (float)x) * dy;
         
         if (x < end_x) {
            WORKER(y, x, > 0.5f * dx, 1, 1, -dx, dy, FIRST)
         }
         
         while (x < end_x) {
            WORKER(y, x, > 0.5f * dx, 1, 1, -dx, dy, STEP)
         }
         
         if (x <= end_x) {
            WORKER(y, x, > 0.5f * dx, 1, 1, -dx, dy, LAST)
            
         }
      } else {
         int x = floorf(x1);
         int y = floorf(y1 + 0.5f);
         
         float err = (x1 - (float)x) * dy - (y1 - (float)y) * dx;
         
         if (y < end_y) {
            WORKER(x, y, > 0.5f * dy, 1, 1, -dy, dx, FIRST)
         }
         
         while (y < end_y) {
            WORKER(x, y, > 0.5f * dy, 1, 1, -dy, dx, STEP)
         }
         
         if (y <= end_y) {
            WORKER(x, y, > 0.5f * dy, 1, 1, -dy, dx, LAST)
         }
      }
   } else {
      if (-dx > dy) {
         int x = floorf(x1 + 0.5f);
         int y = floorf(y1);
         
         float err = (y1 - (float)y) * dx - (x1 - (float)x) * dy;
         
         if (x > end_x) {
            WORKER(y, x, <= 0.5f * dx, 1, -1, -dx, -dy, FIRST)
         }
         
         while (x > end_x) {
            WORKER(y, x, <= 0.5f * dx, 1, -1, -dx, -dy, STEP)
         }
         
         if (x >= end_x) {
            WORKER(y, x, <= 0.5f * dx, 1, -1, -dx, -dy, LAST)
         }
      } else {
         int x = floorf(x1);
         int y = floorf(y1 + 0.5f);
         
         float err = (x1 - (float)x) * dy - (y1 - (float)y) * dx;
         
         /*
         This is the only correction that needs to be made in the opposite direction of dy (or dx)
         */
         if (err > 0.5f * dy) {
            x += 1;
            err -= dy;
         }
         
         if (y < end_y) {
            WORKER(x, y, <= -0.5f * dy, -1, 1, dy, dx, FIRST)
         }
         
         while (y < end_y) {
            WORKER(x, y, <= -0.5f * dy, -1, 1, dy, dx, STEP)
         }
         
         if (y <= end_y) {
            WORKER(x, y, <= -0.5f * dy, -1, 1, dy, dx, LAST)
         }
      }
   }
#undef FIRST
#undef LAST
#undef STEP
#undef WORKER
}

/*
This one will check to see what exactly we need to draw...
I.e. this will call all of the actual renderers and set the appropriate callbacks
*/
void _al_line_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   int shade = 1;
   int grad = 1;
   int op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha;
   ALLEGRO_COLOR v1c, v2c;

   v1c = v1->color;
   v2c = v2->color;
   
   al_get_separate_blender(&op, &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);
   if (_AL_DEST_IS_ZERO && _AL_SRC_NOT_MODIFIED) {
      shade = 0;
   }
   
   if (v1c.r == v2c.r && v1c.g == v2c.g && v1c.b == v2c.b && v1c.a == v2c.a) {
      grad = 0;
   }
   
   if (texture) {
      if (grad) {
         state_texture_grad_any_2d state;
         state.solid.texture = texture;

         if (shade) {
            al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_texture_grad_any_first, shader_texture_grad_any_step, shader_texture_solid_any_draw_shade);
         } else {
            al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_texture_grad_any_first, shader_texture_grad_any_step, shader_texture_solid_any_draw_opaque);
         }
      } else {
         int white = 0;
         state_texture_solid_any_2d state;

         if (v1c.r == 1 && v1c.g == 1 && v1c.b == 1 && v1c.a == 1) {
            white = 1;
         }
         state.texture = texture;

         if (shade) {
            if(white) {
               al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_shade_white);
            } else {
               al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_shade);
            }
         } else {
            if(white) {
               al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_opaque_white);
            } else {
               al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_opaque);
            }
         }
      }
   } else {
      if (grad) {
         state_grad_any_2d state;
         if (shade) {
            al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_grad_any_first, shader_grad_any_step, shader_solid_any_draw_shade);
         } else {
            al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_grad_any_first, shader_grad_any_step, shader_solid_any_draw_opaque);
         }
      } else {
         state_solid_any_2d state;
         if (shade) {
            al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_solid_any_first, shader_solid_any_step, shader_solid_any_draw_shade);
         } else {
            al_draw_soft_line(v1, v2, (uintptr_t)&state, shader_solid_any_first, shader_solid_any_step, shader_solid_any_draw_opaque);
         }
      }
   }
}

/* Function: al_draw_soft_line
 */
void al_draw_soft_line(ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, uintptr_t state,
   void (*first)(uintptr_t, int, int, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
   void (*step)(uintptr_t, int),
   void (*draw)(uintptr_t, int, int))
{
   /*
   Copy the vertices, because we need to alter them a bit before drawing.
   */
   ALLEGRO_VERTEX vtx1 = *v1;
   ALLEGRO_VERTEX vtx2 = *v2;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   int need_unlock = 0;
   ALLEGRO_LOCKED_REGION *lr;
   int min_x, max_x, min_y, max_y;
   int clip_min_x, clip_min_y, clip_max_x, clip_max_y;
   
   al_get_clipping_rectangle(&clip_min_x, &clip_min_y, &clip_max_x, &clip_max_y);
   clip_max_x += clip_min_x;
   clip_max_y += clip_min_y;
   
   /*
   TODO: Need to clip them first, make a copy of the vertices first then
   */
   
   /*
   Lock the region we are drawing to. We are choosing the minimum and maximum
   possible pixels touched from the formula (easily verified by following the
   above algorithm.
   */

   if (vtx1.x >= vtx2.x) {
      max_x = (int)ceilf(vtx1.x) + 1;
      min_x = (int)floorf(vtx2.x) - 1;
   } else {
      max_x = (int)ceilf(vtx2.x) + 1;
      min_x = (int)floorf(vtx1.x) - 1;
   }
   if (vtx1.y >= vtx2.y) {
      max_y = (int)ceilf(vtx1.y) + 1;
      min_y = (int)floorf(vtx2.y) - 1;
   } else {
      max_y = (int)ceilf(vtx2.y) + 1;
      min_y = (int)floorf(vtx1.y) - 1;
   }
   /*
   TODO: This bit is temporary, the min max's will be guaranteed to be within the bitmap
   once clipping is implemented
   */
   if (min_x >= clip_max_x || min_y >= clip_max_y)
      return;
   if (max_x >= clip_max_x)
      max_x = clip_max_x;
   if (max_y >= clip_max_y)
      max_y = clip_max_y;

   if (max_x < clip_min_x || max_y < clip_min_y)
      return;
   if (min_x < clip_min_x)
      min_x = clip_min_x;
   if (min_y < clip_min_y)
      min_y = clip_min_y;

   if (al_is_bitmap_locked(target)) {
      if (!_al_bitmap_region_is_locked(target, min_x, min_y, max_x - min_x, max_y - min_y))
         return;
   } else {
      if (!(lr = al_lock_bitmap_region(target, min_x, min_y, max_x - min_x, max_y - min_y, ALLEGRO_PIXEL_FORMAT_ANY, 0)))
         return;
      need_unlock = 1;
   }

   line_stepper(state, first, step, draw, &vtx1, &vtx2);

   if (need_unlock)
      al_unlock_bitmap(target);
}
