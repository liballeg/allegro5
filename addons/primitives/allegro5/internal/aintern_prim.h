#ifndef AINTERN_PRIM_H
#define AINTERN_PRIM_H

int _al_bitmap_region_is_locked(ALLEGRO_BITMAP* bmp, int x1, int y1, int x2, int y2);

struct ALLEGRO_VBUFFER {
   void* data;
   void* priv;
   int lock_start;
   int lock_end;
   int len;
   int flags;
};

#endif
