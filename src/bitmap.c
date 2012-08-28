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
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_system.h"

ALLEGRO_DEBUG_CHANNEL("bitmap")

static ALLEGRO_COLOR solid_white = {1, 1, 1, 1};

struct TO_BE_CONVERTED_LIST {
   ALLEGRO_MUTEX *mutex;
   _AL_VECTOR bitmaps;
};

static struct TO_BE_CONVERTED_LIST to_be_converted;

static void cleanup_to_be_converted_bitmaps(void)
{
   _al_vector_free(&to_be_converted.bitmaps);
   al_destroy_mutex(to_be_converted.mutex);    
}

/* This is called in al_install_system. Exit functions are called in
 * al_uninstall_system.
 */
void _al_init_to_be_converted_bitmaps(void)
{
   to_be_converted.mutex = al_create_mutex_recursive();
   _al_vector_init(&to_be_converted.bitmaps, sizeof(ALLEGRO_BITMAP *));
   _al_add_exit_func(cleanup_to_be_converted_bitmaps,
      "cleanup_to_be_converted_bitmaps");
}

static void check_to_be_converted_list_add(ALLEGRO_BITMAP *bitmap)
{
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
      return;
   if (bitmap->flags & ALLEGRO_CONVERT_BITMAP) {
      ALLEGRO_BITMAP **back;
      al_lock_mutex(to_be_converted.mutex);
      back = _al_vector_alloc_back(&to_be_converted.bitmaps);
      *back = bitmap;
      al_unlock_mutex(to_be_converted.mutex);
   }
}


static void check_to_be_converted_list_remove(ALLEGRO_BITMAP *bitmap)
{
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
      return;
   if (bitmap->flags & ALLEGRO_CONVERT_BITMAP) {
      al_lock_mutex(to_be_converted.mutex);
      _al_vector_find_and_delete(&to_be_converted.bitmaps, &bitmap);
      al_unlock_mutex(to_be_converted.mutex);
   }
}


/* Creates a memory bitmap.
 */
static ALLEGRO_BITMAP *_al_create_memory_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   int pitch;
   int format = al_get_new_bitmap_format();

   format = _al_get_real_pixel_format(al_get_current_display(), format);

   bitmap = al_calloc(1, sizeof *bitmap);

   pitch = w * al_get_pixel_size(format);

   bitmap->vt = NULL;
   bitmap->format = format;

   /* If this is really a video bitmap, we add it to the list of to
    * be converted bitmaps.
    */
   bitmap->flags = al_get_new_bitmap_flags() | ALLEGRO_MEMORY_BITMAP;
   bitmap->flags &= ~ALLEGRO_VIDEO_BITMAP;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->pitch = pitch;
   bitmap->display = NULL;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr_excl = w;
   bitmap->cb_excl = h;
   al_identity_transform(&bitmap->transform);
   al_identity_transform(&bitmap->inverse_transform);
   bitmap->inverse_transform_dirty = false;
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;
   bitmap->memory = al_malloc(pitch * h);
   
   check_to_be_converted_list_add(bitmap);
   return bitmap;
}



static void _al_destroy_memory_bitmap(ALLEGRO_BITMAP *bmp)
{
   check_to_be_converted_list_remove(bmp);

   if (bmp->memory)
      al_free(bmp->memory);
   al_free(bmp);
}



static ALLEGRO_BITMAP *do_create_bitmap(int w, int h,
   bool (*custom_upload)(ALLEGRO_BITMAP *bitmap, void *data),
   void *custom_data)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP **back;
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   ALLEGRO_DISPLAY *current_display = al_get_current_display();
   int64_t mul;
   bool result;

   /* Reject bitmaps where a calculation pixel_size*w*h would overflow
    * int.  Supporting such bitmaps would require a lot more work.
    */
   mul = 4 * (int64_t) w * (int64_t) h;
   if (mul > (int64_t) INT_MAX) {
      ALLEGRO_WARN("Rejecting %dx%d bitmap\n", w, h);
      return NULL;
   }

   if ((al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) ||
         (!current_display || !current_display->vt ||
         current_display->vt->create_bitmap == NULL) ||
         (system->displays._size < 1)) {

      if (al_get_new_bitmap_flags() & ALLEGRO_VIDEO_BITMAP)
         return NULL;

      return _al_create_memory_bitmap(w, h);
   }

   /* Else it's a display bitmap */

   bitmap = current_display->vt->create_bitmap(current_display, w, h);
   if (!bitmap) {
      ALLEGRO_ERROR("failed to create display bitmap");
      return NULL;
   }

   bitmap->display = current_display;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->locked = false;
   bitmap->cl = 0;
   bitmap->ct = 0;
   bitmap->cr_excl = w;
   bitmap->cb_excl = h;
   al_identity_transform(&bitmap->transform);
   al_identity_transform(&bitmap->inverse_transform);
   bitmap->inverse_transform_dirty = false;
   bitmap->parent = NULL;
   bitmap->xofs = 0;
   bitmap->yofs = 0;
   bitmap->flags |= ALLEGRO_VIDEO_BITMAP;
   bitmap->dirty = !(bitmap->flags & ALLEGRO_NO_PRESERVE_TEXTURE);

   /* The display driver should have set the bitmap->memory field if
    * appropriate; video bitmaps may leave it NULL.
    */

   if (custom_upload) {
      result = custom_upload(bitmap, custom_data);
   }
   else {
      ASSERT(bitmap->pitch >= w * al_get_pixel_size(bitmap->format));
      result = bitmap->vt->upload_bitmap(bitmap);
   }

   if (!result) {
      al_destroy_bitmap(bitmap);
      if (al_get_new_bitmap_flags() & ALLEGRO_VIDEO_BITMAP)
         return NULL;
      /* With ALLEGRO_CONVERT_BITMAP, just use a memory bitmap instead if
      * video failed.
      */
      return _al_create_memory_bitmap(w, h);
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
   ALLEGRO_BITMAP *bitmap = do_create_bitmap(w, h, NULL, NULL);
   if (bitmap) {
      _al_register_destructor(_al_dtor_list, bitmap,
         (void (*)(void *))al_destroy_bitmap);
   }
   
   return bitmap;
}


/* Function: al_create_custom_bitmap
 */
ALLEGRO_BITMAP *al_create_custom_bitmap(int w, int h,
   bool (*upload)(ALLEGRO_BITMAP *bitmap, void *data), void *data)
{
   ALLEGRO_BITMAP *bitmap = do_create_bitmap(w, h, upload, data);
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

   _al_unregister_destructor(_al_dtor_list, bitmap);

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

static void _bitmap_drawer(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_DISPLAY *display = dest->display;
   ASSERT(bitmap->parent == NULL);
   ASSERT(!(flags & (ALLEGRO_FLIP_HORIZONTAL | ALLEGRO_FLIP_VERTICAL)));
   ASSERT(bitmap != dest && bitmap != dest->parent);

   /* If destination is memory, do a memory blit */
   if (dest->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh, 0, 0, flags);
   }
   else {
      /* if source is memory or incompatible */
      if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
          (!al_is_compatible_bitmap(bitmap)))
      {
         if (display && display->vt->draw_memory_bitmap_region) {
            display->vt->draw_memory_bitmap_region(display, bitmap,
               sx, sy, sw, sh, flags);
         }
         else {
            _al_draw_bitmap_region_memory(bitmap, tint, sx, sy, sw, sh, 0, 0, flags);
         }
      }
      else {
         /* Compatible display bitmap, use full acceleration */
         bitmap->vt->draw_bitmap_region(bitmap, tint, sx, sy, sw, sh, flags);
      }
   }
}


static void _draw_tinted_rotated_scaled_bitmap_region(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint, float cx, float cy, float angle,
   float xscale, float yscale,
   float sx, float sy, float sw, float sh, float dx, float dy,
   int flags)
{
   ALLEGRO_TRANSFORM backup;
   ALLEGRO_TRANSFORM t;
   ALLEGRO_BITMAP *parent = bitmap;
   float const orig_sw = sw;
   float const orig_sh = sh;
   ASSERT(bitmap);

   al_copy_transform(&backup, al_get_current_transform());
   al_identity_transform(&t);
   
   if (bitmap->parent) {
      parent = bitmap->parent;
      sx += bitmap->xofs;
      sy += bitmap->yofs;
   }
   
   if (sx < 0) {
      sw += sx;
      al_translate_transform(&t, -sx, 0);
      sx = 0;
   }
   if (sy < 0) {
      sh += sy;
      al_translate_transform(&t, 0, -sy);
      sy = 0;
   }
   if (sx + sw > parent->w)
      sw = parent->w - sx;
   if (sy + sh > parent->h)
      sh = parent->h - sy;

   if (flags & ALLEGRO_FLIP_HORIZONTAL) {
      al_scale_transform(&t, -1, 1);
      al_translate_transform(&t, orig_sw, 0);
      flags &= ~ALLEGRO_FLIP_HORIZONTAL;
   }

   if (flags & ALLEGRO_FLIP_VERTICAL) {
      al_scale_transform(&t, 1, -1);
      al_translate_transform(&t, 0, orig_sh);
      flags &= ~ALLEGRO_FLIP_VERTICAL;
   }

   al_translate_transform(&t, -cx, -cy);
   al_scale_transform(&t, xscale, yscale);
   al_rotate_transform(&t, angle);
   al_translate_transform(&t, dx, dy);
   al_compose_transform(&t, &backup);

   al_use_transform(&t);
   _bitmap_drawer(parent, tint, sx, sy, sw, sh, flags);
   al_use_transform(&backup);
}


/* Function: al_draw_tinted_bitmap_region
 */
void al_draw_tinted_bitmap_region(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, float dx, float dy,
   int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      0, 0, 0, 1, 1, sx, sy, sw, sh, dx, dy, flags);
}


/* Function: al_draw_tinted_bitmap
 */
void al_draw_tinted_bitmap(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint,
   float dx, float dy, int flags)
{
   ASSERT(bitmap);
   al_draw_tinted_bitmap_region(bitmap, tint, 0, 0,
      bitmap->w, bitmap->h, dx, dy, flags);
}


/* Function: al_draw_bitmap
 */
void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
   al_draw_tinted_bitmap(bitmap, solid_white, dx, dy, flags);
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
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      0, 0, 0,
      dw / sw, dh / sh,
      sx, sy, sw, sh, dx, dy, flags);
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
   al_draw_tinted_scaled_rotated_bitmap(bitmap, tint, cx, cy, dx, dy,
      1, 1, angle, flags);
}

/* Function: al_draw_rotated_bitmap
 */
void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float angle, int flags)
{
   al_draw_tinted_rotated_bitmap(bitmap, solid_white, cx, cy, dx, dy,
      angle, flags);
}


/* Function: al_draw_tinted_scaled_rotated_bitmap
 */
void al_draw_tinted_scaled_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      cx, cy, angle,
      xscale, yscale,
      0, 0, bitmap->w, bitmap->h, dx, dy, flags);
}


/* Function: al_draw_tinted_scaled_rotated_bitmap_region
 */
void al_draw_tinted_scaled_rotated_bitmap_region(ALLEGRO_BITMAP *bitmap,
   float sx, float sy, float sw, float sh,
   ALLEGRO_COLOR tint,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   _draw_tinted_rotated_scaled_bitmap_region(bitmap, tint,
      cx, cy, angle,
      xscale, yscale,
      sx, sy, sw, sh, dx, dy, flags);
}


/* Function: al_draw_scaled_rotated_bitmap
 */
void al_draw_scaled_rotated_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   al_draw_tinted_scaled_rotated_bitmap(bitmap, solid_white,
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

   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP) &&
         !(flags & ALLEGRO_LOCK_READONLY))
      bitmap->dirty = true;

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
      ASSERT(bitmap->memory);
      if (format == ALLEGRO_PIXEL_FORMAT_ANY || bitmap->format == format || f == bitmap->format) {
         bitmap->locked_region.data = bitmap->memory
            + bitmap->pitch * y + x * al_get_pixel_size(bitmap->format);
         bitmap->locked_region.format = bitmap->format;
         bitmap->locked_region.pitch = bitmap->pitch;
         bitmap->locked_region.pixel_size = al_get_pixel_size(bitmap->format);
      }
      else {
         bitmap->locked_region.pitch = al_get_pixel_size(f) * width;
         bitmap->locked_region.data = al_malloc(bitmap->locked_region.pitch*height);
         bitmap->locked_region.format = f;
         bitmap->locked_region.pixel_size = al_get_pixel_size(f);
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
   
   bitmap = al_calloc(1, sizeof *bitmap);
   bitmap->vt = parent->vt;

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
   al_identity_transform(&bitmap->inverse_transform);
   bitmap->inverse_transform_dirty = false;
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


static void transfer_bitmap_data(ALLEGRO_BITMAP *src, ALLEGRO_BITMAP *dst)
{
   ALLEGRO_LOCKED_REGION *dst_region;
   ALLEGRO_LOCKED_REGION *src_region;

   if (!(src_region = al_lock_bitmap(src, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY)))
      return;

   if (!(dst_region = al_lock_bitmap(dst, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY))) {
      al_unlock_bitmap(src);
      return;
   }

   _al_convert_bitmap_data(
      src_region->data, src_region->format, src_region->pitch,
      dst_region->data, dst_region->format, dst_region->pitch,
      0, 0, 0, 0, src->w, src->h);

   al_unlock_bitmap(src);
   al_unlock_bitmap(dst);
}


/* Function: al_clone_bitmap
 */
ALLEGRO_BITMAP *al_clone_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *clone;
   ASSERT(bitmap);

   clone = al_create_bitmap(bitmap->w, bitmap->h);
   if (!clone)
      return NULL;
   transfer_bitmap_data(bitmap, clone);
   return clone;
}


/* Function: al_convert_bitmap
 */
void al_convert_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP temp = *bitmap;
   ALLEGRO_BITMAP *clone;
   bool want_memory = (al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) != 0;
   bool clone_memory;

   /* TODO: Could check here if the bitmap isn't converted already.
    * However both flags and format need to be checked.
    */

   if (bitmap->parent) {
      bool parent_mem = (bitmap->parent->flags & ALLEGRO_MEMORY_BITMAP) != 0;
      if (parent_mem != want_memory) {
         al_convert_bitmap(bitmap->parent);
      }
      clone = al_create_sub_bitmap(bitmap->parent,
         bitmap->xofs, bitmap->yofs, bitmap->w, bitmap->h);
   }
   else if (!(bitmap->flags & ALLEGRO_NO_PRESERVE_TEXTURE)) {
      ASSERT(bitmap->memory);
      clone = al_create_bitmap(bitmap->w, bitmap->h);
      if (clone != 0){
         clone->flags &= ~ALLEGRO_NO_PRESERVE_TEXTURE;
         temp.memory = clone->memory;
         clone->memory = bitmap->memory;
         if (!(clone->flags & ALLEGRO_MEMORY_BITMAP)) {
            ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(
                                                       clone,
                                                       bitmap->format,
                                                       ALLEGRO_LOCK_WRITEONLY
                                                      );
            if (lr) {
               int line_size, y;
               line_size = al_get_pixel_size(bitmap->format) * bitmap->w;
               for (y = 0; y < bitmap->h; y++) {
                  unsigned char *p = ((unsigned char *)lr->data) + lr->pitch * y;
                  unsigned char *p2 = ((unsigned char *)bitmap->memory) + line_size * y;
                  memcpy(p, p2, line_size);
               }
               al_unlock_bitmap(clone);
            }
         }
      }
   }
   else {
      clone = al_clone_bitmap(bitmap);
   }

   if (!clone) {
      return;
   }

   clone_memory = (clone->flags & ALLEGRO_MEMORY_BITMAP) != 0;

   if (clone_memory != want_memory) {
      /* We cannot convert. */
      al_destroy_bitmap(clone);
      return;
   }

   /* TODO: Managing those lists is a pain, maybe there is a way to
    * do this more easily?
    */

   /* Manually remove the clone from various lists it can end up in. */
   if (clone->display) {
      _al_vector_find_and_delete(&clone->display->bitmaps, &clone);
   }
   
   check_to_be_converted_list_remove(clone);

   /* Manually remove the bitmap from lists it should not be in after
    * the conversion.
    */
   if (bitmap->display && !clone->display) {
      _al_vector_find_and_delete(&bitmap->display->bitmaps, &bitmap);
   }
   
   check_to_be_converted_list_remove(bitmap);

   /* Manually add the bitmap to lists it should be in after the
    * conversion.
    */
   if (!bitmap->display && clone->display) {
      ALLEGRO_BITMAP **back;
      back = _al_vector_alloc_back(&clone->display->bitmaps);
      *back = bitmap;
   }

   *bitmap = *clone; /* We get the new bitmap's extra pointer. */
   /* Preserve bitmap state. */
   bitmap->cl = temp.cl;
   bitmap->ct = temp.ct;
   bitmap->cr_excl = temp.cr_excl;
   bitmap->cb_excl = temp.cb_excl;
   bitmap->transform = temp.transform;
   bitmap->inverse_transform = temp.inverse_transform;
   bitmap->inverse_transform_dirty = temp.inverse_transform_dirty;
   
   check_to_be_converted_list_add(bitmap);

   *clone = temp; /* So e.g. the extra pointer can be deleted properly. */
   al_destroy_bitmap(clone);
}


/* Function: al_convert_bitmaps
 */
void al_convert_bitmaps(void)
{
   ALLEGRO_STATE backup;
   ALLEGRO_DISPLAY *display = al_get_current_display();
   _AL_VECTOR copy;
   size_t i;
   if (!display) return;
   
   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

   al_lock_mutex(to_be_converted.mutex);
   
   _al_vector_init(&copy, sizeof(ALLEGRO_BITMAP *));
   for (i = 0; i <  _al_vector_size(&to_be_converted.bitmaps); i++) {
      ALLEGRO_BITMAP **bptr, **bptr2;
      bptr = _al_vector_ref(&to_be_converted.bitmaps, i);
      bptr2 = _al_vector_alloc_back(&copy);
      *bptr2 = *bptr;
   }
   _al_vector_free(&to_be_converted.bitmaps);
   _al_vector_init(&to_be_converted.bitmaps, sizeof(ALLEGRO_BITMAP *));
   for (i = 0; i < _al_vector_size(&copy); i++) {
      ALLEGRO_BITMAP **bptr;
      int flags;
      bptr = _al_vector_ref(&copy, i);
      flags = (*bptr)->flags;
      flags &= ~ALLEGRO_MEMORY_BITMAP;
      al_set_new_bitmap_flags(flags);
      al_set_new_bitmap_format((*bptr)->format);
      
      ALLEGRO_DEBUG("converting memory bitmap %p to display bitmap\n", *bptr);
      
      al_convert_bitmap(*bptr);
   }

   _al_vector_free(&copy);

   al_unlock_mutex(to_be_converted.mutex);

   al_restore_state(&backup);
}


/* Function: al_is_bitmap_locked
 */
bool al_is_bitmap_locked(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->locked;
}


/* Converts a memory bitmap to a display bitmap preserving its contents.
 * The created bitmap belongs to the current display.
 * 
 * If this is called for a sub-bitmap, the parent also is converted.
 */
void _al_convert_to_display_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_STATE backup;
   /* Do nothing if it is a display bitmap already. */
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
      return;

   ALLEGRO_DEBUG("converting memory bitmap %p to display bitmap\n", bitmap);

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   al_set_new_bitmap_format(bitmap->format);
   al_convert_bitmap(bitmap);
   al_restore_state(&backup);
}


/* Converts a display bitmap to a memory bitmap preserving its contents.
 * If this is called for a sub-bitmap, the parent also is converted.
 */
void _al_convert_to_memory_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_STATE backup;
   bool is_any = (bitmap->flags & ALLEGRO_CONVERT_BITMAP) != 0;

   /* Do nothing if it is a memory bitmap already. */
   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP)
      return;

   ALLEGRO_DEBUG("converting display bitmap %p to memory bitmap\n", bitmap);

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   al_set_new_bitmap_format(bitmap->format);
   al_convert_bitmap(bitmap);
   if (is_any) {
      /* We force-converted to memory above, but we still want to
       * keep the ANY flag if it was set so the bitmap can be
       * back-converted later.
       */
      bitmap->flags |= ALLEGRO_CONVERT_BITMAP;
      check_to_be_converted_list_add(bitmap);
   }
   al_restore_state(&backup);
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
