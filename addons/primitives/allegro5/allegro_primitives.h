#ifndef __al_included_allegro5_allegro_primitives_h
#define __al_included_allegro5_allegro_primitives_h

#include <allegro5/allegro.h>

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
#ifndef ALLEGRO_STATICLINK
#ifdef ALLEGRO_PRIMITIVES_SRC
#define _ALLEGRO_PRIM_DLL __declspec(dllexport)
#else
#define _ALLEGRO_PRIM_DLL __declspec(dllimport)
#endif
#else
#define _ALLEGRO_PRIM_DLL
#endif
#endif

#if defined ALLEGRO_MSVC
#define ALLEGRO_PRIM_FUNC(type, name, args)      _ALLEGRO_PRIM_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
#define ALLEGRO_PRIM_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
#define ALLEGRO_PRIM_FUNC(type, name, args)      extern _ALLEGRO_PRIM_DLL type name args
#else
#define ALLEGRO_PRIM_FUNC      AL_FUNC
#endif

#ifdef __cplusplus
extern "C"
{
#endif


/* Enum: ALLEGRO_PRIM_TYPE
 */
typedef enum ALLEGRO_PRIM_TYPE
{
  ALLEGRO_PRIM_LINE_LIST,
  ALLEGRO_PRIM_LINE_STRIP,
  ALLEGRO_PRIM_LINE_LOOP,
  ALLEGRO_PRIM_TRIANGLE_LIST,
  ALLEGRO_PRIM_TRIANGLE_STRIP,
  ALLEGRO_PRIM_TRIANGLE_FAN,
  ALLEGRO_PRIM_POINT_LIST,
  ALLEGRO_PRIM_NUM_TYPES
} ALLEGRO_PRIM_TYPE;

enum
{
   ALLEGRO_PRIM_MAX_USER_ATTR = _ALLEGRO_PRIM_MAX_USER_ATTR
};

/* Enum: ALLEGRO_PRIM_ATTR
 */
typedef enum ALLEGRO_PRIM_ATTR
{
   ALLEGRO_PRIM_POSITION = 1,
   ALLEGRO_PRIM_COLOR_ATTR,
   ALLEGRO_PRIM_TEX_COORD,
   ALLEGRO_PRIM_TEX_COORD_PIXEL,
   ALLEGRO_PRIM_USER_ATTR,
   ALLEGRO_PRIM_ATTR_NUM = ALLEGRO_PRIM_USER_ATTR + ALLEGRO_PRIM_MAX_USER_ATTR
} ALLEGRO_PRIM_ATTR;

/* Enum: ALLEGRO_PRIM_STORAGE
 */
typedef enum ALLEGRO_PRIM_STORAGE
{
   ALLEGRO_PRIM_FLOAT_2,
   ALLEGRO_PRIM_FLOAT_3,
   ALLEGRO_PRIM_SHORT_2,
   ALLEGRO_PRIM_FLOAT_1,
   ALLEGRO_PRIM_FLOAT_4,
   ALLEGRO_PRIM_UBYTE_4,
   ALLEGRO_PRIM_SHORT_4,
   ALLEGRO_PRIM_NORMALIZED_UBYTE_4,
   ALLEGRO_PRIM_NORMALIZED_SHORT_2,
   ALLEGRO_PRIM_NORMALIZED_SHORT_4,
   ALLEGRO_PRIM_NORMALIZED_USHORT_2,
   ALLEGRO_PRIM_NORMALIZED_USHORT_4,
   ALLEGRO_PRIM_HALF_FLOAT_2,
   ALLEGRO_PRIM_HALF_FLOAT_4
} ALLEGRO_PRIM_STORAGE;

/* Enum: ALLEGRO_LINE_JOIN
 */
typedef enum ALLEGRO_LINE_JOIN
{
   ALLEGRO_LINE_JOIN_NONE,
   ALLEGRO_LINE_JOIN_BEVEL,
   ALLEGRO_LINE_JOIN_ROUND,
   ALLEGRO_LINE_JOIN_MITER,
   ALLEGRO_LINE_JOIN_MITRE = ALLEGRO_LINE_JOIN_MITER
} ALLEGRO_LINE_JOIN;

/* Enum: ALLEGRO_LINE_CAP
 */
typedef enum ALLEGRO_LINE_CAP
{
   ALLEGRO_LINE_CAP_NONE,
   ALLEGRO_LINE_CAP_SQUARE,
   ALLEGRO_LINE_CAP_ROUND,
   ALLEGRO_LINE_CAP_TRIANGLE,
   ALLEGRO_LINE_CAP_CLOSED
} ALLEGRO_LINE_CAP;

/* Enum: ALLEGRO_PRIM_BUFFER_FLAGS
 */
typedef enum ALLEGRO_PRIM_BUFFER_FLAGS
{
   ALLEGRO_PRIM_BUFFER_STREAM       = 0x01,
   ALLEGRO_PRIM_BUFFER_STATIC       = 0x02,
   ALLEGRO_PRIM_BUFFER_DYNAMIC      = 0x04,
   ALLEGRO_PRIM_BUFFER_READWRITE    = 0x08
} ALLEGRO_PRIM_BUFFER_FLAGS;

/* Enum: ALLEGRO_VERTEX_CACHE_SIZE
 */
#define ALLEGRO_VERTEX_CACHE_SIZE 256

/* Enum: ALLEGRO_PRIM_QUALITY
 */
#define ALLEGRO_PRIM_QUALITY 10

/* Type: ALLEGRO_VERTEX_ELEMENT
 */
typedef struct ALLEGRO_VERTEX_ELEMENT ALLEGRO_VERTEX_ELEMENT;

struct ALLEGRO_VERTEX_ELEMENT {
   int attribute;
   int storage;
   int offset;
};

/* Type: ALLEGRO_VERTEX_DECL
 */
typedef struct ALLEGRO_VERTEX_DECL ALLEGRO_VERTEX_DECL;

/* Duplicated in allegro5/internal/aintern_tri_soft.h */
#ifndef _ALLEGRO_VERTEX_DEFINED
#define _ALLEGRO_VERTEX_DEFINED

/* Type: ALLEGRO_VERTEX
 */
typedef struct ALLEGRO_VERTEX ALLEGRO_VERTEX;

struct ALLEGRO_VERTEX {
  float x, y, z;
  float u, v;
  ALLEGRO_COLOR color;
};
#endif

/* Type: ALLEGRO_VERTEX_BUFFER
 */
typedef struct ALLEGRO_VERTEX_BUFFER ALLEGRO_VERTEX_BUFFER;

/* Type: ALLEGRO_INDEX_BUFFER
 */
typedef struct ALLEGRO_INDEX_BUFFER ALLEGRO_INDEX_BUFFER;

ALLEGRO_PRIM_FUNC(uint32_t, al_get_allegro_primitives_version, (void));

/*
* Primary Functions
*/
ALLEGRO_PRIM_FUNC(bool, al_init_primitives_addon, (void));
ALLEGRO_PRIM_FUNC(bool, al_is_primitives_addon_initialized, (void));
ALLEGRO_PRIM_FUNC(void, al_shutdown_primitives_addon, (void));
ALLEGRO_PRIM_FUNC(int, al_draw_prim, (const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture, int start, int end, int type));
ALLEGRO_PRIM_FUNC(int, al_draw_indexed_prim, (const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type));
ALLEGRO_PRIM_FUNC(int, al_draw_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, int start, int end, int type));
ALLEGRO_PRIM_FUNC(int, al_draw_indexed_buffer, (ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type));

ALLEGRO_PRIM_FUNC(ALLEGRO_VERTEX_DECL*, al_create_vertex_decl, (const ALLEGRO_VERTEX_ELEMENT* elements, int stride));
ALLEGRO_PRIM_FUNC(void, al_destroy_vertex_decl, (ALLEGRO_VERTEX_DECL* decl));

/*
 * Vertex buffers
 */
ALLEGRO_PRIM_FUNC(ALLEGRO_VERTEX_BUFFER*, al_create_vertex_buffer, (ALLEGRO_VERTEX_DECL* decl, const void* initial_data, int num_vertices, int flags));
ALLEGRO_PRIM_FUNC(void, al_destroy_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* buffer));
ALLEGRO_PRIM_FUNC(void*, al_lock_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* buffer, int offset, int length, int flags));
ALLEGRO_PRIM_FUNC(void, al_unlock_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* buffer));
ALLEGRO_PRIM_FUNC(int, al_get_vertex_buffer_size, (ALLEGRO_VERTEX_BUFFER* buffer));

/*
 * Index buffers
 */
ALLEGRO_PRIM_FUNC(ALLEGRO_INDEX_BUFFER*, al_create_index_buffer, (int index_size, const void* initial_data, int num_indices, int flags));
ALLEGRO_PRIM_FUNC(void, al_destroy_index_buffer, (ALLEGRO_INDEX_BUFFER* buffer));
ALLEGRO_PRIM_FUNC(void*, al_lock_index_buffer, (ALLEGRO_INDEX_BUFFER* buffer, int offset, int length, int flags));
ALLEGRO_PRIM_FUNC(void, al_unlock_index_buffer, (ALLEGRO_INDEX_BUFFER* buffer));
ALLEGRO_PRIM_FUNC(int, al_get_index_buffer_size, (ALLEGRO_INDEX_BUFFER* buffer));

/*
* Utilities for high level primitives.
*/
ALLEGRO_PRIM_FUNC(bool, al_triangulate_polygon, (const float* vertices, size_t vertex_stride, const int* vertex_counts, void (*emit_triangle)(int, int, int, void*), void* userdata));


/*
* Custom primitives
*/
ALLEGRO_PRIM_FUNC(void, al_draw_soft_triangle, (ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3, uintptr_t state,
                                           void (*init)(uintptr_t, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
                                           void (*first)(uintptr_t, int, int, int, int),
                                           void (*step)(uintptr_t, int),
                                           void (*draw)(uintptr_t, int, int, int)));
ALLEGRO_PRIM_FUNC(void, al_draw_soft_line, (ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, uintptr_t state,
                                       void (*first)(uintptr_t, int, int, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
                                       void (*step)(uintptr_t, int),
                                       void (*draw)(uintptr_t, int, int)));

/*
*High level primitives
*/
ALLEGRO_PRIM_FUNC(void, al_draw_line, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_rectangle, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_rounded_rectangle, (float x1, float y1, float x2, float y2, float rx, float ry, ALLEGRO_COLOR color, float thickness));

ALLEGRO_PRIM_FUNC(void, al_calculate_arc, (float* dest, int stride, float cx, float cy, float rx, float ry, float start_theta, float delta_theta, float thickness, int num_points));
ALLEGRO_PRIM_FUNC(void, al_draw_circle, (float cx, float cy, float r, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_arc, (float cx, float cy, float r, float start_theta, float delta_theta, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_elliptical_arc, (float cx, float cy, float rx, float ry, float start_theta, float delta_theta, ALLEGRO_COLOR color, float thickness));
ALLEGRO_PRIM_FUNC(void, al_draw_pieslice, (float cx, float cy, float r, float start_theta, float delta_theta, ALLEGRO_COLOR color, float thickness));

ALLEGRO_PRIM_FUNC(void, al_calculate_spline, (float* dest, int stride, const float points[8], float thickness, int num_segments));
ALLEGRO_PRIM_FUNC(void, al_draw_spline, (const float points[8], ALLEGRO_COLOR color, float thickness));

ALLEGRO_PRIM_FUNC(void, al_calculate_ribbon, (float* dest, int dest_stride, const float *points, int points_stride, float thickness, int num_segments));
ALLEGRO_PRIM_FUNC(void, al_draw_ribbon, (const float *points, int points_stride, ALLEGRO_COLOR color, float thickness, int num_segments));

ALLEGRO_PRIM_FUNC(void, al_draw_filled_triangle, (float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_rectangle, (float x1, float y1, float x2, float y2, ALLEGRO_COLOR color));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_ellipse, (float cx, float cy, float rx, float ry, ALLEGRO_COLOR color));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_circle, (float cx, float cy, float r, ALLEGRO_COLOR color));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_pieslice, (float cx, float cy, float r, float start_theta, float delta_theta, ALLEGRO_COLOR color));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_rounded_rectangle, (float x1, float y1, float x2, float y2, float rx, float ry, ALLEGRO_COLOR color));

ALLEGRO_PRIM_FUNC(void, al_draw_polyline, (const float* vertices, int vertex_stride, int vertex_count, int join_style, int cap_style, ALLEGRO_COLOR color, float thickness, float miter_limit));

ALLEGRO_PRIM_FUNC(void, al_draw_polygon, (const float* vertices, int vertex_count, int join_style, ALLEGRO_COLOR color, float thickness, float miter_limit));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_polygon, (const float* vertices, int vertex_count, ALLEGRO_COLOR color));
ALLEGRO_PRIM_FUNC(void, al_draw_filled_polygon_with_holes, (const float* vertices, const int* vertex_counts, ALLEGRO_COLOR color));


#ifdef __cplusplus
}
#endif

#endif
