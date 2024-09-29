#ifndef __al_included_allegro5_bitmap_draw_h
#define __al_included_allegro5_bitmap_draw_h

#include "allegro5/bitmap.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Flags for the blitting functions */
enum {
   A5O_FLIP_HORIZONTAL = 0x00001,
   A5O_FLIP_VERTICAL   = 0x00002
};

/* Blitting */
AL_FUNC(void, al_draw_bitmap, (A5O_BITMAP *bitmap, float dx, float dy, int flags));
AL_FUNC(void, al_draw_bitmap_region, (A5O_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, int flags));
AL_FUNC(void, al_draw_scaled_bitmap, (A5O_BITMAP *bitmap, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags));
AL_FUNC(void, al_draw_rotated_bitmap, (A5O_BITMAP *bitmap, float cx, float cy, float dx, float dy, float angle, int flags));
AL_FUNC(void, al_draw_scaled_rotated_bitmap, (A5O_BITMAP *bitmap, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags));

/* Tinted blitting */
AL_FUNC(void, al_draw_tinted_bitmap, (A5O_BITMAP *bitmap, A5O_COLOR tint, float dx, float dy, int flags));
AL_FUNC(void, al_draw_tinted_bitmap_region, (A5O_BITMAP *bitmap, A5O_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, int flags));
AL_FUNC(void, al_draw_tinted_scaled_bitmap, (A5O_BITMAP *bitmap, A5O_COLOR tint, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh, int flags));
AL_FUNC(void, al_draw_tinted_rotated_bitmap, (A5O_BITMAP *bitmap, A5O_COLOR tint, float cx, float cy, float dx, float dy, float angle, int flags));
AL_FUNC(void, al_draw_tinted_scaled_rotated_bitmap, (A5O_BITMAP *bitmap, A5O_COLOR tint, float cx, float cy, float dx, float dy, float xscale, float yscale, float angle, int flags));
AL_FUNC(void, al_draw_tinted_scaled_rotated_bitmap_region, (
   A5O_BITMAP *bitmap,
   float sx, float sy, float sw, float sh,
   A5O_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags));


#ifdef __cplusplus
   }
#endif

#endif
