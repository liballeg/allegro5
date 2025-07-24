#ifndef __al_included_allegro5_aintern_prim_soft_h
#define __al_included_allegro5_aintern_prim_soft_h

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_primitives.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ALLEGRO_BITMAP;
struct ALLEGRO_VERTEX;
enum ALLEGRO_BITMAP_WRAP;

int _al_fix_texcoord(float var, int max_var, ALLEGRO_BITMAP_WRAP wrap);
int _al_draw_prim_soft(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_soft(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);

void _al_line_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2);
void _al_point_2d(ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v);

void _al_draw_soft_line(ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, uintptr_t state,
   void (*first)(uintptr_t, int, int, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
   void (*step)(uintptr_t, int),
   void (*draw)(uintptr_t, int, int));

#ifdef __cplusplus
}
#endif

#endif
