#ifndef ALLEGRO_INTERNAL_BITMAP_NEW_H
#define ALLEGRO_INTERNAL_BITMAP_NEW_H

#include "allegro/display_new.h"
#include "allegro/bitmap_new.h"

typedef struct AL_BITMAP_INTERFACE AL_BITMAP_INTERFACE;

struct BITMAP;

struct AL_BITMAP
{
   AL_BITMAP_INTERFACE *vt;
   AL_DISPLAY *display;
   int format;
   int flags;
   int w, h;
   /* 
    * clipping enabled?
    * clip left, right, top, bottom
    * Clip anything outside of this
    */
   int clip;
   int cl, cr, ct, cb;
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
   AL_LOCKED_REGION locked_region;

   /* Info for sub-bitmaps */
   AL_BITMAP *parent;
   int xofs;
   int yofs;

   /* A memory copy of the bitmap data. May be NULL for an empty bitmap. */
   unsigned char *memory;

   /* TODO: Is it needed? */
   /*unsigned char *palette;*/
};

struct AL_BITMAP_INTERFACE
{
   int id;
   void (*draw_bitmap)(struct AL_BITMAP *bitmap, float x, float y, int flags);
   void (*draw_bitmap_region)(AL_BITMAP *bitmap, float sx, float sy,
      float sw, float sh, float dx, float dy, int flags);
   void (*draw_scaled_bitmap)(AL_BITMAP *bitmap, float sx, float sy,
      float sw, float sh, float dx, float dy, float dw, float dh, int flags);
   void (*draw_rotated_bitmap)(AL_BITMAP *bitmap, float cx, float cy,
      float angle, float dx, float dy, int flags);
   void (*draw_rotated_scaled_bitmap)(AL_BITMAP *bitmap, float cx, float cy,
      float angle, float dx, float dy, float xscale, float yscale,
      float flags);
   /* After the memory-copy of the bitmap has been modified, need to call this
    * to update the display-specific copy. E.g. with an OpenGL driver, this
    * might create/update a texture. Returns false on failure.
    */
   bool (*upload_bitmap)(AL_BITMAP *bitmap, int x, int y, int width, int height);
   /* If the display version of the bitmap has been modified, use this to update
    * the memory copy accordingly. E.g. with an OpenGL driver, this might
    * read the contents of an associated texture.
    */

   void (*destroy_bitmap)(AL_BITMAP *bitmap);

   AL_LOCKED_REGION * (*lock_region)(AL_BITMAP *bitmap,
   	int x, int y, int w, int h,
   	AL_LOCKED_REGION *locked_region,
	int flags);

   void (*unlock_region)(AL_BITMAP *bitmap);

   void (*set_bitmap_clip)(AL_BITMAP *bitmap);
};

void _al_blit_memory_bitmap(AL_BITMAP *source, AL_BITMAP *dest,
   int source_x, int source_y, int dest_x, int dest_y, int w, int h);
//AL_BITMAP_INTERFACE *_al_bitmap_xdummy_driver(void);
AL_BITMAP_INTERFACE *_al_bitmap_d3ddummy_driver(void);

AL_COLOR* _al_map_rgba(int format, AL_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a);
AL_COLOR* _al_map_rgba_f(int format, AL_COLOR *color,
	float r, float g, float b, float a);
AL_COLOR* _al_map_rgba_i(int format, AL_COLOR *color,
	int r, int g, int b, int a);

void _al_unmap_rgba(int format, AL_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);
void _al_unmap_rgba_f(int format, AL_COLOR *color,
	float *r, float *g, float *b, float *a);
void _al_unmap_rgba_i(int format, AL_COLOR *color,
	int *r, int *g, int *b, int *a);

/* Bitmap conversion */
void _al_convert_bitmap_data(
	void *src, int src_format, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
void _al_convert_compat_bitmap(
	BITMAP *src,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
int _al_get_pixel_value(int src_format, AL_COLOR *src_color);

void _al_push_bitmap_parameters();
void _al_pop_bitmap_parameters();

/* Memory bitmap blitting */
void _al_draw_bitmap_region_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags);
void _al_draw_bitmap_memory(AL_BITMAP *bitmap,
   int dx, int dy, int flags);
void _al_draw_scaled_bitmap_memory(AL_BITMAP *bitmap,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int flags);
void _al_draw_rotated_bitmap_memory(AL_BITMAP *bitmap,
   int center_x, int center_y, int dx, int dy, float angle, int flags);
void _al_draw_rotated_bitmap_memory(AL_BITMAP *bitmap,
   int center_x, int center_y, int dx, int dy,
   float angle, int flags);
void _al_draw_rotated_scaled_bitmap_memory(AL_BITMAP *bitmap,
   int center_x, int center_y, int dx, int dy,
   float xscale, float yscale, float angle, int flags);

#endif
