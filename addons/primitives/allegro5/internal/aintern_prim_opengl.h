#ifndef AINTERN_PRIM_OPENGL_H
#define AINTERN_PRIM_OPENGL_H

struct ALLEGRO_BITMAP;
struct ALLEGRO_VERTEX;

int _al_draw_prim_opengl(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_opengl(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);
void _al_use_transform_opengl(const ALLEGRO_TRANSFORM* trans);

#endif
