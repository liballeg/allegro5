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
 *      Polyline primitive.
 *
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_list.h"
#include "allegro5/internal/aintern_prim.h"
#include <float.h>
#include <math.h>


/*
 * Computes direction, normal direction and the length of the segment.
 */
static float compute_direction_and_normal(const float* begin, const float* end, float* dir, float* normal)
{
   float length;

   /* Determine direction, normal direction and length of the line segment. */
   dir[0] = end[0] - begin[0];
   dir[1] = end[1] - begin[1];

   length = _al_prim_normalize(dir);

   normal[0] = -dir[1];
   normal[1] =  dir[0];

   return length;
}

/*
 * Compute end cross points.
 */
static void compute_end_cross_points(const float* v0, const float* v1, float radius, float* p0, float* p1)
{
   float dir[2];
   float normal[2];
   /* XXX delete this parameter? */
   (void)radius;

   compute_direction_and_normal(v0, v1, dir, normal);

   p0[0] = v1[0] + normal[0] * radius;
   p0[1] = v1[1] + normal[1] * radius;
   p1[0] = v1[0] - normal[0] * radius;
   p1[1] = v1[1] - normal[1] * radius;
}

/*
 * Compute cross points.
 */
static void compute_cross_points(const float* v0, const float* v1, const float* v2, float radius,
   float* l0, float* l1, float* r0, float* r1, float* out_middle, float* out_angle, float* out_miter_distance)
{
   float normal_0[2], normal_1[2];
   float dir_0[2], dir_1[2];
   float middle[2];
   float diff[2];
   float miter_distance;
   float angle;
   float len_0, len_1;
   bool sharp = false;

   /* Compute directions. */
   len_0 = compute_direction_and_normal(v0, v1, dir_0, normal_0);
   len_1 = compute_direction_and_normal(v1, v2, dir_1, normal_1);

   /* Compute angle of deflection between segments. */
   diff[0] =   dir_0[0] * dir_1[0] + dir_0[1] * dir_1[1];
   diff[1] = -(dir_0[0] * dir_1[1] - dir_0[1] * dir_1[0]);

   angle = (diff[0] || diff[1]) ? atan2f(diff[1], diff[0]) : 0.0f;

   /* Calculate miter distance. */
   miter_distance = angle != 0.0f ? radius / cosf(fabsf(angle) * 0.5f) : radius;

   /* If the angle is too sharp, we give up on trying not to overdraw. */
   if (miter_distance < 0) {
      sharp = true;
      miter_distance = 0;
   }
   if (miter_distance > len_0) {
      sharp = true;
      miter_distance = len_0;
   }
   if(miter_distance > len_1) {
      sharp = true;
      miter_distance = len_1;
   }

   middle[0] = normal_0[0] + normal_1[0];
   middle[1] = normal_0[1] + normal_1[1];

   if (middle[0] == middle[1] && middle[0] == 0.0) {
      middle[0] = dir_1[0];
      middle[1] = dir_1[1];
   }

   _al_prim_normalize(middle);

   /* Compute points. */
   if (angle > 0.0f)
   {
      l0[0] = v1[0] + normal_0[0] * radius;
      l0[1] = v1[1] + normal_0[1] * radius;
      r0[0] = v1[0] + normal_1[0] * radius;
      r0[1] = v1[1] + normal_1[1] * radius;

      if (sharp) {
         l1[0] = v1[0] - normal_0[0] * radius;
         l1[1] = v1[1] - normal_0[1] * radius;
         r1[0] = v1[0] - normal_1[0] * radius;
         r1[1] = v1[1] - normal_1[1] * radius;
      }
      else {
         l1[0] = r1[0] = v1[0] - middle[0] * miter_distance;
         l1[1] = r1[1] = v1[1] - middle[1] * miter_distance;
      }
   }
   else
   {
      middle[0] = -middle[0];
      middle[1] = -middle[1];

      l1[0] = v1[0] - normal_0[0] * radius;
      l1[1] = v1[1] - normal_0[1] * radius;
      r1[0] = v1[0] - normal_1[0] * radius;
      r1[1] = v1[1] - normal_1[1] * radius;

      if (sharp) {
         l0[0] = v1[0] + normal_0[0] * radius;
         l0[1] = v1[1] + normal_0[1] * radius;
         r0[0] = v1[0] + normal_1[0] * radius;
         r0[1] = v1[1] + normal_1[1] * radius;
      }
      else {
         l0[0] = r0[0] = v1[0] - middle[0] * miter_distance;
         l0[1] = r0[1] = v1[1] - middle[1] * miter_distance;
      }
   }

   if (out_angle)
      *out_angle = angle;

   if (out_miter_distance)
      *out_miter_distance = miter_distance;

   if (out_middle)
      memcpy(out_middle, middle, sizeof(float) * 2);
}

/*
 * Emits filled arc.
 *
 * Arc is defined by pivot point, radius, start and end angle.
 * Starting and ending angle are wrapped to two pi range.
 */
static void emit_arc(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, float start, float end, float radius, int segments)
{
   float arc;
   float c, s, t;
   float v0[2];
   float v1[2];
   float cp[2];
   int i;

   /* This is very small arc, we will draw nothing. */
   if (fabsf(end - start) < 0.001f)
      return;

   /* Make sure start both start angle is located in the
    * range [0, 2 * pi) and end angle is greater than
    * start angle.
    */
   start = fmodf(start, A5O_PI * 2.0f);
   end   = fmodf(end,   A5O_PI * 2.0f);
   if (end <= start)
      end += A5O_PI * 2.0f;

   arc = end - start;

   segments = (int)(segments * arc / A5O_PI * 2.0f);
   if (segments < 1)
      segments = 1;

   c = cosf(arc / segments);
   s = sinf(arc / segments);
   cp[0] = cosf(start) * radius;
   cp[1] = sinf(start) * radius;
   v0[0] = cp[0] + pivot[0];
   v0[1] = cp[1] + pivot[1];

   for (i = 0; i < segments - 1; ++i)
   {
      t = cp[0];
      cp[0] = c * cp[0] - s * cp[1];
      cp[1] = s * t     + c * cp[1];

      v1[0] = cp[0] + pivot[0];
      v1[1] = cp[1] + pivot[1];

      _al_prim_cache_push_triangle(cache, v0, pivot, v1);

      v0[0] = v1[0];
      v0[1] = v1[1];
   }

   v1[0] = cosf(end) * radius + pivot[0];
   v1[1] = sinf(end) * radius + pivot[1];
   _al_prim_cache_push_triangle(cache, v0, pivot, v1);
}

/*
 * Emits square cap.
 *
 * Square cap is an rectangle with edges lengths equal to radius
 * and double radius, first along direction second along normal
 * direction.
 */
static void emit_square_end_cap(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, const float* dir, const float* normal, float radius)
{
   /* Prepare all four vertices of the rectangle. */
   float v0[2] = { pivot[0] + normal[0] * radius, pivot[1] + normal[1] * radius };
   float v1[2] = { pivot[0] - normal[0] * radius, pivot[1] - normal[1] * radius };
   float v2[2] = {       v0[0] + dir[0] * radius,       v0[1] + dir[1] * radius };
   float v3[2] = {       v1[0] + dir[0] * radius,       v1[1] + dir[1] * radius };

   /* Emit. */
   _al_prim_cache_push_triangle(cache, v0, v2, v3);
   _al_prim_cache_push_triangle(cache, v0, v3, v1);
}

/*
 * Emits triangular cap.
 */
static void emit_triange_end_cap(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, const float* dir, const float* normal, float radius)
{
   /* Prepare all four vertices of the rectangle. */
   float v0[2] = { pivot[0] + normal[0] * radius, pivot[1] + normal[1] * radius };
   float v1[2] = { pivot[0] - normal[0] * radius, pivot[1] - normal[1] * radius };
   float v2[2] = { pivot[0] +    dir[0] * radius, pivot[1] +    dir[1] * radius };

   /* Emit. */
   _al_prim_cache_push_triangle(cache, v0, v2, v1);
}

/*
 * Emits rounded cap.
 */
static void emit_round_end_cap(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, const float* dir, const float* normal, float radius)
{
   float angle = atan2f(-normal[1], -normal[0]);
   /* XXX delete these parameters? */
   (void)dir;
   (void)radius;

   emit_arc(cache, pivot, angle, angle + A5O_PI, radius, 16);
}

/*
 * Emits end cap.
 *
 * Direction of the end cap is defined by vector [v1 - v0]. p0 and p1 are starting
 * and ending point of the cap. Both should be located on the circle with center
 * in v1 and specified radius. p0 have to be located on the negative and p0 on the
 * positive half plane defined by direction vector.
 */
static void emit_end_cap(A5O_PRIM_VERTEX_CACHE* cache, int cap_style, const float* v0, const float* v1, float radius)
{
   float dir[2];
   float normal[2];

   /* Do do not want you to call this function for closed cap.
    * It is special and there is nothing we can do with it there.
    */
   ASSERT(cap_style != A5O_LINE_CAP_CLOSED);

   /* There nothing we can do for this kind of ending cap. */
   if (cap_style == A5O_LINE_CAP_NONE)
      return;

   /* Compute normal and direction for our segment. */
   compute_direction_and_normal(v0, v1, dir, normal);

   /* Emit vertices for cap. */
   if (cap_style == A5O_LINE_CAP_SQUARE)
      emit_square_end_cap(cache, v1, dir, normal, radius);
   else if (cap_style == A5O_LINE_CAP_TRIANGLE)
      emit_triange_end_cap(cache, v1, dir, normal, radius);
   else if (cap_style == A5O_LINE_CAP_ROUND)
      emit_round_end_cap(cache, v1, dir, normal, radius);
   else {

      ASSERT("Unknown or unsupported style of ending cap." && false);
   }
}

/*
 * Emits bevel join.
 */
static void emit_bevel_join(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, const float* p0, const float* p1)
{
   _al_prim_cache_push_triangle(cache, pivot, p0, p1);
}

/*
 * Emits round join.
 */
static void emit_round_join(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, const float* p0, const float* p1, float radius)
{
   float start = atan2f(p1[1] - pivot[1], p1[0] - pivot[0]);
   float end   = atan2f(p0[1] - pivot[1], p0[0] - pivot[0]);

   if (end < start)
      end += A5O_PI * 2.0f;

   emit_arc(cache, pivot, start, end, radius, 16);
}

/*
 * Emits miter join.
 */
static void emit_miter_join(A5O_PRIM_VERTEX_CACHE* cache, const float* pivot, const float* p0, const float* p1,
   float radius, const float* middle, float angle, float miter_distance, float max_miter_distance)
{
   /* XXX delete this parameter? */
   (void)radius;

   if (miter_distance > max_miter_distance) {

      float normal[2] = { -middle[1], middle[0] };

      float offset = (miter_distance - max_miter_distance) * tanf((A5O_PI - fabsf(angle)) * 0.5f);

      float v0[2] = {
         pivot[0] + middle[0] * max_miter_distance + normal[0] * offset,
         pivot[1] + middle[1] * max_miter_distance + normal[1] * offset
      };

      float v1[2] = {
         pivot[0] + middle[0] * max_miter_distance - normal[0] * offset,
         pivot[1] + middle[1] * max_miter_distance - normal[1] * offset
      };

      _al_prim_cache_push_triangle(cache, pivot, v0, v1);
      _al_prim_cache_push_triangle(cache, pivot, p0, v0);
      _al_prim_cache_push_triangle(cache, pivot, p1, v1);
   }
   else {

      float miter[2] = {
         pivot[0] + middle[0] * miter_distance,
         pivot[1] + middle[1] * miter_distance,
      };

      _al_prim_cache_push_triangle(cache, pivot, p0, miter);
      _al_prim_cache_push_triangle(cache, pivot, miter, p1);
   }
}


/* Emit join between segments.
 */
static void emit_join(A5O_PRIM_VERTEX_CACHE* cache, int join_style, const float* pivot,
   const float* p0, const float* p1, float radius, const float* middle,
   float angle, float miter_distance, float miter_limit)
{
   /* There is nothing to do for this type of join. */
   if (join_style == A5O_LINE_JOIN_NONE)
      return;

   if (join_style == A5O_LINE_JOIN_BEVEL)
      emit_bevel_join(cache, pivot, p0, p1);
   else if (join_style == A5O_LINE_JOIN_ROUND)
      emit_round_join(cache, pivot, p0, p1, radius);
   else if (join_style == A5O_LINE_JOIN_MITER)
      emit_miter_join(cache, pivot, p0, p1, radius, middle, angle, miter_distance, miter_limit * radius);
   else {

      ASSERT("Unknown or unsupported style of join." && false);
   }
}

static void emit_polyline(A5O_PRIM_VERTEX_CACHE* cache, const float* vertices, int vertex_stride, int vertex_count, int join_style, int cap_style, float thickness, float miter_limit)
{
# define VERTEX(index)  ((const float*)(((uint8_t*)vertices) + vertex_stride * ((vertex_count + (index)) % vertex_count)))

   float l0[2], l1[2];
   float r0[2], r1[2];
   float p0[2], p1[2];
   float radius;
   int steps;
   int i;

   ASSERT(thickness > 0.0f);

   /* Discard invalid lines. */
   if (vertex_count < 2)
      return;

   radius = 0.5f * thickness;

   /* Single line cannot be closed. If user forgot to explicitly specify
   * most desired alternative cap style, we just disable capping at all.
   */
   if (vertex_count == 2 && cap_style == A5O_LINE_CAP_CLOSED)
      cap_style = A5O_LINE_CAP_NONE;

   /* Prepare initial set of vertices. */
   if (cap_style != A5O_LINE_CAP_CLOSED)
   {
      /* We can emit ending caps right now.
      *
      * VERTEX(-2) and similar are safe, because it at this place
      * it is guaranteed that there are at least two vertices
      * in the buffer.
      */
      emit_end_cap(cache, cap_style,  VERTEX(1),  VERTEX(0), radius);
      emit_end_cap(cache, cap_style, VERTEX(-2), VERTEX(-1), radius);

      /* Compute points on the left side of the very first segment. */
      compute_end_cross_points(VERTEX(1), VERTEX(0), radius, p1, p0);

      /* For non-closed line we have N - 1 steps, but since we iterate
      * from one, N is right value.
      */
      steps = vertex_count;
   }
   else
   {
      /* Compute points on the left side of the very first segment. */
      compute_cross_points(VERTEX(-1), VERTEX(0), VERTEX(1), radius, l0, l1, p0, p1, NULL, NULL, NULL);

      /* Closed line use N steps, because last vertex have to be
      * connected with first one.
      */
      steps = vertex_count + 1;
   }

   /* Process segments. */
   for (i = 1; i < steps; ++i)
   {
      /* Pick vertex and their neighbors. */
      const float* v0 = VERTEX(i - 1);
      const float* v1 = VERTEX(i);
      const float* v2 = VERTEX(i + 1);

      /* Choose correct cross points. */
      if ((cap_style == A5O_LINE_CAP_CLOSED) || (i < steps - 1)) {

         float middle[2];
         float miter_distance;
         float angle;

         /* Compute cross points. */
         compute_cross_points(v0, v1, v2, radius, l0, l1, r0, r1, middle, &angle, &miter_distance);

         /* Emit join. */
         if (angle >= 0.0f)
            emit_join(cache, join_style, v1, l0, r0, radius, middle, angle, miter_distance, miter_limit);
         else
            emit_join(cache, join_style, v1, r1, l1, radius, middle, angle, miter_distance, miter_limit);
      }
      else
         compute_end_cross_points(v0, v1, radius, l0, l1);

      /* Emit triangles. */
      _al_prim_cache_push_triangle(cache, v0, v1, l1);
      _al_prim_cache_push_triangle(cache, v0, l1, p1);
      _al_prim_cache_push_triangle(cache, v0, p0, l0);
      _al_prim_cache_push_triangle(cache, v0, l0, v1);

      /* Save current most right vertices. */
      memcpy(p0, r0, sizeof(float) * 2);
      memcpy(p1, r1, sizeof(float) * 2);
   }

# undef VERTEX
}

static void do_draw_polyline(A5O_PRIM_VERTEX_CACHE* cache, const float* vertices, int vertex_stride, int vertex_count, int join_style, int cap_style, A5O_COLOR color, float thickness, float miter_limit)
{
   if (thickness > 0.0f)
   {
      _al_prim_cache_init(cache, A5O_PRIM_VERTEX_CACHE_TRIANGLE, color);
      emit_polyline(cache, vertices, vertex_stride, vertex_count, join_style, cap_style, thickness, miter_limit);
      _al_prim_cache_term(cache);
   }
   else
   {
# define VERTEX(index)  ((const float*)(((uint8_t*)vertices) + vertex_stride * ((vertex_count + (index)) % vertex_count)))

      int i;

      _al_prim_cache_init(cache, A5O_PRIM_VERTEX_CACHE_LINE_STRIP, color);

      for (i = 0; i < vertex_count; ++i) {
         if (cache->size >= (A5O_VERTEX_CACHE_SIZE - 2))
            _al_prim_cache_flush(cache);

         _al_prim_cache_push_point(cache, VERTEX(i));
      }

      if (cap_style == A5O_LINE_CAP_CLOSED && vertex_count > 2) {
         if (cache->size >= (A5O_VERTEX_CACHE_SIZE - 2))
            _al_prim_cache_flush(cache);

         _al_prim_cache_push_point(cache, VERTEX(0));
      }

      _al_prim_cache_term(cache);

# undef VERTEX
   }
}

/* Function: al_draw_polyline
 */
void al_draw_polyline(const float* vertices, int vertex_stride,
   int vertex_count, int join_style, int cap_style,
   A5O_COLOR color, float thickness, float miter_limit)
{
   A5O_PRIM_VERTEX_CACHE cache;
   do_draw_polyline(&cache, vertices, vertex_stride, vertex_count, join_style, cap_style, color, thickness, miter_limit);
}

/* vim: set sts=3 sw=3 et: */
