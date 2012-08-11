#ifndef __al_included_allegro5_aintern_prim_h
#define __al_included_allegro5_aintern_prim_h

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int x2, int y2);

enum ALLEGRO_PRIM_VERTEX_CACHE_TYPE
{
   ALLEGRO_PRIM_VERTEX_CACHE_TRIANGLE,
   ALLEGRO_PRIM_VERTEX_CACHE_LINE_STRIP
};

struct ALLEGRO_VERTEX_DECL {
   ALLEGRO_VERTEX_ELEMENT* elements;
   int stride;
   void* d3d_decl;
   void* d3d_dummy_shader;
};

typedef struct ALLEGRO_PRIM_VERTEX_CACHE {
   ALLEGRO_VERTEX  buffer[ALLEGRO_VERTEX_CACHE_SIZE];
   ALLEGRO_VERTEX* current;
   size_t          size;
   ALLEGRO_COLOR   color;
   int             prim_type;
   void*           user_data;
} ALLEGRO_PRIM_VERTEX_CACHE;

struct ALLEGRO_VERTEX_BUFFER {
   ALLEGRO_VERTEX_DECL* decl;
   uintptr_t handle;
   bool write_only;

   bool is_locked;
   void* locked_memory;
   /* These two are in bytes */
   size_t local_buffer_length;
   size_t lock_offset;
   size_t lock_length;
   int lock_flags;
};

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
float     _al_prim_wrap_two_pi(float angle);
float     _al_prim_wrap_pi_to_pi(float angle);
float     _al_prim_get_angle(const float* p0, const float* origin, const float* p1);
int       _al_prim_test_line_side(const float* origin, const float* normal, const float* point);
bool      _al_prim_is_point_in_triangle(const float* point, const float* v0, const float* v1, const float* v2);
bool      _al_prim_intersect_segment(const float* v0, const float* v1, const float* p0, const float* p1, float* point, float* t0, float* t1);
bool      _al_prim_are_points_equal(const float* point_a, const float* point_b);

#endif
