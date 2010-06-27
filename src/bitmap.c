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
#include "allegro5/internal/aintern_system.h"

static ALLEGRO_COLOR solid_white = {1, 1, 1, 1};

/* Creates a memory bitmap.
 */
static ALLEGRO_BITMAP *_al_create_memory_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   int pitch;
   int format = al_get_new_bitmap_format();
   
   format = _al_get_real_pixel_format(al_get_current_display(), format);
   
   bitmap = al_malloc(sizeof *bitmap);
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
   bitmap->memory = al_malloc(pitch * h);
   bitmap->preserve_texture = !(al_get_new_bitmap_flags() & ALLEGRO_NO_PRESERVE_TEXTURE);
   return bitmap;
}



static void _al_destroy_memory_bitmap(ALLEGRO_BITMAP *bmp)
{
   al_free(bmp->memory);
   al_free(bmp);
}



/* Function: al_create_bitmap
 */
ALLEGRO_BITMAP *al_create_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP **back;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
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
      bitmap->memory = al_malloc(bytes);
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
      al_free(bitmap);
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
      al_free(bitmap->memory);

   al_free(bitmap);
}


/* Function: al_draw_tinted_bitmap
 */
void al_draw_tinted_bitmap(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint,
   float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_DISPLAY *display = dest->display;
   
   /* If destination is memory, do a memory bitmap */
   if (dest->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_bitmap_memory(bitmap, tint, dx, dy, flags);
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
            _al_draw_bitmap_memory(bitmap, tint, dx, dy, flags);
         }
      }
      else {
         /* Compatible display bitmap, use full acceleration */
         bitmap->vt->draw_bitmap(bitmap, tint, dx, dy, flags);
      }
   }
}


/* Function: al_draw_bitmap
 */
void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
   al_draw_tinted_bitmap(bitmap, solid_white, dx, dy, flags);
}


/* Function: al_draw_tinted_bitmap_region
 */
void al_draw_tinted_bitmap_region(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_DISPLAY *display = dest->display;
   
   /* If destination is memory, do a memory bitmap */
   if (dest->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh, dx, dy, flags);
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
            _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh,
               dx, dy, flags);
         }
      }
      else {
         /* Compatible display bitmap, use full acceleration */
         bitmap->vt->draw_bitmap_region(bitmap, tint, sx, sy, sw, sh,
            dx, dy, flags);
      }
   }
}


/* Function: al_draw_bitmap_region
 */
void al_draw_bitmap_region(ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh, float dx, float dy, int flags)
{
   al_draw_tinted_bitmap_region(bitmap, solid_white, sx, sy, sw, sh,
      dx, dy, flags);
}


/* Function: al_draw_tinted_scaled_bitmap
 */
void al_draw_tinted_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_BITMAP *back = al_get_backbuffer(al_get_current_display());

   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP) ||
       (!al_is_compatible_bitmap(bitmap)) ||
       (bitmap == back) || (bitmap->parent == back))
   {
      _al_draw_scaled_bitmap_memory(bitmap, tint, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
   else {
      bitmap->vt->draw_scaled_bitmap(bitmap, tint, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
}

/* Function: al_draw_scaled_bitmap
 */
void al_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh, int flags)
{
   al_draw_tinted_scaled_bitmap(bitmap, solid_white, sx, sy, sw, sh,
      dx, dy, dw, dh, flags);
}



/* Function: al_draw_tinted_rotated_bitmap
 * 
 * angle is specified in radians and moves clockwise
 * on the screen.
 */
void al_draw_tinted_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float angle, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_BITMAP *back = al_get_backbuffer(al_get_current_display());

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP) ||
       (!al_is_compatible_bitmap(bitmap)) ||
       (bitmap == back) || (bitmap->parent == back))
   {
      _al_draw_rotated_bitmap_memory(bitmap, tint, cx, cy,
         dx, dy, angle, flags);
   }
   else {
      bitmap->vt->draw_rotated_bitmap(bitmap, tint, cx, cy, dx, dy,
         angle, flags);
   }
}

/* Function: al_draw_rotated_bitmap
 */
void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float angle, int flags)
{
   al_draw_tinted_rotated_bitmap(bitmap, solid_white, cx, cy, dx, dy,
      angle, flags);
}


/* Function: al_draw_tinted_rotated_scaled_bitmap
 */
void al_draw_tinted_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_BITMAP *back = al_get_backbuffer(al_get_current_display());

   /* If one is a memory bitmap, do memory blit */
    if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP) ||
       (!al_is_compatible_bitmap(bitmap)) ||
       (bitmap == back) || (bitmap->parent == back))
   {
      _al_draw_rotated_scaled_bitmap_memory(bitmap, tint, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
   else {
      bitmap->vt->draw_rotated_scaled_bitmap(bitmap, tint, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
}


/* Function: al_draw_rotated_scaled_bitmap
 */
void al_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   al_draw_tinted_rotated_scaled_bitmap(bitmap, solid_white,
      cx, cy, dx, dy, xscale, yscale, angle, flags);
}


/* Function: al_lock_bitmap_region
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int width, int height, int format, int flags)
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
      int f = _al_get_real_pixel_format(al_get_current_display(), format);
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
         bitmap->locked_region.data = al_malloc(bitmap->locked_region.pitch*height);
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
ALLEGRO_LOCKED_REGION *al_lock_bitmap(ALLEGRO_BITMAP *bitmap,
   int format, int flags)
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
         al_free(bitmap->locked_region.data);
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
   
   ALLEGRO_TRANSFORM old_trans;
   ALLEGRO_TRANSFORM identity_trans;
   al_copy_transform(al_get_current_transform(), &old_trans);
   al_identity_transform(&identity_trans);
   al_use_transform(&identity_trans);

   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   
   al_set_target_bitmap(bitmap);

   if (!(lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
      TRACE("al_convert_mask_to_alpha: Couldn't lock bitmap.\n");
      return;
   }

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
   al_use_transform(&old_trans);
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



/* Function: al_set_clipping_rectangle
 */
void al_set_clipping_rectangle(int x, int y, int width, int height)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   ASSERT(bitmap);

   if (x < 0) {
      width += x;
      x = 0;
   }
   if (y < 0) {
      height += y;
      y = 0;
   }
   if (x + width > bitmap->w) {
      width = bitmap->w - x;
   }
   if (y + height > bitmap->h) {
      height = bitmap->h - y;
   }

   bitmap->cl = x;
   bitmap->ct = y;
   bitmap->cr_excl = x + width;
   bitmap->cb_excl = y + height;

   if (bitmap->vt && bitmap->vt->update_clipping_rectangle) {
      bitmap->vt->update_clipping_rectangle(bitmap);
   }
}



/* Function: al_get_clipping_rectangle
 */
void al_get_clipping_rectangle(int *x, int *y, int *w, int *h)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   ASSERT(bitmap);

   if (x) *x = bitmap->cl;
   if (y) *y = bitmap->ct;
   if (w) *w = bitmap->cr_excl - bitmap->cl;
   if (h) *h = bitmap->cb_excl - bitmap->ct;
}



/* Function: al_create_sub_bitmap
 */
ALLEGRO_BITMAP *al_create_sub_bitmap(ALLEGRO_BITMAP *parent,
   int x, int y, int w, int h)
{
   ALLEGRO_BITMAP *bitmap;

   if (parent->parent) {
      x += parent->xofs;
      y += parent->yofs;
      parent = parent->parent;
   }

   if (parent->display && parent->display->vt &&
         parent->display->vt->create_sub_bitmap)
   {
      bitmap = parent->display->vt->create_sub_bitmap(
         parent->display, parent, x, y, w, h);
   }
   else {
      bitmap = al_malloc(sizeof *bitmap);
      memset(bitmap, 0, sizeof *bitmap);
      bitmap->size = sizeof *bitmap;
      bitmap->vt = parent->vt;
   }

   bitmap->format = parent->format;
   bitmap->flags = parent->flags;

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
   ALLEGRO_BITMAP **vid;
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
      bitmap->vt = al_get_backbuffer(d)->vt;
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

   /* Preserve bitmap contents. */
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_set_target_bitmap(tmp);
   al_draw_bitmap(bitmap, 0, 0, 0);
   tmp->cb_excl = bitmap->cb_excl;
   tmp->cr_excl = bitmap->cr_excl;
   tmp->cl = bitmap->cl;
   tmp->ct = bitmap->ct;
   
   al_restore_state(&backup);

   /* Free the memory bitmap's memory. */
   al_free(bitmap->memory);

   /* Put the contents back to the bitmap. */
   ASSERT(tmp->size > sizeof(ALLEGRO_BITMAP));
   ASSERT(bitmap->size > sizeof(ALLEGRO_BITMAP));
   ASSERT(tmp->size == bitmap->size);
   memcpy(bitmap, tmp, tmp->size);

   vid = _al_vector_alloc_back(&d->bitmaps);
   *vid = bitmap;

   /* Remove the temporary bitmap from the display bitmap list, added
    * automatically by al_create_bitmap()*/
   _al_vector_find_and_delete(&d->bitmaps, &tmp);
   al_free(tmp);
}


/* Converts a display bitmap to a memory bitmap preserving its contents.
 * Driver specific resources occupied by the display bitmap are freed.
 * A converted sub bitmap is invalid until its parent is converted.
 */
void _al_convert_to_memory_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *tmp;
   ALLEGRO_STATE backup;
   size_t old_size;

   /* Do nothing if it is a memory bitmap already. */
   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP)
      return;

   if (bitmap->parent) {
      _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);

      //al_realloc(bitmap, sizeof(ALLEGRO_BITMAP));
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
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
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
   /*al_realloc(bitmap, tmp->size);
     bitmap->size = tmp->size*/

   /* Put the contents back to the bitmap. */
   old_size = bitmap->size;
   memcpy(bitmap, tmp, tmp->size);
   bitmap->size = old_size;

   al_free(tmp);
}

void _al_convert_bitmap_data(
   void *src, int src_format, int src_pitch,
   void *dst, int dst_format, int dst_pitch,
   int sx, int sy, int dx, int dy, int width, int height)
{
   ASSERT(_al_pixel_format_is_real(dst_format));

   /* Use memcpy if no conversion is needed. */
   if (src_format == dst_format) {
      int y;
      int size = al_get_pixel_size(src_format);
      char *src_ptr = ((char *)src) + sy * src_pitch + sx * size;
      char *dst_ptr = ((char *)dst) + dy * dst_pitch + dx * size;
      width *= size;
      for (y = 0; y < height; y++) {
         memcpy(dst_ptr, src_ptr, width);
         src_ptr += src_pitch;
         dst_ptr += dst_pitch;
      }
      return;
   }

   (_al_convert_funcs[src_format][dst_format])(src, src_pitch,
      dst, dst_pitch, sx, sy, dx, dy, width, height);
}

/* vim: set ts=8 sts=3 sw=3 et: */
