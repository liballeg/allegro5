#ifndef __al_included_allegro5_aintern_prim_h
#define __al_included_allegro5_aintern_prim_h

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int x2, int y2);

struct ALLEGRO_VERTEX_DECL {
   ALLEGRO_VERTEX_ELEMENT* elements;
   int stride;
   void* d3d_decl;
   void* d3d_dummy_shader;
};

/* Internal functions. */
float     _al_prim_wrap_two_pi(float angle);
float     _al_prim_wrap_pi_to_pi(float angle);
float     _al_prim_get_angle(const float* p0, const float* origin, const float* p1);
int       _al_prim_test_line_side(const float* origin, const float* normal, const float* point);
bool      _al_prim_is_point_in_triangle(const float* point, const float* v0, const float* v1, const float* v2);
bool      _al_prim_intersect_segment(const float* v0, const float* v1, const float* p0, const float* p1, float* point, float* t0, float* t1);
bool      _al_prim_are_points_equal(const float* point_a, const float* point_b);

#endif
