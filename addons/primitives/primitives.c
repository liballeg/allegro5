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

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern.h"
#include <math.h>

#ifdef ALLEGRO_CFG_OPENGL
#include "allegro5/allegro_opengl.h"
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
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);

   target = al_get_target_bitmap();

   /* In theory, if we ever get a camera concept for this addon, the transformation into
    * view space should occur here
    */
   
   if (target->flags & ALLEGRO_MEMORY_BITMAP || (texture && texture->flags & ALLEGRO_MEMORY_BITMAP)) {
      ret =  _al_draw_prim_soft(texture, vtxs, decl, start, end, type);
   } else {
      int flags = al_get_display_flags(target->display);
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
   
   if (target->flags & ALLEGRO_MEMORY_BITMAP || (texture && texture->flags & ALLEGRO_MEMORY_BITMAP)) {
      ret =  _al_draw_prim_indexed_soft(texture, vtxs, decl, indices, num_vtx, type);
   } else {
      int flags = al_get_display_flags(target->display);
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
   int flags;

   ASSERT(addon_initialized);

   ret = al_malloc(sizeof(ALLEGRO_VERTEX_DECL));
   ret->elements = al_calloc(1, sizeof(ALLEGRO_VERTEX_ELEMENT) * ALLEGRO_PRIM_ATTR_NUM);
   while(elements->attribute) {
      ret->elements[elements->attribute] = *elements;
      elements++;
   }
   
   display = al_get_current_display();
   flags = al_get_display_flags(display);
   if (flags & ALLEGRO_DIRECT3D) {
      _al_set_d3d_decl(display, ret);
   }
   
   ret->stride = stride;
   return ret;
}

/* Function: al_destroy_vertex_decl
 */
void al_destroy_vertex_decl(ALLEGRO_VERTEX_DECL* decl)
{
   al_free(decl->elements);
   /*
    * TODO: Somehow free the d3d_decl
    */
   al_free(decl);
}

/* Function: al_create_vertex_buffer
 */
ALLEGRO_VERTEX_BUFFER* al_create_vertex_buffer(ALLEGRO_VERTEX_DECL* decl,
   const void* initial_data, size_t num_vertices, bool write_only, int hints)
{
   ALLEGRO_VERTEX_BUFFER* ret;
   int flags = al_get_display_flags(al_get_current_display());
   ASSERT(addon_initialized);
   ret = al_calloc(1, sizeof(ALLEGRO_VERTEX_BUFFER));
   ret->write_only = write_only;
   ret->decl = decl;

#if defined ALLEGRO_IPHONE || defined ALLEGRO_ANDROID
   if (!write_only)
      goto fail;
#endif

   if (flags & ALLEGRO_OPENGL) {
      if (_al_create_vertex_buffer_opengl(ret, initial_data, num_vertices, hints))
         return ret;
   }
   else if (flags & ALLEGRO_DIRECT3D) {
      if (_al_create_vertex_buffer_directx(ret, initial_data, num_vertices, hints))
         return ret;
   }

   /* Silence the warning */
   goto fail;
fail:
   al_free(ret);
   return 0;
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

/* Function: al_lock_vertex_buffer
 */
void* al_lock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer, size_t offset,
   size_t length, int flags)
{
   void* ret;
   int stride;
   int disp_flags = al_get_display_flags(al_get_current_display());
   ASSERT(buffer);
   ASSERT(addon_initialized);
   if (buffer->is_locked || (buffer->write_only && flags != ALLEGRO_LOCK_WRITEONLY))
      return 0;

   stride = buffer->decl ? buffer->decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   buffer->lock_offset = offset * stride;
   buffer->lock_length = length * stride;
   buffer->lock_flags = flags;

   if (disp_flags & ALLEGRO_OPENGL) {
      ret = _al_lock_vertex_buffer_opengl(buffer);
   }
   else if (disp_flags & ALLEGRO_DIRECT3D) {
      ret = _al_lock_vertex_buffer_directx(buffer);
   }
   else {
      ret = NULL;
   }

   buffer->is_locked = true;

   return ret;
}

/* Function: al_unlock_vertex_buffer
 */
void al_unlock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   int flags = al_get_display_flags(al_get_current_display());
   ASSERT(buffer);
   ASSERT(addon_initialized);

   if (!buffer->is_locked)
      return;

   if (flags & ALLEGRO_OPENGL) {
      _al_unlock_vertex_buffer_opengl(buffer);
   }
   else if (flags & ALLEGRO_DIRECT3D) {
      _al_unlock_vertex_buffer_directx(buffer);
   }

   buffer->is_locked = false;
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
   ASSERT(type >= 0 && type < ALLEGRO_PRIM_NUM_TYPES);
   ASSERT(vertex_buffer);
   ASSERT(!vertex_buffer->is_locked);

   target = al_get_target_bitmap();

   if (target->flags & ALLEGRO_MEMORY_BITMAP || (texture && texture->flags & ALLEGRO_MEMORY_BITMAP)) {
      void* vtx;
      ASSERT(!vertex_buffer->write_only);
      vtx = al_lock_vertex_buffer(vertex_buffer, start, end, ALLEGRO_LOCK_READONLY);
      ASSERT(vtx);
      ret = _al_draw_prim_soft(texture, vtx, vertex_buffer->decl, 0, end - start, type);
      al_unlock_vertex_buffer(vertex_buffer);
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
