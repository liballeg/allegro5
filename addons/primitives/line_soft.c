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


#include "allegro5/a5_primitives.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include <math.h>

typedef void (*shader_draw)(ALLEGRO_BITMAP*, uintptr_t, int, int);
typedef void (*shader_first)(uintptr_t, int, int, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*);
typedef void (*shader_step)(uintptr_t, int);

typedef struct {
   ALLEGRO_COLOR minor_color;
   ALLEGRO_COLOR major_color;
   ALLEGRO_COLOR cur_color;
} state_grad_any_2d;

/*
Nomenclature
shader_{grad,solid}_{any,rgb888,rgba8888,etc}_{2d,3d}_{draw_{shade,opaque},step,first}
*/
static void shader_grad_any_2d_draw_shade(ALLEGRO_BITMAP* dest, uintptr_t state, int x, int y)
{
   /*
   This is a little bad, to say the least
   Both the blend and put_pixel involve a lot of needless checks and calculations...
   One'd need to change the API a good bit to alter this
   How?
   I'd need these exposed:
   A raw blend (Allegro color to Allegro color)
   A raw Allegro color to int converter
   A raw putpixel (I could use bmp_write)
   
   Otherwise, I could implement them myself
   */
   
   state_grad_any_2d* s = (state_grad_any_2d*)state;
   (void)dest;
   al_draw_pixel(x, y, s->cur_color);
}

static void shader_grad_any_2d_draw_opaque(ALLEGRO_BITMAP* dest, uintptr_t state, int x, int y)
{
   state_grad_any_2d* s = (state_grad_any_2d*)state;
   (void)dest;
   al_put_pixel(x, y, s->cur_color);
}

static void shader_grad_any_2d_first(uintptr_t state, int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   float dx = v2->x - v1->x;
   float dy = v2->y - v1->y;
   float lensq = dx * dx + dy * dy;
   float param;
   float minor_delta_param;
   float major_delta_param;
   ALLEGRO_COLOR diff;
   state_grad_any_2d* st;
   
   if (lensq == 0) {
      lensq = 0.0001f;
      param = 0;
   } else {
      param = ((float)start_x - v1->x) * dx + ((float)start_y - v1->y) * dy;
      param /= lensq;
   }
   
   
   dx = fabs(dx);
   dy = fabs(dy);
   if (dx > dy)
      minor_delta_param = dx / lensq;
   else
      minor_delta_param = dy / lensq;
   major_delta_param = (dx + dy) / lensq;
   
   st = (state_grad_any_2d*)state;
   
   diff.a = v2->a - v1->a;
   diff.r = v2->r - v1->r;
   diff.g = v2->g - v1->g;
   diff.b = v2->b - v1->b;
   
   st->cur_color.a = v1->a + diff.a * param;
   st->cur_color.r = v1->r + diff.r * param;
   st->cur_color.g = v1->g + diff.g * param;
   st->cur_color.b = v1->b + diff.b * param;
   
   st->minor_color.a = diff.a * minor_delta_param;
   st->minor_color.r = diff.r * minor_delta_param;
   st->minor_color.g = diff.g * minor_delta_param;
   st->minor_color.b = diff.b * minor_delta_param;
   
   st->major_color.a = diff.a * major_delta_param;
   st->major_color.r = diff.r * major_delta_param;
   st->major_color.g = diff.g * major_delta_param;
   st->major_color.b = diff.b * major_delta_param;
}

static void shader_grad_any_2d_step(uintptr_t state, int minor_step)
{
   state_grad_any_2d* s = (state_grad_any_2d*)state;
   if (minor_step) {
      s->cur_color.a += s->minor_color.a;
      s->cur_color.r += s->minor_color.r;
      s->cur_color.g += s->minor_color.g;
      s->cur_color.b += s->minor_color.b;
   } else {
      s->cur_color.a += s->major_color.a;
      s->cur_color.r += s->major_color.r;
      s->cur_color.g += s->major_color.g;
      s->cur_color.b += s->major_color.b;
   }
}

typedef struct {
   ALLEGRO_COLOR color;
} state_solid_any_2d;

static void shader_solid_any_2d_draw_shade(ALLEGRO_BITMAP* dest, uintptr_t state, int x, int y)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   (void)dest;
   al_draw_pixel(x, y, s->color);
}

static void shader_solid_any_2d_draw_opaque(ALLEGRO_BITMAP* dest, uintptr_t state, int x, int y)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   (void)dest;
   al_put_pixel(x, y, s->color);
}

static void shader_solid_any_2d_first(uintptr_t state, int start_x, int start_y, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   s->color.r = v1->r;
   s->color.g = v1->g;
   s->color.b = v1->b;
   s->color.a = v1->a;

   (void)start_x;
   (void)start_y;
   (void)v2;
}

static void shader_solid_any_2d_step(uintptr_t state, int minor_step)
{
   (void)state;
   (void)minor_step;
}

static void line_stepper(ALLEGRO_BITMAP* dest, uintptr_t state, shader_first first, shader_step step, shader_draw draw, ALLEGRO_VERTEX* vtx1, ALLEGRO_VERTEX* vtx2)
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
   
   /*TODO: These offsets give me the best result... maybe they are not universal though?*/
   vtx1->x -= 0.49f;
   vtx1->y -= 0.51f;
   vtx2->x -= 0.49f;
   vtx2->y -= 0.51f;

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
      draw(dest, state, x, y);
   
#define STEP                                                               \
   step(state, minor);                                                     \
   draw(dest, state, x, y);
   
#define LAST                                                               \
   step(state, minor);                                                     \
   if((x1 - x2) * ((float)x - x2) + (y1 - y2) * ((float)y - y2) > 0)       \
      draw(dest, state, x, y);
   
   
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
   /*
   Copy the vertices, because we need to alter them a bit before drawing.
   */
   ALLEGRO_VERTEX vtx1 = *v1;
   ALLEGRO_VERTEX vtx2 = *v2;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   int need_unlock = 0;
   ALLEGRO_LOCKED_REGION *lr;
   int min_x, max_x, min_y, max_y;
   int shade = 1;
   int grad = 1;
   int src_mode, dst_mode, src_alpha, dst_alpha;
   int width = al_get_bitmap_width(target);
   int height = al_get_bitmap_height(target);
   ALLEGRO_COLOR ic;
   
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
   if (max_x >= width)
      max_x = width - 1;
   if (max_y >= height)
      max_y = height - 1;
   if (min_x >= width)
      min_x = width - 1;
   if (min_y >= height)
      min_y = height - 1;

   if (max_x < 0)
      max_x = 0;
   if (max_y < 0)
      max_y = 0;
   if (min_x < 0)
      min_x = 0;
   if (min_y < 0)
      min_y = 0;

   // FIXME: Something is wrong here. Above, the integer pixels are
   // clipped to the range 0...w-1. So e.g. in a 1x1 bitmap, min_x and
   // max_x are both 0, no matter how long the line is...
   if (al_is_bitmap_locked(target)) {
      if (!_al_bitmap_region_is_locked(target, min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y))
         return;
   } else {
      if (!(lr = al_lock_bitmap_region(target, min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, ALLEGRO_PIXEL_FORMAT_ANY, 0)))
         return;
      need_unlock = 1;
   }
   
   /*
   TODO: Make more specialized functions (constant colour, no blending etc and etc)
   */
   
   al_get_separate_blender(&src_mode, &dst_mode, &src_alpha, &dst_alpha, &ic);
   if (src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE &&
      dst_mode == ALLEGRO_ZERO && dst_alpha == ALLEGRO_ZERO &&
         ic.r == 1.0f && ic.g == 1.0f && ic.b == 1.0f && ic.a == 1.0f) {
      shade = 0;
   }
   
   if (vtx1.r == vtx2.r && vtx1.g == vtx2.g && vtx1.b == vtx2.b && vtx1.a == vtx2.a) {
      grad = 0;
   }
   
   if (texture) {
   
   } else {
      if (grad) {
         state_grad_any_2d state;
         if (shade) {
            line_stepper(target, (uintptr_t)&state, shader_grad_any_2d_first, shader_grad_any_2d_step, shader_grad_any_2d_draw_shade, &vtx1, &vtx2);
         } else {
            line_stepper(target, (uintptr_t)&state, shader_grad_any_2d_first, shader_grad_any_2d_step, shader_grad_any_2d_draw_opaque, &vtx1, &vtx2);
         }
      } else {
         state_solid_any_2d state;
         if (shade) {
            line_stepper(target, (uintptr_t)&state, shader_solid_any_2d_first, shader_solid_any_2d_step, shader_solid_any_2d_draw_shade, &vtx1, &vtx2);
         } else {
            line_stepper(target, (uintptr_t)&state, shader_solid_any_2d_first, shader_solid_any_2d_step, shader_solid_any_2d_draw_opaque, &vtx1, &vtx2);
         }
      }
   }
   
   if (need_unlock)
      al_unlock_bitmap(target);
}
