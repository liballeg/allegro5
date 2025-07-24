#ifndef __al_included_allegro5_aintern_prim_addon_h
#define __al_included_allegro5_aintern_prim_addon_h

#include <allegro5/internal/aintern_primitives_types.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ALLEGRO_PRIM_VERTEX_CACHE_TYPE
{
   ALLEGRO_PRIM_VERTEX_CACHE_TRIANGLE,
   ALLEGRO_PRIM_VERTEX_CACHE_LINE_STRIP
};

typedef struct ALLEGRO_PRIM_VERTEX_CACHE {
   ALLEGRO_VERTEX  buffer[ALLEGRO_VERTEX_CACHE_SIZE];
   ALLEGRO_VERTEX* current;
   size_t          size;
   ALLEGRO_COLOR   color;
   int             prim_type;
   void*           user_data;
} ALLEGRO_PRIM_VERTEX_CACHE;

/* Internal cache for primitives. */
void _al_prim_cache_init(ALLEGRO_PRIM_VERTEX_CACHE* cache, int prim_type, ALLEGRO_COLOR color);
void _al_prim_cache_init_ex(ALLEGRO_PRIM_VERTEX_CACHE* cache, int prim_type, ALLEGRO_COLOR color, void* user_data);
void _al_prim_cache_term(ALLEGRO_PRIM_VERTEX_CACHE* cache);
void _al_prim_cache_flush(ALLEGRO_PRIM_VERTEX_CACHE* cache);
void _al_prim_cache_push_point(ALLEGRO_PRIM_VERTEX_CACHE* cache, const float* v);
void _al_prim_cache_push_triangle(ALLEGRO_PRIM_VERTEX_CACHE* cache, const float* v0, const float* v1, const float* v2);


/* Internal functions. */
float     _al_prim_get_scale(void);
float     _al_prim_normalize(float* vector);
int       _al_prim_test_line_side(const float* origin, const float* normal, const float* point);
bool      _al_prim_is_point_in_triangle(const float* point, const float* v0, const float* v1, const float* v2);
bool      _al_prim_intersect_segment(const float* v0, const float* v1, const float* p0, const float* p1, float* point, float* t0, float* t1);
bool      _al_prim_are_points_equal(const float* point_a, const float* point_b);

#ifdef __cplusplus
}
#endif

#endif
