#ifndef __al_included_allegro5_aintern_prim_opengl_h
#define __al_included_allegro5_aintern_prim_opengl_h

int _al_draw_prim_opengl(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_opengl(ALLEGRO_BITMAP *target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);
bool _al_create_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int usage_hints);
void _al_destroy_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf);
void* _al_lock_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf);
void _al_unlock_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf);
int _al_draw_vertex_buffer_opengl(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type);

#endif
