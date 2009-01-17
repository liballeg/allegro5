#ifndef AINTERN_PRIM_SOFT_H
#define AINTERN_PRIM_SOFT_H

struct ALLEGRO_BITMAP;
struct ALLEGRO_VBUFFER;

void _al_create_vbuff_soft(ALLEGRO_VBUFFER* vbuff);
void _al_destroy_vbuff_soft(ALLEGRO_VBUFFER* vbuff);

int _al_draw_prim_soft(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int start, int end, int type);
int _al_draw_prim_indexed_soft(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int* indices, int num_vtx, int type);

void _al_line_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2);
void _al_triangle_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3);

int _al_prim_lock_vbuff_range_soft(ALLEGRO_VBUFFER* vbuff, int start, int end, int type);
void _al_prim_unlock_vbuff_soft(ALLEGRO_VBUFFER* vbuff);

void _al_set_vbuff_pos_soft(ALLEGRO_VBUFFER* vbuff, int idx, float x, float y, float z);
void _al_set_vbuff_normal_soft(ALLEGRO_VBUFFER* vbuff, int idx, float nx, float ny, float nz);
void _al_set_vbuff_uv_soft(ALLEGRO_VBUFFER* vbuff, int idx, float u, float v);
void _al_set_vbuff_color_soft(ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_COLOR col);

void _al_get_vbuff_vertex_soft(ALLEGRO_VBUFFER* vbuff, int idx, ALLEGRO_VERTEX *vtx);
void _al_set_vbuff_vertex_soft(ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_VERTEX *vtx);

#endif
