#ifndef __al_included_allegro5_aintern_transform_h
#define __al_included_allegro5_aintern_transform_h

#include "allegro5/allegro.h"
#include "allegro5/transformations.h"


bool _al_transform_is_translation(const ALLEGRO_TRANSFORM* trans,
   float *dx, float *dy);

#define M(i, j) trans->m[i][j]

AL_INLINE(void, _al_transform_coordinates_3d, (const ALLEGRO_TRANSFORM *trans, float *x, float *y, float *z),
{
   float rx;
   float ry;
   float rz;

   rx = M(0, 0) * *x + M(1, 0) * *y + M(2, 0) * *z + M(3, 0);
   ry = M(0, 1) * *x + M(1, 1) * *y + M(2, 1) * *z + M(3, 1);
   rz = M(0, 2) * *x + M(1, 2) * *y + M(2, 2) * *z + M(3, 2);

   *x = rx;
   *y = ry;
   *z = rz;
})

#undef M

#endif
