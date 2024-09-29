#ifndef __al_included_allegro5_aintern_prim_soft_h
#define __al_included_allegro5_aintern_prim_soft_h

struct A5O_BITMAP;
struct A5O_VERTEX;
enum A5O_BITMAP_WRAP;

#ifdef __cplusplus
extern "C" {
#endif

int _al_fix_texcoord(float var, int max_var, A5O_BITMAP_WRAP wrap);
int _al_draw_prim_soft(A5O_BITMAP* texture, const void* vtxs, const A5O_VERTEX_DECL* decl, int start, int end, int type);
int _al_draw_prim_indexed_soft(A5O_BITMAP* texture, const void* vtxs, const A5O_VERTEX_DECL* decl, const int* indices, int num_vtx, int type);

void _al_line_2d(A5O_BITMAP* texture, A5O_VERTEX* v1, A5O_VERTEX* v2);
void _al_point_2d(A5O_BITMAP* texture, A5O_VERTEX* v);

#ifdef __cplusplus
}
#endif

#endif
