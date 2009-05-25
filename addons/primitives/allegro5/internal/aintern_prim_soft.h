#ifndef AINTERN_PRIM_SOFT_H
#define AINTERN_PRIM_SOFT_H

struct ALLEGRO_BITMAP;
struct ALLEGRO_VERTEX;

int _al_draw_prim_soft(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* vtxs, int start, int end, int type);
int _al_draw_prim_indexed_soft(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* vtxs, const int* indices, int num_vtx, int type);

void _al_line_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2);
void _al_triangle_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3);

#endif
