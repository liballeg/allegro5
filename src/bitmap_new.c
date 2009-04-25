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
 *      New bitmap routines.
 *
 *      By Elias Pschernig and Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Bitmap routines
 */


#include <string.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_system.h"


/* Creates a memory bitmap.
 */
static ALLEGRO_BITMAP *_al_create_memory_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   int pitch;
   int format = al_get_new_bitmap_format();
   
   format = _al_get_real_pixel_format(format);
   
   bitmap = _AL_MALLOC(sizeof *bitmap);
   memset(bitmap, 0, sizeof(*bitmap));
   bitmap->size = sizeof(*bitmap);

   pitch = w * al_get_pixel_size(format);

   bitmap->vt = NULL;
   bitmap->format = format;
   bitmap->flags = al_get_new_bitmap_flags() | ALLEGRO_MEMORY_BITMAP;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->pitch = pitch;
   bitmap->display = NULL;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr_excl = w;
   bitmap->cb_excl = h;
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;
   bitmap->memory = _AL_MALLOC(pitch * h);
   bitmap->preserve_texture = !(al_get_new_bitmap_flags() & ALLEGRO_NO_PRESERVE_TEXTURE);
   return bitmap;
}



static void _al_destroy_memory_bitmap(ALLEGRO_BITMAP *bmp)
{
   _AL_FREE(bmp->memory);
   _AL_FREE(bmp);
}



/* Function: al_create_bitmap
 */
ALLEGRO_BITMAP *al_create_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP **back;
   ALLEGRO_SYSTEM *system = al_system_driver();
   ALLEGRO_DISPLAY *current_display = al_get_current_display();

   if ((al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) ||
         (!current_display || !current_display->vt || current_display->vt->create_bitmap == NULL) ||
         (system->displays._size < 1)) {
      return _al_create_memory_bitmap(w, h);
   }

   /* Else it's a display bitmap */

   bitmap = current_display->vt->create_bitmap(current_display, w, h);
   if (!bitmap) {
      TRACE("al_create_bitmap: failed to create display bitmap\n");
      return NULL;
   }

   /* XXX the ogl_display driver sets some of these variables. It's not clear
    * who should be responsible for setting what.
    * The pitch must be set by the driver.
    */
   bitmap->display = current_display;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->locked = false;
   bitmap->cl = 0;
   bitmap->ct = 0;
   bitmap->cr_excl = w;
   bitmap->cb_excl = h;
   bitmap->parent = NULL;
   bitmap->xofs = 0;
   bitmap->yofs = 0;
   bitmap->preserve_texture = !(al_get_new_bitmap_flags() & ALLEGRO_NO_PRESERVE_TEXTURE);

   ASSERT(bitmap->pitch >= w * al_get_pixel_size(bitmap->format));

   /* If the true height is greater than bitmap->h then the display driver
    * should allocate the memory itself.
    */
   if (!bitmap->memory) {
      size_t bytes = bitmap->pitch * h;
      bitmap->memory = _AL_MALLOC_ATOMIC(bytes);
      //memset(bitmap->memory, 0, bytes);
   }

   if (!bitmap->vt->upload_bitmap(bitmap)) {
      al_destroy_bitmap(bitmap);
      return NULL;
   }

   /* We keep a list of bitmaps depending on the current display so that we can
    * convert them to memory bimaps when the display is destroyed. */
   back = _al_vector_alloc_back(&current_display->bitmaps);
   *back = bitmap;

   return bitmap;
}



/* Function: al_destroy_bitmap
 */
void al_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   if (!bitmap) {
      return;
   }

   if (bitmap->parent) {
      /* It's a sub-bitmap */
      if (bitmap->display)
         _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);
      _AL_FREE(bitmap);
      return;
   }

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_destroy_memory_bitmap(bitmap);
      return;
   }

   /* Else it's a display bitmap */

   if (bitmap->locked)
      al_unlock_bitmap(bitmap);

   if (bitmap->vt)
      bitmap->vt->destroy_bitmap(bitmap);

   if (bitmap->display)
      _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);

   if (bitmap->memory)
      _AL_FREE(bitmap->memory);

   _AL_FREE(bitmap);
}


#if 0

/* TODO
 * This will load a bitmap from a file into a memory bitmap, keeping it in the
 * same format as on disk. That is, a paletted image will be stored as indices
 * plus the palette, 16-bit color channels stay as 16-bit, and so on. If
 * a format is not supported by Allegro, the closest one will be used. (E.g. we
 * likely will throw away things like frame timings or gamma correction.)
 */
static ALLEGRO_BITMAP *_al_load_memory_bitmap(char const *filename)
{
   PALETTE pal;
   BITMAP *file_data;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_STATE backup;

   // TODO:
   // The idea is, load_bitmap returns a memory representation of the bitmap,
   // maybe in fixed RGBA or whatever format is set as default (e.g. for HDR
   // images, should not reduce to 8-bit channels).
   // Then, it is converted to a display specific bitmap, which can be used
   // for blitting to the current display.
   int flags = al_get_new_bitmap_flags();

   // FIXME: should not use the 4.2 function here of course
   //set_color_conversion(COLORCONV_NONE);
   file_data = load_bitmap(filename, pal);
   if (!file_data) {
      return NULL;
   }
   select_palette(pal);

   if (flags & ALLEGRO_KEEP_BITMAP_FORMAT) {
      al_store_state(ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
      al_set_new_bitmap_format(_al_get_compat_bitmap_format(file_data));
   }

   bitmap = al_create_bitmap(file_data->w, file_data->h);

   if (flags & ALLEGRO_KEEP_BITMAP_FORMAT) {
      al_restore_state();
   }

   if (!bitmap)
      return NULL;

   _al_convert_compat_bitmap(file_data, bitmap->memory, bitmap->format,
      bitmap->pitch, 0, 0, 0, 0, file_data->w, file_data->h);

   destroy_bitmap(file_data);
   return bitmap;
}



/* Function: al_load_bitmap
 */
ALLEGRO_BITMAP *al_load_bitmap(char const *filename)
{
   ALLEGRO_BITMAP *bitmap;

   bitmap = _al_load_memory_bitmap(filename);

   /* If it's a display bitmap */
   if ((bitmap!=NULL) && ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) == 0)) {
      bitmap->vt->upload_bitmap(bitmap);
   }

   return bitmap;
}
#endif


/* Function: al_draw_bitmap
 */
void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_DISPLAY *display = al_get_current_display();
   
   /* If destination is memory, do a memory bitmap */
   if (dest->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_bitmap_memory(bitmap, dx, dy, flags);
   }
   else {
      /* if source is memory or incompatible */
      if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
          (!al_is_compatible_bitmap(bitmap)))
      {
         if (display && display->vt->draw_memory_bitmap_region) {
            display->vt->draw_memory_bitmap_region(display, bitmap,
               0, 0, bitmap->w, bitmap->h, dx, dy, flags);
         }
         else {
            _al_draw_bitmap_memory(bitmap, dx, dy, flags);
         }
      }
      else {
         /* Compatible display bitmap, use full acceleration */
         bitmap->vt->draw_bitmap(bitmap, dx, dy, flags);
      }
   }
}



/* Function: al_draw_bitmap_region
 */
void al_draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_DISPLAY* display = al_get_current_display();
   
   /* If destination is memory, do a memory bitmap */
   if (dest->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
   }
   else {
      /* if source is memory or incompatible */
      if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
          (!al_is_compatible_bitmap(bitmap)))
      {
         if (display && display->vt->draw_memory_bitmap_region) {
            display->vt->draw_memory_bitmap_region(display, bitmap,
               sx, sy, sw, sh, dx, dy, flags);
         }
         else {
            _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy,
               flags);
         }
      }
      else {
         /* Compatible display bitmap, use full acceleration */
         bitmap->vt->draw_bitmap_region(bitmap, sx, sy, sw, sh, dx, dy, flags);
      }
   }
}



/* Function: al_draw_scaled_bitmap
 */
void al_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP) ||
       (!al_is_compatible_bitmap(bitmap)))
   {
      _al_draw_scaled_bitmap_memory(bitmap, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
   else {
      bitmap->vt->draw_scaled_bitmap(bitmap, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
}



/* Function: al_draw_rotated_bitmap
 * 
 * angle is specified in radians and moves clockwise
 * on the screen.
 */
void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float angle, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP) ||
       (!al_is_compatible_bitmap(bitmap)))
   {
      _al_draw_rotated_bitmap_memory(bitmap, cx, cy,
         dx, dy, angle, flags);
   }
   else {
      bitmap->vt->draw_rotated_bitmap(bitmap, cx, cy, dx, dy, angle, flags);
   }
}



/* Function: al_draw_rotated_scaled_bitmap
 */
void al_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float xscale, float yscale, float angle,
	int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
    if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP) ||
       (!al_is_compatible_bitmap(bitmap)))
   {
      _al_draw_rotated_scaled_bitmap_memory(bitmap, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
   else {
      bitmap->vt->draw_rotated_scaled_bitmap(bitmap, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
}



/* Function: al_lock_bitmap_region
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap_region(ALLEGRO_BITMAP *bitmap,
	int x, int y,
	int width, int height,
        int format,
	int flags)
{
   ASSERT(x >= 0);
   ASSERT(y >= 0);
   ASSERT(width >= 0);
   ASSERT(height >= 0);

   /* For sub-bitmaps */
   if (bitmap->parent) {
      x += bitmap->xofs;
      y += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked)
      return NULL;

   ASSERT(x+width <= bitmap->w);
   ASSERT(y+height <= bitmap->h);

   bitmap->lock_x = x;
   bitmap->lock_y = y;
   bitmap->lock_w = width;
   bitmap->lock_h = height;
   bitmap->lock_flags = flags;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      int f = _al_get_real_pixel_format(format);
      if (f < 0) {
         return NULL;
      }
      if (format == ALLEGRO_PIXEL_FORMAT_ANY || bitmap->format == format || f == bitmap->format) {
         bitmap->locked_region.data = bitmap->memory
            + bitmap->pitch * y + x * al_get_pixel_size(bitmap->format);
         bitmap->locked_region.format = bitmap->format;
         bitmap->locked_region.pitch = bitmap->pitch;
      }
      else {
         bitmap->locked_region.pitch = al_get_pixel_size(f) * width;
         bitmap->locked_region.data = _AL_MALLOC(bitmap->locked_region.pitch*height);
         bitmap->locked_region.format = f;
         if (!(bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY)) {
            _al_convert_bitmap_data(
               bitmap->memory, bitmap->format, bitmap->pitch,
               bitmap->locked_region.data, f, bitmap->locked_region.pitch,
               x, y, 0, 0, width, height);
         }
      }
   }
   else {
      if (!bitmap->vt->lock_region(bitmap, x, y, width, height, format, flags)) {
         return NULL;
      }
   }

   bitmap->locked = true;

   return &bitmap->locked_region;
}



/* Function: al_lock_bitmap
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap(ALLEGRO_BITMAP *bitmap, int format, int flags)
{
   return al_lock_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h, format, flags);
}



/* Function: al_unlock_bitmap
 */
void al_unlock_bitmap(ALLEGRO_BITMAP *bitmap)
{
   /* For sub-bitmaps */
   if (bitmap->parent) {
      bitmap = bitmap->parent;
   }

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
      bitmap->vt->unlock_region(bitmap);
   }
   else {
      if (bitmap->locked_region.format != 0 && bitmap->locked_region.format != bitmap->format) {
         if (!(bitmap->lock_flags & ALLEGRO_LOCK_READONLY)) {
            _al_convert_bitmap_data(
               bitmap->locked_region.data, bitmap->locked_region.format, bitmap->locked_region.pitch,
               bitmap->memory, bitmap->format, bitmap->pitch,
               0, 0, bitmap->lock_x, bitmap->lock_y, bitmap->lock_w, bitmap->lock_h);
         }
         _AL_FREE(bitmap->locked_region.data);
      }
   }

   bitmap->locked = false;
}



/* Function: al_convert_mask_to_alpha
 */
void al_convert_mask_to_alpha(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR mask_color)
{
   ALLEGRO_LOCKED_REGION *lr;
   int x, y;
   ALLEGRO_COLOR pixel;
   ALLEGRO_COLOR alpha_pixel;
   ALLEGRO_STATE backup;

   if (!(lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
      TRACE("al_convert_mask_to_alpha: Couldn't lock bitmap.\n");
      return;
   }

   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   
   al_set_target_bitmap(bitmap);

   alpha_pixel = al_map_rgba(0, 0, 0, 0);

   for (y = 0; y < bitmap->h; y++) {
      for (x = 0; x < bitmap->w; x++) {
         pixel = al_get_pixel(bitmap, x, y);
         if (memcmp(&pixel, &mask_color, sizeof(ALLEGRO_COLOR)) == 0) {
            al_put_pixel(x, y, alpha_pixel);
         }
      }
   }

   al_restore_state(&backup);

   al_unlock_bitmap(bitmap);
}



/* Function: al_get_bitmap_width
 */
int al_get_bitmap_width(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->w;
}



/* Function: al_get_bitmap_height
 */
int al_get_bitmap_height(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->h;
}



/* Function: al_get_bitmap_format
 */
int al_get_bitmap_format(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->format;
}



/* Function: al_get_bitmap_flags
 */
int al_get_bitmap_flags(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->flags;
}



/* Function: al_create_sub_bitmap
 */
ALLEGRO_BITMAP *al_create_sub_bitmap(ALLEGRO_BITMAP *parent,
   int x, int y, int w, int h)
{
   ALLEGRO_BITMAP *bitmap;

   if (parent->display && parent->display->vt &&
         parent->display->vt->create_sub_bitmap)
   {
      bitmap = parent->display->vt->create_sub_bitmap(
         parent->display, parent, x, y, w, h);
   }
   else {
      bitmap = _AL_MALLOC(sizeof *bitmap);
      memset(bitmap, 0, sizeof *bitmap);
      bitmap->size = sizeof *bitmap;
      bitmap->vt = parent->vt;
   }

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
   bitmap->cr_excl = w;
   bitmap->cb_excl = h;
   bitmap->parent = parent;
   bitmap->xofs = x;
   bitmap->yofs = y;
   bitmap->memory = NULL;

   if (bitmap->display) {
      ALLEGRO_BITMAP **back;
      back = _al_vector_alloc_back(&bitmap->display->bitmaps);
      *back = bitmap;
   }

   return bitmap;
}


/* Function: al_is_sub_bitmap
 */
bool al_is_sub_bitmap(ALLEGRO_BITMAP *bitmap)
{
   return (bitmap->parent != NULL);
}


/* Function: al_clone_bitmap
 */
ALLEGRO_BITMAP *al_clone_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *clone = al_create_bitmap(bitmap->w, bitmap->h);
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_LOCKED_REGION *src_region;

   if (!clone)
      return NULL;

   if (!(src_region = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY)))
      return NULL;

   if (!(dst_region = al_lock_bitmap(clone, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY))) {
      al_unlock_bitmap(bitmap);
      return NULL;
   }

   _al_convert_bitmap_data(
	src_region->data, src_region->format, src_region->pitch,
        dst_region->data, dst_region->format, dst_region->pitch,
        0, 0, 0, 0, bitmap->w, bitmap->h);

   al_unlock_bitmap(bitmap);
   al_unlock_bitmap(clone);

   return clone;
}


/* Function: al_is_bitmap_locked
 */
bool al_is_bitmap_locked(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->locked;
}


/* Converts a memory bitmap to a display bitmap preserving its contents.
 * The created bitmap belongs to the current display. A converted sub
 * bitmap is invalid until its parent is converted.
 * WARNING: This function will fail for any memory bitmap not previously
 * processed by _al_convert_to_memory_bitmap().
 */
void _al_convert_to_display_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *tmp;
   ALLEGRO_STATE backup;
   ALLEGRO_DISPLAY *d = al_get_current_display();

   /* Do nothing if it is a display bitmap already. */
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
      return;

   if (bitmap->parent) {
      /* FIXME
       * We cannot safely assume that the backbuffer and bitmaps use the same
       * vtable.
       */
      bitmap->vt = al_get_backbuffer()->vt;
      bitmap->display = d;
      bitmap->flags &= !ALLEGRO_MEMORY_BITMAP;
      return;
   }

   /* Allocate a temporary bitmap which will hold the data
    * during the conversion process. */

   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);

   al_set_new_bitmap_flags(0);
   al_set_new_bitmap_format(bitmap->format);
   tmp = al_create_bitmap(bitmap->w, bitmap->h);
   /* Remove the temporary bitmap from the display bitmap list, added
    * automatically by al_create_bitmap()*/
   _al_vector_find_and_delete(&d->bitmaps, &tmp);

   /* Preserve bitmap contents. */
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));
   al_set_target_bitmap(tmp);
   al_draw_bitmap(bitmap, 0, 0, 0);
   tmp->cb_excl = bitmap->cb_excl;
   tmp->cr_excl = bitmap->cr_excl;
   tmp->cl = bitmap->cl;
   tmp->ct = bitmap->ct;
   
   al_restore_state(&backup);

   /* Free the memory bitmap's memory. */
   _AL_FREE(bitmap->memory);

   /* Put the contents back to the bitmap. */
   ASSERT(tmp->size > sizeof(ALLEGRO_BITMAP));
   ASSERT(bitmap->size > sizeof(ALLEGRO_BITMAP));
   ASSERT(tmp->size == bitmap->size);
   memcpy(bitmap, tmp, tmp->size);

   _AL_FREE(tmp);
}


/* Converts a display bitmap to a memory bitmap preserving its contents.
 * Driver specific resources occupied by the display bitmap are freed.
 * A converted sub bitmap is invalid until its parent is converted.
 */
void _al_convert_to_memory_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *tmp;
   ALLEGRO_STATE backup;

   /* Do nothing if it is a memory bitmap already. */
   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP)
      return;

   if (bitmap->parent) {
      _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);

      //_AL_REALLOC(bitmap, sizeof(ALLEGRO_BITMAP));
      bitmap->display = NULL;
      bitmap->flags |= ALLEGRO_MEMORY_BITMAP;
      return;
   }

   /* Allocate a temporary bitmap which will hold the data
    * during the conversion process. */
   
   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   al_set_new_bitmap_format(bitmap->format);
   tmp = al_create_bitmap(bitmap->w, bitmap->h);

   /* Preserve bitmap contents. */
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));
   al_set_target_bitmap(tmp);
   al_draw_bitmap(bitmap, 0, 0, 0);
   tmp->cb_excl = bitmap->cb_excl;
   tmp->cr_excl = bitmap->cr_excl;
   tmp->cl = bitmap->cl;
   tmp->ct = bitmap->ct;
   
   al_restore_state(&backup);

   /* Destroy the display bitmap to free driver-specific resources. */
   if (bitmap->vt)
      bitmap->vt->destroy_bitmap(bitmap);

   _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);

   /* Do not shrink the bitmap object. This way we can convert back to the
    * display bitmap.
    */
   /*_AL_REALLOC(bitmap, tmp->size);
     bitmap->size = tmp->size*/

   /* Put the contents back to the bitmap. */
   memcpy(bitmap, tmp, tmp->size);

   _AL_FREE(tmp);
}

/* vim: set ts=8 sts=3 sw=3 et: */
