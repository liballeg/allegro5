#ifndef ALLEGRO_INTERNAL_BITMAP_NEW_H
#define ALLEGRO_INTERNAL_BITMAP_NEW_H

#include "allegro/display_new.h"
#include "allegro/internal/aintern_display.h"

typedef struct AL_BITMAP_INTERFACE AL_BITMAP_INTERFACE;

struct BITMAP;

struct AL_BITMAP
{
   AL_BITMAP_INTERFACE *vt;
   AL_DISPLAY *display;
   int format;
   int flags;
   int w, h;
   AL_COLOR light_color;  /* color to tint to when drawing with AL_LIT */
   bool locked;
   int lock_x;
   int lock_y;
   int lock_width;
   int lock_height;
   int lock_flags;

   /* A memory copy of the bitmap data. May be NULL for an empty bitmap. */
   unsigned char *memory;

   /* TODO: Is it needed? */
   /*unsigned char *palette;*/
};

struct AL_BITMAP_INTERFACE
{
   int id;
   void (*blit)(int flag, struct AL_BITMAP *src, struct AL_BITMAP *dest,
   	float dx, float dy);
   void (*blit_region)(int flag, struct AL_BITMAP *src, float sx, float sy,
	struct AL_BITMAP *dest, float dx, float dy, float w, float h);
   void (*blit_scaled)(int flag,
   	struct AL_BITMAP *src,	float sx, float sy, float sw, float sh,
	struct AL_BITMAP *dest, float dx, float dy, float dw, float dh);
   void (*rotate_bitmap)(int flag, struct AL_BITMAP *source,
	float source_center_x, float source_center_y,
	struct AL_BITMAP *dest, float dest_x, float dest_y,
	float angle);
   void (*rotate_scaled)(int flag, struct AL_BITMAP *source,
	float source_center_x, float source_center_y,
	struct AL_BITMAP *dest, float dest_x, float dest_y,
	float xscale, float yscale,
	float angle);
   void (*blit_region_3)(int flag,
	struct AL_BITMAP *source1, int source1_x, int source1_y,
	struct AL_BITMAP *source2, int source2_x, int source2_y,
	struct AL_BITMAP *dest, int dest_x, int dest_y,
	int dest_w, int dest_h);
//   void (*draw_bitmap)(int flag, struct AL_BITMAP *bitmap, float x, float y);
   void (*draw_sub)(struct AL_BITMAP *bitmap, float x, float y,
      float sx, float sy, float sw, float sh);
   /* After the memory-copy of the bitmap has been modified, need to call this
    * to update the display-specific copy. E.g. with an OpenGL driver, this
    * might create/update a texture.
    */
   void (*upload_bitmap)(struct AL_BITMAP *bitmap, int x, int y, int width, int height);
   /* If the display version of the bitmap has been modified, use this to update
    * the memory copy accordingly. E.g. with an OpenGL driver, this might
    * read the contents of an associated texture.
    */
   /*
    * This is used by the compatibility layer after a BITMAP has
    * been changed.
    */
   //void (*upload_compat_bitmap)(struct BITMAP *bitmap, int x, int y, int width, int height);
   void (*download_bitmap)(struct AL_BITMAP *bitmap);
   /* Destroy any driver specific stuff. The struct AL_BITMAP and its memory copy
    * itself should not be touched.
    */
   void (*destroy_bitmap)(struct AL_BITMAP *bitmap);

   /*
    * Make this into the "screen" bitmap (for compatibility layer)
    */
   //void (*make_compat_screen)(struct AL_BITMAP *bitmap);
   AL_LOCKED_RECTANGLE * (*lock_region)(struct AL_BITMAP *bitmap,
   	int x, int y, int w, int h,
   	AL_LOCKED_RECTANGLE *locked_rectangle,
	int flags);

   void (*unlock_region)(struct AL_BITMAP *bitmap);
};

void _al_blit_memory_bitmap(struct AL_BITMAP *source, struct AL_BITMAP *dest,
   int source_x, int source_y, int dest_x, int dest_y, int w, int h);
//AL_BITMAP_INTERFACE *_al_bitmap_xdummy_driver(void);
AL_BITMAP_INTERFACE *_al_bitmap_d3ddummy_driver(void);

#endif
