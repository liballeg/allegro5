#ifndef __al_included_allegro5_allegro_primitives_h
#define __al_included_allegro5_allegro_primitives_h

#include <allegro5/allegro.h>

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
#ifndef A5O_STATICLINK
#ifdef A5O_PRIMITIVES_SRC
#define _A5O_PRIM_DLL __declspec(dllexport)
#else
#define _A5O_PRIM_DLL __declspec(dllimport)
#endif
#else
#define _A5O_PRIM_DLL
#endif
#endif

#if defined A5O_MSVC
#define A5O_PRIM_FUNC(type, name, args)      _A5O_PRIM_DLL type __cdecl name args
#elif defined A5O_MINGW32
#define A5O_PRIM_FUNC(type, name, args)      extern type name args
#elif defined A5O_BCC32
#define A5O_PRIM_FUNC(type, name, args)      extern _A5O_PRIM_DLL type name args
#else
#define A5O_PRIM_FUNC      AL_FUNC
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/* Enum: A5O_PRIM_TYPE
 */
typedef enum A5O_PRIM_TYPE
{
  A5O_PRIM_LINE_LIST,
  A5O_PRIM_LINE_STRIP,
  A5O_PRIM_LINE_LOOP,
  A5O_PRIM_TRIANGLE_LIST,
  A5O_PRIM_TRIANGLE_STRIP,
  A5O_PRIM_TRIANGLE_FAN,
  A5O_PRIM_POINT_LIST,
  A5O_PRIM_NUM_TYPES
} A5O_PRIM_TYPE;

enum
{
   A5O_PRIM_MAX_USER_ATTR = _A5O_PRIM_MAX_USER_ATTR
};

/* Enum: A5O_PRIM_ATTR
 */
typedef enum A5O_PRIM_ATTR
{
   A5O_PRIM_POSITION = 1,
   A5O_PRIM_COLOR_ATTR,
   A5O_PRIM_TEX_COORD,
   A5O_PRIM_TEX_COORD_PIXEL,
   A5O_PRIM_USER_ATTR,
   A5O_PRIM_ATTR_NUM = A5O_PRIM_USER_ATTR + A5O_PRIM_MAX_USER_ATTR
} A5O_PRIM_ATTR;

/* Enum: A5O_PRIM_STORAGE
 */
typedef enum A5O_PRIM_STORAGE
{
   A5O_PRIM_FLOAT_2,
   A5O_PRIM_FLOAT_3,
   A5O_PRIM_SHORT_2,
   A5O_PRIM_FLOAT_1,
   A5O_PRIM_FLOAT_4,
   A5O_PRIM_UBYTE_4,
   A5O_PRIM_SHORT_4,
   A5O_PRIM_NORMALIZED_UBYTE_4,
   A5O_PRIM_NORMALIZED_SHORT_2,
   A5O_PRIM_NORMALIZED_SHORT_4,
   A5O_PRIM_NORMALIZED_USHORT_2,
   A5O_PRIM_NORMALIZED_USHORT_4,
   A5O_PRIM_HALF_FLOAT_2,
   A5O_PRIM_HALF_FLOAT_4
} A5O_PRIM_STORAGE;

/* Enum: A5O_LINE_JOIN
 */
typedef enum A5O_LINE_JOIN
{
   A5O_LINE_JOIN_NONE,
   A5O_LINE_JOIN_BEVEL,
   A5O_LINE_JOIN_ROUND,
   A5O_LINE_JOIN_MITER,
   A5O_LINE_JOIN_MITRE = A5O_LINE_JOIN_MITER
} A5O_LINE_JOIN;

/* Enum: A5O_LINE_CAP
 */
typedef enum A5O_LINE_CAP
{
   A5O_LINE_CAP_NONE,
   A5O_LINE_CAP_SQUARE,
   A5O_LINE_CAP_ROUND,
   A5O_LINE_CAP_TRIANGLE,
   A5O_LINE_CAP_CLOSED
} A5O_LINE_CAP;

/* Enum: A5O_PRIM_BUFFER_FLAGS
 */
typedef enum A5O_PRIM_BUFFER_FLAGS
{
   A5O_PRIM_BUFFER_STREAM       = 0x01,
   A5O_PRIM_BUFFER_STATIC       = 0x02,
   A5O_PRIM_BUFFER_DYNAMIC      = 0x04,
   A5O_PRIM_BUFFER_READWRITE    = 0x08
} A5O_PRIM_BUFFER_FLAGS;

/* Enum: A5O_VERTEX_CACHE_SIZE
 */
#define A5O_VERTEX_CACHE_SIZE 256

/* Enum: A5O_PRIM_QUALITY
 */
#define A5O_PRIM_QUALITY 10

/* Type: A5O_VERTEX_ELEMENT
 */
typedef struct A5O_VERTEX_ELEMENT A5O_VERTEX_ELEMENT;

struct A5O_VERTEX_ELEMENT {
   int attribute;
   int storage;
   int offset;
};

/* Type: A5O_VERTEX_DECL
 */
typedef struct A5O_VERTEX_DECL A5O_VERTEX_DECL;

/* Duplicated in allegro5/internal/aintern_tri_soft.h */
#ifndef _A5O_VERTEX_DEFINED
#define _A5O_VERTEX_DEFINED

/* Type: A5O_VERTEX
 */
typedef struct A5O_VERTEX A5O_VERTEX;

struct A5O_VERTEX {
  float x, y, z;
  float u, v;
  A5O_COLOR color;
};
#endif

/* Type: A5O_VERTEX_BUFFER
 */
typedef struct A5O_VERTEX_BUFFER A5O_VERTEX_BUFFER;

/* Type: A5O_INDEX_BUFFER
 */
typedef struct A5O_INDEX_BUFFER A5O_INDEX_BUFFER;

A5O_PRIM_FUNC(uint32_t, al_get_allegro_primitives_version, (void));

/*
* Primary Functions
*/
A5O_PRIM_FUNC(bool, al_init_primitives_addon, (void));
A5O_PRIM_FUNC(bool, al_is_primitives_addon_initialized, (void));
A5O_PRIM_FUNC(void, al_shutdown_primitives_addon, (void));
A5O_PRIM_FUNC(int, al_draw_prim, (const void* vtxs, const A5O_VERTEX_DECL* decl, A5O_BITMAP* texture, int start, int end, int type));
A5O_PRIM_FUNC(int, al_draw_indexed_prim, (const void* vtxs, const A5O_VERTEX_DECL* decl, A5O_BITMAP* texture, const int* indices, int num_vtx, int type));
A5O_PRIM_FUNC(int, al_draw_vertex_buffer, (A5O_VERTEX_BUFFER* vertex_buffer, A5O_BITMAP* texture, int start, int end, int type));
A5O_PRIM_FUNC(int, al_draw_indexed_buffer, (A5O_VERTEX_BUFFER* vertex_buffer, A5O_BITMAP* texture, A5O_INDEX_BUFFER* index_buffer, int start, int end, int type));

A5O_PRIM_FUNC(A5O_VERTEX_DECL*, al_create_vertex_decl, (const A5O_VERTEX_ELEMENT* elements, int stride));
A5O_PRIM_FUNC(void, al_destroy_vertex_decl, (A5O_VERTEX_DECL* decl));

/*
 * Vertex buffers
 */
A5O_PRIM_FUNC(A5O_VERTEX_BUFFER*, al_create_vertex_buffer, (A5O_VERTEX_DECL* decl, const void* initial_data, int num_vertices, int flags));
A5O_PRIM_FUNC(void, al_destroy_vertex_buffer, (A5O_VERTEX_BUFFER* buffer));
A5O_PRIM_FUNC(void*, al_lock_vertex_buffer, (A5O_VERTEX_BUFFER* buffer, int offset, int length, int flags));
A5O_PRIM_FUNC(void, al_unlock_vertex_buffer, (A5O_VERTEX_BUFFER* buffer));
A5O_PRIM_FUNC(int, al_get_vertex_buffer_size, (A5O_VERTEX_BUFFER* buffer));

/*
 * Index buffers
 */
A5O_PRIM_FUNC(A5O_INDEX_BUFFER*, al_create_index_buffer, (int index_size, const void* initial_data, int num_indices, int flags));
A5O_PRIM_FUNC(void, al_destroy_index_buffer, (A5O_INDEX_BUFFER* buffer));
A5O_PRIM_FUNC(void*, al_lock_index_buffer, (A5O_INDEX_BUFFER* buffer, int offset, int length, int flags));
A5O_PRIM_FUNC(void, al_unlock_index_buffer, (A5O_INDEX_BUFFER* buffer));
A5O_PRIM_FUNC(int, al_get_index_buffer_size, (A5O_INDEX_BUFFER* buffer));

/*
* Utilities for high level primitives.
*/
A5O_PRIM_FUNC(bool, al_triangulate_polygon, (const float* vertices, size_t vertex_stride, const int* vertex_counts, void (*emit_triangle)(int, int, int, void*), void* userdata));


/*
* Custom primitives
*/
A5O_PRIM_FUNC(void, al_draw_soft_triangle, (A5O_VERTEX* v1, A5O_VERTEX* v2, A5O_VERTEX* v3, uintptr_t state,
                                           void (*init)(uintptr_t, A5O_VERTEX*, A5O_VERTEX*, A5O_VERTEX*),
                                           void (*first)(uintptr_t, int, int, int, int),
                                           void (*step)(uintptr_t, int),
                                           void (*draw)(uintptr_t, int, int, int)));
A5O_PRIM_FUNC(void, al_draw_soft_line, (A5O_VERTEX* v1, A5O_VERTEX* v2, uintptr_t state,
                                       void (*first)(uintptr_t, int, int, A5O_VERTEX*, A5O_VERTEX*),
                                       void (*step)(uintptr_t, int),
                                       void (*draw)(uintptr_t, int, int)));

/*
*High level primitives
*/
A5O_PRIM_FUNC(void, al_draw_line, (float x1, float y1, float x2, float y2, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_rectangle, (float x1, float y1, float x2, float y2, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_rounded_rectangle, (float x1, float y1, float x2, float y2, float rx, float ry, A5O_COLOR color, float thickness));

A5O_PRIM_FUNC(void, al_calculate_arc, (float* dest, int stride, float cx, float cy, float rx, float ry, float start_theta, float delta_theta, float thickness, int num_points));
A5O_PRIM_FUNC(void, al_draw_circle, (float cx, float cy, float r, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_ellipse, (float cx, float cy, float rx, float ry, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_arc, (float cx, float cy, float r, float start_theta, float delta_theta, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_elliptical_arc, (float cx, float cy, float rx, float ry, float start_theta, float delta_theta, A5O_COLOR color, float thickness));
A5O_PRIM_FUNC(void, al_draw_pieslice, (float cx, float cy, float r, float start_theta, float delta_theta, A5O_COLOR color, float thickness));

A5O_PRIM_FUNC(void, al_calculate_spline, (float* dest, int stride, const float points[8], float thickness, int num_segments));
A5O_PRIM_FUNC(void, al_draw_spline, (const float points[8], A5O_COLOR color, float thickness));

A5O_PRIM_FUNC(void, al_calculate_ribbon, (float* dest, int dest_stride, const float *points, int points_stride, float thickness, int num_segments));
A5O_PRIM_FUNC(void, al_draw_ribbon, (const float *points, int points_stride, A5O_COLOR color, float thickness, int num_segments));

A5O_PRIM_FUNC(void, al_draw_filled_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, A5O_COLOR color));
A5O_PRIM_FUNC(void, al_draw_filled_rectangle, (float x1, float y1, float x2, float y2, A5O_COLOR color));
A5O_PRIM_FUNC(void, al_draw_filled_ellipse, (float cx, float cy, float rx, float ry, A5O_COLOR color));
A5O_PRIM_FUNC(void, al_draw_filled_circle, (float cx, float cy, float r, A5O_COLOR color));
A5O_PRIM_FUNC(void, al_draw_filled_pieslice, (float cx, float cy, float r, float start_theta, float delta_theta, A5O_COLOR color));
A5O_PRIM_FUNC(void, al_draw_filled_rounded_rectangle, (float x1, float y1, float x2, float y2, float rx, float ry, A5O_COLOR color));

A5O_PRIM_FUNC(void, al_draw_polyline, (const float* vertices, int vertex_stride, int vertex_count, int join_style, int cap_style, A5O_COLOR color, float thickness, float miter_limit));

A5O_PRIM_FUNC(void, al_draw_polygon, (const float* vertices, int vertex_count, int join_style, A5O_COLOR color, float thickness, float miter_limit));
A5O_PRIM_FUNC(void, al_draw_filled_polygon, (const float* vertices, int vertex_count, A5O_COLOR color));
A5O_PRIM_FUNC(void, al_draw_filled_polygon_with_holes, (const float* vertices, const int* vertex_counts, A5O_COLOR color));


#ifdef __cplusplus
}
#endif

#endif
