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
 *      Software implementation of some of the primitive routines.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"

#include "allegro5/a5_primitives.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_prim.h"

#ifdef ALLEGRO_CFG_D3D
#include "allegro5/a5_direct3d.h"
#endif

extern ALLEGRO_TRANSFORM _al_global_trans;

/*
The vertex cache allows for bulk transformation of vertices, for faster run speeds
*/
ALLEGRO_VERTEX vertex_cache[ALLEGRO_VERTEX_CACHE_SIZE];

void _al_create_vbuff_soft(ALLEGRO_VBUFFER* vbuff)
{
   vbuff->data = (ALLEGRO_VERTEX*)malloc(sizeof(ALLEGRO_VERTEX) * vbuff->len);
   ASSERT(vbuff->data);
   memset(vbuff->data, sizeof(ALLEGRO_VERTEX) * vbuff->len, 0);
}

void _al_destroy_vbuff_soft(ALLEGRO_VBUFFER* vbuff)
{
   free(vbuff->data);
}

int _al_draw_prim_soft(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int start, int end, int type)
{
   int num_primitives;
   int num_vtx;
   int use_cache;

   ASSERT(!al_vbuff_is_locked(vbuff));
   
   num_primitives = 0;
   num_vtx = end - start;
   use_cache = num_vtx < ALLEGRO_VERTEX_CACHE_SIZE;
   
   if (!al_lock_vbuff_range(vbuff, start, end, ALLEGRO_VBUFFER_READ))
      return 0;
      
   if (use_cache) {
      int ii;
      int n = 0;
      for (ii = start; ii < end; ii++) {
         al_get_vbuff_vertex(vbuff, ii, &vertex_cache[n]);
         
         al_transform_vertex(&_al_global_trans, &vertex_cache[n]);
         n++;
      }
   }
   
#define SET_VERTEX(v, idx)                             \
   al_get_vbuff_vertex(vbuff, idx, &v);                \
   al_transform_vertex(&_al_global_trans, &v);         \
    
   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         if (use_cache) {
            int ii;
            for (ii = 0; ii < num_vtx - 1; ii += 2) {
               _al_line_2d(texture, &vertex_cache[ii], &vertex_cache[ii + 1]);
            }
         } else {
            int ii;
            for (ii = start; ii < end - 1; ii += 2) {
               ALLEGRO_VERTEX v1, v2;
               SET_VERTEX(v1, ii);
               SET_VERTEX(v2, ii + 1);
               
               _al_line_2d(texture, &v1, &v2);
            }
         }
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         if (use_cache) {
            int ii;
            for (ii = 1; ii < num_vtx; ii++) {
               _al_line_2d(texture, &vertex_cache[ii - 1], &vertex_cache[ii]);
            }
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(vtx[0], start);
            for (ii = start + 1; ii < end; ii++) {
               SET_VERTEX(vtx[idx], ii)
               _al_line_2d(texture, &vtx[0], &vtx[1]);
               idx = 1 - idx;
            }
         }
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         if (use_cache) {
            int ii;
            for (ii = 1; ii < num_vtx; ii++) {
               _al_line_2d(texture, &vertex_cache[ii - 1], &vertex_cache[ii]);
            }
            _al_line_2d(texture, &vertex_cache[0], &vertex_cache[num_vtx - 1]);
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(vtx[0], start);
            for (ii = start + 1; ii < end; ii++) {
               SET_VERTEX(vtx[idx], ii)
               _al_line_2d(texture, &vtx[0], &vtx[1]);
               idx = 1 - idx;
            }
            SET_VERTEX(vtx[idx], start)
            _al_line_2d(texture, &vtx[0], &vtx[1]);
         }
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         if (use_cache) {
            int ii;
            for (ii = 0; ii < num_vtx - 2; ii += 3) {
               _al_triangle_2d(texture, &vertex_cache[ii], &vertex_cache[ii + 1], &vertex_cache[ii + 2]);
            }
         } else {
            int ii;
            for (ii = start; ii < end - 2; ii += 3) {
               ALLEGRO_VERTEX v1, v2, v3;
               SET_VERTEX(v1, ii);
               SET_VERTEX(v2, ii + 1);
               SET_VERTEX(v3, ii + 2);
               
               _al_triangle_2d(texture, &v1, &v2, &v3);
            }
         }
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         if (use_cache) {
            int ii;
            for (ii = 2; ii < num_vtx; ii++) {
               _al_triangle_2d(texture, &vertex_cache[ii - 2], &vertex_cache[ii - 1], &vertex_cache[ii]);
            }
         } else {
            int ii;
            int idx = 2;
            ALLEGRO_VERTEX vtx[3];
            SET_VERTEX(vtx[0], start);
            SET_VERTEX(vtx[1], start + 1);
            for (ii = start + 2; ii < end; ii++) {
               SET_VERTEX(vtx[idx], ii);
               
               _al_triangle_2d(texture, &vtx[0], &vtx[1], &vtx[2]);
               idx = (idx + 1) % 3;
            }
         }
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         if (use_cache) {
            int ii;
            for (ii = 1; ii < num_vtx; ii++) {
               _al_triangle_2d(texture, &vertex_cache[0], &vertex_cache[ii], &vertex_cache[ii - 1]);
            }
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX v0;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(v0, start);
            SET_VERTEX(vtx[0], start + 1);
            for (ii = start + 1; ii < end; ii++) {
               SET_VERTEX(vtx[idx], ii)
               _al_triangle_2d(texture, &v0, &vtx[0], &vtx[1]);
               idx = 1 - idx;
            }
         }
         num_primitives = num_vtx - 2;
         break;
      };
   }
   
   al_unlock_vbuff(vbuff);
   
   return num_primitives;
}

int _al_draw_prim_indexed_soft(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int* indices, int num_vtx, int type)
{
   int num_primitives;
   int use_cache;
   int min_idx, max_idx;
   int ii;

   ASSERT(!al_vbuff_is_locked(vbuff));

   num_primitives = 0;   
   use_cache = 1;
   min_idx = indices[0];
   max_idx = indices[0];

   /*
   Determine the range we are dealing with
   */
   for (ii = 1; ii < num_vtx; ii++) {
      int idx = indices[ii];
      if (max_idx < idx)
         max_idx = idx;
      else if (min_idx > indices[ii])
         min_idx = idx;
   }
   if (max_idx - min_idx >= ALLEGRO_VERTEX_CACHE_SIZE) {
      use_cache = 0;
   }
   
   if (!al_lock_vbuff_range(vbuff, min_idx, max_idx, ALLEGRO_VBUFFER_READ))
      return 0;
      
   if (use_cache) {
      int ii;
      for (ii = 0; ii < num_vtx; ii++) {
         int idx = indices[ii];
         al_get_vbuff_vertex(vbuff, idx, &vertex_cache[idx - min_idx]);
         al_transform_vertex(&_al_global_trans, &vertex_cache[idx - min_idx]);
      }
   }
   
#define SET_VERTEX(v, idx)                             \
   al_get_vbuff_vertex(vbuff, idx, &v);                \
   al_transform_vertex(&_al_global_trans, &v);         \
    
   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         if (use_cache) {
            int ii;
            for (ii = 0; ii < num_vtx - 1; ii += 2) {
               int idx1 = indices[ii] - min_idx;
               int idx2 = indices[ii + 1] - min_idx;
               
               _al_line_2d(texture, &vertex_cache[idx1], &vertex_cache[idx2]);
            }
         } else {
            int ii;
            for (ii = 0; ii < num_vtx - 1; ii += 2) {
               int idx1 = indices[ii];
               int idx2 = indices[ii + 1];
               
               ALLEGRO_VERTEX v1, v2;
               SET_VERTEX(v1, idx1);
               SET_VERTEX(v2, idx2);
               
               _al_line_2d(texture, &v1, &v2);
            }
         }
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         if (use_cache) {
            int ii;
            for (ii = 1; ii < num_vtx; ii++) {
               int idx1 = indices[ii - 1] - min_idx;
               int idx2 = indices[ii] - min_idx;
               
               _al_line_2d(texture, &vertex_cache[idx1], &vertex_cache[idx2]);
            }
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(vtx[0], indices[0]);
            for (ii = 1; ii < num_vtx; ii++) {
               SET_VERTEX(vtx[idx], indices[ii])
               _al_line_2d(texture, &vtx[0], &vtx[1]);
               idx = 1 - idx;
            }
         }
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         if (use_cache) {
            int ii;
            int idx1, idx2;
            
            for (ii = 1; ii < num_vtx; ii++) {
               int idx1 = indices[ii - 1] - min_idx;
               int idx2 = indices[ii] - min_idx;
               
               _al_line_2d(texture, &vertex_cache[idx1], &vertex_cache[idx2]);
            }
            idx1 = indices[0] - min_idx;
            idx2 = indices[num_vtx - 1] - min_idx;
            
            _al_line_2d(texture, &vertex_cache[idx1], &vertex_cache[idx2]);
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(vtx[0], indices[0]);
            for (ii = 1; ii < num_vtx; ii++) {
               SET_VERTEX(vtx[idx], indices[ii])
               _al_line_2d(texture, &vtx[0], &vtx[1]);
               idx = 1 - idx;
            }
            SET_VERTEX(vtx[idx], indices[0])
            _al_line_2d(texture, &vtx[0], &vtx[1]);
         }
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         if (use_cache) {
            int ii;
            for (ii = 0; ii < num_vtx - 2; ii += 3) {
               int idx1 = indices[ii] - min_idx;
               int idx2 = indices[ii + 1] - min_idx;
               int idx3 = indices[ii + 2] - min_idx;
               _al_triangle_2d(texture, &vertex_cache[idx1], &vertex_cache[idx2], &vertex_cache[idx3]);
            }
         } else {
            int ii;
            for (ii = 0; ii < num_vtx - 2; ii += 3) {
               int idx1 = indices[ii];
               int idx2 = indices[ii + 1];
               int idx3 = indices[ii + 2];
               
               ALLEGRO_VERTEX v1, v2, v3;
               SET_VERTEX(v1, idx1);
               SET_VERTEX(v2, idx2);
               SET_VERTEX(v3, idx3);
               
               _al_triangle_2d(texture, &v1, &v2, &v3);
            }
         }
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         if (use_cache) {
            int ii;
            for (ii = 2; ii < num_vtx; ii++) {
               int idx1 = indices[ii - 2] - min_idx;
               int idx2 = indices[ii - 1] - min_idx;
               int idx3 = indices[ii] - min_idx;
               _al_triangle_2d(texture, &vertex_cache[idx1], &vertex_cache[idx2], &vertex_cache[idx3]);
            }
         } else {
            int ii;
            int idx = 2;
            ALLEGRO_VERTEX vtx[3];
            SET_VERTEX(vtx[0], indices[0]);
            SET_VERTEX(vtx[1], indices[1]);
            for (ii = 2; ii < num_vtx; ii ++) {
               SET_VERTEX(vtx[idx], indices[ii]);
               
               _al_triangle_2d(texture, &vtx[0], &vtx[1], &vtx[2]);
               idx = (idx + 1) % 3;
            }
         }
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         if (use_cache) {
            int ii;
            int idx0 = indices[0];
            for (ii = 1; ii < num_vtx; ii++) {
               int idx1 = indices[ii];
               int idx2 = indices[ii - 1];
               _al_triangle_2d(texture, &vertex_cache[idx0], &vertex_cache[idx1], &vertex_cache[idx2]);
            }
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX v0;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(v0, indices[0]);
            SET_VERTEX(vtx[0], indices[1]);
            for (ii = 2; ii < num_vtx; ii ++) {
               SET_VERTEX(vtx[idx], indices[ii])
               _al_triangle_2d(texture, &v0, &vtx[0], &vtx[1]);
               idx = 1 - idx;
            }
         }
         num_primitives = num_vtx - 2;
         break;
      };
   }
   
   al_unlock_vbuff(vbuff);
   
   return num_primitives;
}

void _al_prim_unlock_vbuff_soft(ALLEGRO_VBUFFER* vbuff)
{
   vbuff->flags &= ~ALLEGRO_VBUFFER_LOCKED;
}

int _al_prim_lock_vbuff_range_soft(ALLEGRO_VBUFFER* vbuff, int start, int end, int type)
{
   vbuff->flags |= ALLEGRO_VBUFFER_LOCKED;
   vbuff->lock_start = start;
   vbuff->lock_end = end;
   (void)type;
   return 1;
}

void _al_set_vbuff_pos_soft(ALLEGRO_VBUFFER* vbuff, int idx, float x, float y, float z)
{
   ALLEGRO_VERTEX* vtx = &((ALLEGRO_VERTEX*)vbuff->data)[idx];
   vtx->x = x;
   vtx->y = y;
   vtx->z = z;
}

void _al_set_vbuff_normal_soft(ALLEGRO_VBUFFER* vbuff, int idx, float nx, float ny, float nz)
{
   ALLEGRO_VERTEX* vtx = &((ALLEGRO_VERTEX*)vbuff->data)[idx];
   vtx->nx = nx;
   vtx->ny = ny;
   vtx->nz = nz;
}

void _al_set_vbuff_uv_soft(ALLEGRO_VBUFFER* vbuff, int idx, float u, float v)
{
   ALLEGRO_VERTEX* vtx = &((ALLEGRO_VERTEX*)vbuff->data)[idx];
   vtx->u = u;
   vtx->v = v;
}

void _al_set_vbuff_color_soft(ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_COLOR col)
{
   ALLEGRO_VERTEX* vtx = &((ALLEGRO_VERTEX*)vbuff->data)[idx];

   vtx->r = col.r;
   vtx->g = col.g;
   vtx->b = col.b;
   vtx->a = col.a;
#ifdef ALLEGRO_CFG_D3D
   vtx->d3d_color = D3DCOLOR_COLORVALUE(col.r, col.g, col.b, col.a);
#endif
}

void _al_get_vbuff_vertex_soft(ALLEGRO_VBUFFER* vbuff, int idx, ALLEGRO_VERTEX *vtx)
{
   *vtx = ((ALLEGRO_VERTEX*)vbuff->data)[idx];
}

void _al_set_vbuff_vertex_soft(ALLEGRO_VBUFFER* vbuff, int idx, const ALLEGRO_VERTEX *vtx)
{
   ((ALLEGRO_VERTEX*)vbuff->data)[idx] = *vtx;
#ifdef ALLEGRO_CFG_D3D
   #define vert ((ALLEGRO_VERTEX *)vbuff->data)[idx]
   vert.d3d_color = D3DCOLOR_COLORVALUE(vert.r, vert.g, vert.b, vert.a);
   #undef vert
#endif
}
