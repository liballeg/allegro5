#ifndef AINTERN_PRIM_SOFT_H
#define AINTERN_PRIM_SOFT_H

struct ALLEGRO_BITMAP;
struct ALLEGRO_VERTEX;

int _al_draw_prim_soft(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_soft(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);

void _al_line_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2);
void _al_triangle_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3);
void _al_point_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v);

#define _DEST_IS_ZERO \
   dst_mode == ALLEGRO_ZERO &&  dst_alpha == ALLEGRO_ZERO && \
   op != ALLEGRO_DEST_MINUS_SRC && op_alpha != ALLEGRO_DEST_MINUS_SRC

#define _SRC_NOT_MODIFIED \
   src_mode == ALLEGRO_ONE && src_alpha == ALLEGRO_ONE && \
   ic.r == 1.0f && ic.g == 1.0f && ic.b == 1.0f && ic.a == 1.0f

#endif
