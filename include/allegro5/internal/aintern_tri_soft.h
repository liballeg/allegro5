#ifndef __al_included_allegro5_aintern_tri_soft_h
#define __al_included_allegro5_aintern_tri_soft_h

struct A5O_BITMAP;

/* Duplicated in allegro_primitives.h */
#ifndef _A5O_VERTEX_DEFINED
#define _A5O_VERTEX_DEFINED

typedef struct A5O_VERTEX A5O_VERTEX;

struct A5O_VERTEX {
  float x, y, z;
  float u, v;
  A5O_COLOR color;
};
#endif

AL_FUNC(void, _al_triangle_2d, (A5O_BITMAP* texture, A5O_VERTEX* v1, A5O_VERTEX* v2, A5O_VERTEX* v3));
AL_FUNC(void, _al_draw_soft_triangle, (
   A5O_VERTEX* v1, A5O_VERTEX* v2, A5O_VERTEX* v3, uintptr_t state,
   void (*init)(uintptr_t, A5O_VERTEX*, A5O_VERTEX*, A5O_VERTEX*),
   void (*first)(uintptr_t, int, int, int, int),
   void (*step)(uintptr_t, int),
   void (*draw)(uintptr_t, int, int, int)));

#endif
