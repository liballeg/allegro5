#ifndef __al_included_allegro5_aintern_tri_soft_h
#define __al_included_allegro5_aintern_tri_soft_h

struct ALLEGRO_VERTEX;
struct ALLEGRO_BITMAP;

AL_FUNC(void, _al_triangle_2d, (ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3));
AL_FUNC(void, _al_draw_soft_triangle, (
   ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3, uintptr_t state,
   void (*init)(uintptr_t, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
   void (*first)(uintptr_t, int, int, int, int),
   void (*step)(uintptr_t, int),
   void (*draw)(uintptr_t, int, int, int)));

#endif
