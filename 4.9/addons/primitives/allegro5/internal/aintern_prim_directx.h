#ifndef AINTERN_PRIM_DIRECTX_H
#define AINTERN_PRIM_DIRECTX_H

struct ALLEGRO_BITMAP;
struct ALLEGRO_VBUFFER;

int _al_draw_prim_directx(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int start, int end, int type);
int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int* indices, int num_vtx, int type);
void _al_use_transform_directx(ALLEGRO_TRANSFORM* trans);

#endif
