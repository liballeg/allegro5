#ifndef AINTERN_PRIM_H
#define AINTERN_PRIM_H

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int x2, int y2);

struct ALLEGRO_VERTEX_DECL {
   ALLEGRO_VERTEX_ELEMENT* elements;
   int stride;
   void* d3d_decl;
};

#endif
