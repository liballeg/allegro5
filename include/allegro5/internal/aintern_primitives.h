#ifndef __al_included_allegro5_aintern_primitives_h
#define __al_included_allegro5_aintern_primitives_h

#include "allegro5/color.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_primitives_types.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct ALLEGRO_VERTEX_DECL {
   ALLEGRO_VERTEX_ELEMENT* elements;
   int stride;
   void* d3d_decl;
   void* d3d_dummy_shader;
};

typedef struct _AL_BUFFER_COMMON {
   uintptr_t handle;
   bool write_only;
   /* In elements */
   int size;

   bool is_locked;
   int lock_flags;
   void* locked_memory;
    /* These three are in bytes */
   int local_buffer_length;
   int lock_offset;
   int lock_length;
} _AL_BUFFER_COMMON;

struct ALLEGRO_VERTEX_BUFFER {
   ALLEGRO_VERTEX_DECL* decl;
   _AL_BUFFER_COMMON common;
};

struct ALLEGRO_INDEX_BUFFER {
   int index_size;
   _AL_BUFFER_COMMON common;
};

/*
* Primary Functions
*/
AL_FUNC(int, _al_draw_prim, (const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture, int start, int end, int type));
AL_FUNC(int, _al_draw_indexed_prim, (const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture, const int* indices, int num_vtx, int type));
AL_FUNC(int, _al_draw_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, int start, int end, int type));
AL_FUNC(int, _al_draw_indexed_buffer, (ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type));

AL_FUNC(ALLEGRO_VERTEX_DECL*, _al_create_vertex_decl, (const ALLEGRO_VERTEX_ELEMENT* elements, int stride));
AL_FUNC(void, _al_destroy_vertex_decl, (ALLEGRO_VERTEX_DECL* decl));

/*
 * Vertex buffers
 */
AL_FUNC(ALLEGRO_VERTEX_BUFFER*, _al_create_vertex_buffer, (ALLEGRO_VERTEX_DECL* decl, const void* initial_data, int num_vertices, int flags));
AL_FUNC(void, _al_destroy_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* buffer));
AL_FUNC(void*, _al_lock_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* buffer, int offset, int length, int flags));
AL_FUNC(void, _al_unlock_vertex_buffer, (ALLEGRO_VERTEX_BUFFER* buffer));
AL_FUNC(int, _al_get_vertex_buffer_size, (ALLEGRO_VERTEX_BUFFER* buffer));

/*
 * Index buffers
 */
AL_FUNC(ALLEGRO_INDEX_BUFFER*, _al_create_index_buffer, (int index_size, const void* initial_data, int num_indices, int flags));
AL_FUNC(void, _al_destroy_index_buffer, (ALLEGRO_INDEX_BUFFER* buffer));
AL_FUNC(void*, _al_lock_index_buffer, (ALLEGRO_INDEX_BUFFER* buffer, int offset, int length, int flags));
AL_FUNC(void, _al_unlock_index_buffer, (ALLEGRO_INDEX_BUFFER* buffer));
AL_FUNC(int, _al_get_index_buffer_size, (ALLEGRO_INDEX_BUFFER* buffer));

AL_FUNC(int, _al_draw_buffer_common_soft, (ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_BITMAP* texture, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type));

#ifdef __cplusplus
   }
#endif

#endif
