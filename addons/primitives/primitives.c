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

#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include <math.h>

#ifdef ALLEGRO_CFG_OPENGL
#include "allegro5/allegro_opengl.h"
#endif

#ifndef ALLEGRO_DIRECT3D
#define ALLEGRO_DIRECT3D ALLEGRO_DIRECT3D_INTERNAL
#endif

ALLEGRO_DEBUG_CHANNEL("primitives")

static bool addon_initialized = false;

/* Function: al_init_primitives_addon
 */
bool al_init_primitives_addon(void)
{
   bool ret = true;
   ret &= _al_init_d3d_driver();
   
   addon_initialized = ret;
   
   _al_add_exit_func(al_shutdown_primitives_addon, "primitives_shutdown");
   
   return ret;
}

/* Function: al_is_primitives_addon_initialized
 */
bool al_is_primitives_addon_initialized(void)
{
   return addon_initialized;
}

/* Function: al_shutdown_primitives_addon
 */
void al_shutdown_primitives_addon(void)
{
   _al_shutdown_d3d_driver();
   addon_initialized = false;
}

/* Function: al_draw_prim
 */
int al_draw_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{  
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(addon_initialized);
   ASSERT(vtxs);
   ASSERT(end >= start);
   ASSERT(start >= 0);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();

   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret =  _al_draw_prim_soft(texture, vtxs, decl, start, end, type);
   } else {
      int flags = al_get_display_flags(_al_get_bitmap_display(target));
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_opengl(target, texture, vtxs, decl, start, end, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_directx(target, texture, vtxs, decl, start, end, type);
      }
   }
   
   return ret;
}

/* Function: al_draw_indexed_prim
 */
int al_draw_indexed_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;
 
   ASSERT(addon_initialized);
   ASSERT(vtxs);
   ASSERT(indices);
   ASSERT(num_vtx > 0);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();
   
   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret =  _al_draw_prim_indexed_soft(texture, vtxs, decl, indices, num_vtx, type);
   } else {
      int flags = al_get_display_flags(_al_get_bitmap_display(target));
      if (flags & ALLEGRO_OPENGL) {
         ret =  _al_draw_prim_indexed_opengl(target, texture, vtxs, decl, indices, num_vtx, type);
      } else if (flags & ALLEGRO_DIRECT3D) {
         ret =  _al_draw_prim_indexed_directx(target, texture, vtxs, decl, indices, num_vtx, type);
      }
   }
   
   return ret;
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

/* Function: al_get_allegro_primitives_version
 */
uint32_t al_get_allegro_primitives_version(void)
{
   return ALLEGRO_VERSION_INT;
}

/* Function: al_create_vertex_decl
 */
ALLEGRO_VERTEX_DECL* al_create_vertex_decl(const ALLEGRO_VERTEX_ELEMENT* elements, int stride)
{
   ALLEGRO_VERTEX_DECL* ret;
   ALLEGRO_DISPLAY* display;
   ALLEGRO_VERTEX_ELEMENT* e;
   int flags;

   ASSERT(addon_initialized);

   ret = al_malloc(sizeof(ALLEGRO_VERTEX_DECL));
   ret->elements = al_calloc(1, sizeof(ALLEGRO_VERTEX_ELEMENT) * ALLEGRO_PRIM_ATTR_NUM);
   while(elements->attribute) {
#ifdef ALLEGRO_CFG_OPENGLES
      if (elements->storage == ALLEGRO_PRIM_HALF_FLOAT_2 ||
          elements->storage == ALLEGRO_PRIM_HALF_FLOAT_4) {
         ALLEGRO_WARN("This platform does not support ALLEGRO_PRIM_HALF_FLOAT_2 or ALLEGRO_PRIM_HALF_FLOAT_4.\n");
         goto fail;
      }
#endif
      ret->elements[elements->attribute] = *elements;
      elements++;
   }

   e = &ret->elements[ALLEGRO_PRIM_POSITION];
   if (e->attribute) {
      if (e->storage != ALLEGRO_PRIM_FLOAT_2 &&
          e->storage != ALLEGRO_PRIM_FLOAT_3 &&
          e->storage != ALLEGRO_PRIM_SHORT_2) {
         ALLEGRO_WARN("Invalid storage for ALLEGRO_PRIM_POSITION.\n");
         goto fail;
      }
   }

   e = &ret->elements[ALLEGRO_PRIM_TEX_COORD];
   if(!e->attribute)
      e = &ret->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
   if (e->attribute) {
      if (e->storage != ALLEGRO_PRIM_FLOAT_2 &&
          e->storage != ALLEGRO_PRIM_SHORT_2) {
         ALLEGRO_WARN("Invalid storage for %s.\n", ret->elements[ALLEGRO_PRIM_TEX_COORD].attribute ? "ALLEGRO_PRIM_TEX_COORD" : "ALLEGRO_PRIM_TEX_COORD_PIXEL");
         goto fail;
      }
   }

   display = al_get_current_display();
   flags = al_get_display_flags(display);
   if (flags & ALLEGRO_DIRECT3D) {
      _al_set_d3d_decl(display, ret);
   }
   
   ret->stride = stride;
   return ret;
fail:
   al_free(ret->elements);
   al_free(ret);
   return NULL;
}

/* Function: al_destroy_vertex_decl
 */
void al_destroy_vertex_decl(ALLEGRO_VERTEX_DECL* decl)
{
   if (!decl)
      return;
   al_free(decl->elements);
   /*
    * TODO: Somehow free the d3d_decl
    */
   al_free(decl);
}

/* Function: al_create_vertex_buffer
 */
ALLEGRO_VERTEX_BUFFER* al_create_vertex_buffer(ALLEGRO_VERTEX_DECL* decl,
   const void* initial_data, int num_vertices, int flags)
{
   ALLEGRO_VERTEX_BUFFER* ret;
   int display_flags = al_get_display_flags(al_get_current_display());
   ASSERT(addon_initialized);
   ret = al_calloc(1, sizeof(ALLEGRO_VERTEX_BUFFER));
   ret->common.size = num_vertices;
   ret->common.write_only = !(flags & ALLEGRO_PRIM_BUFFER_READWRITE);
   ret->decl = decl;

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   if (flags & ALLEGRO_PRIM_BUFFER_READWRITE)
      goto fail;
#endif

   if (display_flags & ALLEGRO_OPENGL) {
      if (_al_create_vertex_buffer_opengl(ret, initial_data, num_vertices, flags))
         return ret;
   }
   else if (display_flags & ALLEGRO_DIRECT3D) {
      if (_al_create_vertex_buffer_directx(ret, initial_data, num_vertices, flags))
         return ret;
   }

   /* Silence the warning */
   goto fail;
fail:
   al_free(ret);
   return 0;
}

/* Function: al_create_index_buffer
 */
ALLEGRO_INDEX_BUFFER* al_create_index_buffer(int index_size,
    const void* initial_data, int num_indices, int flags)
{
   ALLEGRO_INDEX_BUFFER* ret;
   int display_flags = al_get_display_flags(al_get_current_display());
   ASSERT(addon_initialized);
   ASSERT(index_size == 2 || index_size == 4);
   ret = al_calloc(1, sizeof(ALLEGRO_INDEX_BUFFER));
   ret->common.size = num_indices;
   ret->common.write_only = !(flags & ALLEGRO_PRIM_BUFFER_READWRITE);
   ret->index_size = index_size;

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   if (flags & ALLEGRO_PRIM_BUFFER_READWRITE)
      goto fail;
#endif

#if defined ALLEGRO_IPHONE
   if (index_size == 4)
      goto fail;
#endif

   if (display_flags & ALLEGRO_OPENGL) {
      if (_al_create_index_buffer_opengl(ret, initial_data, num_indices, flags))
         return ret;
   }
   else if (display_flags & ALLEGRO_DIRECT3D) {
      if (_al_create_index_buffer_directx(ret, initial_data, num_indices, flags))
         return ret;
   }

   /* Silence the warning */
   goto fail;
fail:
   al_free(ret);
   return NULL;
}

/* Function: al_destroy_vertex_buffer
 */
void al_destroy_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   int flags = al_get_display_flags(al_get_current_display());
   ASSERT(addon_initialized);

   if (buffer == 0)
      return;

   al_unlock_vertex_buffer(buffer);

   if (flags & ALLEGRO_OPENGL) {
      _al_destroy_vertex_buffer_opengl(buffer);
   }
   else if (flags & ALLEGRO_DIRECT3D) {
      _al_destroy_vertex_buffer_directx(buffer);
   }

   al_free(buffer);
}

/* Function: al_destroy_index_buffer
 */
void al_destroy_index_buffer(ALLEGRO_INDEX_BUFFER* buffer)
{
   int flags = al_get_display_flags(al_get_current_display());
   ASSERT(addon_initialized);

   if (buffer == 0)
      return;

   al_unlock_index_buffer(buffer);

   if (flags & ALLEGRO_OPENGL) {
      _al_destroy_index_buffer_opengl(buffer);
   }
   else if (flags & ALLEGRO_DIRECT3D) {
      _al_destroy_index_buffer_directx(buffer);
   }

   al_free(buffer);
}

/* The sizes are in bytes here */
static bool lock_buffer_common(ALLEGRO_BUFFER_COMMON* common, int offset, int length, int flags)
{
   if (common->is_locked || (common->write_only && flags != ALLEGRO_LOCK_WRITEONLY))
      return false;

   common->lock_offset = offset;
   common->lock_length = length;
   common->lock_flags = flags;
   common->is_locked = true;
   return true;
}

/* Function: al_lock_vertex_buffer
 */
void* al_lock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer, int offset,
   int length, int flags)
{
   int stride;
   int disp_flags = al_get_display_flags(al_get_current_display());
   ASSERT(buffer);
   ASSERT(addon_initialized);

   if (offset + length > buffer->common.size)
      return NULL;

   stride = buffer->decl ? buffer->decl->stride : (int)sizeof(ALLEGRO_VERTEX);

   if (!lock_buffer_common(&buffer->common, offset * stride, length * stride, flags))
      return NULL;

   if (disp_flags & ALLEGRO_OPENGL) {
      return _al_lock_vertex_buffer_opengl(buffer);
   }
   else if (disp_flags & ALLEGRO_DIRECT3D) {
      return _al_lock_vertex_buffer_directx(buffer);
   }
   else {
      return NULL;
   }
}

/* Function: al_lock_index_buffer
 */
void* al_lock_index_buffer(ALLEGRO_INDEX_BUFFER* buffer, int offset,
    int length, int flags)
{
   int disp_flags = al_get_display_flags(al_get_current_display());
   ASSERT(buffer);
   ASSERT(addon_initialized);

   if (offset + length > buffer->common.size)
      return NULL;

   if (!lock_buffer_common(&buffer->common, offset * buffer->index_size, length * buffer->index_size, flags))
      return NULL;

   if (disp_flags & ALLEGRO_OPENGL) {
      return _al_lock_index_buffer_opengl(buffer);
   }
   else if (disp_flags & ALLEGRO_DIRECT3D) {
      return _al_lock_index_buffer_directx(buffer);
   }
   else {
      return NULL;
   }
}

/* Function: al_unlock_vertex_buffer
 */
void al_unlock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   int flags = al_get_display_flags(al_get_current_display());
   ASSERT(buffer);
   ASSERT(addon_initialized);

   if (!buffer->common.is_locked)
      return;

   buffer->common.is_locked = false;

   if (flags & ALLEGRO_OPENGL) {
      _al_unlock_vertex_buffer_opengl(buffer);
   }
   else if (flags & ALLEGRO_DIRECT3D) {
      _al_unlock_vertex_buffer_directx(buffer);
   }
}

/* Function: al_unlock_index_buffer
 */
void al_unlock_index_buffer(ALLEGRO_INDEX_BUFFER* buffer)
{
	int flags = al_get_display_flags(al_get_current_display());
   ASSERT(buffer);
   ASSERT(addon_initialized);

   if (!buffer->common.is_locked)
      return;

   buffer->common.is_locked = false;

   if (flags & ALLEGRO_OPENGL) {
      _al_unlock_index_buffer_opengl(buffer);
   }
   else if (flags & ALLEGRO_DIRECT3D) {
      _al_unlock_index_buffer_directx(buffer);
   }
}

/* Software fallback for buffer drawing */
int _al_draw_buffer_common_soft(ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
   void* vtx;
   int num_primitives = 0;
   int num_vtx = end - start;
   int vtx_lock_start = index_buffer ? 0 : start;
   int vtx_lock_len = index_buffer ? al_get_vertex_buffer_size(vertex_buffer) : num_vtx;
   if (vertex_buffer->common.write_only || (index_buffer && index_buffer->common.write_only)) {
      return 0;
   }

   vtx = al_lock_vertex_buffer(vertex_buffer, vtx_lock_start, vtx_lock_len, ALLEGRO_LOCK_READONLY);
   ASSERT(vtx);

   if (index_buffer) {
      void* idx;
      int* int_idx = NULL;
      int ii;

      idx = al_lock_index_buffer(index_buffer, start, num_vtx, ALLEGRO_LOCK_READONLY);
      ASSERT(idx);

      if (index_buffer->index_size != 4) {
         int_idx = al_malloc(num_vtx * sizeof(int));
         for (ii = 0; ii < num_vtx; ii++) {
            int_idx[ii] = ((unsigned short*)idx)[ii];
         }
         idx = int_idx;
      }

      num_primitives = _al_draw_prim_indexed_soft(texture, vtx, vertex_buffer->decl, idx, num_vtx, type);

      al_unlock_index_buffer(index_buffer);
      al_free(int_idx);
   }
   else {
      num_primitives = _al_draw_prim_soft(texture, vtx, vertex_buffer->decl, 0, num_vtx, type);
   }

   al_unlock_vertex_buffer(vertex_buffer);
   return num_primitives;
}

/* Function: al_draw_vertex_buffer
 */
int al_draw_vertex_buffer(ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;

   ASSERT(addon_initialized);
   ASSERT(end >= start);
   ASSERT(start >= 0);
   ASSERT(end <= al_get_vertex_buffer_size(vertex_buffer));
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);
   ASSERT(vertex_buffer);
   ASSERT(!vertex_buffer->common.is_locked);

   target = al_get_target_bitmap();

   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret = _al_draw_buffer_common_soft(vertex_buffer, texture, NULL, start, end, type);
   } else {
      int flags = al_get_display_flags(al_get_current_display());
      if (flags & ALLEGRO_OPENGL) {
         ret = _al_draw_vertex_buffer_opengl(target, texture, vertex_buffer, start, end, type);
      }
      else if (flags & ALLEGRO_DIRECT3D) {
         ret = _al_draw_vertex_buffer_directx(target, texture, vertex_buffer, start, end, type);
      }
   }

   return ret;
}

/* Function: al_draw_indexed_buffer
 */
int al_draw_indexed_buffer(ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer,
   int start, int end, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;

   ASSERT(addon_initialized);
   ASSERT(end >= start);
   ASSERT(start >= 0);
   ASSERT(end <= al_get_index_buffer_size(index_buffer));
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);
   ASSERT(vertex_buffer);
   ASSERT(!vertex_buffer->common.is_locked);
   ASSERT(index_buffer);
   ASSERT(!index_buffer->common.is_locked);

   target = al_get_target_bitmap();

   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret = _al_draw_buffer_common_soft(vertex_buffer, texture, index_buffer, start, end, type);
   } else {
      int flags = al_get_display_flags(al_get_current_display());
      if (flags & ALLEGRO_OPENGL) {
         ret = _al_draw_indexed_buffer_opengl(target, texture, vertex_buffer, index_buffer, start, end, type);
      }
      else if (flags & ALLEGRO_DIRECT3D) {
         ret = _al_draw_indexed_buffer_directx(target, texture, vertex_buffer, index_buffer, start, end, type);
      }
   }

   return ret;
}

/* Function: al_get_vertex_buffer_size
 */
int al_get_vertex_buffer_size(ALLEGRO_VERTEX_BUFFER* buffer)
{
   ASSERT(buffer);
   return buffer->common.size;
}

/* Function: al_get_index_buffer_size
 */
int al_get_index_buffer_size(ALLEGRO_INDEX_BUFFER* buffer)
{
   ASSERT(buffer);
   return buffer->common.size;
}
