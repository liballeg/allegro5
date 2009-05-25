/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Core primitive addon functions.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_primitives.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_prim.h"
#include <math.h>

#ifdef ALLEGRO_CFG_OPENGL
#include "allegro5/a5_opengl.h"
#endif
#ifdef ALLEGRO_CFG_D3D
#include "allegro5/a5_direct3d.h"
#endif

/*
TODO: This is a hack... I need to know the values of these without actually including the respective system headers
*/
#ifndef ALLEGRO_OPENGL
#define ALLEGRO_OPENGL   4
#endif
#ifndef ALLEGRO_DIRECT3D
#define ALLEGRO_DIRECT3D 8
#endif

ALLEGRO_TRANSFORM _al_global_trans = 
{
   {1, 0, 0, 0},
   {0, 1, 0, 0},
   {0, 0, 1, 0},
   {0, 0, 0, 1}
};

static void temp_trans(float x, float y)
{
   int flags = al_get_display_flags();
   al_translate_transform(&_al_global_trans, x, y);
   if (flags & ALLEGRO_OPENGL) {
      _al_use_transform_opengl(&_al_global_trans);
   } else if (flags & ALLEGRO_DIRECT3D) {
      _al_use_transform_directx(&_al_global_trans);
   }
}

/* Function: al_draw_prim
 */
int al_draw_prim(ALLEGRO_VERTEX* vtxs, ALLEGRO_BITMAP* texture,
   int start, int end, int type)
{  
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(vtxs);
   ASSERT(end >= start);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();

   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (target->parent) {
      /*
       * For sub-bitmaps
       */
      temp_trans((float)target->xofs, (float)target->yofs);
   }
   
   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      ret =  _al_draw_prim_soft(texture, vtxs, start, end, type);
   } else {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_opengl(texture, vtxs, start, end, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_directx(texture, vtxs, start, end, type);
      }
   }
   
   if (target->parent) {
      /*
       * Move it back, if rounding errors become an issue we can do a copy-restore instead
       */
      temp_trans(-(float)target->xofs, -(float)target->yofs);
   }
   
   return ret;
}

/* Function: al_draw_indexed_prim
 */
int al_draw_indexed_prim(ALLEGRO_VERTEX* vtxs, ALLEGRO_BITMAP* texture,
   const int* indices, int num_vtx, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(vtxs);
   ASSERT(indices);
   ASSERT(num_vtx > 0);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();
   
   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (target->parent) {
      /*
       * For sub-bitmaps
       */
      temp_trans((float)target->xofs, (float)target->yofs);
   }
   
   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      ret =  _al_draw_prim_indexed_soft(texture, vtxs, indices, num_vtx, type);
   } else {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_indexed_opengl(texture, vtxs, indices, num_vtx, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_indexed_directx(texture, vtxs, indices, num_vtx, type);
      }
   }
   
   if (target->parent) {
      /*
       * Move it back, if rounding errors become an issue we can do a copy-restore instead
       */
      temp_trans(-(float)target->xofs, -(float)target->yofs);
   }
   
   return ret;
}

ALLEGRO_COLOR al_get_allegro_color(ALLEGRO_PRIM_COLOR col)
{
   return al_map_rgba_f(col.r, col.g, col.b, col.a);
}

ALLEGRO_PRIM_COLOR al_get_prim_color(ALLEGRO_COLOR col)
{
   ALLEGRO_PRIM_COLOR ret;
   ret.r = col.r;
   ret.g = col.g;
   ret.b = col.b;
   ret.a = col.a;
#ifdef ALLEGRO_CFG_D3D
   ret.z = 0;
   ret.d3d_color = D3DCOLOR_COLORVALUE(col.r, col.g, col.b, col.a);
#endif
   return ret;
}

/* Function: al_copy_transform
 */
void al_copy_transform(ALLEGRO_TRANSFORM* src, ALLEGRO_TRANSFORM* dest)
{
   ASSERT(src);
   ASSERT(dest);
   
   memcpy(dest, src, sizeof(ALLEGRO_TRANSFORM));
}

/* Function: al_use_transform
 */
void al_use_transform(ALLEGRO_TRANSFORM* trans)
{
   int flags;
   ASSERT(trans);
   
   al_copy_transform(trans, &_al_global_trans);
   flags = al_get_display_flags();
   if (flags & ALLEGRO_OPENGL) {
      _al_use_transform_opengl(&_al_global_trans);
   } else if (flags & ALLEGRO_DIRECT3D) {
      _al_use_transform_directx(&_al_global_trans);
   }
}

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int w, int h)
{
   ASSERT(bmp);
   
   if (!al_is_bitmap_locked(bmp))
      return 0;
   if (x1 + w > bmp->lock_x && y1 + h > bmp->lock_y && x1 < bmp->lock_x + bmp->lock_w && y1 < bmp->lock_y + bmp->lock_h)
      return 1;
   return 0;
}

/* Function: al_identity_transform
 */
void al_identity_transform(ALLEGRO_TRANSFORM* trans)
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
}

/* Function: al_build_transform
 */
void al_build_transform(ALLEGRO_TRANSFORM* trans, float x, float y,
   float sx, float sy, float theta)
{
   float c, s;
   ASSERT(trans);
   
   c = cosf(theta);
   s = sinf(theta);
   
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
}

/* Function: al_translate_transform
 */
void al_translate_transform(ALLEGRO_TRANSFORM* trans, float x, float y)
{
   ASSERT(trans);
   
   (*trans)[3][0] += x;
   (*trans)[3][1] += y;
}

/* Function: al_rotate_transform
 */
void al_rotate_transform(ALLEGRO_TRANSFORM* trans, float theta)
{
   float c, s;
   float t[2];
   ASSERT(trans);
   
   c = cosf(theta);
   s = sinf(theta);
   
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
}

/* Function: al_scale_transform
 */
void al_scale_transform(ALLEGRO_TRANSFORM* trans, float sx, float sy)
{
   ASSERT(trans);
   
   (*trans)[0][0] *= sx;
   (*trans)[0][1] *= sx;
   
   (*trans)[1][0] *= sy;
   (*trans)[1][1] *= sy;
}

/* Function: al_transform_vertex
 */
void al_transform_vertex(ALLEGRO_TRANSFORM* trans, ALLEGRO_VERTEX* vtx)
{
   float t;
   ASSERT(trans);
   ASSERT(vtx);

   t = vtx->x;
   
   vtx->x = t * (*trans)[0][0] + vtx->y * (*trans)[1][0] + (*trans)[3][0];
   vtx->y = t * (*trans)[0][1] + vtx->y * (*trans)[1][1] + (*trans)[3][1];
}

/* Function: al_transform_transform
 */
void al_transform_transform(ALLEGRO_TRANSFORM* trans, ALLEGRO_TRANSFORM* trans2)
{
   float t;
   ASSERT(trans);
   ASSERT(trans2);
   
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
}
