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

int _al_prim_global_flags[ALLEGRO_PRIM_NUM_FLAGS];
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
int al_draw_prim(ALLEGRO_VBUFFER* vbuff, ALLEGRO_BITMAP* texture,
   int start, int end, int type)
{  
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(vbuff);
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
      ret =  _al_draw_prim_soft(texture, vbuff, start, end, type);
   } else {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_opengl(texture, vbuff, start, end, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_directx(texture, vbuff, start, end, type);
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
int al_draw_indexed_prim(ALLEGRO_VBUFFER* vbuff, ALLEGRO_BITMAP* texture,
   const int* indices, int num_vtx, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(vbuff);
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
      ret =  _al_draw_prim_indexed_soft(texture, vbuff, indices, num_vtx, type);
   } else {
      int flags = al_get_display_flags();
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_indexed_opengl(texture, vbuff, indices, num_vtx, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_indexed_directx(texture, vbuff, indices, num_vtx, type);
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

/* Function: al_lock_vbuff_range
 */
int al_lock_vbuff_range(ALLEGRO_VBUFFER* vbuff, int start, int end, int type)
{
   ASSERT(vbuff);
   ASSERT(end >= start);
   
   if (vbuff->flags & ALLEGRO_VBUFFER_LOCKED)
      return 0;
   if (type & ALLEGRO_VBUFFER_WRITE && !(vbuff->flags & ALLEGRO_VBUFFER_WRITE))
      return 0;
   if (type & ALLEGRO_VBUFFER_READ && !(vbuff->flags & ALLEGRO_VBUFFER_READ))
      return 0;
      
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      return _al_prim_lock_vbuff_range_soft(vbuff, start, end, type);
   }
   return 0;
}

/* Function: al_lock_vbuff
 */
int al_lock_vbuff(ALLEGRO_VBUFFER* vbuff, int type)
{
   ASSERT(vbuff);
   
   return al_lock_vbuff_range(vbuff, 0, vbuff->len, type);
}

/* Function: al_unlock_vbuff
 */
void al_unlock_vbuff(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   
   if (!(vbuff->flags & ALLEGRO_VBUFFER_LOCKED)) {
      return;
   }
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_prim_unlock_vbuff_soft(vbuff);
   }
}

/* Function: al_vbuff_is_locked
 */
int al_vbuff_is_locked(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   
   return vbuff->flags & ALLEGRO_VBUFFER_LOCKED;
}

/* Function: al_vbuff_range_is_locked
 */
int al_vbuff_range_is_locked(ALLEGRO_VBUFFER* vbuff, int start, int end)
{
   ASSERT(vbuff);
   
   if (!vbuff->flags & ALLEGRO_VBUFFER_LOCKED)
      return 0;
   if (start >= vbuff->lock_start && end <= vbuff->lock_end) {
      return 1;
   }
   return 0;
}

int _al_prim_check_lock_pos(ALLEGRO_VBUFFER* vbuff, int idx, int type)
{
   ASSERT(vbuff);
   ASSERT(idx >= 0 && idx < vbuff->len);
   
   if (vbuff->flags & ALLEGRO_VBUFFER_LOCKED) {
      return 0;
   } else {
      al_lock_vbuff_range(vbuff, idx, idx, type);
      return 1;
   }
}

/* Function: al_set_vbuff_pos
 */
void al_set_vbuff_pos(ALLEGRO_VBUFFER* vbuff, int idx,
   float x, float y, float z)
{
   int unlock;
   ASSERT(vbuff);
   
   unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_pos_soft(vbuff, idx, x, y, z);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

/* Function: al_set_vbuff_normal
 */
void al_set_vbuff_normal(ALLEGRO_VBUFFER* vbuff, int idx,
   float nx, float ny, float nz)
{
   int unlock;
   ASSERT(vbuff);
   
   unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_normal_soft(vbuff, idx, nx, ny, nz);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

/* Function: al_set_vbuff_uv
 */
void al_set_vbuff_uv(ALLEGRO_VBUFFER* vbuff, int idx, float u, float v)
{
   int unlock;
   ASSERT(vbuff);
   
   unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_uv_soft(vbuff, idx, u, v);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

/* Function: al_set_vbuff_vertex
 */
void al_set_vbuff_vertex(ALLEGRO_VBUFFER* vbuff, int idx,
   const ALLEGRO_VERTEX *vtx)
{
   int unlock;
   ASSERT(vbuff);
   ASSERT(vtx);
   
   unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_READ);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_vertex_soft(vbuff, idx, vtx);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

/* Function: al_set_vbuff_color
 */
void al_set_vbuff_color(ALLEGRO_VBUFFER* vbuff, int idx,
   const ALLEGRO_COLOR col)
{
   int unlock;
   ASSERT(vbuff);
   
   unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_color_soft(vbuff, idx, col);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

/* Function: al_get_vbuff_vertex
 */
void al_get_vbuff_vertex(ALLEGRO_VBUFFER* vbuff, int idx, ALLEGRO_VERTEX *vtx)
{
   int unlock;
   ASSERT(vbuff);
   ASSERT(vtx);
   
   unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_READ);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_get_vbuff_vertex_soft(vbuff, idx, vtx);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

/* Function: al_get_vbuff_flags
 */
int al_get_vbuff_flags(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   return vbuff->flags;
}

/* Function: al_get_vbuff_len
 */
int al_get_vbuff_len(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   return vbuff->len;
}

/* Function: al_get_vbuff_lock_range
 */
void al_get_vbuff_lock_range(ALLEGRO_VBUFFER* vbuff, int* start, int* end)
{
   ASSERT(vbuff);
   ASSERT(start);
   ASSERT(end);
   ASSERT(al_vbuff_is_locked(vbuff));
   
   *start = vbuff->lock_start;
   *end = vbuff->lock_end;
}

/* Function: al_create_vbuff
 */
ALLEGRO_VBUFFER* al_create_vbuff(int len, int type)
{
   ALLEGRO_VBUFFER* ret = (ALLEGRO_VBUFFER*)malloc(sizeof(ALLEGRO_VBUFFER));
   ret->len = len;
   ret->flags = 0;
   if (type & ALLEGRO_VBUFFER_SOFT) {
      ret->flags |= ALLEGRO_VBUFFER_SOFT;
   } else if (type & ALLEGRO_VBUFFER_VIDEO) {
      ret->flags |= ALLEGRO_VBUFFER_VIDEO;
   }
   ret->flags |= type & ALLEGRO_VBUFFER_WRITE;
   ret->flags |= type & ALLEGRO_VBUFFER_READ;
   
   if (ret->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_create_vbuff_soft(ret);
   } else {
      free(ret);
      ret = 0;
   }
   return ret;
}

/* Function: al_destroy_vbuff
 */
void al_destroy_vbuff(ALLEGRO_VBUFFER* vbuff)
{
   if (!vbuff)
      return;
   al_unlock_vbuff(vbuff);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_destroy_vbuff_soft(vbuff);
   }
   free(vbuff);
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

/* Function: al_set_prim_flag
 */
void al_set_prim_flag(int flag, int value)
{
   if (flag > 0 && flag < ALLEGRO_PRIM_NUM_FLAGS) {
      _al_prim_global_flags[flag] = value;
   }
}

/* Function: al_get_prim_flag
 */
int al_get_prim_flag(int flag)
{
   if (flag > 0 && flag < ALLEGRO_PRIM_NUM_FLAGS) {
      return _al_prim_global_flags[flag];
   }
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
