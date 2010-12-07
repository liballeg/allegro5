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
 *      Polygon with holes drawing routines.
 *
 *
 *      By Michał Cichoń.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim.h"
#include <math.h>

#ifdef ALLEGRO_MSVC
   #define hypotf(x, y) _hypotf((x), (y))
#endif

typedef enum POLYGON_PRIM_TYPE {
   ALLEGRO_POLY_PRIM_TRIANGLE,
   ALLEGRO_POLY_PRIM_LINE,
} POLYGON_PRIM_TYPE;

typedef struct POLYGON_VERTEX_CACHE {
   ALLEGRO_VERTEX          cache[ALLEGRO_VERTEX_CACHE_SIZE];
   size_t                  size;
   POLYGON_PRIM_TYPE   prim_type;
   ALLEGRO_VERTEX*         current;
   const float*            vertices;
   ALLEGRO_COLOR           color;
} POLYGON_VERTEX_CACHE;

static void polygon_cache_init(POLYGON_VERTEX_CACHE* cache, POLYGON_PRIM_TYPE prim_type, const float* vertices, ALLEGRO_COLOR color)
{
   cache->vertices  = vertices;
   cache->size      = 0;
   cache->prim_type = prim_type;
   cache->current   = cache->cache;
   cache->color     = color;
}

static void polygon_cache_flush(POLYGON_VERTEX_CACHE* cache)
{
   if (cache->size == 0)
      return;

   if (cache->prim_type == ALLEGRO_POLY_PRIM_TRIANGLE)
      al_draw_prim(cache->cache, NULL, NULL, 0, cache->size, ALLEGRO_PRIM_TRIANGLE_LIST);
   else /* if (cache->prim_type == ALLEGRO_POLY_PRIM_LINE) */
      al_draw_prim(cache->cache, NULL, NULL, 0, cache->size, ALLEGRO_PRIM_LINE_STRIP);

   cache->current = cache->cache;
   cache->size    = 0;
}

static void polygon_cache_push_triangle(POLYGON_VERTEX_CACHE* cache, const float* v0, const float* v1, const float* v2)
{
   if (cache->size >= (ALLEGRO_VERTEX_CACHE_SIZE - 3))
      polygon_cache_flush(cache);

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

static void polygon_cache_push_triangle_callback(int i0, int i1, int i2, void* user_data)
{
   POLYGON_VERTEX_CACHE* cache = (POLYGON_VERTEX_CACHE*)user_data;

   const float* v0 = cache->vertices + (i0 * 2);
   const float* v1 = cache->vertices + (i1 * 2);
   const float* v2 = cache->vertices + (i2 * 2);

   polygon_cache_push_triangle(cache, v0, v1, v2);
}


static float compute_cross_edge(const float* v0, const float* v1, const float* v2, float* left_0, float* left_1, float* right_0, float* right_1, float* center, float radius)
{
   float normal_0[2];
   float normal_1[2];

   float dir_0[2] = { v1[0] - v0[0], v1[1] - v0[1] };
   float dir_1[2] = { v2[0] - v1[0], v2[1] - v1[1] };

   float length_sq_0 = dir_0[0] * dir_0[0] + dir_0[1] * dir_0[1];
   float length_sq_1 = dir_1[0] * dir_1[0] + dir_1[1] * dir_1[1];

   float inv_length_0 = length_sq_0 > 0.0f ? 1.0f / sqrtf(length_sq_0) : 0.0f;
   float inv_length_1 = length_sq_1 > 0.0f ? 1.0f / sqrtf(length_sq_1) : 0.0f;

   float angle;

   dir_0[0] *= inv_length_0;
   dir_0[1] *= inv_length_0;
   dir_1[0] *= inv_length_1;
   dir_1[1] *= inv_length_1;

   normal_0[0] = -dir_0[1];
   normal_0[1] =  dir_0[0];
   normal_1[0] = -dir_1[1];
   normal_1[1] =  dir_1[0];

   angle = _al_prim_wrap_pi_to_pi(_al_prim_get_angle(v0, v1, v2));

   if (fabsf(angle) >= ALLEGRO_PI / 2.0f) {

      float hole_angle = ALLEGRO_PI - fabsf(angle);

      float offset = radius * tanf(hole_angle / 2.0f);

      left_0[0]  = v1[0] + radius * -normal_0[0] - offset * dir_0[0];
      left_0[1]  = v1[1] + radius * -normal_0[1] - offset * dir_0[1];

      left_1[0]  = v1[0] + radius *  normal_0[0] - offset * dir_0[0];
      left_1[1]  = v1[1] + radius *  normal_0[1] - offset * dir_0[1];

      right_0[0] = v1[0] + radius * -normal_1[0] + offset * dir_1[0];
      right_0[1] = v1[1] + radius * -normal_1[1] + offset * dir_1[1];

      right_1[0] = v1[0] + radius *  normal_1[0] + offset * dir_1[0];
      right_1[1] = v1[1] + radius *  normal_1[1] + offset * dir_1[1];

      if (angle > 0.0f) {

         center[0]  = right_0[0];
         center[1]  = right_0[1];
      }
      else {

         center[0]  = right_1[0];
         center[1]  = right_1[1];
      }
   }
   else {
      float hole_angle = fabsf(angle);

      float offset = radius / tanf(hole_angle / 2.0f);

      float ray_dir_0[2];
      float ray_dir_1[2];

      float dist_0, dist_1;

      if (angle > 0.0f) {

         dist_0 = 1.0f;
         dist_1 = 0.5f;
      }
      else {

         dist_0 = 0.5f;
         dist_1 = 1.0f;
      }

      left_0[0]  = v1[0] + radius * -normal_0[0] - offset * dir_0[0] * dist_0;
      left_0[1]  = v1[1] + radius * -normal_0[1] - offset * dir_0[1] * dist_0;

      left_1[0]  = v1[0] + radius *  normal_0[0] - offset * dir_0[0] * dist_1;
      left_1[1]  = v1[1] + radius *  normal_0[1] - offset * dir_0[1] * dist_1;

      right_0[0] = v1[0] + radius * -normal_1[0] + offset * dir_1[0] * dist_0;
      right_0[1] = v1[1] + radius * -normal_1[1] + offset * dir_1[1] * dist_0;

      right_1[0] = v1[0] + radius *  normal_1[0] + offset * dir_1[0] * dist_1;
      right_1[1] = v1[1] + radius *  normal_1[1] + offset * dir_1[1] * dist_1;

      if (angle > 0.0f) {

         ray_dir_0[0] = left_1[0] - normal_0[0];
         ray_dir_0[1] = left_1[1] - normal_0[1];

         ray_dir_1[0] = right_1[0] - normal_1[0];
         ray_dir_1[1] = right_1[1] - normal_1[1];

         _al_prim_intersect_segment(left_1, ray_dir_0, right_1, ray_dir_1, center, NULL, NULL);
      }
      else {

         ray_dir_0[0] = left_0[0] + normal_0[0];
         ray_dir_0[1] = left_0[1] + normal_0[1];

         ray_dir_1[0] = right_0[0] + normal_1[0];
         ray_dir_1[1] = right_0[1] + normal_1[1];

         _al_prim_intersect_segment(left_0, ray_dir_0, right_0, ray_dir_1, center, NULL, NULL);
      }
   }

   return angle;
}

static void emit_arc_between_edges(POLYGON_VERTEX_CACHE* cache, const float* left_edge, const float* center, const float* right_edge, float radius, int segments)
{
   float start = atan2f( left_edge[1] - center[1],  left_edge[0] - center[0]);
   float end   = atan2f(right_edge[1] - center[1], right_edge[0] - center[0]);
   float step, angle, arc;
   float v0[2];
   float v1[2];
   int i;

   // This is very small arc, we will draw nothing.
   if (fabsf(end - start) < 0.001f)
      return;

   start = fmodf(start, ALLEGRO_PI * 2.0f);
   end   = fmodf(end,   ALLEGRO_PI * 2.0f);
   if (end <= start)
      end += ALLEGRO_PI * 2.0f;

   arc = end - start;

   segments = (int)(segments * arc / ALLEGRO_PI * 2.0f);
   if (segments < 1)
      segments = 1;

   step = (end - start) / segments;

   angle = start;

   v0[0] = center[0] + cosf(angle) * radius;
   v0[1] = center[1] + sinf(angle) * radius;
   for (i = 0; i < segments; ++i, angle += step)
   {
      v1[0] = center[0] + cosf(angle + step) * radius;
      v1[1] = center[1] + sinf(angle + step) * radius;

      polygon_cache_push_triangle(cache, v0, center, v1);

      v0[0] = v1[0];
      v0[1] = v1[1];
   }
}

/*
 * Make an estimate of the scale of the current transformation.
 */
static float get_scale(void)
{
   const ALLEGRO_TRANSFORM* t = al_get_current_transform();
   return (hypotf(t->m[0][0], t->m[0][1]) + hypotf(t->m[1][0], t->m[1][1])) / 2;
}

static void do_draw_polygon(POLYGON_VERTEX_CACHE* cache, const float* vertices, int vertex_stride, int vertex_count, ALLEGRO_COLOR color, float thickness)
{
# define VERTEX(index)  ((const float*)(((uint8_t*)vertices) + vertex_stride * ((vertex_count + (index)) % vertex_count)))

   if (thickness > 0.0f) {

      float very_left_0[2] = { 0 };
      float very_left_1[2] = { 0 };
      float left_0[2];
      float left_1[2];
      float right_0[2];
      float right_1[2];
      float center[2];
      float radius;
      float inner_radius;
      float last_angle = 0.0;
      int segments;
      int i;

      radius = thickness * 0.5f;

      segments = ALLEGRO_PRIM_QUALITY * get_scale() * sqrtf(radius);

      polygon_cache_init(cache, ALLEGRO_POLY_PRIM_TRIANGLE, vertices, color);

      for (i = 0; i <= vertex_count; ++i) {

         const float* v0 = VERTEX(i - 1);
         const float* v1 = VERTEX(i);
         const float* v2 = VERTEX(i + 1);

         float inner_angle = compute_cross_edge(v0, v1, v2, left_0, left_1, right_0, right_1, center, radius);

         if (0 == i) {

            very_left_0[0] = right_0[0];
            very_left_0[1] = right_0[1];
            very_left_1[0] = right_1[0];
            very_left_1[1] = right_1[1];
            last_angle     = inner_angle;
            continue;
         }

         if (last_angle > 0.0f) {

            polygon_cache_push_triangle(cache, very_left_0, left_0, very_left_1);
            polygon_cache_push_triangle(cache, very_left_1, left_0,      left_1);
         }
         else {
            polygon_cache_push_triangle(cache, very_left_1, left_1, very_left_0);
            polygon_cache_push_triangle(cache, very_left_0, left_1,      left_0);
         }

         if (fabsf(inner_angle) < ALLEGRO_PI / 2.0f) {

            //ALLEGRO_COLOR color = cache->color;
            //cache->color = al_map_rgb(0, 255, 0);
            polygon_cache_push_triangle(cache, right_0, right_1, center);
            polygon_cache_push_triangle(cache,  left_1,  left_0, center);
            //cache->color = color;

            if (inner_angle > 0.0f)
               inner_radius = hypotf(right_1[0] - center[0], right_1[1] - center[1]);
            else
               inner_radius = hypotf(left_0[0] - center[0], left_0[1] - center[1]);
         }
         else
            inner_radius = thickness;

         //ALLEGRO_COLOR color = cache->color;
         //cache->color = al_map_rgb(0, 0, 255);
         if (inner_angle > 0.0f)
            emit_arc_between_edges(cache, right_1, center, left_1, inner_radius, segments);
         else
            emit_arc_between_edges(cache, left_0, center, right_0, inner_radius, segments);
         //cache->color = color;

         //al_draw_line(center[0], center[1], v1[0], v1[1], al_map_rgb(255, 0, 255), 2.0f);

         very_left_0[0] = right_0[0];
         very_left_0[1] = right_0[1];
         very_left_1[0] = right_1[0];
         very_left_1[1] = right_1[1];
         last_angle     = inner_angle;
      }

      polygon_cache_flush(cache);
   }
   else {

      const float* vertex;
      int i;

      polygon_cache_init(cache, ALLEGRO_POLY_PRIM_LINE, vertices, color);

      for (i = 0; i <= vertex_count; ++i) {

         if (cache->size >= (ALLEGRO_VERTEX_CACHE_SIZE - 2))
            polygon_cache_flush(cache);

         vertex = VERTEX(i);

         cache->current->x     = vertex[0];
         cache->current->y     = vertex[1];
         cache->current->z     = 0.0f;
         cache->current->color = cache->color;

         ++cache->current;
         ++cache->size;
      }

      polygon_cache_flush(cache);
   }

# undef VERTEX
}

void al_draw_polygon(const float* vertices, int vertex_count, ALLEGRO_COLOR color, float thickness)
{
   POLYGON_VERTEX_CACHE cache;
   do_draw_polygon(&cache, vertices, sizeof(float) * 2, vertex_count, color, thickness);
}

void al_draw_filled_polygon(const float* vertices, int vertex_count, ALLEGRO_COLOR color)
{
   POLYGON_VERTEX_CACHE cache;

   polygon_cache_init(&cache, ALLEGRO_POLY_PRIM_TRIANGLE, vertices, color);

   al_triangulate_polygon(
      vertices, sizeof(float) * 2, vertex_count, &vertex_count, sizeof(int), 1,
      polygon_cache_push_triangle_callback, &cache);

   polygon_cache_flush(&cache);
}

void al_draw_polygon_with_holes(const float* vertices, int vertex_count, const int* holes, int hole_count, ALLEGRO_COLOR color, float thickness)
{
# define VERTEX(index)  ((const float*)(((uint8_t*)vertices) + (sizeof(float) * 2) * ((vertex_count + (index)) % vertex_count)))
# define HOLE(index)    (*((int*)(((uint8_t*)holes) + sizeof(int) * ((hole_count + index) % hole_count))))

   POLYGON_VERTEX_CACHE cache;
   int i;

   if (hole_count <= 0)
      return;

   do_draw_polygon(&cache, vertices, sizeof(float) * 2, HOLE(0), color, thickness);

   for (i = 1; i < hole_count; ++i)
      do_draw_polygon(&cache, VERTEX(HOLE(i) - 1), -(int)(sizeof(float) * 2), HOLE(i) - HOLE(i - 1), color, thickness);

# undef VERTEX
# undef HOLE
}

void al_draw_filled_polygon_with_holes(const float* vertices, int vertex_count, const int* holes, int hole_count, ALLEGRO_COLOR color)
{
   POLYGON_VERTEX_CACHE cache;

   polygon_cache_init(&cache, ALLEGRO_POLY_PRIM_TRIANGLE, vertices, color);

   al_triangulate_polygon(
      vertices, sizeof(float) * 2, vertex_count, holes, sizeof(int), hole_count,
      polygon_cache_push_triangle_callback, &cache);

   polygon_cache_flush(&cache);
}
