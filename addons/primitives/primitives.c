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

int al_draw_prim(ALLEGRO_VBUFFER* vbuff, ALLEGRO_BITMAP* texture, int start, int end, int type)
{
   ASSERT(vbuff);
   ASSERT(end >= start);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);
   
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   int ret = 0;
   int flags = al_get_display_flags();
   
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

int al_draw_indexed_prim(ALLEGRO_VBUFFER* vbuff, ALLEGRO_BITMAP* texture, int* indices, int num_vtx, int type)
{
   ASSERT(vbuff);
   ASSERT(indices);
   ASSERT(num_vtx > 0);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);
   
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   int ret = 0;
   int flags = al_get_display_flags();
   
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

int al_lock_vbuff(ALLEGRO_VBUFFER* vbuff, int type)
{
   ASSERT(vbuff);
   
   return al_lock_vbuff_range(vbuff, 0, vbuff->len, type);
}

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

int al_vbuff_is_locked(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   
   return vbuff->flags & ALLEGRO_VBUFFER_LOCKED;
}

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

void al_set_vbuff_pos(ALLEGRO_VBUFFER* vbuff, int idx, float x, float y, float z)
{
   ASSERT(vbuff);
   
   int unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_pos_soft(vbuff, idx, x, y, z);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

void al_set_vbuff_normal(ALLEGRO_VBUFFER* vbuff, int idx, float nx, float ny, float nz)
{
   ASSERT(vbuff);
   
   int unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_normal_soft(vbuff, idx, nx, ny, nz);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

void al_set_vbuff_uv(ALLEGRO_VBUFFER* vbuff, int idx, float u, float v)
{
   ASSERT(vbuff);
   
   int unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_uv_soft(vbuff, idx, u, v);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

void al_set_vbuff_vertex(ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_VERTEX *vtx)
{
   ASSERT(vbuff);
   ASSERT(vtx);
   
   int unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_READ);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_vertex_soft(vbuff, idx, vtx);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

void al_set_vbuff_color(ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_COLOR col)
{
   ASSERT(vbuff);
   
   int unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_WRITE);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_set_vbuff_color_soft(vbuff, idx, col);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

void al_get_vbuff_vertex(ALLEGRO_VBUFFER* vbuff, int idx, ALLEGRO_VERTEX *vtx)
{
   ASSERT(vbuff);
   ASSERT(vtx);
   
   int unlock = _al_prim_check_lock_pos(vbuff, idx, ALLEGRO_VBUFFER_READ);
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      _al_get_vbuff_vertex_soft(vbuff, idx, vtx);
   } else {
      ASSERT(0);
   }
   if (unlock) {
      al_unlock_vbuff(vbuff);
   }
}

int al_get_vbuff_flags(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   return vbuff->flags;
}

int al_get_vbuff_len(ALLEGRO_VBUFFER* vbuff)
{
   ASSERT(vbuff);
   return vbuff->len;
}

void al_get_vbuff_lock_range(ALLEGRO_VBUFFER* vbuff, int* start, int* end)
{
   ASSERT(vbuff);
   ASSERT(start);
   ASSERT(end);
   ASSERT(al_vbuff_is_locked(vbuff));
   
   *start = vbuff->lock_start;
   *end = vbuff->lock_end;
}

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

void al_copy_transform(ALLEGRO_TRANSFORM* src, ALLEGRO_TRANSFORM* dest)
{
   ASSERT(src);
   ASSERT(dest);
   
   memcpy(dest, src, sizeof(ALLEGRO_TRANSFORM));
}

void al_use_transform(ALLEGRO_TRANSFORM* trans)
{
   ASSERT(trans);
   
   al_copy_transform(trans, &_al_global_trans);
   int flags = al_get_display_flags();
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

void al_set_prim_flag(int flag, int value)
{
   if (flag > 0 && flag < ALLEGRO_PRIM_NUM_FLAGS) {
      _al_prim_global_flags[flag] = value;
   }
}

int al_get_prim_flag(int flag)
{
   if (flag > 0 && flag < ALLEGRO_PRIM_NUM_FLAGS) {
      return _al_prim_global_flags[flag];
   }
   return 0;
}
