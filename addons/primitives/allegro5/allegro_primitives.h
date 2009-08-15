#ifndef ALLEGRO_PRIMITIVES_H
#define ALLEGRO_PRIMITIVES_H

#include <allegro5/allegro5.h>

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
#ifndef ALLEGRO_STATICLINK
#ifdef A5_PRIMITIVES_SRC
#define _A5_PRIM_DLL __declspec(dllexport)
#else
#define _A5_PRIM_DLL __declspec(dllimport)
#endif
#else
#define _A5_PRIM_DLL
#endif
#endif

#if defined ALLEGRO_MSVC
#define A5_PRIM_FUNC(type, name, args)      _A5_PRIM_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
#define A5_PRIM_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
#define A5_PRIM_FUNC(type, name, args)      extern _A5_PRIM_DLL type name args
#else
#define A5_PRIM_FUNC      AL_FUNC
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/* Enum: ALLEGRO_PRIM_TYPE
 */
enum ALLEGRO_PRIM_TYPE {
  ALLEGRO_PRIM_LINE_LIST,
  ALLEGRO_PRIM_LINE_STRIP,
  ALLEGRO_PRIM_LINE_LOOP,
  ALLEGRO_PRIM_TRIANGLE_LIST,
  ALLEGRO_PRIM_TRIANGLE_STRIP,
  ALLEGRO_PRIM_TRIANGLE_FAN,
  ALLEGRO_PRIM_NUM_TYPES
};

/* Enum: ALLEGRO_VERTEX_CACHE_SIZE
 */
#define ALLEGRO_VERTEX_CACHE_SIZE 256

/* Enum: ALLEGRO_PRIM_QUALITY
 */
#define ALLEGRO_PRIM_QUALITY 10

/* Type: ALLEGRO_PRIM_COLOR
 */
typedef struct ALLEGRO_PRIM_COLOR ALLEGRO_PRIM_COLOR;

struct ALLEGRO_PRIM_COLOR {
   uint32_t d3d_color;
   float r, g, b, a;
};

/* Type: ALLEGRO_VERTEX
 */
typedef struct ALLEGRO_VERTEX ALLEGRO_VERTEX;

struct ALLEGRO_VERTEX {
  float x, y;
  ALLEGRO_PRIM_COLOR color;
  float u, v;
};


/* Type: ALLEGRO_TRANSFORM
 */
typedef float ALLEGRO_TRANSFORM[4][4];
   
/*
* Primary Functions
*/
A5_PRIM_FUNC(int, al_draw_prim, (ALLEGRO_VERTEX* vtxs, ALLEGRO_BITMAP* texture, int start, int end, int type));
A5_PRIM_FUNC(int, al_draw_indexed_prim, (ALLEGRO_VERTEX* vtxs, ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type));


A5_PRIM_FUNC(ALLEGRO_COLOR, al_get_allegro_color, (ALLEGRO_PRIM_COLOR col));
A5_PRIM_FUNC(ALLEGRO_PRIM_COLOR, al_get_prim_color, (ALLEGRO_COLOR col));

/*
* Custom primitives
*/
A5_PRIM_FUNC(void, al_draw_soft_triangle, (ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3, uintptr_t state,
                                           void (*init)(uintptr_t, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
                                           void (*first)(uintptr_t, int, int, int, int),
                                           void (*step)(uintptr_t, int), 
                                           void (*draw)(uintptr_t, int, int, int)));
A5_PRIM_FUNC(void, al_draw_soft_line, (ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, uintptr_t state,
                                       void (*first)(uintptr_t, int, int, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
                                       void (*step)(uintptr_t, int), 
                                       void (*draw)(uintptr_t, int, int)));


/*
* Transformations
*/
A5_PRIM_FUNC(void, al_use_transform, (ALLEGRO_TRANSFORM* trans));
A5_PRIM_FUNC(void, al_copy_transform, (ALLEGRO_TRANSFORM* src, ALLEGRO_TRANSFORM* dest));
A5_PRIM_FUNC(void, al_identity_transform, (ALLEGRO_TRANSFORM* trans));
A5_PRIM_FUNC(void, al_build_transform, (ALLEGRO_TRANSFORM* trans, float x, float y, float sx, float sy, float theta));
A5_PRIM_FUNC(void, al_translate_transform, (ALLEGRO_TRANSFORM* trans, float x, float y));
A5_PRIM_FUNC(void, al_rotate_transform, (ALLEGRO_TRANSFORM* trans, float theta));
A5_PRIM_FUNC(void, al_scale_transform, (ALLEGRO_TRANSFORM* trans, float sx, float sy));
A5_PRIM_FUNC(void, al_transform_vertex, (ALLEGRO_TRANSFORM* trans, ALLEGRO_VERTEX* vtx));
A5_PRIM_FUNC(void, al_transform_transform, (ALLEGRO_TRANSFORM* trans, ALLEGRO_TRANSFORM* trans2));


/*
*High level primitives
*/
A5_PRIM_FUNC(void, al_draw_line, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_rectangle, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_rounded_rectangle, (float x1, float y1, float x2, float y2, float rx, float ry, ALLEGRO_COLOR color, float thickness));

A5_PRIM_FUNC(void, al_calculate_arc, (float* dest, int stride, float cx, float cy, float rx, float ry, float start_theta, float delta_theta, float thickness, int num_segments));
A5_PRIM_FUNC(void, al_draw_circle, (float cx, float cy, float r, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_arc, (float cx, float cy, float r, float start_theta, float delta_theta, ALLEGRO_COLOR color, float thickness));

A5_PRIM_FUNC(void, al_calculate_spline, (float* dest, int stride, float points[8], float thickness, int num_segments));
A5_PRIM_FUNC(void, al_draw_spline, (float points[8], ALLEGRO_COLOR color, float thickness));

A5_PRIM_FUNC(void, al_calculate_ribbon, (float* dest, int dest_stride, const float *points, int points_stride, float thickness, int num_segments));
A5_PRIM_FUNC(void, al_draw_ribbon, (const float *points, int points_stride, ALLEGRO_COLOR color, float thickness, int num_segments));

A5_PRIM_FUNC(void, al_draw_filled_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_rectangle, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_circle, (float cx, float cy, float r, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_rounded_rectangle, (float x1, float y1, float x2, float y2, float rx, float ry, ALLEGRO_COLOR color));
   
   
#ifdef __cplusplus
}
#endif

#endif
