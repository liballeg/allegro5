#ifndef __al_included_allegro5_aintern_prim_directx_h
#define __al_included_allegro5_aintern_prim_directx_h

struct ALLEGRO_BITMAP;
struct ALLEGRO_VERTEX;

#ifdef __cplusplus
extern "C" {
#endif

int _al_draw_prim_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);
void _al_set_d3d_decl(ALLEGRO_DISPLAY* display, ALLEGRO_VERTEX_DECL* ret);

bool _al_create_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int usage_hints);
void _al_destroy_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf);
void* _al_lock_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf);
void _al_unlock_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf);
int _al_draw_vertex_buffer_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type);

bool _al_init_d3d_driver(void);
void _al_shutdown_d3d_driver(void);

void* _al_create_default_shader(void* dev);
void _al_setup_default_shader(void* dev, void* shader);

void _al_setup_shader(void* dev, const ALLEGRO_VERTEX_DECL* decl);
void _al_create_shader(void* dev, ALLEGRO_VERTEX_DECL* decl);
void _al_set_texture_matrix(void* dev, float* mat);

#ifdef __cplusplus
}
#endif

#endif
