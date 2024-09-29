#ifndef __al_included_allegro5_aintern_prim_h
#define __al_included_allegro5_aintern_prim_h

#ifdef __cplusplus
extern "C" {
#endif

enum A5O_PRIM_VERTEX_CACHE_TYPE
{
   A5O_PRIM_VERTEX_CACHE_TRIANGLE,
   A5O_PRIM_VERTEX_CACHE_LINE_STRIP
};

struct A5O_VERTEX_DECL {
   A5O_VERTEX_ELEMENT* elements;
   int stride;
   void* d3d_decl;
   void* d3d_dummy_shader;
};

typedef struct A5O_PRIM_VERTEX_CACHE {
   A5O_VERTEX  buffer[A5O_VERTEX_CACHE_SIZE];
   A5O_VERTEX* current;
   size_t          size;
   A5O_COLOR   color;
   int             prim_type;
   void*           user_data;
} A5O_PRIM_VERTEX_CACHE;

typedef struct A5O_BUFFER_COMMON {
   uintptr_t handle;
   bool write_only;
   /* In elements */
   int size;

   bool is_locked;
   int lock_flags;
   void* locked_memory;
    /* These three are in bytes */
   int local_buffer_length;
   int lock_offset;
   int lock_length;
} A5O_BUFFER_COMMON;

struct A5O_VERTEX_BUFFER {
   A5O_VERTEX_DECL* decl;
   A5O_BUFFER_COMMON common;
};

struct A5O_INDEX_BUFFER {
   int index_size;
   A5O_BUFFER_COMMON common;
};

/* Internal cache for primitives. */
void _al_prim_cache_init(A5O_PRIM_VERTEX_CACHE* cache, int prim_type, A5O_COLOR color);
void _al_prim_cache_init_ex(A5O_PRIM_VERTEX_CACHE* cache, int prim_type, A5O_COLOR color, void* user_data);
void _al_prim_cache_term(A5O_PRIM_VERTEX_CACHE* cache);
void _al_prim_cache_flush(A5O_PRIM_VERTEX_CACHE* cache);
void _al_prim_cache_push_point(A5O_PRIM_VERTEX_CACHE* cache, const float* v);
void _al_prim_cache_push_triangle(A5O_PRIM_VERTEX_CACHE* cache, const float* v0, const float* v1, const float* v2);


/* Internal functions. */
float     _al_prim_get_scale(void);
float     _al_prim_normalize(float* vector);
int       _al_prim_test_line_side(const float* origin, const float* normal, const float* point);
bool      _al_prim_is_point_in_triangle(const float* point, const float* v0, const float* v1, const float* v2);
bool      _al_prim_intersect_segment(const float* v0, const float* v1, const float* p0, const float* p1, float* point, float* t0, float* t1);
bool      _al_prim_are_points_equal(const float* point_a, const float* point_b);

int _al_bitmap_region_is_locked(A5O_BITMAP* bmp, int x1, int y1, int x2, int y2);
int _al_draw_buffer_common_soft(A5O_VERTEX_BUFFER* vertex_buffer, A5O_BITMAP* texture, A5O_INDEX_BUFFER* index_buffer, int start, int end, int type);

#ifdef __cplusplus
}
#endif

#endif
