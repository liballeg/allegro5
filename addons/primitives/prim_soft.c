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

#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"

#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern_tri_soft.h"

/*
The vertex cache allows for bulk transformation of vertices, for faster run speeds
*/
#define LOCAL_VERTEX_CACHE  ALLEGRO_VERTEX vertex_cache[ALLEGRO_VERTEX_CACHE_SIZE]

static void convert_vtx(ALLEGRO_BITMAP* texture, const char* src, ALLEGRO_VERTEX* dest, const ALLEGRO_VERTEX_DECL* decl)
{
   ALLEGRO_VERTEX_ELEMENT* e;
   if(!decl) {
      *dest = *((ALLEGRO_VERTEX*)src);
      return;
   }
   e = &decl->elements[ALLEGRO_PRIM_POSITION];
   if(e->attribute) {
      switch(e->storage) {
         case ALLEGRO_PRIM_FLOAT_2:
         case ALLEGRO_PRIM_FLOAT_3:
         {
            float *ptr = (float*)(src + e->offset);
            dest->x = *(ptr);
            dest->y = *(ptr + 1);
            break;
         }
         case ALLEGRO_PRIM_SHORT_2:
         {
            short *ptr = (short*)(src + e->offset);
            dest->x = (float)*(ptr);
            dest->y = (float)*(ptr + 1);
            break;
         }
      }
   } else {
      dest->x = 0;
      dest->y = 0;
   }

   e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
   if(!e->attribute)
      e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
   if(e->attribute) {
      switch(e->storage) {
         case ALLEGRO_PRIM_FLOAT_2:
         case ALLEGRO_PRIM_FLOAT_3:
         {
            float *ptr = (float*)(src + e->offset);
            dest->u = *(ptr);
            dest->v = *(ptr + 1);
            break;
         }
         case ALLEGRO_PRIM_SHORT_2:
         {
            short *ptr = (short*)(src + e->offset);
            dest->u = (float)*(ptr);
            dest->v = (float)*(ptr + 1);
            break;
         }
      }
      if(texture && e->attribute == ALLEGRO_PRIM_TEX_COORD) {
         dest->u *= (float)al_get_bitmap_width(texture);
         dest->v *= (float)al_get_bitmap_height(texture);
      }
   } else {
      dest->u = 0;
      dest->v = 0;
   }

   e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
   if(e->attribute) {
      dest->color = *(ALLEGRO_COLOR*)(src + e->offset);
   } else {
      dest->color = al_map_rgba_f(1,1,1,1);
   }
}

int _al_draw_prim_soft(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
   LOCAL_VERTEX_CACHE;
   int num_primitives;
   int num_vtx;
   int use_cache;
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   const ALLEGRO_TRANSFORM* global_trans = al_get_current_transform();
   
   num_primitives = 0;
   num_vtx = end - start;
   use_cache = num_vtx < ALLEGRO_VERTEX_CACHE_SIZE;

   if (texture)
      al_lock_bitmap(texture, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
      
   if (use_cache) {
      int ii;
      int n = 0;
      const char* vtxptr = (const char*)vtxs + start * stride;
      for (ii = 0; ii < num_vtx; ii++) {
         convert_vtx(texture, vtxptr, &vertex_cache[ii], decl);
         al_transform_coordinates(global_trans, &vertex_cache[ii].x, &vertex_cache[ii].y);
         n++;
         vtxptr += stride;
      }
   }
   
#define SET_VERTEX(v, idx)                                             \
   convert_vtx(texture, (const char*)vtxs + stride * (idx), &v, decl); \
   al_transform_coordinates(global_trans, &v.x, &v.y);            \
    
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
            _al_line_2d(texture, &vertex_cache[num_vtx - 1], &vertex_cache[0]);
         } else {
            int ii;
            int idx = 1;
            ALLEGRO_VERTEX vtx[2];
            SET_VERTEX(vtx[0], start);
            for (ii = start + 1; ii < end; ii++) {
               SET_VERTEX(vtx[idx], ii)
               _al_line_2d(texture, &vtx[idx], &vtx[1 - idx]);
               idx = 1 - idx;
            }
            SET_VERTEX(vtx[idx], start)
            _al_line_2d(texture, &vtx[idx], &vtx[1 - idx]);
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
      case ALLEGRO_PRIM_POINT_LIST: {
         if (use_cache) {
            int ii;
            for (ii = 0; ii < num_vtx; ii++) {
               _al_point_2d(texture, &vertex_cache[ii]);
            }
         } else {
            int ii;
            for (ii = start; ii < end; ii++) {
               ALLEGRO_VERTEX v;
               SET_VERTEX(v, ii);

               _al_point_2d(texture, &v);
            }
         }
         num_primitives = num_vtx;
         break;
      };
   }
   
   if(texture)
       al_unlock_bitmap(texture);
   
   return num_primitives;
#undef SET_VERTEX
}

int _al_draw_prim_indexed_soft(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   const int* indices, int num_vtx, int type)
{
   LOCAL_VERTEX_CACHE;
   int num_primitives;
   int use_cache;
   int min_idx, max_idx;
   int ii;
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   const ALLEGRO_TRANSFORM* global_trans = al_get_current_transform();

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

   if (texture)
      al_lock_bitmap(texture, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
      
   if (use_cache) {
      int ii;
      for (ii = 0; ii < num_vtx; ii++) {
         int idx = indices[ii];
         convert_vtx(texture, (const char*)vtxs + idx * stride, &vertex_cache[idx - min_idx], decl);
         al_transform_coordinates(global_trans, &vertex_cache[idx - min_idx].x, &vertex_cache[idx - min_idx].y);
      }
   }
   
#define SET_VERTEX(v, idx)                                             \
   convert_vtx(texture, (const char*)vtxs + stride * (idx), &v, decl); \
   al_transform_coordinates(global_trans, &v.x, &v.y);            \
    
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
            
            _al_line_2d(texture, &vertex_cache[idx2], &vertex_cache[idx1]);
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
            int idx0 = indices[0] - min_idx;
            for (ii = 1; ii < num_vtx; ii++) {
               int idx1 = indices[ii] - min_idx;
               int idx2 = indices[ii - 1] - min_idx;
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
      case ALLEGRO_PRIM_POINT_LIST: {
         if (use_cache) {
            int ii;
            for (ii = 0; ii < num_vtx; ii++) {
               int idx = indices[ii] - min_idx;
               _al_point_2d(texture, &vertex_cache[idx]);
            }
         } else {
            int ii;
            for (ii = 0; ii < num_vtx; ii++) {
               ALLEGRO_VERTEX v;
               SET_VERTEX(v, indices[ii]);

               _al_point_2d(texture, &v);
            }
         }
         num_primitives = num_vtx;
         break;
      };
   }

   if(texture)
       al_unlock_bitmap(texture);
   
   return num_primitives;
#undef SET_VERTEX
}

/* Function: al_draw_soft_triangle
 */
void al_draw_soft_triangle(
   ALLEGRO_VERTEX* v1, ALLEGRO_VERTEX* v2, ALLEGRO_VERTEX* v3, uintptr_t state,
   void (*init)(uintptr_t, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*, ALLEGRO_VERTEX*),
   void (*first)(uintptr_t, int, int, int, int),
   void (*step)(uintptr_t, int),
   void (*draw)(uintptr_t, int, int, int))
{
   _al_draw_soft_triangle(v1, v2, v3, state, init, first, step, draw);
}

/* vim: set sts=3 sw=3 et: */
