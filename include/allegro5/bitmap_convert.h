#ifndef __al_included_allegro5_bitmap_convert_h
#define __al_included_allegro5_bitmap_convert_h

#include "allegro5/bitmap.h"

#ifdef __cplusplus
   extern "C" {
#endif


AL_FUNC(ALLEGRO_BITMAP *, al_clone_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(void, al_convert_bitmap, (ALLEGRO_BITMAP *bitmap));
AL_FUNC(void, al_convert_bitmaps, (void));


#ifdef __cplusplus
   }
#endif

#endif
