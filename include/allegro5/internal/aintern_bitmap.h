#ifndef __al_included_allegro5_aintern_bitmap_h
#define __al_included_allegro5_aintern_bitmap_h

#include "allegro5/bitmap.h"
#include "allegro5/bitmap_lock.h"
#include "allegro5/display.h"
#include "allegro5/transformations.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_BITMAP_INTERFACE ALLEGRO_BITMAP_INTERFACE;

struct ALLEGRO_BITMAP
{
   ALLEGRO_BITMAP_INTERFACE *vt;
   ALLEGRO_DISPLAY *display;
   int format;
   int flags;
   int w, h;
   /*
    * The number of bytes between a pixel at (x,y) and (x,y+1).
    * This is larger than w * pixel_size if there is padding between lines.
    */
   int pitch;
   /* 
    * clip left, right, top, bottom
    * Clip anything outside of this. cr/cb are exclusive, that is (0, 0, 1, 1)
    * is the single pixel spawning a rectangle from floating point 0/0 to 1/1 -
    * or in other words, the single pixel 0/0.
    *
    * There is always confusion as to whether cr/cb are exclusive, leading to
    * subtle bugs.  The suffixes are supposed to help with that.
    */
   int cl;
   int cr_excl;
   int ct;
   int cb_excl;
   /*
    * Locking info.
    *
    * locked - locked or not?
    * lock_x/y - top left of the locked region
    * lock_w/h - width and height of the locked region
    * lock_flags - flags the region was locked with
    * locked_region - a copy of the locked rectangle
    */
   bool locked;
   int lock_x;
   int lock_y;
   int lock_w;
   int lock_h;
   int lock_flags;
   ALLEGRO_LOCKED_REGION locked_region;

   /* Transformation for this bitmap */
   ALLEGRO_TRANSFORM transform;

   /* Info for sub-bitmaps */
   ALLEGRO_BITMAP *parent;
   int xofs;
   int yofs;

   /* A memory copy of the bitmap data. May be NULL for an empty bitmap. */
   unsigned char *memory;

   /* Size of the bitmap object. Used only by functions to convert bitmap
      storage type. Can be missleading. */
   size_t size;

   bool preserve_texture;
};

struct ALLEGRO_BITMAP_INTERFACE
{
   int id;

   void (*draw_bitmap_region)(ALLEGRO_BITMAP *bitmap,
      ALLEGRO_COLOR tint,float sx, float sy,
      float sw, float sh, int flags);

   /* After the memory-copy of the bitmap has been modified, need to call this
    * to update the display-specific copy. E.g. with an OpenGL driver, this
    * might create/update a texture. Returns false on failure.
    */
   bool (*upload_bitmap)(ALLEGRO_BITMAP *bitmap);
   /* If the display version of the bitmap has been modified, use this to update
    * the memory copy accordingly. E.g. with an OpenGL driver, this might
    * read the contents of an associated texture.
    */

   void (*update_clipping_rectangle)(ALLEGRO_BITMAP *bitmap);

   void (*destroy_bitmap)(ALLEGRO_BITMAP *bitmap);

   ALLEGRO_LOCKED_REGION * (*lock_region)(ALLEGRO_BITMAP *bitmap,
   	int x, int y, int w, int h, int format,
	int flags);

   void (*unlock_region)(ALLEGRO_BITMAP *bitmap);
};

extern void (*_al_convert_funcs[ALLEGRO_NUM_PIXEL_FORMATS]
   [ALLEGRO_NUM_PIXEL_FORMATS])(void *, int, void *, int,
   int, int, int, int, int, int);

/* Bitmap conversion */
void _al_convert_bitmap_data(
	void *src, int src_format, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
void _al_convert_to_memory_bitmap(ALLEGRO_BITMAP *bitmap);
void _al_convert_to_display_bitmap(ALLEGRO_BITMAP *bitmap);

#ifdef ALLEGRO_GP2XWIZ
/* Optimized blitters */
void _al_draw_bitmap_region_optimized_rgba_4444_to_rgb_565(
   ALLEGRO_BITMAP *src, int sx, int sy, int sw, int sh,
   ALLEGRO_BITMAP *dest, int dx, int dy, int flags);
void _al_draw_bitmap_region_optimized_rgb_565_to_rgb_565(
   ALLEGRO_BITMAP *src, int sx, int sy, int sw, int sh,
   ALLEGRO_BITMAP *dest, int dx, int dy, int flags);
void _al_draw_bitmap_region_optimized_rgba_4444_to_rgba_4444(
   ALLEGRO_BITMAP *src, int sx, int sy, int sw, int sh,
   ALLEGRO_BITMAP *dest, int dx, int dy, int flags);
#endif

/* Simple bitmap drawing */
/* _al_put_pixel was inadvertently exported in 5.0.x releases. */
AL_FUNC(void, _al_put_pixel, (ALLEGRO_BITMAP *bitmap, int x, int y, ALLEGRO_COLOR color));

/* Bitmap I/O */
void _al_init_iio_table(void);

#ifdef __cplusplus
}
#endif

#endif
