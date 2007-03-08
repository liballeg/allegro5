#ifndef ALLEGRO_INTERNAL_BITMAP_NEW_H
#define ALLEGRO_INTERNAL_BITMAP_NEW_H

typedef struct AL_BITMAP_INTERFACE AL_BITMAP_INTERFACE;

struct AL_BITMAP_INTERFACE
{
   int id;
   void (*draw_bitmap)(AL_BITMAP *bitmap, float x, float y);
   void (*draw_sub)(AL_BITMAP *bitmap, float x, float y,
      float sx, float sy, float sw, float sh);
   /* After the memory-copy of the bitmap has been modified, need to call this
    * to update the display-specific copy. E.g. with an OpenGL driver, this
    * might create/update a texture.
    */
   void (*upload_bitmap)(AL_BITMAP *bitmap);
   /* If the display version of the bitmap has been modified, use this to update
    * the memory copy accordingly. E.g. with an OpenGL driver, this might
    * read the contents of an associated texture.
    */
   void (*download_bitmap)(AL_BITMAP *bitmap);
   /* Destroy any driver specific stuff. The AL_BITMAP and its memory copy
    * itself should not be touched.
    */
   void (*destroy_bitmap)(AL_BITMAP *bitmap);
};

struct AL_BITMAP
{
   AL_BITMAP_INTERFACE *vt;
   int flags;
   int w, h;
   AL_DISPLAY *display; /* May be NULL for memory bitmaps. */

   /* A memory copy of the bitmap data. May be NULL for an empty bitmap. */
   unsigned char *memory;

   /* TODO: Is it needed? */
   unsigned char *palette;
};

void _al_blit_memory_bitmap(AL_BITMAP *source, AL_BITMAP *dest,
   int source_x, int source_y, int dest_x, int dest_y, int w, int h);
AL_BITMAP_INTERFACE *_al_bitmap_xdummy_driver(void);

#endif
