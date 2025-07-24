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

#include <float.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_list.h"
#include "allegro5/internal/aintern_prim_addon.h"

#ifdef ALLEGRO_MSVC
   #define hypotf(x, y) _hypotf((x), (y))
#endif

#define AL_EPSILON         0.001f


/*
 * Make an estimate of the scale of the current transformation.
 */
float _al_prim_get_scale(void)
{
   const ALLEGRO_TRANSFORM* t = al_get_current_transform();
   return (hypotf(t->m[0][0], t->m[0][1]) + hypotf(t->m[1][0], t->m[1][1])) / 2;
}


/*
 * Normalizes vector.
 */
float _al_prim_normalize(float* vector)
{
   float length;
   float inv_length;

   length     = hypotf(vector[0], vector[1]);
   inv_length = length > 0.0f ? 1.0f / length : 1.0f;

   vector[0] *= inv_length;
   vector[1] *= inv_length;

   return length;
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
   if (d < 0.0f)/*-AL_EPSILON)*/
      return -1;
   else if (d > 0.0f)/*AL_EPSILON)*/
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

   if (edge_side_0 && edge_side_1 && edge_side_2)
      return (edge_side_0 == edge_side_1) && (edge_side_0 == edge_side_2);
   else if (0 == edge_side_0)
      return (edge_side_1 == edge_side_2);
   else if (0 == edge_side_1)
      return (edge_side_0 == edge_side_2);
   else /*if (0 == edge_side_2)*/
      return (edge_side_0 == edge_side_1);
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


/*
 *
 */
void _al_prim_cache_init(ALLEGRO_PRIM_VERTEX_CACHE* cache, int prim_type, ALLEGRO_COLOR color)
{
   _al_prim_cache_init_ex(cache, prim_type, color, NULL);
}

void _al_prim_cache_init_ex(ALLEGRO_PRIM_VERTEX_CACHE* cache, int prim_type, ALLEGRO_COLOR color, void* user_data)
{
   cache->size      = 0;
   cache->current   = cache->buffer;
   cache->color     = color;
   cache->prim_type = prim_type;
   cache->user_data = user_data;
}

void _al_prim_cache_term(ALLEGRO_PRIM_VERTEX_CACHE* cache)
{
   _al_prim_cache_flush(cache);
}

void _al_prim_cache_flush(ALLEGRO_PRIM_VERTEX_CACHE* cache)
{
   if (cache->size == 0)
      return;

   if (cache->prim_type == ALLEGRO_PRIM_VERTEX_CACHE_TRIANGLE)
      al_draw_prim(cache->buffer, NULL, NULL, 0, cache->size, ALLEGRO_PRIM_TRIANGLE_LIST);
   else if (cache->prim_type == ALLEGRO_PRIM_VERTEX_CACHE_LINE_STRIP)
      al_draw_prim(cache->buffer, NULL, NULL, 0, cache->size, ALLEGRO_PRIM_LINE_STRIP);

   if (cache->prim_type == ALLEGRO_PRIM_VERTEX_CACHE_LINE_STRIP)
   {
      cache->buffer[0] = *(cache->current - 1);
      cache->current   = cache->buffer + 1;
      cache->size      = 1;
   }
   else
   {
      cache->current = cache->buffer;
      cache->size    = 0;
   }
}

void _al_prim_cache_push_triangle(ALLEGRO_PRIM_VERTEX_CACHE* cache, const float* v0, const float* v1, const float* v2)
{
   if (cache->size >= (ALLEGRO_VERTEX_CACHE_SIZE - 3))
      _al_prim_cache_flush(cache);

   cache->current->x     = v0[0];
   cache->current->y     = v0[1];
   cache->current->z     = 0.0f;
   cache->current->color = cache->color;

   ++cache->current;

   cache->current->x     = v1[0];
   cache->current->y     = v1[1];
   cache->current->z     = 0.0f;
   cache->current->color = cache->color;

   ++cache->current;

   cache->current->x     = v2[0];
   cache->current->y     = v2[1];
   cache->current->z     = 0.0f;
   cache->current->color = cache->color;

   ++cache->current;

   cache->size += 3;

   //al_draw_triangle(v0[0], v0[1], v1[0], v1[1], v2[0], v2[1], cache->color, 1.0f);
}

void _al_prim_cache_push_point(ALLEGRO_PRIM_VERTEX_CACHE* cache, const float* v)
{
   if (cache->size >= (ALLEGRO_VERTEX_CACHE_SIZE - 1))
      _al_prim_cache_flush(cache);

   cache->current->x     = v[0];
   cache->current->y     = v[1];
   cache->current->z     = 0.0f;
   cache->current->color = cache->color;

   ++cache->current;
   ++cache->size;
}
