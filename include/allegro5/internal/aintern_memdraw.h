#ifndef __al_included_allegro5_aintern_memdraw_h
#define __al_included_allegro5_aintern_memdraw_h

#ifdef __cplusplus
   extern "C" {
#endif


void _al_clear_bitmap_by_locking(A5O_BITMAP *bitmap, A5O_COLOR *color);
void _al_draw_pixel_memory(A5O_BITMAP *bmp, float x, float y, A5O_COLOR *color);


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
