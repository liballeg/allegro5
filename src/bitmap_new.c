/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      New bitmap routines
 *
 *      By Elias Pschernig
 *      Heavily modified by Trent Gamblin.
 *
 */

#include <string.h>
#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern_display.h"
#include "allegro/internal/aintern_bitmap.h"

/* Creates a memory bitmap. A memory bitmap can only be drawn to other memory
 * bitmaps, not to a display.
 */
static ALLEGRO_BITMAP *_al_create_memory_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap = _AL_MALLOC(sizeof *bitmap);
   int format = al_get_new_bitmap_format();
   ALLEGRO_COLOR mask_color;
   
   memset(bitmap, 0, sizeof *bitmap);
   if (format == ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA)
      format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
   else if (format == ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA)
      format = ALLEGRO_PIXEL_FORMAT_RGBX_8888;
   bitmap->format = format;
   bitmap->flags = al_get_new_bitmap_flags();
   bitmap->w = w;
   bitmap->h = h;
   bitmap->display = NULL;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr = w-1;
   bitmap->cb = h-1;
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;
   // FIXME: Of course, we do need to handle all the possible different formats,
   // this will easily fill up its own file of 1000 lines, but for now,
   // RGBA with 8-bit per component is hardcoded.
   bitmap->memory = _AL_MALLOC(w * h * al_get_pixel_size(format));
   memset(bitmap->memory, 0, w * h * al_get_pixel_size(format));
   al_set_bitmap_mask_color(bitmap, al_map_rgb(bitmap, &mask_color, 255, 0, 255));
   return bitmap;
}

static void _al_destroy_memory_bitmap(ALLEGRO_BITMAP *bmp)
{
   _AL_FREE(bmp->memory);
   _AL_FREE(bmp);
}

/* Creates an empty display bitmap. A display bitmap can only be drawn to a
 * display.
 */
ALLEGRO_BITMAP *al_create_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_COLOR mask_color;
   
   if (al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) {
   	return _al_create_memory_bitmap(w, h);
   }

   /* Else it's a display bitmap */

   bitmap = _al_current_display->vt->create_bitmap(_al_current_display, w, h);

   bitmap->display = _al_current_display;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr = w-1;
   bitmap->cb = h-1;
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;

   if (!bitmap->memory) {
   	bitmap->memory = _AL_MALLOC(w * h * al_get_pixel_size(bitmap->format));
        memset(bitmap->memory, 0, w * h * al_get_pixel_size(bitmap->format));
   }

   if (!bitmap->vt->upload_bitmap(bitmap, 0, 0, w, h)) {
   	al_destroy_bitmap(bitmap);
	return NULL;
   }

   al_set_bitmap_mask_color(bitmap, al_map_rgb(bitmap, &mask_color, 255, 0, 255));

   return bitmap;
}

/* Destroy a (memory or display) bitmap. The passed pointer will be invalid
 * afterwards, so best set it to NULL.
 */
void al_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
   	_al_destroy_memory_bitmap(bitmap);
	return;
   }
   else if (bitmap->parent) {
      /* It's a sub-bitmap */
      _AL_FREE(bitmap);
      return;
   }

   /* Else it's a display bitmap */

   if (bitmap->locked)
      al_unlock_bitmap(bitmap);

   if (bitmap->vt)
      bitmap->vt->destroy_bitmap(bitmap);

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
static ALLEGRO_BITMAP *_al_load_memory_bitmap(char const *filename)
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
   ALLEGRO_BITMAP *bitmap = al_create_bitmap(file_data->w, file_data->h);

   _al_convert_compat_bitmap(
   	file_data,
	bitmap->memory, bitmap->format, al_get_pixel_size(bitmap->format)*bitmap->w,
	0, 0, 0, 0,
	file_data->w, file_data->h);

   destroy_bitmap(file_data);
   return bitmap;
}

/* Load a bitmap from a file into a display bitmap, ready to be drawn.
 */
ALLEGRO_BITMAP *al_load_bitmap(char const *filename)
{
   ALLEGRO_BITMAP *bitmap;
   
   if (al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) {
   	return _al_load_memory_bitmap(filename);
   }

   /* Else it's a display bitmap */

   bitmap = _al_load_memory_bitmap(filename);

   bitmap->vt->upload_bitmap(bitmap, 0, 0, bitmap->w, bitmap->h);

   return bitmap;
}

void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) || (dest->flags & ALLEGRO_MEMORY_BITMAP)) {
      if (_al_current_display->vt->draw_memory_bitmap_region)
         _al_current_display->vt->draw_memory_bitmap_region(_al_current_display,
	    bitmap, 0, 0, bitmap->w, bitmap->h, dx, dy, flags);
      else
         _al_draw_bitmap_memory(bitmap, dx, dy, flags);
   }
   else if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_bitmap(bitmap, dx, dy, flags);
}

void al_draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) || (dest->flags & ALLEGRO_MEMORY_BITMAP)) {
      if (_al_current_display->vt->draw_memory_bitmap_region)
         _al_current_display->vt->draw_memory_bitmap_region(_al_current_display,
	    bitmap, sx, sy, sw, sh, dx, dy, flags);
      else
         _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
   }
   else if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_bitmap_region(bitmap, sx, sy, sw, sh, dx, dy, flags);
}

void al_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) || (dest->flags & ALLEGRO_MEMORY_BITMAP)) {
      _al_draw_scaled_bitmap_memory(bitmap, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
   else if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_scaled_bitmap(bitmap, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
}

void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float angle, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) || (dest->flags & ALLEGRO_MEMORY_BITMAP)) {
      _al_draw_rotated_bitmap_memory(bitmap, cx, cy,
         dx, dy, angle, flags);
   }
   else if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_rotated_bitmap(bitmap, cx, cy, dx, dy, angle, flags);
}

void al_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float xscale, float yscale, float angle,
	int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) || (dest->flags & ALLEGRO_MEMORY_BITMAP)) {
      _al_draw_rotated_scaled_bitmap_memory(bitmap, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
   else if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_rotated_scaled_bitmap(bitmap, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
}

ALLEGRO_LOCKED_REGION *al_lock_bitmap_region(ALLEGRO_BITMAP *bitmap,
	int x, int y,
	int width, int height,
	ALLEGRO_LOCKED_REGION *locked_region,
	int flags)
{
   /* For sub-bitmaps */
   if (bitmap->parent) {
      x += bitmap->xofs;
      y += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked)
      return NULL;

   bitmap->locked = true;
   bitmap->lock_x = x;
   bitmap->lock_y = y;
   bitmap->lock_w = width;
   bitmap->lock_h = height;
   bitmap->lock_flags = flags;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP || bitmap->flags & ALLEGRO_SYNC_MEMORY_COPY) {
     locked_region->data = bitmap->memory+(bitmap->w*y+x)*al_get_pixel_size(bitmap->format);
      locked_region->format = bitmap->format;
      locked_region->pitch = bitmap->w*al_get_pixel_size(bitmap->format);
   }
   else {
      locked_region = bitmap->vt->lock_region(bitmap,
         x, y, width, height,
         locked_region,
         flags);
   }

   if (locked_region)
      memcpy(&bitmap->locked_region, locked_region, sizeof(ALLEGRO_LOCKED_REGION));
   else
      bitmap->locked = false;

   return locked_region;
}

ALLEGRO_LOCKED_REGION *al_lock_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_LOCKED_REGION *locked_region,
   int flags)
{
   return al_lock_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h,
      locked_region, flags);
}

void al_unlock_bitmap(ALLEGRO_BITMAP *bitmap)
{
   /* For sub-bitmaps */
   if (bitmap->parent) {
      bitmap = bitmap->parent;
   }

   if (bitmap->flags & ALLEGRO_SYNC_MEMORY_COPY && !(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
      bitmap->vt->upload_bitmap(bitmap,
      bitmap->lock_x,
      bitmap->lock_y,
      bitmap->lock_w,
      bitmap->lock_h);
   }
   else if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
      bitmap->vt->unlock_region(bitmap);
   }

   bitmap->locked = false;
}

void al_convert_mask_to_alpha(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *mask_color)
{
   ALLEGRO_LOCKED_REGION lr;
   int x, y;
   ALLEGRO_COLOR pixel;
   ALLEGRO_COLOR alpha_pixel;

   if (!al_lock_bitmap(bitmap, &lr, 0)) {
      TRACE("al_convert_mask_to_alpha: Couldn't lock bitmap.\n");
      return;
   }

   _al_push_target_bitmap();
   al_set_target_bitmap(bitmap);

   al_map_rgba(bitmap, &alpha_pixel, 0, 0, 0, 0);

   for (y = 0; y < bitmap->h; y++) {
      for (x = 0; x < bitmap->w; x++) {
         al_get_pixel(bitmap, x, y, &pixel);
         if (memcmp(&pixel, mask_color, sizeof(ALLEGRO_COLOR)) == 0) {
            al_put_pixel(x, y, &alpha_pixel);
         }
      }
   }

   _al_pop_target_bitmap();

   al_unlock_bitmap(bitmap);
}

int al_get_bitmap_width(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->w;
}

int al_get_bitmap_height(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->h;
}

int al_get_bitmap_format(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->format;
}

int al_get_bitmap_flags(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->flags;
}

void al_set_clipping_rectangle(int x, int y, int width, int height)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   if (x < 0) {
      width += x;
      x = 0;
   }
   if (y < 0) {
      height += y;
      y = 0;
   }
   if (x+width > bitmap->w) {
      width = bitmap->w - x;
   }
   if (y+height > bitmap->h) {
      height = bitmap->h - y;
   }

   bitmap->cl = x;
   bitmap->ct = y;
   bitmap->cr = x + width;
   bitmap->cb = y + height;
}

void al_get_clipping_rectangle(int *x, int *y, int *w, int *h)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();
   if (x) *x = bitmap->cl;
   if (y) *y = bitmap->ct;
   if (w) *w = bitmap->cr - bitmap->cl + 1;
   if (h) *h = bitmap->cb - bitmap->ct + 1;
}

ALLEGRO_BITMAP *al_create_sub_bitmap(ALLEGRO_BITMAP *parent,
   int x, int y, int w, int h)
{
   ALLEGRO_BITMAP *bitmap;

   if (parent->display && parent->display->vt &&
         parent->display->vt->create_sub_bitmap) {
      bitmap = parent->display->vt->create_sub_bitmap(
         parent->display, parent, x, y, w, h);
   }
   else {
      bitmap = _AL_MALLOC(sizeof *bitmap);
   }

   memset(bitmap, 0, sizeof *bitmap);

   bitmap->format = parent->format;
   bitmap->flags = parent->flags;

   /* Clip */
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (x+w > parent->w) {
      w = parent->w - x;
   }
   if (y+h > parent->h) {
      h = parent->h - y;
   }

   bitmap->w = w;
   bitmap->h = h;
   bitmap->display = parent->display;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr = w-1;
   bitmap->cb = h-1;
   bitmap->parent = parent;
   bitmap->xofs = x;
   bitmap->yofs = y;
   bitmap->memory = NULL;

   return bitmap;
}

/*
 * Set the mask color of the target bitmap
 */
void al_set_bitmap_mask_color(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color)
{
   if (bitmap->vt && bitmap->vt->set_mask_color) {
      bitmap->vt->set_mask_color(bitmap, color);
   }
   else {
      memcpy(&bitmap->mask_color, color, sizeof(ALLEGRO_COLOR));
   }
}

/*
 * Get the mask color of the target bitmap
 */
ALLEGRO_COLOR *al_get_bitmap_mask_color(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR *color)
{
   memcpy(color, &bitmap->mask_color, sizeof(ALLEGRO_COLOR));
   return color;
}

