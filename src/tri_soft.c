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
 *      Software triangle implementation functions.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_blend.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_primitives.h"
#include "allegro5/internal/aintern_tri_soft.h"
#include <math.h>

ALLEGRO_DEBUG_CHANNEL("tri_soft")

#define MIN _ALLEGRO_MIN
#define MAX _ALLEGRO_MAX

typedef void (*shader_draw)(uintptr_t, int, int, int);
typedef void (*shader_init)(uintptr_t, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*);
typedef void (*shader_first)(uintptr_t, int, int, int, int);
typedef void (*shader_step)(uintptr_t, int);

typedef struct {
   ALLEGRO_BITMAP *target;
   ALLEGRO_COLOR cur_color;
} state_solid_any_2d;

static void shader_solid_any_init(uintptr_t state, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3)
{
   state_solid_any_2d* s = (state_solid_any_2d*)state;
   s->target = al_get_target_bitmap();
   s->cur_color = v1->color;

   (void)v2;
   (void)v3;
}

static void shader_solid_any_first(uintptr_t state, int x1, int y, int left_minor, int left_major)
{
   (void)state;
   (void)x1;
   (void)y;
   (void)left_minor;
   (void)left_major;
}

static void shader_solid_any_step(uintptr_t state, int minor)
{
   (void)state;
   (void)minor;
}

/*----------------------------------------------------------------------------*/

typedef struct {
   state_solid_any_2d solid;

   ALLEGRO_COLOR color_dx;
   ALLEGRO_COLOR color_dy;
   ALLEGRO_COLOR color_const;

   /*
   These are catched for the left edge walking
   */
   ALLEGRO_COLOR minor_color;
   ALLEGRO_COLOR major_color;

   /*
   We use these to increase the precision of interpolation
   */
   float off_x;
   float off_y;
} state_grad_any_2d;

#define PLANE_DETS(var, u1, u2, u3)                             \
   float var##_det = u1 * minor3 - u2 * minor2 + u3 * minor1;   \
   float var##_det_x = u1 * y32 - u2 * y31 + u3 * y21;          \
   float var##_det_y = u1 * x23 - u2 * x13 + u3 * x12;

#define INIT_PREAMBLE \
   const float x1 = 0;                      \
   const float y1 = 0;                      \
                                            \
   const float x2 = v2->x - v1->x;          \
   const float y2 = v2->y - v1->y;          \
                                            \
   const float x3 = v3->x - v1->x;          \
   const float y3 = v3->y - v1->y;          \
                                            \
   const float minor1 = x1 * y2 - x2 * y1;  \
   const float minor2 = x1 * y3 - x3 * y1;  \
   const float minor3 = x2 * y3 - x3 * y2;  \
                                            \
   const float y32 = y3 - y2;               \
   const float y31 = y3 - y1;               \
   const float y21 = y2 - y1;               \
                                            \
   const float x23 = x2 - x3;               \
   const float x13 = x1 - x3;               \
   const float x12 = x1 - x2;               \
                                            \
   const float det_u = minor3 - minor1 + minor2;

static void shader_grad_any_init(uintptr_t state, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3)
{
   INIT_PREAMBLE

   ALLEGRO_COLOR v1c = v1->color;
   ALLEGRO_COLOR v2c = v2->color;
   ALLEGRO_COLOR v3c = v3->color;

   PLANE_DETS(r, v1c.r, v2c.r, v3c.r)
   PLANE_DETS(g, v1c.g, v2c.g, v3c.g)
   PLANE_DETS(b, v1c.b, v2c.b, v3c.b)
   PLANE_DETS(a, v1c.a, v2c.a, v3c.a)

   state_grad_any_2d* s = (state_grad_any_2d*)state;

   s->solid.target = al_get_target_bitmap();

   s->off_x = v1->x - 0.5f;
   s->off_y = v1->y + 0.5f;

   if (det_u == 0.0) {
      s->color_dx = s->color_dy = s->color_const = al_map_rgba_f(0, 0, 0, 0);
   }
   else {
      s->color_dx.r    = -r_det_x / det_u;
      s->color_dy.r    = -r_det_y / det_u;
      s->color_const.r = r_det / det_u;

      s->color_dx.g    = -g_det_x / det_u;
      s->color_dy.g    = -g_det_y / det_u;
      s->color_const.g = g_det / det_u;

      s->color_dx.b    = -b_det_x / det_u;
      s->color_dy.b    = -b_det_y / det_u;
      s->color_const.b = b_det / det_u;

      s->color_dx.a    = -a_det_x / det_u;
      s->color_dy.a    = -a_det_y / det_u;
      s->color_const.a = a_det / det_u;
   }
}

static void shader_grad_any_first(uintptr_t state, int x1, int y, int left_minor, int left_major)
{
   state_grad_any_2d* s = (state_grad_any_2d*)state;

   const float cur_x = (float)x1 - s->off_x;
   const float cur_y = (float)y - s->off_y;

   s->solid.cur_color.r = cur_x * s->color_dx.r + cur_y * s->color_dy.r + s->color_const.r;
   s->solid.cur_color.g = cur_x * s->color_dx.g + cur_y * s->color_dy.g + s->color_const.g;
   s->solid.cur_color.b = cur_x * s->color_dx.b + cur_y * s->color_dy.b + s->color_const.b;
   s->solid.cur_color.a = cur_x * s->color_dx.a + cur_y * s->color_dy.a + s->color_const.a;

   s->minor_color.r = (float)left_minor * s->color_dx.r + s->color_dy.r;
   s->minor_color.g = (float)left_minor * s->color_dx.g + s->color_dy.g;
   s->minor_color.b = (float)left_minor * s->color_dx.b + s->color_dy.b;
   s->minor_color.a = (float)left_minor * s->color_dx.a + s->color_dy.a;

   s->major_color.r = (float)left_major * s->color_dx.r + s->color_dy.r;
   s->major_color.g = (float)left_major * s->color_dx.g + s->color_dy.g;
   s->major_color.b = (float)left_major * s->color_dx.b + s->color_dy.b;
   s->major_color.a = (float)left_major * s->color_dx.a + s->color_dy.a;
}

static void shader_grad_any_step(uintptr_t state, int minor)
{
   state_grad_any_2d* s = (state_grad_any_2d*)state;
   if (minor) {
      s->solid.cur_color.r += s->minor_color.r;
      s->solid.cur_color.g += s->minor_color.g;
      s->solid.cur_color.b += s->minor_color.b;
      s->solid.cur_color.a += s->minor_color.a;
   } else {
      s->solid.cur_color.r += s->major_color.r;
      s->solid.cur_color.g += s->major_color.g;
      s->solid.cur_color.b += s->major_color.b;
      s->solid.cur_color.a += s->major_color.a;
   }
}

/*========================== Textured Shaders ================================*/

#define SHADE_COLORS(A, B) \
   A.r = B.r * A.r;        \
   A.g = B.g * A.g;        \
   A.b = B.b * A.b;        \
   A.a = B.a * A.a;

typedef struct {
   ALLEGRO_BITMAP *target;
   ALLEGRO_COLOR cur_color;

   float du_dx, du_dy, u_const;
   float dv_dx, dv_dy, v_const;

   double u, v;
   double minor_du;
   double minor_dv;
   double major_du;
   double major_dv;

   float off_x;
   float off_y;

   ALLEGRO_BITMAP* texture;
   ALLEGRO_BITMAP_WRAP default_wrap;
   int w, h;
} state_texture_solid_any_2d;

static void shader_texture_solid_any_init(uintptr_t state, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3)
{
   INIT_PREAMBLE

   PLANE_DETS(u, v1->u, v2->u, v3->u)
   PLANE_DETS(v, v1->v, v2->v, v3->v)

   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;

   s->target = al_get_target_bitmap();
   s->cur_color = v1->color;

   s->off_x = v1->x - 0.5f;
   s->off_y = v1->y + 0.5f;

   s->w = al_get_bitmap_width(s->texture);
   s->h = al_get_bitmap_height(s->texture);

   if (det_u == 0.0f) {
      s->du_dx = s->du_dy = s->u_const = 0.0f;
      s->dv_dx = s->dv_dy = s->v_const = 0.0f;
   }
   else {
      s->du_dx    = -u_det_x / det_u;
      s->du_dy    = -u_det_y / det_u;
      s->u_const  = u_det / det_u;

      s->dv_dx    = -v_det_x / det_u;
      s->dv_dy    = -v_det_y / det_u;
      s->v_const  = v_det / det_u;
   }
}

static void shader_texture_solid_any_first(uintptr_t state, int x1, int y, int left_minor, int left_major)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;

   const float cur_x = (float)x1 - s->off_x;
   const float cur_y = (float)y - s->off_y;

   s->u = cur_x * s->du_dx + cur_y * s->du_dy + s->u_const;
   s->v = cur_x * s->dv_dx + cur_y * s->dv_dy + s->v_const;

   s->minor_du = (double)left_minor * s->du_dx + s->du_dy;
   s->minor_dv = (double)left_minor * s->dv_dx + s->dv_dy;

   s->major_du = (float)left_major * s->du_dx + s->du_dy;
   s->major_dv = (float)left_major * s->dv_dx + s->dv_dy;
}

static void shader_texture_solid_any_step(uintptr_t state, int minor)
{
   state_texture_solid_any_2d* s = (state_texture_solid_any_2d*)state;
   if (minor) {
      s->u += s->minor_du;
      s->v += s->minor_dv;
   } else {
      s->u += s->major_du;
      s->v += s->major_dv;
   }
}

/*----------------------------------------------------------------------------*/

typedef struct {
   state_texture_solid_any_2d solid;

   ALLEGRO_COLOR color_dx;
   ALLEGRO_COLOR color_dy;
   ALLEGRO_COLOR color_const;

   /*
   These are catched for the left edge walking
   */
   ALLEGRO_COLOR minor_color;
   ALLEGRO_COLOR major_color;
} state_texture_grad_any_2d;

static void shader_texture_grad_any_init(uintptr_t state, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3)
{
   INIT_PREAMBLE

   ALLEGRO_COLOR v1c = v1->color;
   ALLEGRO_COLOR v2c = v2->color;
   ALLEGRO_COLOR v3c = v3->color;

   PLANE_DETS(r, v1c.r, v2c.r, v3c.r)
   PLANE_DETS(g, v1c.g, v2c.g, v3c.g)
   PLANE_DETS(b, v1c.b, v2c.b, v3c.b)
   PLANE_DETS(a, v1c.a, v2c.a, v3c.a)
   PLANE_DETS(u, v1->u, v2->u, v3->u)
   PLANE_DETS(v, v1->v, v2->v, v3->v)

   state_texture_grad_any_2d* s = (state_texture_grad_any_2d*)state;

   s->solid.target = al_get_target_bitmap();
   s->solid.w = al_get_bitmap_width(s->solid.texture);
   s->solid.h = al_get_bitmap_height(s->solid.texture);

   s->solid.off_x = v1->x - 0.5f;
   s->solid.off_y = v1->y + 0.5f;

   if (det_u == 0.0) {
      s->solid.du_dx = s->solid.du_dy = s->solid.u_const = 0.0;
      s->solid.dv_dx = s->solid.dv_dy = s->solid.v_const = 0.0;
      s->color_dx = s->color_dy = s->color_const = al_map_rgba_f(0, 0, 0, 0);
   }
   else {
      s->solid.du_dx    = -u_det_x / det_u;
      s->solid.du_dy    = -u_det_y / det_u;
      s->solid.u_const  = u_det / det_u;

      s->solid.dv_dx    = -v_det_x / det_u;
      s->solid.dv_dy    = -v_det_y / det_u;
      s->solid.v_const  = v_det / det_u;

      s->color_dx.r    = -r_det_x / det_u;
      s->color_dy.r    = -r_det_y / det_u;
      s->color_const.r = r_det / det_u;

      s->color_dx.g    = -g_det_x / det_u;
      s->color_dy.g    = -g_det_y / det_u;
      s->color_const.g = g_det / det_u;

      s->color_dx.b    = -b_det_x / det_u;
      s->color_dy.b    = -b_det_y / det_u;
      s->color_const.b = b_det / det_u;

      s->color_dx.a    = -a_det_x / det_u;
      s->color_dy.a    = -a_det_y / det_u;
      s->color_const.a = a_det / det_u;
   }
}

static void shader_texture_grad_any_first(uintptr_t state, int x1, int y, int left_minor, int left_major)
{
   state_texture_grad_any_2d* s = (state_texture_grad_any_2d*)state;
   float cur_x;
   float cur_y;

   shader_texture_solid_any_first(state, x1, y, left_minor, left_major);

   cur_x = (float)x1 - s->solid.off_x;
   cur_y = (float)y - s->solid.off_y;

   s->solid.cur_color.r = cur_x * s->color_dx.r + cur_y * s->color_dy.r + s->color_const.r;
   s->solid.cur_color.g = cur_x * s->color_dx.g + cur_y * s->color_dy.g + s->color_const.g;
   s->solid.cur_color.b = cur_x * s->color_dx.b + cur_y * s->color_dy.b + s->color_const.b;
   s->solid.cur_color.a = cur_x * s->color_dx.a + cur_y * s->color_dy.a + s->color_const.a;

   s->minor_color.r = (float)left_minor * s->color_dx.r + s->color_dy.r;
   s->minor_color.g = (float)left_minor * s->color_dx.g + s->color_dy.g;
   s->minor_color.b = (float)left_minor * s->color_dx.b + s->color_dy.b;
   s->minor_color.a = (float)left_minor * s->color_dx.a + s->color_dy.a;

   s->major_color.r = (float)left_major * s->color_dx.r + s->color_dy.r;
   s->major_color.g = (float)left_major * s->color_dx.g + s->color_dy.g;
   s->major_color.b = (float)left_major * s->color_dx.b + s->color_dy.b;
   s->major_color.a = (float)left_major * s->color_dx.a + s->color_dy.a;
}

static void shader_texture_grad_any_step(uintptr_t state, int minor)
{
   state_texture_grad_any_2d* s = (state_texture_grad_any_2d*)state;
   shader_texture_solid_any_step(state, minor);
   if (minor) {
      s->solid.cur_color.r += s->minor_color.r;
      s->solid.cur_color.g += s->minor_color.g;
      s->solid.cur_color.b += s->minor_color.b;
      s->solid.cur_color.a += s->minor_color.a;
   } else {
      s->solid.cur_color.r += s->major_color.r;
      s->solid.cur_color.g += s->major_color.g;
      s->solid.cur_color.b += s->major_color.b;
      s->solid.cur_color.a += s->major_color.a;
   }
}


/* Include generated routines. */
#include "scanline_drawers.inc"


static void triangle_stepper(uintptr_t state,
   shader_init init, shader_first first, shader_step step, shader_draw draw,
   ALLEGRO_VERTEX* vtx1, ALLEGRO_VERTEX* vtx2, ALLEGRO_VERTEX* vtx3)
{
   float Coords[6] = {vtx1->x - 0.5f, vtx1->y + 0.5f, vtx2->x - 0.5f, vtx2->y + 0.5f, vtx3->x - 0.5f, vtx3->y + 0.5f};
   float *V1 = Coords, *V2 = &Coords[2], *V3 = &Coords[4], *s;

   float left_error = 0;
   float right_error = 0;

   float left_y_delta;
   float right_y_delta;

   float left_x_delta;
   float right_x_delta;

   int left_first, right_first, left_step, right_step;
   int left_x, right_x, cur_y, mid_y, end_y;
   float left_d_er, right_d_er;

   /*
   The reason these things are declared implicitly, is because we need to determine which
   of the edges is on the left, and which is on the right (because they are treated differently,
   as described above)
   We then can reuse these values in the actual calculation
   */
   float major_x_delta, major_y_delta, minor_x_delta, minor_y_delta;
   int major_on_the_left;

   // sort vertices so that V1 <= V2 <= V3
   if (V2[1] < V1[1]) {
      s = V2;
      V2 = V1;
      V1 = s;
   }
   if (V3[1] < V1[1]) {
      s = V3;
      V3 = V1;
      V1 = s;
   }
   if (V3[1] < V2[1]) {
      s = V3;
      V3 = V2;
      V2 = s;
   }

   /*
   We set our integer based coordinates to be above their floating point counterparts
   */
   cur_y = ceilf(V1[1]);
   mid_y = ceilf(V2[1]);
   end_y = ceilf(V3[1]);

   if (cur_y == end_y)
      return;

   /*
   As per definition, we take the ceiling
   */
   left_x = ceilf(V1[0]);

   /*
   Determine which edge is the left one
   V1-V2
   |  /
   V3
   When the cross product is negative, the major is on the left
   */

   major_x_delta = V3[0] - V1[0];
   major_y_delta = V3[1] - V1[1];
   minor_x_delta = V2[0] - V1[0];
   minor_y_delta = V2[1] - V1[1];

   if (major_x_delta * minor_y_delta - major_y_delta * minor_x_delta < 0)
      major_on_the_left = 1;
   else
      major_on_the_left = 0;

   init(state, vtx1, vtx2, vtx3);

   /*
   Do the first segment, if it exists
   */
   if (cur_y != mid_y) {
      /*
      As per definition, we take the floor
      */
      right_x = floorf(V1[0]);

      /*
      Depending on where V2 is located, choose the correct delta's
      */
      if (major_on_the_left) {
         left_x_delta = major_x_delta;
         right_x_delta = minor_x_delta;
         left_y_delta = major_y_delta;
         right_y_delta = minor_y_delta;
      } else {
         left_x_delta = minor_x_delta;
         right_x_delta = major_x_delta;
         left_y_delta = minor_y_delta;
         right_y_delta = major_y_delta;
      }

      /*
      Calculate the initial errors... doesn't look too pretty, but it only has to be done a couple of times
      per triangle drawing operation, so its not that bad
      */
      left_error = ((float)cur_y - V1[1]) * left_x_delta - ((float)left_x - V1[0]) * left_y_delta;
      right_error = ((float)cur_y - V1[1]) * right_x_delta - ((float)right_x - V1[0]) * right_y_delta;

      /*
      Calculate the first step of the edge steppers, it is potentially different from all other steps
      */
      left_first = ceilf((left_error) / left_y_delta);
      /*
      Introduce a tiny bias into the calculation, a problem with the floorf implementation
      because it does not have a properly defined 0 point. I think this is a hack,
      however, so if anyone has a better idea of how to fix this, by all means implement it.

      N.B. the same offset in the bottom segment as well
      */
      right_first = floorf((right_error) / right_y_delta - 0.000001f);

      /*
      Calculate the normal steps
      */
      left_step = ceilf(left_x_delta / left_y_delta);
      left_d_er = -(float)left_step * left_y_delta;

      right_step = ceilf(right_x_delta / right_y_delta);
      right_d_er = -(float)right_step * right_y_delta;

      /*
      Take the first step
      */
      if (cur_y < mid_y) {
         left_x += left_first;
         left_error -= (float)left_first * left_y_delta;

         right_x += right_first;
         right_error -= (float)right_first * right_y_delta;

         first(state, left_x, cur_y, left_step, left_step - 1);

         if (right_x >= left_x) {
            draw(state, left_x, cur_y, right_x);
         }

         cur_y++;
         left_error += left_x_delta;
         right_error += right_x_delta;
      }

      /*
      ...and then continue taking normal steps until we finish the segment
      */
      while (cur_y < mid_y) {
         left_error += left_d_er;
         left_x += left_step;

         /*
         If we dip to the right of the line, we shift one pixel to the left
         If dx > 0, this corresponds to taking the minor step
         If dx < 0, this corresponds to taking the major step
         */
         if (left_error + left_y_delta <= 0) {
            left_error += left_y_delta;
            left_x -= 1;
            step(state, 0);
         } else
            step(state, 1);

         right_error += right_d_er;
         right_x += right_step;

         if (right_error <= 0) {
            right_error += right_y_delta;
            right_x -= 1;
         }

         if (right_x >= left_x) {
            draw(state, left_x, cur_y, right_x);
         }

         cur_y++;
         left_error += left_x_delta;
         right_error += right_x_delta;
      }
   }

   /*
   Draw the second segment, if possible
   */
   if (cur_y < end_y) {
      if (major_on_the_left) {
         right_x = ceilf(V2[0]);

         left_x_delta = major_x_delta;
         right_x_delta = V3[0] - V2[0];
         left_y_delta = major_y_delta;
         right_y_delta = V3[1] - V2[1];

         left_error = ((float)cur_y - V1[1]) * left_x_delta - ((float)left_x - V1[0]) * left_y_delta;
         right_error = ((float)cur_y - V2[1]) * right_x_delta - ((float)right_x - V2[0]) * right_y_delta;
      } else {
         right_x = floorf(V2[0]);

         left_x_delta = V3[0] - V2[0];
         right_x_delta = major_x_delta;
         left_y_delta = V3[1] - V2[1];
         right_y_delta = major_y_delta;

         left_error = ((float)cur_y - V2[1]) * left_x_delta - ((float)left_x - V2[0]) * left_y_delta;
         right_error = ((float)cur_y - V1[1]) * right_x_delta - ((float)right_x - V1[0]) * right_y_delta;
      }

      left_first = ceilf((left_error) / left_y_delta);
      right_first = floorf((right_error) / right_y_delta - 0.000001f);

      left_step = ceilf(left_x_delta / left_y_delta);
      left_d_er = -(float)left_step * left_y_delta;

      right_step = ceilf(right_x_delta / right_y_delta);
      right_d_er = -(float)right_step * right_y_delta;

      if (cur_y < end_y) {
         left_x += left_first;
         left_error -= (float)left_first * left_y_delta;

         right_x += right_first;
         right_error -= (float)right_first * right_y_delta;

         first(state, left_x, cur_y, left_step, left_step - 1);

         if (right_x >= left_x) {
            draw(state, left_x, cur_y, right_x);
         }

         cur_y++;
         left_error += left_x_delta;
         right_error += right_x_delta;
      }

      while (cur_y < end_y) {
         left_error += left_d_er;
         left_x += left_step;

         if (left_error + left_y_delta <= 0) {
            left_error += left_y_delta;
            left_x -= 1;
            step(state, 0);
         } else
            step(state, 1);

         right_error += right_d_er;
         right_x += right_step;

         if (right_error <= 0) {
            right_error += right_y_delta;
            right_x -= 1;
         }

         if (right_x >= left_x) {
            draw(state, left_x, cur_y, right_x);
         }

         cur_y++;
         left_error += left_x_delta;
         right_error += right_x_delta;
      }
   }
}

/*
This one will check to see what exactly we need to draw...
I.e. this will call all of the actual renderers and set the appropriate callbacks
*/
void _al_triangle_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3)
{
   int shade = 1;
   int grad = 1;
   int op, src_mode, dst_mode, op_alpha, src_alpha, dst_alpha;
   ALLEGRO_COLOR v1c, v2c, v3c;

   v1c = v1->color;
   v2c = v2->color;
   v3c = v3->color;

   al_get_separate_bitmap_blender(&op,
      &src_mode, &dst_mode, &op_alpha, &src_alpha, &dst_alpha);
   if (_AL_DEST_IS_ZERO && _AL_SRC_NOT_MODIFIED) {
      shade = 0;
   }

   if ((v1c.r == v2c.r && v2c.r == v3c.r) &&
         (v1c.g == v2c.g && v2c.g == v3c.g) &&
         (v1c.b == v2c.b && v2c.b == v3c.b) &&
         (v1c.a == v2c.a && v2c.a == v3c.a)) {
      grad = 0;
   }

   if (texture) {
      ALLEGRO_BITMAP_WRAP wrap_u, wrap_v;
      _al_get_bitmap_wrap(texture, &wrap_u, &wrap_v);

      /*
      XXX: Why are we using repeat for regular bitmaps by default? For some
      reason the scanline drawers were always designed to repeat even for
      regular bitmaps. This does not match what we do with OpenGL/Direct3D.
      */
      bool repeat = (wrap_u == ALLEGRO_BITMAP_WRAP_DEFAULT || wrap_u == ALLEGRO_BITMAP_WRAP_REPEAT) &&
         (wrap_v == ALLEGRO_BITMAP_WRAP_DEFAULT || wrap_v == ALLEGRO_BITMAP_WRAP_REPEAT);
      if (grad) {
         state_texture_grad_any_2d state;
         state.solid.texture = texture;

         if (shade) {
            _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_grad_any_init, shader_texture_grad_any_first, shader_texture_grad_any_step, shader_texture_grad_any_draw_shade);
         } else {
            _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_grad_any_init, shader_texture_grad_any_first, shader_texture_grad_any_step, shader_texture_grad_any_draw_opaque);
         }
      } else {
         int white = 0;
         state_texture_solid_any_2d state;

         if (v1c.r == 1 && v1c.g == 1 && v1c.b == 1 && v1c.a == 1) {
            white = 1;
         }
         state.texture = texture;
         if (shade) {
            if (white) {
               if (repeat) {
                  _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_solid_any_init, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_shade_white_repeat);
               } else {
                  _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_solid_any_init, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_shade_white);
               }
            } else {
               if (repeat) {
                  _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_solid_any_init, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_shade_repeat);
               } else {
                  _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_solid_any_init, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_shade);
               }
            }
         } else {
            if (white) {
               _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_solid_any_init, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_opaque_white);
            } else {
               _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_texture_solid_any_init, shader_texture_solid_any_first, shader_texture_solid_any_step, shader_texture_solid_any_draw_opaque);
            }
         }
      }
   } else {
      if (grad) {
         state_grad_any_2d state;
         if (shade) {
            _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_grad_any_init, shader_grad_any_first, shader_grad_any_step, shader_grad_any_draw_shade);
         } else {
            _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_grad_any_init, shader_grad_any_first, shader_grad_any_step, shader_grad_any_draw_opaque);
         }
      } else {
         state_solid_any_2d state;
         if (shade) {
            _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_solid_any_init, shader_solid_any_first, shader_solid_any_step, shader_solid_any_draw_shade);
         } else {
            _al_draw_soft_triangle(v1, v2, v3, (uintptr_t)&state, shader_solid_any_init, shader_solid_any_first, shader_solid_any_step, shader_solid_any_draw_opaque);
         }
      }
   }
}

static int bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int w, int h)
{
   ASSERT(bmp);

   if (!al_is_bitmap_locked(bmp))
      return 0;
   if (x1 + w > bmp->lock_x && y1 + h > bmp->lock_y && x1 < bmp->lock_x + bmp->lock_w && y1 < bmp->lock_y + bmp->lock_h)
      return 1;
   return 0;
}

void _al_draw_soft_triangle(
   ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3, uintptr_t state,
   void (*init)(uintptr_t, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
   void (*first)(uintptr_t, int, int, int, int),
   void (*step)(uintptr_t, int),
   void (*draw)(uintptr_t, int, int, int))
{
   /*
   ALLEGRO_VERTEX copy_v1, copy_v2; <- may be needed for clipping later on
   */
   ALLEGRO_VERTEX* vtx1 = v1;
   ALLEGRO_VERTEX* vtx2 = v2;
   ALLEGRO_VERTEX* vtx3 = v3;
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

   min_x = (int)floorf(MIN(vtx1->x, MIN(vtx2->x, vtx3->x))) - 1;
   min_y = (int)floorf(MIN(vtx1->y, MIN(vtx2->y, vtx3->y))) - 1;
   max_x = (int)ceilf(MAX(vtx1->x, MAX(vtx2->x, vtx3->x))) + 1;
   max_y = (int)ceilf(MAX(vtx1->y, MAX(vtx2->y, vtx3->y))) + 1;

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
      if (!bitmap_region_is_locked(target, min_x, min_y, max_x - min_x, max_y - min_y) ||
          _al_pixel_format_is_video_only(target->locked_region.format))
         return;
   } else {
      if (!(lr = al_lock_bitmap_region(target, min_x, min_y, max_x - min_x, max_y - min_y, ALLEGRO_PIXEL_FORMAT_ANY, 0)))
         return;
      need_unlock = 1;
   }

   triangle_stepper(state, init, first, step, draw, v1, v2, v3);

   if (need_unlock)
      al_unlock_bitmap(target);
}

/* vim: set sts=3 sw=3 et: */
