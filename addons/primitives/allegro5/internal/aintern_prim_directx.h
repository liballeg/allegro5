#ifndef AINTERN_PRIM_DIRECTX_H
#define AINTERN_PRIM_DIRECTX_H

struct ALLEGRO_BITMAP;
struct ALLEGRO_VERTEX;

int _al_draw_prim_directx(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* vtxs, int start, int end, int type);
int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* vtxs, const int* indices, int num_vtx, int type);
void _al_use_transform_directx(ALLEGRO_TRANSFORM* trans);

#endif
