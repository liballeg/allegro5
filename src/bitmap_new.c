#include <string.h>
#include "allegro.h"
#include "internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"

// FIXME: does not work if the areas overlap or anything is outside
void _al_blit_memory_bitmap(AL_BITMAP *source, AL_BITMAP *dest,
   int source_x, int source_y, int dest_x, int dest_y, int w, int h)
{
   int y;
   unsigned char *sptr = source->memory + (source_y * source->w + source_x) * 4;
   unsigned char *dptr = dest->memory + (dest_y * dest->w + dest_x) * 4;
   for (y = 0; y < h; y++)
   {
      memcpy(dptr, sptr, w * 4);
      sptr += source->w * 4;
      dptr += dest->w * 4;
   }
}

/* Creates a memory bitmap. A memory bitmap can only be drawn to other memory
 * bitmaps, not to a display.
 */
AL_BITMAP *al_create_memory_bitmap(int w, int h, int flags)
{
   AL_BITMAP *bitmap = _AL_MALLOC(sizeof *bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->flags = flags;
   bitmap->w = w;
   bitmap->h = h;
   // FIXME: Of course, we do need to handle all the possible different formats,
   // this will easily fill up its own file of 1000 lines, but for now,
   // RGBA with 8-bit per component is hardcoded.
   bitmap->memory = _AL_MALLOC(w * h * 4);
   memset(bitmap->memory, 0, w * h * 4);
   return bitmap;
}

void al_destroy_memory_bitmap(AL_BITMAP *bmp)
{
   _AL_FREE(bmp->memory);
   _AL_FREE(bmp);
}

/* Creates an empty display bitmap. A display bitmap can only be drawn to a
 * display.
 */
AL_BITMAP *al_create_bitmap(int w, int h, int flags)
{
   AL_BITMAP *bitmap = _al_current_display->vt->create_bitmap(
       _al_current_display, w, h, flags);
   if (!bitmap->memory) {
   	bitmap->memory = _AL_MALLOC(w * h * 4);
        memset(bitmap->memory, 0, w * h * 4);
   }
   bitmap->vt->upload_bitmap(bitmap, 0, 0, w, h);
   return bitmap;
}

/* Destroy a (memory or display) bitmap. The passed pointer will be invalid
 * afterwards, so best set it to NULL.
 */
void al_destroy_bitmap(AL_BITMAP *bitmap)
{
   if (bitmap->vt) bitmap->vt->destroy_bitmap(bitmap);
   if (bitmap->memory)
      _AL_FREE(bitmap->memory);
   _AL_FREE(bitmap);
}

/* TODO
 * This will load a bitmap from a file into a memory bitmap, keeping it in the
 * same format as on disk. That is, a paletted image will be stored as indices
 * plus the palette, 16-bit color channels stay as 16-bit, and so on. If
 * a format is not supported by Allegro, the closest one will be used. (E.g. we
 * likely will throw away things like frame timings or gamma correction.)
 */
AL_BITMAP *al_load_memory_bitmap(char const *filename, int flags)
{
   // TODO:
   // The idea is, load_bitmap returns a memory representation of the bitmap,
   // maybe in fixed RGBA or whatever format is set as default (e.g. for HDR
   // images, should not reduce to 8-bit channels).
   // Then, it is converted to a display specific bitmap, which can be used
   // for blitting to the current display.
   set_color_conversion(COLORCONV_NONE);
   // FIXME: should not use the 4.2 function here of course
   PALETTE pal;
   BITMAP *file_data = load_bitmap(filename, pal);
   select_palette(pal);
   if (!file_data)
      return NULL;
   AL_BITMAP *bitmap = al_create_bitmap(file_data->w, file_data->h, flags);
   int x, y;
   int alpha = _bitmap_has_alpha(file_data);
   unsigned char *ptr = bitmap->memory;
   for (y = 0; y < file_data->h; y++) {
      for (x = 0; x < file_data->w; x++) {
         int c = getpixel(file_data, x, y);
         *(ptr++) = getr(c);
         *(ptr++) = getg(c);
         *(ptr++) = getb(c);
         *(ptr++) = alpha ? geta(c) : 255;
      }
   }
   destroy_bitmap(file_data);
   return bitmap;
}

/* Load a bitmap from a file into a display bitmap, ready to be drawn.
 */
AL_BITMAP *al_load_bitmap(char const *filename, int flags)
{
   AL_BITMAP *bitmap = al_load_memory_bitmap(filename, flags);
   bitmap->vt->upload_bitmap(bitmap, 0, 0, bitmap->w, bitmap->h);
   return bitmap;
}

/*
void al_draw_bitmap(int flag, AL_BITMAP *bitmap, float x, float y)
{
   // TODO: What if the bitmap cannot be drawn to the current display?
   bitmap->vt->draw_bitmap(flag, bitmap, x, y);
}
*/


/*
 * Draw an entire bitmap to another. Use NULL for the destination
 * bitmap to draw to the current display.
 */
void al_blit(int flag, AL_BITMAP *src, AL_BITMAP *dest, float dx, float dy)
{
	src->vt->blit(flag, src, dest, dx, dy);
}

/*
 * Draw a rectangle of one bitmap to another. Use NULL for the
 * destination bitmap to draw to the current display.
 */
void al_blit_region(int flag, AL_BITMAP *src, float sx, float sy,
	AL_BITMAP *dest, float dx, float dy, float w, float h)
{
	src->vt->blit_region(flag, src, sx, sy, dest, dx, dy, w, h);
}

/*
 * Draw a rectangle of one bitmap to another. Use NULL for the
 * destination bitmap to draw to the current display.
 */
void al_blit_scaled(int flag,
	AL_BITMAP *src,	float sx, float sy, float sw, float sh,
	AL_BITMAP *dest, float dx, float dy, float dw, float dh)
{
	src->vt->blit_scaled(flag, src, sx, sy, sw, sh, dest, dx, dy, dw, dh);
}

void al_rotate_bitmap(int flag,
	AL_BITMAP *src,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float angle)
{
	src->vt->rotate_bitmap(flag, src, source_center_x, source_center_y, dest, dest_x, dest_y, angle);
}

void al_rotate_scaled(int flag,
	AL_BITMAP *src,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float xscale, float yscale,
	float angle)
{
	src->vt->rotate_scaled(flag, src, source_center_x, source_center_y, dest, dest_x, dest_y, xscale, yscale, angle);
}

void al_draw_sub_bitmap(AL_BITMAP *bitmap, float x, float y,
    float sx, float sy, float sw, float sh)
{
   // TODO
}

void al_set_light_color(AL_BITMAP *bitmap, AL_COLOR *light_color)
{
	memcpy(&bitmap->light_color, light_color, sizeof(AL_COLOR));
}

void al_lock_bitmap(AL_BITMAP *bitmap, int x, int y, int width, int height)
{
	bitmap->locked = true;
	bitmap->lock_x = x;
	bitmap->lock_y = y;
	bitmap->lock_width = width;
	bitmap->lock_height = height;
}

void al_unlock_bitmap(AL_BITMAP *bitmap)
{
	bitmap->vt->upload_bitmap(bitmap,
		bitmap->lock_x,
		bitmap->lock_y,
		bitmap->lock_width,
		bitmap->lock_height);
	bitmap->locked = false;
}

void al_put_pixel(int flag, AL_BITMAP *bitmap, int x, int y, AL_COLOR *color)
{
	bitmap->vt->put_pixel(flag, bitmap, x, y, color);
}
