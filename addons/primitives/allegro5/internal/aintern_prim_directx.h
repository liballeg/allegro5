#ifndef __al_included_allegro5_aintern_prim_directx_h
#define __al_included_allegro5_aintern_prim_directx_h

struct A5O_BITMAP;
struct A5O_VERTEX;

#ifdef __cplusplus
extern "C" {
#endif

int _al_draw_prim_directx(A5O_BITMAP* target, A5O_BITMAP* texture, const void* vtxs, const A5O_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_directx(A5O_BITMAP* target, A5O_BITMAP* texture, const void* vtxs, const A5O_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);
void _al_set_d3d_decl(A5O_DISPLAY* display, A5O_VERTEX_DECL* ret);

bool _al_create_vertex_buffer_directx(A5O_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int flags);
void _al_destroy_vertex_buffer_directx(A5O_VERTEX_BUFFER* buf);
void* _al_lock_vertex_buffer_directx(A5O_VERTEX_BUFFER* buf);
void _al_unlock_vertex_buffer_directx(A5O_VERTEX_BUFFER* buf);

bool _al_create_index_buffer_directx(A5O_INDEX_BUFFER* buf, const void* initial_data, size_t num_indices, int flags);
void _al_destroy_index_buffer_directx(A5O_INDEX_BUFFER* buf);
void* _al_lock_index_buffer_directx(A5O_INDEX_BUFFER* buf);
void _al_unlock_index_buffer_directx(A5O_INDEX_BUFFER* buf);

int _al_draw_vertex_buffer_directx(A5O_BITMAP* target, A5O_BITMAP* texture, A5O_VERTEX_BUFFER* vertex_buffer, int start, int end, int type);
int _al_draw_indexed_buffer_directx(A5O_BITMAP* target, A5O_BITMAP* texture, A5O_VERTEX_BUFFER* vertex_buffer, A5O_INDEX_BUFFER* index_buffer, int start, int end, int type);

bool _al_init_d3d_driver(void);
void _al_shutdown_d3d_driver(void);

void* _al_create_default_primitives_shader(void* dev);
void _al_setup_default_primitives_shader(void* dev, void* shader);

void _al_setup_primitives_shader(void* dev, const A5O_VERTEX_DECL* decl);
void _al_create_primitives_shader(void* dev, A5O_VERTEX_DECL* decl);
void _al_set_texture_matrix(void* dev, float* mat);

#ifdef __cplusplus
}
#endif

#endif
