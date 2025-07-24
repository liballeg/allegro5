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
#include "allegro5/internal/aintern_prim_addon.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_primitives.h"
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
   addon_initialized = true;
   _al_add_exit_func(al_shutdown_primitives_addon, "primitives_shutdown");
   return true;
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
   addon_initialized = false;
}

/* Function: al_draw_prim
 */
int al_draw_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{
   ASSERT(addon_initialized);
   return _al_draw_prim(vtxs, decl, texture, start, end, type);
}

/* Function: al_draw_indexed_prim
 */
int al_draw_indexed_prim(const void* vtxs, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type)
{
   ASSERT(addon_initialized);
   return _al_draw_indexed_prim(vtxs, decl, texture, indices, num_vtx, type);
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
   ASSERT(addon_initialized);
   return _al_create_vertex_decl(elements, stride);
}

/* Function: al_destroy_vertex_decl
 */
void al_destroy_vertex_decl(ALLEGRO_VERTEX_DECL* decl)
{
   _al_destroy_vertex_decl(decl);
}

/* Function: al_create_vertex_buffer
 */
ALLEGRO_VERTEX_BUFFER* al_create_vertex_buffer(ALLEGRO_VERTEX_DECL* decl,
   const void* initial_data, int num_vertices, int flags)
{
   return _al_create_vertex_buffer(decl, initial_data, num_vertices, flags);
}

/* Function: al_create_index_buffer
 */
ALLEGRO_INDEX_BUFFER* al_create_index_buffer(int index_size,
    const void* initial_data, int num_indices, int flags)
{
   return _al_create_index_buffer(index_size, initial_data, num_indices, flags);
}

/* Function: al_destroy_vertex_buffer
 */
void al_destroy_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   _al_destroy_vertex_buffer(buffer);
}

/* Function: al_destroy_index_buffer
 */
void al_destroy_index_buffer(ALLEGRO_INDEX_BUFFER* buffer)
{
   _al_destroy_index_buffer(buffer);
}

/* Function: al_lock_vertex_buffer
 */
void* al_lock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer, int offset,
   int length, int flags)
{
   ASSERT(addon_initialized);
   return _al_lock_vertex_buffer(buffer, offset, length, flags);
}

/* Function: al_lock_index_buffer
 */
void* al_lock_index_buffer(ALLEGRO_INDEX_BUFFER* buffer, int offset,
    int length, int flags)
{
   ASSERT(addon_initialized);
   return _al_lock_index_buffer(buffer, offset, length, flags);
}

/* Function: al_unlock_vertex_buffer
 */
void al_unlock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buffer)
{
   ASSERT(addon_initialized);
   return _al_unlock_vertex_buffer(buffer);
}

/* Function: al_unlock_index_buffer
 */
void al_unlock_index_buffer(ALLEGRO_INDEX_BUFFER* buffer)
{
   ASSERT(addon_initialized);
   _al_unlock_index_buffer(buffer);
}

/* Function: al_draw_vertex_buffer
 */
int al_draw_vertex_buffer(ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   ALLEGRO_BITMAP* texture, int start, int end, int type)
{
   ASSERT(addon_initialized);
   return _al_draw_vertex_buffer(vertex_buffer, texture, start, end, type);
}

/* Function: al_draw_indexed_buffer
 */
int al_draw_indexed_buffer(ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer,
   int start, int end, int type)
{
   ASSERT(addon_initialized);
   return _al_draw_indexed_buffer(vertex_buffer, texture, index_buffer, start, end, type);
}

/* Function: al_get_vertex_buffer_size
 */
int al_get_vertex_buffer_size(ALLEGRO_VERTEX_BUFFER* buffer)
{
   return _al_get_vertex_buffer_size(buffer);
}

/* Function: al_get_index_buffer_size
 */
int al_get_index_buffer_size(ALLEGRO_INDEX_BUFFER* buffer)
{
   return _al_get_index_buffer_size(buffer);
}
