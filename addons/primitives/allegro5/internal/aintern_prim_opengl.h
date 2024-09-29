#ifndef __al_included_allegro5_aintern_prim_opengl_h
#define __al_included_allegro5_aintern_prim_opengl_h

int _al_draw_prim_opengl(A5O_BITMAP* target, A5O_BITMAP* texture, const void* vtxs, const A5O_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_opengl(A5O_BITMAP *target, A5O_BITMAP* texture, const void* vtxs, const A5O_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);

bool _al_create_vertex_buffer_opengl(A5O_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int flags);
void _al_destroy_vertex_buffer_opengl(A5O_VERTEX_BUFFER* buf);
void* _al_lock_vertex_buffer_opengl(A5O_VERTEX_BUFFER* buf);
void _al_unlock_vertex_buffer_opengl(A5O_VERTEX_BUFFER* buf);

bool _al_create_index_buffer_opengl(A5O_INDEX_BUFFER* buf, const void* initial_data, size_t num_indices, int flags);
void _al_destroy_index_buffer_opengl(A5O_INDEX_BUFFER* buf);
void* _al_lock_index_buffer_opengl(A5O_INDEX_BUFFER* buf);
void _al_unlock_index_buffer_opengl(A5O_INDEX_BUFFER* buf);

int _al_draw_vertex_buffer_opengl(A5O_BITMAP* target, A5O_BITMAP* texture, A5O_VERTEX_BUFFER* vertex_buffer, int start, int end, int type);
int _al_draw_indexed_buffer_opengl(A5O_BITMAP* target, A5O_BITMAP* texture, A5O_VERTEX_BUFFER* vertex_buffer, A5O_INDEX_BUFFER* index_buffer, int start, int end, int type);

#endif
