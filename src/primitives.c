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
 *      Common primitive drawing functions.
 */

#include "allegro5/allegro.h"

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_primitives.h"
#include "allegro5/internal/aintern_prim_soft.h"


ALLEGRO_DEBUG_CHANNEL("primitives")

int _al_draw_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;

   ASSERT(vtxs);
   ASSERT(end >= start);
   ASSERT(start >= 0);
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();

   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret =  _al_draw_prim_soft(texture, vtxs, decl, start, end, type);
   } else {
      ALLEGRO_DISPLAY *disp = al_get_current_display();
      ASSERT(disp);
      ASSERT(disp->vt);
      if (disp->vt->draw_prim)
         ret = disp->vt->draw_prim(target, texture, vtxs, decl, start, end, type);
   }

   return ret;
}


int _al_draw_indexed_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type)
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

   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret =  _al_draw_prim_indexed_soft(texture, vtxs, decl, indices, num_vtx, type);
   } else {
      ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
      ASSERT(disp);
      ASSERT(disp->vt);
      if (disp->vt->draw_prim_indexed)
         ret = disp->vt->draw_prim_indexed(target, texture, vtxs, decl, indices, num_vtx, type);
   }

   return ret;
}


ALLEGRO_VERTEX_DECL* _al_create_vertex_decl(const ALLEGRO_VERTEX_ELEMENT* elements, int stride)
{
   ALLEGRO_VERTEX_DECL* ret;
   ALLEGRO_VERTEX_ELEMENT* e;
   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);

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

   ret->stride = stride;
   if (disp->vt->create_vertex_decl) {
      if (!disp->vt->create_vertex_decl(disp, ret))
         goto fail;
   }
   return ret;
fail:
   al_free(ret->elements);
   al_free(ret);
   return NULL;
}


void _al_destroy_vertex_decl(ALLEGRO_VERTEX_DECL* decl)
{
   if (!decl)
      return;
   al_free(decl->elements);
   /*
    * TODO: Somehow free the d3d_decl
    */
   al_free(decl);
}


ALLEGRO_VERTEX_BUFFER* _al_create_vertex_buffer(ALLEGRO_VERTEX_DECL* decl,
   const void* initial_data, int num_vertices, int flags)
{
   ALLEGRO_VERTEX_BUFFER* ret;
   ret = al_calloc(1, sizeof(ALLEGRO_VERTEX_BUFFER));
   ret->common.size = num_vertices;
   ret->common.write_only = !(flags & ALLEGRO_PRIM_BUFFER_READWRITE);
   ret->decl = decl;

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   if (flags & ALLEGRO_PRIM_BUFFER_READWRITE)
      goto fail;
#endif

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->create_vertex_buffer) {
      if (disp->vt->create_vertex_buffer(ret, initial_data, num_vertices, flags))
         return ret;
   }

   /* Silence the warning */
   goto fail;
fail:
   al_free(ret);
   return 0;
}


ALLEGRO_INDEX_BUFFER* _al_create_index_buffer(int index_size,
    const void* initial_data, int num_indices, int flags)
{
   ALLEGRO_INDEX_BUFFER* ret;
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

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->create_index_buffer) {
      if (disp->vt->create_index_buffer(ret, initial_data, num_indices, flags))
         return ret;
   }

   /* Silence the warning */
   goto fail;
fail:
   al_free(ret);
   return NULL;
}


void _al_destroy_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   if (buffer == 0)
      return;

   _al_unlock_vertex_buffer(buffer);

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->destroy_vertex_buffer)
      disp->vt->destroy_vertex_buffer(buffer);

   al_free(buffer);
}


void _al_destroy_index_buffer(ALLEGRO_INDEX_BUFFER* buffer)
{
   if (buffer == 0)
      return;

   _al_unlock_index_buffer(buffer);

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->destroy_index_buffer)
      disp->vt->destroy_index_buffer(buffer);

   al_free(buffer);
}

/* The sizes are in bytes here */
static bool lock_buffer_common(_AL_BUFFER_COMMON* common, int offset, int length, int flags)
{
   if (common->is_locked || (common->write_only && flags != ALLEGRO_LOCK_WRITEONLY))
      return false;

   common->lock_offset = offset;
   common->lock_length = length;
   common->lock_flags = flags;
   common->is_locked = true;
   return true;
}


void* _al_lock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer, int offset,
   int length, int flags)
{
   int stride;
   ASSERT(buffer);

   if (offset + length > buffer->common.size)
      return NULL;

   stride = buffer->decl ? buffer->decl->stride : (int)sizeof(ALLEGRO_VERTEX);

   if (!lock_buffer_common(&buffer->common, offset * stride, length * stride, flags))
      return NULL;

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->lock_vertex_buffer) {
      return disp->vt->lock_vertex_buffer(buffer);
   }
   else {
      return NULL;
   }
}


void* _al_lock_index_buffer(ALLEGRO_INDEX_BUFFER* buffer, int offset,
    int length, int flags)
{
   ASSERT(buffer);

   if (offset + length > buffer->common.size)
      return NULL;

   if (!lock_buffer_common(&buffer->common, offset * buffer->index_size, length * buffer->index_size, flags))
      return NULL;

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->lock_index_buffer) {
      return disp->vt->lock_index_buffer(buffer);
   }
   else {
      return NULL;
   }
}


void _al_unlock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   ASSERT(buffer);

   if (!buffer->common.is_locked)
      return;

   buffer->common.is_locked = false;

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->unlock_vertex_buffer)
      return disp->vt->unlock_vertex_buffer(buffer);
}


void _al_unlock_index_buffer(ALLEGRO_INDEX_BUFFER* buffer)
{
   ASSERT(buffer);

   if (!buffer->common.is_locked)
      return;

   buffer->common.is_locked = false;

   ALLEGRO_DISPLAY *disp = al_get_current_display();
   ASSERT(disp);
   ASSERT(disp->vt);
   if (disp->vt->unlock_index_buffer)
      return disp->vt->unlock_index_buffer(buffer);
}


/* Software fallback for buffer drawing */
int _al_draw_buffer_common_soft(ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
   void* vtx;
   int num_primitives = 0;
   int num_vtx = end - start;
   int vtx_lock_start = index_buffer ? 0 : start;
   int vtx_lock_len = index_buffer ? _al_get_vertex_buffer_size(vertex_buffer) : num_vtx;
   if (vertex_buffer->common.write_only || (index_buffer && index_buffer->common.write_only)) {
      return 0;
   }

   vtx = _al_lock_vertex_buffer(vertex_buffer, vtx_lock_start, vtx_lock_len, ALLEGRO_LOCK_READONLY);
   ASSERT(vtx);

   if (index_buffer) {
      void* idx;
      int* int_idx = NULL;
      int ii;

      idx = _al_lock_index_buffer(index_buffer, start, num_vtx, ALLEGRO_LOCK_READONLY);
      ASSERT(idx);

      if (index_buffer->index_size != 4) {
         int_idx = al_malloc(num_vtx * sizeof(int));
         for (ii = 0; ii < num_vtx; ii++) {
            int_idx[ii] = ((unsigned short*)idx)[ii];
         }
         idx = int_idx;
      }

      num_primitives = _al_draw_prim_indexed_soft(texture, vtx, vertex_buffer->decl, idx, num_vtx, type);

      _al_unlock_index_buffer(index_buffer);
      al_free(int_idx);
   }
   else {
      num_primitives = _al_draw_prim_soft(texture, vtx, vertex_buffer->decl, 0, num_vtx, type);
   }

   _al_unlock_vertex_buffer(vertex_buffer);
   return num_primitives;
}


int _al_draw_vertex_buffer(ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;

   ASSERT(end >= start);
   ASSERT(start >= 0);
   ASSERT(end <= _al_get_vertex_buffer_size(vertex_buffer));
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);
   ASSERT(vertex_buffer);
   ASSERT(!vertex_buffer->common.is_locked);

   target = al_get_target_bitmap();

   if (al_get_bitmap_flags(target) & ALLEGRO_MEMORY_BITMAP ||
       (texture && al_get_bitmap_flags(texture) & ALLEGRO_MEMORY_BITMAP) ||
       _al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      ret = _al_draw_buffer_common_soft(vertex_buffer, texture, NULL, start, end, type);
   } else {
      ALLEGRO_DISPLAY *disp = al_get_current_display();
      ASSERT(disp);
      ASSERT(disp->vt);
      if (disp->vt->draw_vertex_buffer)
         ret = disp->vt->draw_vertex_buffer(target, texture, vertex_buffer, start, end, type);
   }

   return ret;
}


int _al_draw_indexed_buffer(ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer,
   int start, int end, int type)
{
   ALLEGRO_BITMAP *target;
   int ret = 0;

   ASSERT(end >= start);
   ASSERT(start >= 0);
   ASSERT(end <= _al_get_index_buffer_size(index_buffer));
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
      ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
      ASSERT(disp);
      ASSERT(disp->vt);
      if (disp->vt->draw_indexed_buffer)
         ret = disp->vt->draw_indexed_buffer(target, texture, vertex_buffer, index_buffer, start, end, type);
   }

   return ret;
}


int _al_get_vertex_buffer_size(ALLEGRO_VERTEX_BUFFER* buffer)
{
   ASSERT(buffer);
   return buffer->common.size;
}


int _al_get_index_buffer_size(ALLEGRO_INDEX_BUFFER* buffer)
{
   ASSERT(buffer);
   return buffer->common.size;
}

/* vim: set sts=3 sw=3 et: */
