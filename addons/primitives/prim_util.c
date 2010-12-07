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
 *      Common utilities.
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


# define AL_EPSILON         0.001f


/*
 *  Wraps angle to the range [0, 2 pi).
 */
float _al_prim_wrap_two_pi(float angle)
{
   angle = fmodf(angle, 2.0f * ALLEGRO_PI);
   if (angle < 0)
      angle += 2.0f * ALLEGRO_PI;
   return angle;
}


/*
 *  Wraps angle to the range [-pi, pi).
 */
float _al_prim_wrap_pi_to_pi(float angle)
{
   angle = _al_prim_wrap_two_pi(angle);
   if (angle >= ALLEGRO_PI)
      angle = -2.0f * ALLEGRO_PI + angle;
   return angle;
}


/*
 *  Returns angle between two edges.
 *
 *  Angle is in range [-2 pi, 2 pi], so it is good idea to
 *  use _al_prim_wrap_two_pi() for normalization.
 */
float _al_prim_get_angle(const float* p0, const float* origin, const float* p1)
{
   float angle1 = atan2f(origin[1] - p0[1], origin[0] - p0[0]);
   float angle2 = atan2f(origin[1] - p1[1], origin[0] - p1[0]);

   return angle2 - angle1;
}


/*
 *  Tests on which side of the line point is placed.
 *  Positive value will be returned if point is on half plane
 *  determined by normal vector. Negative value will be returned
 *  if point is on negative half plane determined by normal vector.
 *  Zero will be returned if point lie on the line.
 */
int _al_prim_test_line_side(const float* origin, const float* normal, const float* point)
{
   float c = -(origin[0] * normal[0] + origin[1] * normal[1]);
   float d =   point[0]  * normal[0] + point[1]  * normal[1] + c;
   if (d < -AL_EPSILON)
      return -1;
   else if (d > AL_EPSILON)
      return 1;
   else
      return 0;
}


/*
 *  Tests if point is inside of the triangle defined by vertices v0, v1 and v2.
 *
 *  Order of vertices does not have matter.
 */
bool _al_prim_is_point_in_triangle(const float* point, const float* v0, const float* v1, const float* v2)
{
   float edge_normal_0[2] = { -(v1[1] - v0[1]), v1[0] - v0[0] };
   float edge_normal_1[2] = { -(v2[1] - v1[1]), v2[0] - v1[0] };
   float edge_normal_2[2] = { -(v0[1] - v2[1]), v0[0] - v2[0] };

   int edge_side_0 = _al_prim_test_line_side(v0, edge_normal_0, point);
   int edge_side_1 = _al_prim_test_line_side(v1, edge_normal_1, point);
   int edge_side_2 = _al_prim_test_line_side(v2, edge_normal_2, point);

   return (edge_side_0 == edge_side_1) && (edge_side_0 == edge_side_2);
}


/*
 *  Tests for intersection of lines defined by points { v0, v1 }
 *  and { p0, p1 }.
 *
 *  Returns true if intersection point was determined. If pointers
 *  are provided time and exact point of intersection will be returned.
 *  If test fails false will be returned. Intersection point and time
 *  variables will not be altered in this case.
 *
 *  Intersection time is in { v0, v1 } line space.
 */
bool _al_prim_intersect_segment(const float* v0, const float* v1, const float* p0, const float* p1, float* point/* = NULL*/, float* t0/* = NULL*/, float* t1/* = NULL*/)
{
   float num, denom, time;

   denom = (p1[1] - p0[1]) * (v1[0] - v0[0]) - (p1[0] - p0[0]) * (v1[1] - v0[1]);

   if (fabsf(denom) == 0.0f)
      return false;

   num = (p1[0] - p0[0]) * (v0[1] - p0[1]) - (p1[1] - p0[1]) * (v0[0] - p0[0]);

   time = (num / denom);
   if (t0)
      *t0 = time;

   if (t1) {

      const float num2 = (v1[0] - v0[0]) * (v0[1] - p0[1]) - (v1[1] - v0[1]) * (v0[0] - p0[0]);

      *t1 = (num2 / denom);
   }

   if (point) {

      point[0] = v0[0] + time * (v1[0] - v0[0]);
      point[1] = v0[1] + time * (v1[1] - v0[1]);
   }

   return true;
}


/*
 *  Compares two points for equality.
 *
 *  This is not exact comparison but it is sufficient
 *  for our needs.
 */
bool _al_prim_are_points_equal(const float* point_a, const float* point_b)
{
   return (fabsf(point_a[0] - point_b[0]) < AL_EPSILON)
       && (fabsf(point_a[1] - point_b[1]) < AL_EPSILON);
}
