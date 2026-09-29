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
 *      Some high level routines, provided for user's convinience.
 *
 *
 *      By Pavel Sountsov.
 *
 *
 *      Bezier spline plotter By Seymour Shlien.
 *
 *      Optimised version by Sven Sandberg.
 *
 *      I'm not sure wether or not we still use the Castelau Algorithm
 *      described in the book :o)
 *
 *      Interactive Computer Graphics
 *      by Peter Burger and Duncan Gillies
 *      Addison-Wesley Publishing Co 1989
 *      ISBN 0-201-17439-1
 *
 *      The 4 th order Bezier curve is a cubic curve passing
 *      through the first and fourth point. The curve does
 *      not pass through the middle two points. They are merely
 *      guide points which control the shape of the curve. The
 *      curve is tangent to the lines joining points 1 and 2
 *      and points 3 and 4.
 *
 *      See readme.txt for copyright information.
 */


#define ALLEGRO_INTERNAL_UNSTABLE
#include "allegro5/allegro_primitives.h"
#ifdef ALLEGRO_CFG_OPENGL
#include "allegro5/allegro_opengl.h"
#endif
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_prim_addon.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/debug.h"
#include <math.h>

ALLEGRO_DEBUG_CHANNEL("primitives")

#ifdef ALLEGRO_MSVC
   #define hypotf(x, y) _hypotf((x), (y))
#endif

#define INIT_PRIM_PTRS_DYNAMIC(num_vtx_static, num_vtx, num_idx, new_type, old_type, force_idxs) \
      ALLEGRO_DISPLAY *disp = al_get_current_display(); \
      ALLEGRO_VERTEX *vtx = NULL; \
      _AL_BATCH_INDEX_TYPE *idx = NULL; \
      int first_idx = 0; \
      bool use_batching; \
      ALLEGRO_VERTEX stack_vtx[num_vtx_static]; \
      do { \
         if (disp && !disp->use_legacy_drawing_api && !_al_prim_disable_batching()) { \
            if (disp->batch_enabled || force_idxs) { \
               first_idx = disp->vt->prepare_batch(disp, NULL, \
                  new_type, num_vtx, num_idx, (void**)&vtx, (void**)&idx); \
            } \
            else { \
               first_idx = disp->vt->prepare_batch(disp, NULL, \
                  old_type, num_vtx, 0, (void**)&vtx, (void**)&idx); \
            } \
            if (first_idx < 0) \
               return; \
            use_batching = true; \
         } \
         else { \
            vtx = stack_vtx; \
            use_batching = false; \
         } \
      } while (0)

#define INIT_PRIM_PTRS(num_vtx, num_idx, new_type, old_type) INIT_PRIM_PTRS_DYNAMIC(num_vtx, num_vtx, num_idx, new_type, old_type, false)

/*
 * Make an estimate of the scale of the current transformation.
 * We do this by computing the determinants of the 2D section of the transformation matrix.
 */
static float get_scale(void)
{
#define DET2D(T) (fabs((T)->m[0][0] * (T)->m[1][1] - (T)->m[0][1] * (T)->m[1][0]))

   const ALLEGRO_TRANSFORM* t = al_get_current_transform();
   float scale_sq = DET2D(t);
   ALLEGRO_BITMAP* b = al_get_target_bitmap();
   if (b) {
      const ALLEGRO_TRANSFORM* p = al_get_current_projection_transform();
      /* Divide by 4.0f as the screen coordinates range from -1 to 1 on both axes. */
      scale_sq *= DET2D(p) * al_get_bitmap_width(b) * al_get_bitmap_height(b) / 4.0f;
   }

   return sqrtf(scale_sq);

#undef DET2D
}


/*
 * Type used for intermediate coordinate storage.
 */
typedef struct {
   float x, y;
} VTX;


/* Function: al_draw_line
 */
void al_draw_line(float x1, float y1, float x2, float y2,
   ALLEGRO_COLOR color, float thickness)
{
   float len = hypotf(x2 - x1, y2 - y1);
   if (thickness > 0) {
      int ii;
      float tx, ty;

      if (len == 0)
         return;

      INIT_PRIM_PTRS(4, 6, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN);

      tx = 0.5f * thickness * (y2 - y1) / len;
      ty = 0.5f * thickness * -(x2 - x1) / len;

      vtx[3].x = x1 + tx; vtx[3].y = y1 + ty; vtx[3].u = 0.0f; vtx[3].v = -thickness / 2.;
      vtx[2].x = x1 - tx; vtx[2].y = y1 - ty; vtx[2].u = 0.0f; vtx[2].v = thickness / 2.;
      vtx[1].x = x2 - tx; vtx[1].y = y2 - ty; vtx[1].u = len;  vtx[1].v = thickness / 2.;
      vtx[0].x = x2 + tx; vtx[0].y = y2 + ty; vtx[0].u = len;  vtx[0].v = -thickness / 2.;

      for (ii = 0; ii < 4; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            idx[0] = first_idx + 0;
            idx[1] = first_idx + 1;
            idx[2] = first_idx + 2;
            idx[3] = first_idx + 0;
            idx[4] = first_idx + 2;
            idx[5] = first_idx + 3;
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
      }
   } else {
      INIT_PRIM_PTRS(2, 2, ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_LIST);

      vtx[0].x = x1; vtx[0].y = y1; vtx[0].u = 0.0f; vtx[0].v = 0.0f;
      vtx[1].x = x2; vtx[1].y = y2; vtx[1].u = len;  vtx[1].v = 0.0f;

      vtx[0].color = color;
      vtx[1].color = color;
      vtx[0].z = 0;
      vtx[1].z = 0;

      if (use_batching) {
         if (disp->batch_enabled) {
            idx[0] = first_idx + 0;
            idx[1] = first_idx + 1;
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 2, ALLEGRO_PRIM_LINE_LIST);
      }
   }
}

/* Function: al_draw_triangle
 */
void al_draw_triangle(float x1, float y1, float x2, float y2,
   float x3, float y3, ALLEGRO_COLOR color, float thickness)
{
   int ii;
   float min_x = fmin(x1, fmin(x2, x3));
   float min_y = fmin(y1, fmin(y2, y3));

   if (thickness > 0) {
      int jj;
      float side1, side2, side3;
      float perimeter, semi_perimeter;
      float outer_frac, inner_frac;
      float incenter_x, incenter_y;
      float incircle_rad;
      VTX acc_coords[14];
      int acc_idxs[8 * 3];
      int num_vertices = 0;
      int num_indices = 0;

      float x[3] = {x1, x2, x3};
      float y[3] = {y1, y2, y3};
      float cross = (x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]);

      /*
       * If the triangle is very flat, draw it as a line
       */
      if(fabsf(cross) < 0.0001f) {
         /*
          * Find the obtuse vertex via two dot products
          */
         float dot = (x[1] - x[0]) * (x[2] - x[0]) + (y[1] - y[0]) * (y[2] - y[0]);
         if(dot < 0) {
            x1 = x[1]; y1 = y[1];
            x2 = x[2]; y2 = y[2];
         } else {
            dot = (x[0] - x[1]) * (x[2] - x[1]) + (y[0] - y[1]) * (y[2] - y[1]);
            if(dot < 0) {
               x1 = x[0]; y1 = y[0];
               x2 = x[2]; y2 = y[2];
            } else {
               x1 = x[0]; y1 = y[0];
               x2 = x[1]; y2 = y[1];
            }
         }

         al_draw_line(x1, y1, x2, y2, color, thickness);
         return;
      }
      else if(cross > 0) {
         /*
          * Points need to be wound correctly for the algorithm to work
          */
         float t;
         t = x[1];
         x[1] = x[2];
         x[2] = t;

         t = y[1];
         y[1] = y[2];
         y[2] = t;
      }

      side1 = hypotf(x[1] - x[0], y[1] - y[0]);
      side2 = hypotf(x[2] - x[0], y[2] - y[0]);
      side3 = hypotf(x[2] - x[1], y[2] - y[1]);

      perimeter = side1 + side2 + side3;
      semi_perimeter = perimeter / 2.0f;
      if (semi_perimeter < 0.00001f)
         return;

      incircle_rad = sqrtf((semi_perimeter - side1) * (semi_perimeter - side2) * (semi_perimeter - side3) / semi_perimeter);

      if (incircle_rad < 0.00001f)
         return;

      outer_frac = (incircle_rad + thickness / 2) / incircle_rad;
      inner_frac = (incircle_rad - thickness / 2) / incircle_rad;

      if(inner_frac < 0)
         inner_frac = 0;

      incenter_x = (side1 * x[2] + side2 * x[1] + side3 * x[0]) / perimeter;
      incenter_y = (side1 * y[2] + side2 * y[1] + side3 * y[0]) / perimeter;

      /*
       * Draw the triangle as 3 wedges. Each wedge can have either 4 or 5
       * vertices. The 5th vertex is added when the triangle is very flat, and
       * implements a bevel line join. Each iteration establishes the ending 2
       * vertices for the current wedge, and the starting 2 or 3 vertices for
       * the next wedge. The first iteration/wedge does not have the previous
       * vertices computed yet, so it is drawn after the last wedge is drawn to
       * complete the loop.
       */
      int prev_count = 0;
      for(ii = 0; ii < 3; ii++)
      {
         float vert_x = x[ii] - incenter_x;
         float vert_y = y[ii] - incenter_y;

         float o_dx = vert_x * outer_frac;
         float o_dy = vert_y * outer_frac;

         float i_dx = vert_x * inner_frac;
         float i_dy = vert_y * inner_frac;

         float tdx = o_dx - i_dx;
         float tdy = o_dy - i_dy;

         VTX inner_vtx;
         VTX outer_vtx;

         int start_idx = num_vertices - prev_count;
         int num_vertices_in_side = prev_count + 2;

         if(tdx * tdx + tdy * tdy > 16 * thickness * thickness) {
            float x_pos = x[(ii + 1) % 3];
            float y_pos = y[(ii + 1) % 3];

            float x_neg = x[(ii + 2) % 3];
            float y_neg = y[(ii + 2) % 3];

            float x1_x2 = x[ii] - x_pos;
            float y1_y2 = y[ii] - y_pos;

            float x1_x3 = x[ii] - x_neg;
            float y1_y3 = y[ii] - y_neg;

            float mag_1_2 = hypotf(x1_x2, y1_y2);
            float mag_1_3 = hypotf(x1_x3, y1_y3);

            VTX next_vtx;

            x1_x2 *= thickness / 2 / mag_1_2;
            y1_y2 *= thickness / 2 / mag_1_2;

            x1_x3 *= thickness / 2 / mag_1_3;
            y1_y3 *= thickness / 2 / mag_1_3;

            outer_vtx.x = x[ii] + x1_x3 - y1_y3; outer_vtx.y = y[ii] + y1_y3 + x1_x3;
            inner_vtx.x = incenter_x + i_dx; inner_vtx.y = incenter_y + i_dy;
            next_vtx.x = x[ii] + x1_x2 + y1_y2; next_vtx.y = y[ii] + y1_y2 - x1_x2;

            acc_coords[num_vertices++] = inner_vtx;
            acc_coords[num_vertices++] = outer_vtx;
            acc_coords[num_vertices++] = next_vtx;
            prev_count = 3;
         } else {
            inner_vtx.x = incenter_x + i_dx; inner_vtx.y = incenter_y + i_dy;
            outer_vtx.x = incenter_x + o_dx; outer_vtx.y = incenter_y + o_dy;

            acc_coords[num_vertices++] = inner_vtx;
            acc_coords[num_vertices++] = outer_vtx;
            prev_count = 2;
         }

         /* FIXME: We need to flip the indices to make sure the winding is correct. */
         if (ii != 0) {
            int idxs_5[] = {0, 1, 2, 4, 3};
            int idxs_4[] = {0, 1, 3, 2};
            int *side_idxs = num_vertices_in_side == 4 ? idxs_4 : idxs_5;

            for (jj = 0; jj < num_vertices_in_side - 2; jj++) {
               acc_idxs[num_indices++] = side_idxs[0] + start_idx;
               acc_idxs[num_indices++] = side_idxs[jj + 2] + start_idx;
               acc_idxs[num_indices++] = side_idxs[jj + 1] + start_idx;
            }
         }
         if (ii == 2) {
            /* This wedge starts at the end of the last wedge and connects back to beggining. */
            int idxs_5[] = {num_vertices - 3, num_vertices - 2, num_vertices - 1, 1, 0};
            int idxs_4[] = {num_vertices - 2, num_vertices - 1, 1, 0};
            int *side_idxs = prev_count == 2 ? idxs_4 : idxs_5;
            /* Number of vertices is prev_count + 2, and we sub 2 for the triangle fan. */
            for (jj = 0; jj < prev_count; jj++) {
               acc_idxs[num_indices++] = side_idxs[0];
               acc_idxs[num_indices++] = side_idxs[jj + 2];
               acc_idxs[num_indices++] = side_idxs[jj + 1];
            }
         }
      }

      INIT_PRIM_PTRS_DYNAMIC(8, num_vertices, num_indices, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_LIST, true);
      _AL_BATCH_INDEX_TYPE stack_idx[8 * 3];
      if (!use_batching) {
         idx = stack_idx;
      }

      for (ii = 0; ii < num_vertices; ii++) {
         vtx[ii].x = acc_coords[ii].x;
         vtx[ii].y = acc_coords[ii].y;
         vtx[ii].z = 0.;
         vtx[ii].u = vtx[ii].x - min_x;
         vtx[ii].v = vtx[ii].y - min_y;
         vtx[ii].color = color;
      }

      for (ii = 0; ii < num_indices; ii++) {
         idx[ii] = acc_idxs[ii] + first_idx;
      }

      if (use_batching) {
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_indexed_prim(vtx, NULL, NULL, acc_idxs, num_indices, ALLEGRO_PRIM_TRIANGLE_LIST);
      }
   } else {
      INIT_PRIM_PTRS(3, 6, ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_LOOP);

      vtx[0].x = x1; vtx[0].y = y1; vtx[0].u = x1 - min_x; vtx[0].v = y1 - min_y;
      vtx[1].x = x2; vtx[1].y = y2; vtx[1].u = x2 - min_x; vtx[1].v = y2 - min_y;
      vtx[2].x = x3; vtx[2].y = y3; vtx[2].u = x3 - min_x; vtx[2].v = y3 - min_y;

      for (ii = 0; ii < 3; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 3; ii++) {
               idx[2 * ii + 0] = first_idx + ii;
               idx[2 * ii + 1] = first_idx + (ii + 1) % 3;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 3, ALLEGRO_PRIM_LINE_LOOP);
      }
   }
}

/* Function: al_draw_filled_triangle
 */
void al_draw_filled_triangle(float x1, float y1, float x2, float y2,
   float x3, float y3, ALLEGRO_COLOR color)
{
   int ii;
   float min_x = fmin(x1, fmin(x2, x3));
   float min_y = fmin(y1, fmin(y2, y3));
   INIT_PRIM_PTRS(3, 3, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_LIST);

   vtx[0].x = x1; vtx[0].y = y1; vtx[0].u = x1 - min_x; vtx[0].v = y1 - min_y;
   vtx[1].x = x2; vtx[1].y = y2; vtx[1].u = x2 - min_x; vtx[1].v = y2 - min_y;
   vtx[2].x = x3; vtx[2].y = y3; vtx[2].u = x3 - min_x; vtx[2].v = y3 - min_y;

   for (ii = 0; ii < 3; ii++) {
      vtx[ii].color = color;
      vtx[ii].z = 0;
   }

   if (use_batching) {
      if (disp->batch_enabled) {
         idx[0] = first_idx + 0;
         idx[1] = first_idx + 1;
         idx[2] = first_idx + 2;
      }
      if (!disp->batch_enabled) {
         disp->vt->draw_batch(disp);
      }
   }
   else {
      al_draw_prim(vtx, 0, 0, 0, 3, ALLEGRO_PRIM_TRIANGLE_LIST);
   }
}

/* Function: al_draw_rectangle
 */
void al_draw_rectangle(float x1, float y1, float x2, float y2,
   ALLEGRO_COLOR color, float thickness)
{
   int ii;

   if (thickness > 0) {
      float t = thickness / 2;
      INIT_PRIM_PTRS(10, 24, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP);

      vtx[0].x = x1 + t; vtx[0].y = y1 + t; vtx[0].u = t;           vtx[0].v = t;
      vtx[1].x = x1 - t; vtx[1].y = y1 - t; vtx[1].u = -t;          vtx[1].v = -t;
      vtx[3].x = x2 + t; vtx[3].y = y1 - t; vtx[3].u = x2 - x1 + t; vtx[3].v = -t;
      vtx[2].x = x2 - t; vtx[2].y = y1 + t; vtx[2].u = x2 - x1 - t; vtx[2].v = t;
      vtx[5].x = x2 + t; vtx[5].y = y2 + t; vtx[5].u = x2 - x1 + t; vtx[5].v = y2 - y1 + t;
      vtx[4].x = x2 - t; vtx[4].y = y2 - t; vtx[4].u = x2 - x1 - t; vtx[4].v = y2 - y1 - t;
      vtx[7].x = x1 - t; vtx[7].y = y2 + t; vtx[7].u = -t;          vtx[7].v = y2 - y1 + t;
      vtx[6].x = x1 + t; vtx[6].y = y2 - t; vtx[6].u = t;           vtx[6].v = y2 - y1 - t;
      vtx[9].x = x1 - t; vtx[9].y = y1 - t; vtx[9].u = -t;          vtx[9].v = -t;
      vtx[8].x = x1 + t; vtx[8].y = y1 + t; vtx[8].u = t;           vtx[8].v = t;

      for (ii = 0; ii < 10; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 8; ii++) {
               idx[3 * ii + 0] = first_idx + ii + ii % 2;
               idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
               idx[3 * ii + 2] = first_idx + ii + 2;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 10, ALLEGRO_PRIM_TRIANGLE_STRIP);
      }
   } else {
      INIT_PRIM_PTRS(4, 8, ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_LOOP);

      vtx[0].x = x1; vtx[0].y = y1;
      vtx[1].x = x2; vtx[1].y = y1;
      vtx[2].x = x2; vtx[2].y = y2;
      vtx[3].x = x1; vtx[3].y = y2;

      for (ii = 0; ii < 4; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
         vtx[ii].u = vtx[ii].x - x1;
         vtx[ii].v = vtx[ii].y - y1;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 4; ii++) {
               idx[2 * ii + 0] = first_idx + ii + 0;
               idx[2 * ii + 1] = first_idx + (ii + 1) % 4;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 4, ALLEGRO_PRIM_LINE_LOOP);
      }
   }
}

/* Function: al_draw_filled_rectangle
 */
void al_draw_filled_rectangle(float x1, float y1, float x2, float y2,
   ALLEGRO_COLOR color)
{
   int ii;
   INIT_PRIM_PTRS(4, 6, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN);

   vtx[0].x = x1; vtx[0].y = y1; vtx[0].u = 0.0f;    vtx[0].v = 0.0f;
   vtx[1].x = x2; vtx[1].y = y1; vtx[1].u = x2 - x1; vtx[1].v = 0.0f;
   vtx[2].x = x2; vtx[2].y = y2; vtx[2].u = x2 - x1; vtx[2].v = y2 - y1;
   vtx[3].x = x1; vtx[3].y = y2; vtx[3].u = 0.0f;    vtx[3].v = y2 - y1;

   for (ii = 0; ii < 4; ii++) {
      vtx[ii].color = color;
      vtx[ii].z = 0;
   }

   if (use_batching) {
      if (disp->batch_enabled) {
         idx[0] = first_idx + 0;
         idx[1] = first_idx + 1;
         idx[2] = first_idx + 2;
         idx[3] = first_idx + 0;
         idx[4] = first_idx + 2;
         idx[5] = first_idx + 3;
      }
      if (!disp->batch_enabled) {
         disp->vt->draw_batch(disp);
      }
   }
   else {
      al_draw_prim(vtx, 0, 0, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
   }
}

static void calculate_arc(float* dest, float* tex_dest, int stride, float cx, float cy,
   float rx, float ry, float start_theta, float delta_theta, float thickness,
   int num_points)
{
   float theta;
   float u;
   float du;
   float c;
   float s;
   float x, y, ox, oy;
   int ii;

   ASSERT(dest);
   ASSERT(num_points > 1);
   ASSERT(rx >= 0);
   ASSERT(ry >= 0);

   theta = delta_theta / ((float)num_points - 1);
   c = cosf(theta);
   s = sinf(theta);
   x = cosf(start_theta);
   y = sinf(start_theta);
   u = rx * start_theta;
   du = rx * theta;

   if (thickness > 0.0f) {
      if (rx == ry) {
         /*
         The circle case is particularly simple
         */
         float r1 = rx - thickness / 2.0f;
         float r2 = rx + thickness / 2.0f;
         for (ii = 0; ii < num_points; ii ++) {
            *dest =       r2 * x + cx;
            *(dest + 1) = r2 * y + cy;
            dest = (float*)(((char*)dest) + stride);

            *dest =        r1 * x + cx;
            *(dest + 1) =  r1 * y + cy;
            dest = (float*)(((char*)dest) + stride);

            if (tex_dest) {
               *tex_dest = u;
               *(tex_dest + 1) = thickness / 2.0f;
               tex_dest = (float*)(((char*)tex_dest) + stride);

               *tex_dest = u;
               *(tex_dest + 1) = -thickness / 2.0f;
               tex_dest = (float*)(((char*)tex_dest) + stride);
            }

            ox = x;
            oy = y;
            x = c * ox - s * oy;
            y = s * ox + c * oy;
            u += du;
         }
      }
      else {
         if (rx != 0 && ry != 0) {
            for (ii = 0; ii < num_points; ii++) {
               float denom = hypotf(ry * x, rx * y);
               float nx = thickness / 2 * ry * x / denom;
               float ny = thickness / 2 * rx * y / denom;

               *dest =       rx * x + cx + nx;
               *(dest + 1) = ry * y + cy + ny;
               dest = (float*)(((char*)dest) + stride);

               if (tex_dest) {
                  *tex_dest = u;
                  *(tex_dest + 1) = thickness / 2.0f;
                  tex_dest = (float*)(((char*)tex_dest) + stride);

                  *tex_dest = u;
                  *(tex_dest + 1) = -thickness / 2.0f;
                  tex_dest = (float*)(((char*)tex_dest) + stride);
               }

               *dest =       rx * x + cx - nx;
               *(dest + 1) = ry * y + cy - ny;
               dest = (float*)(((char*)dest) + stride);

               ox = x;
               oy = y;
               x = c * ox - s * oy;
               y = s * ox + c * oy;
               u += hypotf(rx * (x - ox), ry * (y - oy));
            }
         }
      }
   }
   else {
      for (ii = 0; ii < num_points; ii++) {
         *dest =       rx * x + cx;
         *(dest + 1) = ry * y + cy;
         dest = (float*)(((char*)dest) + stride);

         if (tex_dest) {
            *tex_dest = u;
            *(tex_dest + 1) = 0.;
            tex_dest = (float*)(((char*)tex_dest) + stride);
         }

         ox = x;
         oy = y;
         x = c * ox - s * oy;
         y = s * ox + c * oy;
         if (rx == ry) {
            u += du;
         }
         else {
            u += hypotf(rx * (x - ox), ry * (y - oy));
         }
      }
   }
}

/* Function: al_calculate_arc
 */
void al_calculate_arc(float* dest, int stride, float cx, float cy,
   float rx, float ry, float start_theta, float delta_theta, float thickness,
   int num_points)
{
   calculate_arc(dest, NULL, stride, cx, cy, rx, ry, start_theta, delta_theta,
         thickness, num_points);
}

/* Function: al_draw_pieslice
 */
void al_draw_pieslice(float cx, float cy, float r, float start_theta,
   float delta_theta, ALLEGRO_COLOR color, float thickness)
{
   float scale = get_scale();
   int num_segments, ii;

   ASSERT(r >= 0);

   /* Just makes things a bit easier */
   if(delta_theta < 0) {
      delta_theta = -delta_theta;
      start_theta -= delta_theta;
   }

   if (thickness <= 0) {
      num_segments = fabs(delta_theta / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * r));

      if (num_segments < 2)
         num_segments = 2;

      if (num_segments + 1 >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1 - 1;
      }

      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments + 1, 2 * (num_segments + 1),
            ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_LOOP, false);

      al_calculate_arc(&(vtx[1].x), sizeof(ALLEGRO_VERTEX), cx, cy, r, r, start_theta, delta_theta, 0, num_segments);
      vtx[0].x = cx; vtx[0].y = cy;

      for (ii = 0; ii < num_segments + 1; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
         vtx[ii].u = vtx[ii].x - cx;
         vtx[ii].v = vtx[ii].y - cy;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < num_segments + 1; ii++) {
               idx[2 * ii + 0] = first_idx + ii + 0;
               idx[2 * ii + 1] = first_idx + (ii + 1) % (num_segments + 1);
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, num_segments + 1, ALLEGRO_PRIM_LINE_LOOP);
      }
   } else {
      float ht = thickness / 2;
      float inner_side_angle = asinf(ht / (r - ht));
      float outer_side_angle = asinf(ht / (r + ht));
      float central_angle = delta_theta - 2 * inner_side_angle;
      /* inverted_winding => pacman shape */
      bool inverted_winding = ((int)(delta_theta / ALLEGRO_PI)) % 2 == 1;
      float midangle = start_theta + (fmodf(delta_theta + ALLEGRO_PI, 2 * ALLEGRO_PI) - ALLEGRO_PI) / 2;
      float midpoint_dir_x = cosf(midangle);
      float midpoint_dir_y = sinf(midangle);
      float side_dir_x = cosf(start_theta);
      float side_dir_y = sinf(start_theta);
      float sine_half_delta = fabsf(side_dir_x * midpoint_dir_y - side_dir_y * midpoint_dir_x);  /* Cross product */
      float connect_len = ht / sine_half_delta;
      bool blunt_tip = connect_len > 2 * thickness;

      /* The angle is big enough for there to be a hole in the middle.
       * Note that central_angle can be NaN here as well due to inner_side_angle
       * being NaN when ht is too high. */
      if (central_angle > 0) {
         VTX coords[ALLEGRO_VERTEX_CACHE_SIZE];
         float central_start_angle = start_theta + inner_side_angle;
         size_t vtx_id;
         int vtx_delta;
         /* Two inner hole vertices and the apex (2 vertices if the apex is blunt) */
         int extra_vtx = blunt_tip ? 4 : 3;

         num_segments = fabs(delta_theta / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * r));

         if (num_segments < 2)
            num_segments = 2;

         if (2 * num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
            num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 1) / 2;
         }

         /* FIXME: This uses 3 calls to do the drawing. It seems difficult to
          * combine them into 1 due to the variable number of vertices we use.
          */
         {
            INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, 2 * num_segments, (2 * num_segments - 2) * 3,
               ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP, false);

            al_calculate_arc(&vtx[0].x, sizeof(ALLEGRO_VERTEX),
                  cx, cy, r, r, central_start_angle + central_angle, -central_angle, thickness, num_segments);

            for (ii = 0; ii < 2 * num_segments; ii++) {
               vtx[ii].color = color;
               vtx[ii].z = 0;
               vtx[ii].u = vtx[ii].x - cx;
               vtx[ii].v = vtx[ii].y - cy;
            }

            if (use_batching) {
               if (disp->batch_enabled) {
                  for (ii = 0; ii < 2 * num_segments - 2; ii++) {
                     idx[3 * ii + 0] = first_idx + ii + ii % 2;
                     idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
                     idx[3 * ii + 2] = first_idx + ii + 2;
                  }
               }
               if (!disp->batch_enabled) {
                  disp->vt->draw_batch(disp);
               }
            }
            else {
               al_draw_prim(vtx, 0, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
            }
         }

         num_segments = (inner_side_angle + outer_side_angle) / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * (r + ht));

         if (num_segments < 2)
            num_segments = 2;

         if (num_segments + extra_vtx >= ALLEGRO_VERTEX_CACHE_SIZE)
            num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1 - extra_vtx;

         al_calculate_arc(&(coords[0].x), sizeof(VTX), cx, cy, r + ht, r + ht,
               central_start_angle - (outer_side_angle + inner_side_angle), outer_side_angle + inner_side_angle, 0, num_segments);

         coords[num_segments].x = cx + (r - thickness / 2) * cosf(central_start_angle);
         coords[num_segments].y = cy + (r - thickness / 2) * sinf(central_start_angle);

         /* Do the tip */
         vtx_id = num_segments + 1 + (inverted_winding ? (1 + (blunt_tip ? 1 : 0)) : 0);
         vtx_delta = inverted_winding ? -1 : 1;
         float clipped_connect_len = fmin(connect_len, r - ht);
         coords[vtx_id].x = cx + clipped_connect_len * midpoint_dir_x;
         coords[vtx_id].y = cy + clipped_connect_len * midpoint_dir_y;
         vtx_id += vtx_delta;

         if (blunt_tip) {
            float vx = ht * (side_dir_y * (inverted_winding ? -1 : 1) - side_dir_x);
            float vy = ht * (-side_dir_x * (inverted_winding ? -1 : 1) - side_dir_y);
            float dot = vx * midpoint_dir_x + vy * midpoint_dir_y;

            coords[vtx_id].x = cx + dot * midpoint_dir_x;
            coords[vtx_id].y = cy + dot * midpoint_dir_y;
            vtx_id += vtx_delta;

            coords[vtx_id].x = cx + vx;
            coords[vtx_id].y = cy + vy;
         } else {
            coords[vtx_id].x = cx - connect_len * midpoint_dir_x;
            coords[vtx_id].y = cy - connect_len * midpoint_dir_y;
         }

         {
            INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments + extra_vtx, (num_segments + extra_vtx - 2) * 3,
               ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN, false);

            for (ii = 0; ii < num_segments + extra_vtx; ii++) {
               vtx[ii].x = coords[ii].x;
               vtx[ii].y = coords[ii].y;
               vtx[ii].color = color;
               vtx[ii].z = 0;
               vtx[ii].u = vtx[ii].x - cx;
               vtx[ii].v = vtx[ii].y - cy;
            }

            if (use_batching) {
               if (disp->batch_enabled) {
                  for (ii = 0; ii < num_segments + extra_vtx - 2; ii++) {
                     idx[3 * ii + 0] = first_idx + 0;
                     idx[3 * ii + 1] = first_idx + ii + 1;
                     idx[3 * ii + 2] = first_idx + ii + 2;
                  }
               }
               if (!disp->batch_enabled) {
                  disp->vt->draw_batch(disp);
               }
            }
            else {
               al_draw_prim(vtx, 0, 0, 0, num_segments + extra_vtx, ALLEGRO_PRIM_TRIANGLE_FAN);
            }
         }

         {
            INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments + extra_vtx, (num_segments + extra_vtx - 2) * 3,
               ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN, false);

            /* Mirror the vertices and draw them again */
            for (ii = 0; ii < num_segments + extra_vtx; ii++) {
               float dot = (coords[ii].x - cx) * midpoint_dir_x + (coords[ii].y - cy) * midpoint_dir_y;
               int jj = (num_segments + extra_vtx - ii) % (num_segments + extra_vtx);
               vtx[jj].x = 2 * cx + 2 * dot * midpoint_dir_x - coords[ii].x;
               vtx[jj].y = 2 * cy + 2 * dot * midpoint_dir_y - coords[ii].y;
               vtx[jj].color = color;
               vtx[jj].z = 0;
               vtx[jj].u = vtx[jj].x - cx;
               vtx[jj].v = vtx[jj].y - cy;
            }

            if (use_batching) {
               if (disp->batch_enabled) {
                  for (ii = 0; ii < num_segments + extra_vtx - 2; ii++) {
                     idx[3 * ii + 0] = first_idx + 0;
                     idx[3 * ii + 1] = first_idx + ii + 1;
                     idx[3 * ii + 2] = first_idx + ii + 2;
                  }
               }
               if (!disp->batch_enabled) {
                  disp->vt->draw_batch(disp);
               }
            }
            else {
               al_draw_prim(vtx, 0, 0, 0, num_segments + extra_vtx, ALLEGRO_PRIM_TRIANGLE_FAN);
            }
         }
      } else {
         /* Apex: 2 vertices if the apex is blunt) */
         int extra_vtx = blunt_tip ? 2 : 1;

         num_segments = (2 * outer_side_angle) / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * (r + ht));

         if (num_segments < 2)
            num_segments = 2;

         if (num_segments + extra_vtx >= ALLEGRO_VERTEX_CACHE_SIZE)
            num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1 - extra_vtx;

         INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments + extra_vtx, (num_segments + extra_vtx - 2) * 3,
            ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN, false);

         al_calculate_arc(&(vtx[1].x), sizeof(ALLEGRO_VERTEX), cx, cy, r + ht, r + ht,
                start_theta - outer_side_angle, 2 * outer_side_angle + delta_theta, 0, num_segments);

         if (blunt_tip) {
            float vx = ht * (side_dir_y - side_dir_x);
            float vy = ht * (-side_dir_x - side_dir_y);
            float dot = vx * midpoint_dir_x + vy * midpoint_dir_y;

            vtx[0].x = cx + vx;
            vtx[0].y = cy + vy;

            vx = 2 * dot * midpoint_dir_x - vx;
            vy = 2 * dot * midpoint_dir_y - vy;

            vtx[num_segments + 1].x = cx + vx;
            vtx[num_segments + 1].y = cy + vy;
         } else {
            vtx[0].x = cx - connect_len * midpoint_dir_x;
            vtx[0].y = cy - connect_len * midpoint_dir_y;
         }

         for (ii = 0; ii < num_segments + extra_vtx; ii++) {
            vtx[ii].color = color;
            vtx[ii].z = 0;
            vtx[ii].u = vtx[ii].x - cx;
            vtx[ii].v = vtx[ii].y - cy;
         }
         if (use_batching) {
            if (disp->batch_enabled) {
               for (ii = 0; ii < num_segments + extra_vtx - 2; ii++) {
                  idx[3 * ii + 0] = first_idx + 0;
                  idx[3 * ii + 1] = first_idx + ii + 1;
                  idx[3 * ii + 2] = first_idx + ii + 2;
               }
            }
            if (!disp->batch_enabled) {
               disp->vt->draw_batch(disp);
            }
         }
         else {
            al_draw_prim(vtx, 0, 0, 0, num_segments + extra_vtx, ALLEGRO_PRIM_TRIANGLE_FAN);
         }
      }
   }
}

/* Function: al_draw_filled_pieslice
 */
void al_draw_filled_pieslice(float cx, float cy, float r, float start_theta,
   float delta_theta, ALLEGRO_COLOR color)
{
   float scale = get_scale();
   int num_segments, ii;

   ASSERT(r >= 0);

   num_segments = fabs(delta_theta / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * r));

   if (num_segments < 2)
      num_segments = 2;

   if (num_segments + 1 >= ALLEGRO_VERTEX_CACHE_SIZE) {
      num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1 - 1;
   }

   INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments + 1, (num_segments - 1) * 3, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN, false);

   al_calculate_arc(&(vtx[1].x), sizeof(ALLEGRO_VERTEX), cx, cy, r, r, start_theta, delta_theta, 0, num_segments);
   vtx[0].x = cx; vtx[0].y = cy;

   for (ii = 0; ii < num_segments + 1; ii++) {
      vtx[ii].color = color;
      vtx[ii].z = 0;
      vtx[ii].u = vtx[ii].x - cx;
      vtx[ii].v = vtx[ii].y - cy;
   }

   if (use_batching) {
      if (disp->batch_enabled) {
         for (ii = 0; ii < num_segments - 1; ii++) {
            idx[3 * ii + 0] = first_idx + 0;
            idx[3 * ii + 1] = first_idx + ii + 1;
            idx[3 * ii + 2] = first_idx + ii + 2;
         }
      }
      if (!disp->batch_enabled) {
         disp->vt->draw_batch(disp);
      }
   }
   else {
      al_draw_prim(vtx, 0, 0, 0, num_segments + 1, ALLEGRO_PRIM_TRIANGLE_FAN);
   }
}

/* Function: al_draw_ellipse
 */
void al_draw_ellipse(float cx, float cy, float rx, float ry,
   ALLEGRO_COLOR color, float thickness)
{
   float scale = get_scale();

   ASSERT(rx >= 0);
   ASSERT(ry >= 0);

   if (thickness > 0) {
      int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f);
      int ii;

      /* In case rx and ry are both 0. */
      if (num_segments < 2)
         return;

      if (2 * num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 1) / 2;
      }
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments * 2, (num_segments * 2 - 2) * 3,
            ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP, false);

      al_calculate_arc(&(vtx[0].x), sizeof(ALLEGRO_VERTEX), cx, cy, rx, ry, 0, -ALLEGRO_PI * 2, thickness, num_segments);
      for (ii = 0; ii < 2 * num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
         vtx[ii].u = vtx[ii].x - cx;
         vtx[ii].v = vtx[ii].y - cy;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 2 * num_segments - 2; ii++) {
               idx[3 * ii + 0] = first_idx + ii + ii % 2;
               idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
               idx[3 * ii + 2] = first_idx + ii + 2;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
      }
   } else {
      int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f);
      int ii;

      /* In case rx and ry are both 0. */
      if (num_segments < 2)
         return;

      if (num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1;
      }

      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments, 2 * num_segments - 2,
            ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_LOOP, false);

      al_calculate_arc(&(vtx[0].x), sizeof(ALLEGRO_VERTEX), cx, cy, rx, ry, 0, ALLEGRO_PI * 2, 0, num_segments);
      for (ii = 0; ii < num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
         vtx[ii].u = vtx[ii].x - cx;
         vtx[ii].v = vtx[ii].y - cy;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < num_segments - 1; ii++) {
               idx[2 * ii + 0] = first_idx + ii;
               idx[2 * ii + 1] = first_idx + (ii + 1) % (num_segments - 1);
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, num_segments - 1, ALLEGRO_PRIM_LINE_LOOP);
      }
   }
}

/* Function: al_draw_filled_ellipse
 */
void al_draw_filled_ellipse(float cx, float cy, float rx, float ry,
   ALLEGRO_COLOR color)
{
   int num_segments, ii;
   float scale = get_scale();

   ASSERT(rx >= 0);
   ASSERT(ry >= 0);

   num_segments = ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f);

   /* In case rx and ry are both close to 0. If al_calculate_arc is passed
    * 0 or 1 it will assert.
    */
   if (num_segments < 2)
      return;

   if (num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
      num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1;
   }

   INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments + 1, (num_segments - 1) * 3, ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN, false);

   al_calculate_arc(&(vtx[1].x), sizeof(ALLEGRO_VERTEX), cx, cy, rx, ry, 0, ALLEGRO_PI * 2, 0, num_segments);
   vtx[0].x = cx; vtx[0].y = cy;

   for (ii = 0; ii < num_segments + 1; ii++) {
      vtx[ii].color = color;
      vtx[ii].z = 0;
      vtx[ii].u = vtx[ii].x - cx;
      vtx[ii].v = vtx[ii].y - cy;
   }

   if (use_batching) {
      if (disp->batch_enabled) {
         for (ii = 0; ii < num_segments - 1; ii++) {
            idx[3 * ii + 0] = first_idx + 0;
            idx[3 * ii + 1] = first_idx + ii + 1;
            idx[3 * ii + 2] = first_idx + ii + 2;
         }
      }
      if (!disp->batch_enabled) {
         disp->vt->draw_batch(disp);
      }
   }
   else {
      al_draw_prim(vtx, 0, 0, 0, num_segments + 1, ALLEGRO_PRIM_TRIANGLE_FAN);
   }
}

/* Function: al_draw_circle
 */
void al_draw_circle(float cx, float cy, float r, ALLEGRO_COLOR color,
   float thickness)
{
   al_draw_ellipse(cx, cy, r, r, color, thickness);
}

/* Function: al_draw_filled_circle
 */
void al_draw_filled_circle(float cx, float cy, float r, ALLEGRO_COLOR color)
{
   al_draw_filled_ellipse(cx, cy, r, r, color);
}

/* Function: al_draw_elliptical_arc
 */
void al_draw_elliptical_arc(float cx, float cy, float rx, float ry, float start_theta,
   float delta_theta, ALLEGRO_COLOR color, float thickness)
{
   float scale = get_scale();

   ASSERT(rx >= 0 && ry >= 0);
   if (thickness > 0) {
      int num_segments = fabs(delta_theta / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f));
      int ii;

      if (num_segments < 2)
         num_segments = 2;

      if (2 * num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 1) / 2;
      }
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments * 2, (num_segments * 2 - 2) * 3,
            ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP, false);

      calculate_arc(&vtx[0].x, &vtx[0].u, sizeof(ALLEGRO_VERTEX),
            cx, cy, rx, ry, start_theta + delta_theta, -delta_theta, thickness, num_segments);

      for (ii = 0; ii < 2 * num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 2 * num_segments - 2; ii++) {
               idx[3 * ii + 0] = first_idx + ii + ii % 2;
               idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
               idx[3 * ii + 2] = first_idx + ii + 2;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
      }
   } else {
      int num_segments = fabs(delta_theta / (2 * ALLEGRO_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f));
      int ii;

      if (num_segments < 2)
         num_segments = 2;

      if (num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1;
      }
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments, 2 * num_segments - 2,
            ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_STRIP, false);

      calculate_arc(&vtx[0].x, &vtx[0].u, sizeof(ALLEGRO_VERTEX), cx, cy, rx, ry, start_theta, delta_theta, 0, num_segments);

      for (ii = 0; ii < num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < num_segments; ii++) {
               idx[2 * ii + 0] = first_idx + ii;
               idx[2 * ii + 1] = first_idx + ii + 1;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, num_segments, ALLEGRO_PRIM_LINE_STRIP);
      }
   }
}

/* Function: al_draw_arc
 */
void al_draw_arc(float cx, float cy, float r, float start_theta,
   float delta_theta, ALLEGRO_COLOR color, float thickness)
{
   al_draw_elliptical_arc(cx, cy, r, r, start_theta, delta_theta, color, thickness);
}

/* Function: al_draw_rounded_rectangle
 */
void al_draw_rounded_rectangle(float x1, float y1, float x2, float y2,
   float rx, float ry, ALLEGRO_COLOR color, float thickness)
{
   float scale = get_scale();

   ASSERT(rx >= 0);
   ASSERT(ry >= 0);

   if (thickness > 0) {
      int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f) / 4;
      int ii;

      /* In case rx and ry are both 0. */
      if (num_segments < 2) {
         al_draw_rectangle(x1, y1, x2, y2, color, thickness);
         return;
      }

      if (8 * num_segments + 2 >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 3) / 8;
      }
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, 8 * num_segments + 2, 8 * num_segments * 3,
            ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP, false);

      al_calculate_arc(&(vtx[0].x), sizeof(ALLEGRO_VERTEX), 0, 0, rx, ry, 0, ALLEGRO_PI / 2, thickness, num_segments);

      for (ii = 0; ii < 2 * num_segments; ii += 2) {
         vtx[ii + 2 * num_segments + 1].x = x1 + rx - vtx[2 * num_segments - 1 - ii].x;
         vtx[ii + 2 * num_segments + 1].y = y1 + ry - vtx[2 * num_segments - 1 - ii].y;
         vtx[ii + 2 * num_segments].x = x1 + rx - vtx[2 * num_segments - 1 - ii - 1].x;
         vtx[ii + 2 * num_segments].y = y1 + ry - vtx[2 * num_segments - 1 - ii - 1].y;

         vtx[ii + 4 * num_segments].x = x1 + rx - vtx[ii].x;
         vtx[ii + 4 * num_segments].y = y2 - ry + vtx[ii].y;
         vtx[ii + 4 * num_segments + 1].x = x1 + rx - vtx[ii + 1].x;
         vtx[ii + 4 * num_segments + 1].y = y2 - ry + vtx[ii + 1].y;

         vtx[ii + 6 * num_segments + 1].x = x2 - rx + vtx[2 * num_segments - 1 - ii].x;
         vtx[ii + 6 * num_segments + 1].y = y2 - ry + vtx[2 * num_segments - 1 - ii].y;
         vtx[ii + 6 * num_segments].x = x2 - rx + vtx[2 * num_segments - 1 - ii - 1].x;
         vtx[ii + 6 * num_segments].y = y2 - ry + vtx[2 * num_segments - 1 - ii - 1].y;
      }
      for (ii = 0; ii < 2 * num_segments; ii += 2) {
         vtx[ii].x = x2 - rx + vtx[ii].x;
         vtx[ii].y = y1 + ry - vtx[ii].y;
         vtx[ii + 1].x = x2 - rx + vtx[ii + 1].x;
         vtx[ii + 1].y = y1 + ry - vtx[ii + 1].y;
      }
      vtx[8 * num_segments] = vtx[0];
      vtx[8 * num_segments + 1] = vtx[1];

      for (ii = 0; ii < 8 * num_segments + 2; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
         vtx[ii].u = vtx[ii].x - x1;
         vtx[ii].v = vtx[ii].y - y1;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 8 * num_segments; ii++) {
               idx[3 * ii + 0] = first_idx + ii + ii % 2;
               idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
               idx[3 * ii + 2] = first_idx + ii + 2;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 8 * num_segments + 2, ALLEGRO_PRIM_TRIANGLE_STRIP);
      }
   } else {
      int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f) / 4;
      int ii;

      /* In case rx and ry are both 0. */
      if (num_segments < 2) {
         al_draw_rectangle(x1, y1, x2, y2, color, thickness);
         return;
      }

      if (num_segments * 4 >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 1) / 4;
      }

      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, 4 * num_segments, 4 * num_segments * 2,
            ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_LOOP, false);

      al_calculate_arc(&(vtx[0].x), sizeof(ALLEGRO_VERTEX), 0, 0, rx, ry, 0, ALLEGRO_PI / 2, 0, num_segments + 1);

      for (ii = 0; ii < num_segments; ii++) {
         vtx[ii + 1 * num_segments].x = x1 + rx - vtx[num_segments - 1 - ii].x;
         vtx[ii + 1 * num_segments].y = y1 + ry - vtx[num_segments - 1 - ii].y;

         vtx[ii + 2 * num_segments].x = x1 + rx - vtx[ii].x;
         vtx[ii + 2 * num_segments].y = y2 - ry + vtx[ii].y;

         vtx[ii + 3 * num_segments].x = x2 - rx + vtx[num_segments - 1 - ii].x;
         vtx[ii + 3 * num_segments].y = y2 - ry + vtx[num_segments - 1 - ii].y;
      }
      for (ii = 0; ii < num_segments; ii++) {
         vtx[ii].x = x2 - rx + vtx[ii].x;
         vtx[ii].y = y1 + ry - vtx[ii].y;
      }

      for (ii = 0; ii < 4 * num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
         vtx[ii].u = vtx[ii].x - x1;
         vtx[ii].v = vtx[ii].y - y1;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 4 * num_segments; ii++) {
               idx[2 * ii + 0] = first_idx + ii;
               idx[2 * ii + 1] = first_idx + (ii + 1) % (4 * num_segments);
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 4 * num_segments, ALLEGRO_PRIM_LINE_LOOP);
      }
   }
}

static void calculate_ribbon(float *dest, float *tex_dest, int dest_stride, const float *points,
   int points_stride, float thickness, int num_segments)
{
   ASSERT(points);
   ASSERT(num_segments >= 2);

   if (thickness > 0) {
      int ii = 0;
      float x, y;

      float cur_dir_x;
      float cur_dir_y;
      float prev_dir_x = 0;
      float prev_dir_y = 0;
      float t = thickness / 2;
      float tx, ty;
      float nx, ny;
      float sign = 1;
      float u = 0.;

      for (ii = 0; ii < 2 * num_segments - 2; ii += 2) {
         float dir_len;
         x = *points;
         y = *(points + 1);

         points = (float*)(((char*)points) + points_stride);

         cur_dir_x = *(points)     - x;
         cur_dir_y = *(points + 1) - y;

         dir_len = hypotf(cur_dir_x, cur_dir_y);

         if(dir_len > 0.000001f) {
            cur_dir_x /= dir_len;
            cur_dir_y /= dir_len;
         } else if (ii == 0) {
            cur_dir_x = 1;
            cur_dir_y = 0;
         } else {
            cur_dir_x = prev_dir_x;
            cur_dir_y = prev_dir_y;
         }

         if (ii == 0) {
            tx = -t * cur_dir_y;
            ty = t * cur_dir_x;
            nx = 0;
            ny = 0;
         } else {
            float dot = cur_dir_x * prev_dir_x + cur_dir_y * prev_dir_y;
            float norm_len, cosine;
            if(dot < 0) {
               /*
                * This is by no means exact, but seems to produce acceptable results
                */
               float tx_;
               tx = cur_dir_x - prev_dir_x;
               ty = cur_dir_y - prev_dir_y;
               norm_len = hypotf(tx, ty);

               tx /= norm_len;
               ty /= norm_len;

               cosine = tx * cur_dir_x + ty * cur_dir_y;

               nx = -t * tx / cosine;
               ny = -t * ty / cosine;
               tx_ = tx;
               tx =  -t * ty * cosine;
               ty =  t * tx_ * cosine;
               sign = -sign;
            } else {
               float new_norm_len;
               tx = cur_dir_y + prev_dir_y;
               ty = -(cur_dir_x + prev_dir_x);
               norm_len = hypotf(tx, ty);

               tx /= norm_len;
               ty /= norm_len;
               cosine = tx * (-cur_dir_y) + ty * (cur_dir_x);
               new_norm_len = t / cosine;

               tx *= new_norm_len;
               ty *= new_norm_len;
               nx = 0;
               ny = 0;
            }
         }

         *dest =       x - sign * tx + nx;
         *(dest + 1) = y - sign * ty + ny;
         dest = (float*)(((char*)dest) + dest_stride);
         *dest =       x + sign * tx + nx;
         *(dest + 1) = y + sign * ty + ny;
         dest = (float*)(((char*)dest) + dest_stride);
         if (tex_dest) {
            *tex_dest =       u;
            *(tex_dest + 1) = -thickness / 2;
            tex_dest = (float*)(((char*)tex_dest) + dest_stride);
            *tex_dest =       u;
            *(tex_dest + 1) = thickness / 2;
            tex_dest = (float*)(((char*)tex_dest) + dest_stride);
         }

         prev_dir_x = cur_dir_x;
         prev_dir_y = cur_dir_y;
         u += dir_len;
      }
      tx = -t * prev_dir_y;
      ty = t * prev_dir_x;

      x = *points;
      y = *(points + 1);

      *dest =       x - sign * tx;
      *(dest + 1) = y - sign * ty;
      dest = (float*)(((char*)dest) + dest_stride);
      *dest =       x + sign * tx;
      *(dest + 1) = y + sign * ty;
      if (tex_dest) {
         *tex_dest =       u;
         *(tex_dest + 1) = -thickness / 2;
         tex_dest = (float*)(((char*)tex_dest) + dest_stride);
         *tex_dest =       u;
         *(tex_dest + 1) = thickness / 2;
         tex_dest = (float*)(((char*)tex_dest) + dest_stride);
      }
   } else {
      int ii;
      float u = 0.;
      float old_x = *points;
      float old_y = *(points + 1);
      for (ii = 0; ii < num_segments; ii++) {
         float x = *points;
         float y = *(points + 1);
         *dest = x;
         *(dest + 1) = y;
         if (tex_dest) {
            u += hypotf(old_x - x, old_y - y);
            *tex_dest =       u;
            *(tex_dest + 1) = 0.0f;
            tex_dest = (float*)(((char*)tex_dest) + dest_stride);
         }
         old_x = x;
         old_y = y;
         dest = (float*)(((char*)dest) + dest_stride);
         points = (float*)(((char*)points) + points_stride);
      }
   }
}

/* Function: al_calculate_ribbon
 */
void al_calculate_ribbon(float* dest, int dest_stride, const float *points,
   int points_stride, float thickness, int num_segments)
{
    calculate_ribbon(dest, NULL, dest_stride, points, points_stride,
            thickness, num_segments);
}

/* Function: al_draw_filled_rounded_rectangle
 */
void al_draw_filled_rounded_rectangle(float x1, float y1, float x2, float y2,
   float rx, float ry, ALLEGRO_COLOR color)
{
   int ii;
   float scale = get_scale();
   int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf(scale * (rx + ry) / 2.0f) / 4;

   ASSERT(rx >= 0);
   ASSERT(ry >= 0);

   /* In case rx and ry are both 0. */
   if (num_segments < 2) {
      al_draw_filled_rectangle(x1, y1, x2, y2, color);
      return;
   }

   if (num_segments * 4 >= ALLEGRO_VERTEX_CACHE_SIZE) {
      num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 1) / 4;
   }

   INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, 4 * num_segments, (4 * num_segments - 2) * 3,
         ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_FAN, false);
   al_calculate_arc(&(vtx[0].x), sizeof(ALLEGRO_VERTEX), 0, 0, rx, ry, ALLEGRO_PI / 2, -ALLEGRO_PI / 2, 0, num_segments + 1);

   for (ii = 0; ii < num_segments; ii++) {
      vtx[ii + 1 * num_segments].x = x2 - rx + vtx[num_segments - 1 - ii].x;
      vtx[ii + 1 * num_segments].y = y2 - ry + vtx[num_segments - 1 - ii].y;

      vtx[ii + 2 * num_segments].x = x1 + rx - vtx[ii].x;
      vtx[ii + 2 * num_segments].y = y2 - ry + vtx[ii].y;

      vtx[ii + 3 * num_segments].x = x1 + rx - vtx[num_segments - 1 - ii].x;
      vtx[ii + 3 * num_segments].y = y1 + ry - vtx[num_segments - 1 - ii].y;
   }
   for (ii = 0; ii < num_segments; ii++) {
      vtx[ii].x = x2 - rx + vtx[ii].x;
      vtx[ii].y = y1 + ry - vtx[ii].y;
   }

   for (ii = 0; ii < 4 * num_segments; ii++) {
      vtx[ii].color = color;
      vtx[ii].z = 0;
      vtx[ii].u = vtx[ii].x - x1;
      vtx[ii].v = vtx[ii].y - y1;
   }

   /* FIXME: Doing this as a triangle fan just doesn't sound all that great because it may create thin triangles. */
   if (use_batching) {
      if (disp->batch_enabled) {
         for (ii = 0; ii < 4 * num_segments - 2; ii++) {
            idx[3 * ii + 0] = first_idx + 0;
            idx[3 * ii + 1] = first_idx + ii + 1;
            idx[3 * ii + 2] = first_idx + ii + 2;
         }
      }
      if (!disp->batch_enabled) {
         disp->vt->draw_batch(disp);
      }
   }
   else {
      al_draw_prim(vtx, 0, 0, 0, 4 * num_segments, ALLEGRO_PRIM_TRIANGLE_FAN);
   }
}

static void calculate_spline(float *dest, float *tex_dest, int stride, const float points[8],
   float thickness, int num_segments)
{
   /* Derivatives of x(t) and y(t). */
   float x, dx, ddx, dddx;
   float y, dy, ddy, dddy;
   int ii = 0;

   /* Temp variables used in the setup. */
   float dt, dt2, dt3;
   float xdt2_term, xdt3_term;
   float ydt2_term, ydt3_term;

   /* This is enough to avoid malloc in ex_prim, which I take as a reasonable
    * guess to what a common number of segments might be.  To be honest, it
    * probably makes no difference.
    */
   float cache_point_buffer_storage[150];
   float* cache_point_buffer = cache_point_buffer_storage;

   ASSERT(num_segments > 1);
   ASSERT(points);

   if (num_segments > (int)(sizeof(cache_point_buffer_storage) / sizeof(float) / 2)) {
      cache_point_buffer = al_malloc(2 * sizeof(float) * num_segments);
   }

   dt = 1.0 / (num_segments - 1);
   dt2 = (dt * dt);
   dt3 = (dt2 * dt);

   /* x coordinates. */
   xdt2_term = 3 * (points[4] - 2 * points[2] + points[0]);
   xdt3_term = points[6] + 3 * (-points[4] + points[2]) - points[0];

   xdt2_term = dt2 * xdt2_term;
   xdt3_term = dt3 * xdt3_term;

   dddx = 6 * xdt3_term;
   ddx = -6 * xdt3_term + 2 * xdt2_term;
   dx = xdt3_term - xdt2_term + 3 * dt * (points[2] - points[0]);
   x = points[0];

   /* y coordinates. */
   ydt2_term = 3 * (points[5] - 2 * points[3] + points[1]);
   ydt3_term = points[7] + 3 * (-points[5] + points[3]) - points[1];

   ydt2_term = dt2 * ydt2_term;
   ydt3_term = dt3 * ydt3_term;

   dddy = 6 * ydt3_term;
   ddy = -6 * ydt3_term + 2 * ydt2_term;
   dy = ydt3_term - ydt2_term + dt * 3 * (points[3] - points[1]);
   y = points[1];

   cache_point_buffer[2 * ii] = x;
   cache_point_buffer[2 * ii + 1] = y;

   for (ii = 1; ii < num_segments; ii++) {
      ddx += dddx;
      dx += ddx;
      x += dx;

      ddy += dddy;
      dy += ddy;
      y += dy;

      cache_point_buffer[2 * ii] = x;
      cache_point_buffer[2 * ii + 1] = y;
   }
   calculate_ribbon(dest, tex_dest, stride, cache_point_buffer, 2 * sizeof(float), thickness, num_segments);

   if (cache_point_buffer != cache_point_buffer_storage) {
      al_free(cache_point_buffer);
   }
}

/* Function: al_calculate_spline
 */
void al_calculate_spline(float* dest, int stride, const float points[8],
   float thickness, int num_segments)
{
   calculate_spline(dest, NULL, stride, points, thickness, num_segments);
}

/* Function: al_draw_spline
 */
void al_draw_spline(const float points[8], ALLEGRO_COLOR color, float thickness)
{
   int ii;
   float scale = get_scale();
   int num_segments = (int)(sqrtf(hypotf(points[2] - points[0], points[3] - points[1]) +
                                  hypotf(points[4] - points[2], points[5] - points[3]) +
                                  hypotf(points[6] - points[4], points[7] - points[5])) *
                            1.2 * ALLEGRO_PRIM_QUALITY * scale / 10);

   if(num_segments < 2)
      num_segments = 2;

   if (thickness > 0) {
      if (2 * num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = (ALLEGRO_VERTEX_CACHE_SIZE - 1) / 2;
      }
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, 2 * num_segments, (2 * num_segments - 2) * 3,
         ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP, false);

      calculate_spline(&vtx[0].x, &vtx[0].u, sizeof(ALLEGRO_VERTEX), points, thickness, num_segments);

      for (ii = 0; ii < 2 * num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 2 * num_segments - 2; ii++) {
               idx[3 * ii + 0] = first_idx + ii + ii % 2;
               idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
               idx[3 * ii + 2] = first_idx + ii + 2;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
      }
   } else {
      if (num_segments >= ALLEGRO_VERTEX_CACHE_SIZE) {
         num_segments = ALLEGRO_VERTEX_CACHE_SIZE - 1;
      }
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments, 2 * num_segments - 2,
            ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_STRIP, false);

      calculate_spline(&vtx[0].x, &vtx[0].u, sizeof(ALLEGRO_VERTEX), points, thickness, num_segments);

      for (ii = 0; ii < num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < num_segments; ii++) {
               idx[2 * ii + 0] = first_idx + ii;
               idx[2 * ii + 1] = first_idx + ii + 1;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, num_segments, ALLEGRO_PRIM_LINE_STRIP);
      }
   }
}

/* Function: al_draw_ribbon
 */
void al_draw_ribbon(const float *points, int points_stride, ALLEGRO_COLOR color,
   float thickness, int num_segments)
{
   int ii;

   if (num_segments * (thickness > 0 ? 2 : 1) > ALLEGRO_VERTEX_CACHE_SIZE) {
      ALLEGRO_ERROR("Ribbon has too many segments.\n");
      return;
   }

   if (thickness > 0) {
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments * 2, (num_segments * 2 - 2) * 3,
            ALLEGRO_PRIM_TRIANGLE_LIST, ALLEGRO_PRIM_TRIANGLE_STRIP, false);
      calculate_ribbon(&vtx[0].x, &vtx[0].u, sizeof(ALLEGRO_VERTEX), points, points_stride, thickness, num_segments);

      for (ii = 0; ii < 2 * num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < 2 * num_segments - 2; ii++) {
               idx[3 * ii + 0] = first_idx + ii + ii % 2;
               idx[3 * ii + 1] = first_idx + ii + (ii + 1) % 2;
               idx[3 * ii + 2] = first_idx + ii + 2;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
      }
   } else {
      INIT_PRIM_PTRS_DYNAMIC(ALLEGRO_VERTEX_CACHE_SIZE, num_segments, 2 * num_segments - 2,
            ALLEGRO_PRIM_LINE_LIST, ALLEGRO_PRIM_LINE_STRIP, false);
      calculate_ribbon(&vtx[0].x, &vtx[0].u, sizeof(ALLEGRO_VERTEX), points, points_stride, thickness, num_segments);

      for (ii = 0; ii < num_segments; ii++) {
         vtx[ii].color = color;
         vtx[ii].z = 0;
      }

      if (use_batching) {
         if (disp->batch_enabled) {
            for (ii = 0; ii < num_segments; ii++) {
               idx[2 * ii + 0] = first_idx + ii;
               idx[2 * ii + 1] = first_idx + ii + 1;
            }
         }
         if (!disp->batch_enabled) {
            disp->vt->draw_batch(disp);
         }
      }
      else {
         al_draw_prim(vtx, 0, 0, 0, num_segments, ALLEGRO_PRIM_LINE_STRIP);
      }
   }
}

/* vim: set sts=3 sw=3 et: */
