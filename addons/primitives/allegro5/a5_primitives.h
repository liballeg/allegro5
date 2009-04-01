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

/* Enum: ALLEGRO_VBUFFER_FLAGS
 */
enum ALLEGRO_VBUFFER_FLAGS {
  ALLEGRO_VBUFFER_SOFT =   1 << 0,
  ALLEGRO_VBUFFER_VIDEO =  1 << 1,
  ALLEGRO_VBUFFER_LOCKED = 1 << 2,
  ALLEGRO_VBUFFER_READ =   1 << 3,
  ALLEGRO_VBUFFER_WRITE =  1 << 4
};

/* Enum: ALLEGRO_VERTEX_CACHE_SIZE
 */
#define ALLEGRO_VERTEX_CACHE_SIZE 256

/* Enum: ALLEGRO_VBUFF_CACHE_SIZE
 */
#define ALLEGRO_VBUFF_CACHE_SIZE 256

/* Enum: ALLEGRO_PRIM_QUALITY
 */
#define ALLEGRO_PRIM_QUALITY 10

/* Type: ALLEGRO_VBUFFER
 */
typedef struct ALLEGRO_VBUFFER ALLEGRO_VBUFFER;

/* Type: ALLEGRO_VERTEX
 */
typedef struct {
  float x, y, z;
  float nx, ny, nz;
  uint32_t d3d_color;
  float u, v;
  float r, g, b, a;
} ALLEGRO_VERTEX;


/* Type: ALLEGRO_TRANSFORM
 */
typedef float ALLEGRO_TRANSFORM[4][4];
   
/*
* Primary Functions
*/
A5_PRIM_FUNC(int, al_draw_prim, (ALLEGRO_VBUFFER* vbuff, ALLEGRO_BITMAP* texture, int start, int end, int type));
A5_PRIM_FUNC(int, al_draw_indexed_prim, (ALLEGRO_VBUFFER* vbuff, ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type));

/*
* VBuff stuff
*/
A5_PRIM_FUNC(ALLEGRO_VBUFFER*, al_create_vbuff, (int len, int type));
A5_PRIM_FUNC(void, al_destroy_vbuff, (ALLEGRO_VBUFFER* vbuff));

A5_PRIM_FUNC(void, al_unlock_vbuff, (ALLEGRO_VBUFFER* vbuff));
A5_PRIM_FUNC(int, al_lock_vbuff, (ALLEGRO_VBUFFER* vbuff, int type));
A5_PRIM_FUNC(int, al_lock_vbuff_range, (ALLEGRO_VBUFFER* vbuff, int start, int end, int type));
A5_PRIM_FUNC(int, al_vbuff_is_locked, (ALLEGRO_VBUFFER* vbuff));
A5_PRIM_FUNC(int, al_vbuff_range_is_locked, (ALLEGRO_VBUFFER* vbuff, int start, int end));
A5_PRIM_FUNC(void, al_set_vbuff_pos, (ALLEGRO_VBUFFER* vbuff, int idx, float x, float y, float z));
A5_PRIM_FUNC(void, al_set_vbuff_normal, (ALLEGRO_VBUFFER* vbuff, int idx, float nx, float ny, float nz));
A5_PRIM_FUNC(void, al_set_vbuff_uv, (ALLEGRO_VBUFFER* vbuff, int idx, float u, float v));
A5_PRIM_FUNC(void, al_set_vbuff_color, (ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_COLOR col));

A5_PRIM_FUNC(void, al_set_vbuff_vertex, (ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_VERTEX *vtx));
A5_PRIM_FUNC(void, al_get_vbuff_vertex, (ALLEGRO_VBUFFER* vbuff, int idx, ALLEGRO_VERTEX *vtx));

A5_PRIM_FUNC(int, al_get_vbuff_flags, (ALLEGRO_VBUFFER* vbuff));
A5_PRIM_FUNC(int, al_get_vbuff_len, (ALLEGRO_VBUFFER* vbuff));
A5_PRIM_FUNC(void, al_get_vbuff_lock_range, (ALLEGRO_VBUFFER* vbuff, int* start, int* end));

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

A5_PRIM_FUNC(void, al_calculate_arc, (ALLEGRO_VBUFFER* vbuff, float cx, float cy, float rx, float ry, float theta, float delta_theta, float thickness, int start, int num_segments));
A5_PRIM_FUNC(void, al_draw_circle, (float cx, float cy, float r, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness));
A5_PRIM_FUNC(void, al_draw_arc, (float cx, float cy, float r, float start_theta, float delta_theta, ALLEGRO_COLOR color, float thickness));

A5_PRIM_FUNC(void, al_calculate_spline, (ALLEGRO_VBUFFER* vbuff, float points[8], float thickness, int start, int num_points));
A5_PRIM_FUNC(void, al_draw_spline, (float points[8], ALLEGRO_COLOR color, float thickness));

A5_PRIM_FUNC(void, al_calculate_ribbon, (ALLEGRO_VBUFFER* vbuff, const float *points, float thickness, int start, int num_segments));
A5_PRIM_FUNC(void, al_draw_ribbon, (const float *points, ALLEGRO_COLOR color, float thickness, int num_segments));

A5_PRIM_FUNC(void, al_draw_filled_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_rectangle, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color));
A5_PRIM_FUNC(void, al_draw_filled_circle, (float cx, float cy, float r, ALLEGRO_COLOR color));
   
   
#ifdef __cplusplus
}
#endif

#endif
