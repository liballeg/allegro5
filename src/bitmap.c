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
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_system.h"

ALLEGRO_DEBUG_CHANNEL("bitmap")


/* Creates a memory bitmap.
 */
static ALLEGRO_BITMAP *create_memory_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   int pitch;
   int format = al_get_new_bitmap_format();

   format = _al_get_real_pixel_format(al_get_current_display(), format);

   bitmap = al_calloc(1, sizeof *bitmap);
   bitmap->size = sizeof(*bitmap);

   pitch = w * al_get_pixel_size(format);

   bitmap->vt = NULL;
   bitmap->format = format;
   bitmap->flags = (al_get_new_bitmap_flags() | ALLEGRO_MEMORY_BITMAP) &
      ~ALLEGRO_VIDEO_BITMAP;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->pitch = pitch;
   bitmap->display = NULL;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr_excl = w;
   bitmap->cb_excl = h;
   al_identity_transform(&bitmap->transform);
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;
   bitmap->memory = al_malloc(pitch * h);
   bitmap->preserve_texture = !(al_get_new_bitmap_flags() & ALLEGRO_NO_PRESERVE_TEXTURE);
   return bitmap;
}



static void destroy_memory_bitmap(ALLEGRO_BITMAP *bmp)
{
   al_free(bmp->memory);
   al_free(bmp);
}



static ALLEGRO_BITMAP *do_create_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP **back;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   ALLEGRO_DISPLAY *current_display = al_get_current_display();
   int64_t mul;

   /* Reject bitmaps where a calculation pixel_size*w*h would overflow
    * int.  Supporting such bitmaps would require a lot more work.
    */
   mul = 4 * (int64_t) w * (int64_t) h;
   if (mul > (int64_t) INT_MAX) {
      ALLEGRO_WARN("Rejecting %dx%d bitmap\n", w, h);
      return NULL;
   }

   if ((al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) ||
         (!current_display || !current_display->vt || current_display->vt->create_bitmap == NULL) ||
         (system->displays._size < 1)) {
      return create_memory_bitmap(w, h);
   }

   /* Else it's a display bitmap */

   bitmap = current_display->vt->create_bitmap(current_display, w, h);
   if (!bitmap) {
      ALLEGRO_ERROR("failed to create display bitmap\n");
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
   al_identity_transform(&bitmap->transform);
   bitmap->parent = NULL;
   bitmap->xofs = 0;
   bitmap->yofs = 0;
   bitmap->preserve_texture = !(al_get_new_bitmap_flags() & ALLEGRO_NO_PRESERVE_TEXTURE);

   ASSERT(bitmap->pitch >= w * al_get_pixel_size(bitmap->format));

   /* The display driver should have set the bitmap->memory field if
    * appropriate; video bitmaps may leave it NULL.
    */

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



/* Function: al_create_bitmap
 */
ALLEGRO_BITMAP *al_create_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap = do_create_bitmap(w, h);
   if (bitmap) {
      _al_register_destructor(_al_dtor_list, bitmap,
         (void (*)(void *))al_destroy_bitmap);
   }
   return bitmap;
}



/* Function: al_destroy_bitmap
 */
void al_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   if (!bitmap) {
      return;
   }

   /* As a convenience, implicitly untarget the bitmap on the calling thread
    * before it is destroyed, but maintain the current display.
    */
   if (bitmap == al_get_target_bitmap()) {
      ALLEGRO_DISPLAY *display = al_get_current_display();
      if (display)
         al_set_target_bitmap(al_get_backbuffer(display));
      else
         al_set_target_bitmap(NULL);
   }

   _al_unregister_destructor(_al_dtor_list, bitmap);

   if (bitmap->parent) {
      /* It's a sub-bitmap */
      if (bitmap->display)
         _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);
      al_free(bitmap);
      return;
   }

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      destroy_memory_bitmap(bitmap);
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



/* Function: al_convert_mask_to_alpha
 */
void al_convert_mask_to_alpha(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR mask_color)
{
   ALLEGRO_LOCKED_REGION *lr;
   int x, y;
   ALLEGRO_COLOR pixel;
   ALLEGRO_COLOR alpha_pixel;
   ALLEGRO_STATE state;

   if (!(lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, 0))) {
      ALLEGRO_ERROR("Couldn't lock bitmap.");
      return;
   }

   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
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

   al_unlock_bitmap(bitmap);

   al_restore_state(&state);
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



/* Function: al_reset_clipping_rectangle
 */
void al_reset_clipping_rectangle(void)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   if (bitmap) {
      int w = al_get_bitmap_width(bitmap);
      int h = al_get_bitmap_height(bitmap);
      al_set_clipping_rectangle(0, 0, w, h);
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
      bitmap = al_calloc(1, sizeof *bitmap);
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
   al_identity_transform(&bitmap->transform);
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


/* Function: al_get_parent_bitmap
 */
ALLEGRO_BITMAP *al_get_parent_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ASSERT(bitmap);
   return bitmap->parent;
}


/* Function: al_clone_bitmap
 */
ALLEGRO_BITMAP *al_clone_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *clone;
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_LOCKED_REGION *src_region;
   ASSERT(bitmap);

   clone = al_create_bitmap(bitmap->w, bitmap->h);
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

   ALLEGRO_DEBUG("converting memory bitmap %p to display bitmap\n", bitmap);

   /* Allocate a temporary bitmap which will hold the data during the
    * conversion process.
    */
   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);

   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   al_set_new_bitmap_format(bitmap->format);
   tmp = do_create_bitmap(bitmap->w, bitmap->h);

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
    * automatically by al_create_bitmap()
    */
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

   ALLEGRO_DEBUG("converting display bitmap %p to memory bitmap\n", bitmap);

   /* Allocate a temporary bitmap which will hold the data
    * during the conversion process. */

   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   al_set_new_bitmap_format(bitmap->format);
   tmp = create_memory_bitmap(bitmap->w, bitmap->h);

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
   ASSERT(src);
   ASSERT(dst);
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
