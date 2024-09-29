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

#ifdef A5O_MSVC
   #define hypotf(x, y) _hypotf((x), (y))
#endif

static void polygon_push_triangle_callback(int i0, int i1, int i2, void* user_data)
{
   A5O_PRIM_VERTEX_CACHE* cache = (A5O_PRIM_VERTEX_CACHE*)user_data;

   const float* vertices = (const float*)cache->user_data;

   const float* v0 = vertices + (i0 * 2);
   const float* v1 = vertices + (i1 * 2);
   const float* v2 = vertices + (i2 * 2);

   _al_prim_cache_push_triangle(cache, v0, v1, v2);
}


/* Function: al_draw_polygon
 */
void al_draw_polygon(const float *vertices, int vertex_count,
   int join_style, A5O_COLOR color, float thickness, float miter_limit)
{
   al_draw_polyline(vertices, 2 * sizeof(float), vertex_count, join_style,
      A5O_LINE_CAP_CLOSED, color, thickness, miter_limit);
}

/* Function: al_draw_filled_polygon
 */
void al_draw_filled_polygon(const float *vertices, int vertex_count,
   A5O_COLOR color)
{
   A5O_PRIM_VERTEX_CACHE cache;
   int vertex_counts[2];

   _al_prim_cache_init_ex(&cache, A5O_PRIM_VERTEX_CACHE_TRIANGLE, color, (void*)vertices);

   vertex_counts[0] = vertex_count;
   vertex_counts[1] = 0; /* terminator */
   al_triangulate_polygon(vertices, sizeof(float) * 2, vertex_counts,
      polygon_push_triangle_callback, &cache);

   _al_prim_cache_flush(&cache);
}

/* Function: al_draw_filled_polygon_with_holes
 */
void al_draw_filled_polygon_with_holes(const float *vertices,
   const int *vertex_counts, A5O_COLOR color)
{
   A5O_PRIM_VERTEX_CACHE cache;

   _al_prim_cache_init_ex(&cache, A5O_PRIM_VERTEX_CACHE_TRIANGLE, color, (void*)vertices);

   al_triangulate_polygon(vertices, sizeof(float) * 2, vertex_counts,
      polygon_push_triangle_callback, &cache);

   _al_prim_cache_flush(&cache);
}

/* vim: set sts=3 sw=3 et: */
