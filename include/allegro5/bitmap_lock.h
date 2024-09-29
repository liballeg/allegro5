#ifndef __al_included_allegro5_bitmap_lock_h
#define __al_included_allegro5_bitmap_lock_h

#include "allegro5/bitmap.h"

#ifdef __cplusplus
   extern "C" {
#endif


/*
 * Locking flags
 */
enum {
   A5O_LOCK_READWRITE  = 0,
   A5O_LOCK_READONLY   = 1,
   A5O_LOCK_WRITEONLY  = 2
};


/* Type: A5O_LOCKED_REGION
 */
typedef struct A5O_LOCKED_REGION A5O_LOCKED_REGION;
struct A5O_LOCKED_REGION {
   void *data;
   int format;
   int pitch;
   int pixel_size;
};


AL_FUNC(A5O_LOCKED_REGION*, al_lock_bitmap, (A5O_BITMAP *bitmap, int format, int flags));
AL_FUNC(A5O_LOCKED_REGION*, al_lock_bitmap_region, (A5O_BITMAP *bitmap, int x, int y, int width, int height, int format, int flags));
AL_FUNC(A5O_LOCKED_REGION*, al_lock_bitmap_blocked, (A5O_BITMAP *bitmap, int flags));
AL_FUNC(A5O_LOCKED_REGION*, al_lock_bitmap_region_blocked, (A5O_BITMAP *bitmap, int x_block, int y_block,
      int width_block, int height_block, int flags));
AL_FUNC(void, al_unlock_bitmap, (A5O_BITMAP *bitmap));
AL_FUNC(bool, al_is_bitmap_locked, (A5O_BITMAP *bitmap));


#ifdef __cplusplus
   }
#endif

#endif
