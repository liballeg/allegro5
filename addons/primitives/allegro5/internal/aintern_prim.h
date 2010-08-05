#ifndef __al_included_allegro5_aintern_prim_h
#define __al_included_allegro5_aintern_prim_h

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int x2, int y2);

struct ALLEGRO_VERTEX_DECL {
   ALLEGRO_VERTEX_ELEMENT* elements;
   int stride;
   void* d3d_decl;
   void* d3d_dummy_shader;
};

#endif
