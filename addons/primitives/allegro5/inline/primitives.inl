#ifndef ALLEGRO_PRIMITIVES_INL
#define ALLEGRO_PRIMITIVES_INL

#include <math.h>

/*
 * Transformation business
 */
AL_PRIM_INLINE(void, al_identity_transform, (ALLEGRO_TRANSFORM* trans), 
{
   ASSERT(trans);
   
   (*trans)[0][0] = 1;
   (*trans)[0][1] = 0;
   (*trans)[0][2] = 0;
   (*trans)[0][3] = 0;
   
   (*trans)[1][0] = 0;
   (*trans)[1][1] = 1;
   (*trans)[1][2] = 0;
   (*trans)[1][3] = 0;
   
   (*trans)[2][0] = 0;
   (*trans)[2][1] = 0;
   (*trans)[2][2] = 1;
   (*trans)[2][3] = 0;
   
   (*trans)[3][0] = 0;
   (*trans)[3][1] = 0;
   (*trans)[3][2] = 0;
   (*trans)[3][3] = 1;
})

AL_PRIM_INLINE(void, al_build_transform, (ALLEGRO_TRANSFORM* trans, float x, float y, float sx, float sy, float theta), 
{
   ASSERT(trans);
   
   float c = cosf(theta);
   float s = sinf(theta);
   
   (*trans)[0][0] = sx * c;
   (*trans)[0][1] = sy * s;
   (*trans)[0][2] = 0;
   (*trans)[0][3] = 0;
   
   (*trans)[1][0] = -sx * s;
   (*trans)[1][1] = sy * c;
   (*trans)[1][2] = 0;
   (*trans)[1][3] = 0;
   
   (*trans)[2][0] = 0;
   (*trans)[2][1] = 0;
   (*trans)[2][2] = 1;
   (*trans)[2][3] = 0;
   
   (*trans)[3][0] = x;
   (*trans)[3][1] = y;
   (*trans)[3][2] = 0;
   (*trans)[3][3] = 1;
})

AL_PRIM_INLINE(void, al_translate_transform, (ALLEGRO_TRANSFORM* trans, float x, float y), 
{
   ASSERT(trans);
   
   (*trans)[3][0] += x;
   (*trans)[3][1] += y;
})

AL_PRIM_INLINE(void, al_rotate_transform, (ALLEGRO_TRANSFORM* trans, float theta), 
{
   ASSERT(trans);
   
   float c = cosf(theta);
   float s = sinf(theta);
   
   float t[2];
   
   /*
   Copy the first column
   */
   t[0] = (*trans)[0][0];
   t[1] = (*trans)[0][1];
   
   /*
   Set first column
   */
   (*trans)[0][0] = t[0] * c + (*trans)[1][0] * s;
   (*trans)[0][1] = t[1] * c + (*trans)[1][1] * s;
   
   /*
   Set second column
   */
   (*trans)[1][0] = (*trans)[1][0] * c - t[0] * s;
   (*trans)[1][1] = (*trans)[1][1] * c - t[1] * s;
})

AL_PRIM_INLINE(void, al_scale_transform, (ALLEGRO_TRANSFORM* trans, float sx, float sy), 
{
   ASSERT(trans);
   
   (*trans)[0][0] *= sx;
   (*trans)[0][1] *= sx;
   
   (*trans)[1][0] *= sy;
   (*trans)[1][1] *= sy;
})

AL_PRIM_INLINE(void, al_transform_vertex, (ALLEGRO_TRANSFORM* trans, ALLEGRO_VERTEX* vtx), 
{
   ASSERT(trans);
   ASSERT(vtx);
   
   float t;
   t = vtx->x;
   
   vtx->x = t * (*trans)[0][0] + vtx->y * (*trans)[1][0] + (*trans)[3][0];
   vtx->y = t * (*trans)[0][1] + vtx->y * (*trans)[1][1] + (*trans)[3][1];
})

AL_PRIM_INLINE(void, al_transform_transform, (ALLEGRO_TRANSFORM* trans, ALLEGRO_TRANSFORM* trans2), 
{
   ASSERT(trans);
   ASSERT(trans2);
   float t;
   
   /*
   First column
   */
   t = (*trans2)[0][0];
   (*trans2)[0][0] = (*trans)[0][0] * t + (*trans)[1][0] * (*trans2)[0][1];
   (*trans2)[0][1] = (*trans)[0][1] * t + (*trans)[1][1] * (*trans2)[0][1];
   
   /*
   Second column
   */
   t = (*trans2)[1][0];
   (*trans2)[1][0] = (*trans)[0][0] * t + (*trans)[1][0] * (*trans2)[1][1];
   (*trans2)[1][1] = (*trans)[0][1] * t + (*trans)[1][1] * (*trans2)[1][1];
   
   /*
   Fourth column
   */
   t = (*trans2)[3][0];
   (*trans2)[3][0] = (*trans)[0][0] * t + (*trans)[1][0] * (*trans2)[3][1] + (*trans)[3][0];
   (*trans2)[3][1] = (*trans)[0][1] * t + (*trans)[1][1] * (*trans2)[3][1] + (*trans)[3][1];
})

#endif
