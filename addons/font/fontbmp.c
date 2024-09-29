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
 *      Read font from a bitmap.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro5/allegro.h"

#include "allegro5/allegro_font.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"

#include "font.h"

A5O_DEBUG_CHANNEL("font")


static void font_find_character(uint32_t *data, int pitch,
   int bmp_w, int bmp_h,
   int *x, int *y, int *w, int *h)
{
   /* The pixel at position 0/0 is used as background color. */
   uint32_t c = data[0];
   pitch >>= 2;

   /* look for top left corner of character */
   while (1) {
      /* Reached border? */
      if (*x >= bmp_w - 1) {
         *x = 0;
         (*y)++;
         if (*y >= bmp_h - 1) {
            *w = 0;
            *h = 0;
            return;
         }
      }
      if (
         data[*x + *y * pitch] == c &&
         data[(*x + 1) + *y * pitch] == c &&
         data[*x + (*y + 1) * pitch] == c &&
         data[(*x + 1) + (*y + 1) * pitch] != c) {
         break;
      }
      (*x)++;
   }

   /* look for right edge of character */
   *w = 1;
   while ((*x + *w + 1 < bmp_w) &&
      data[(*x + *w + 1) + (*y + 1) * pitch] != c) {
      (*w)++;
   }

   /* look for bottom edge of character */
   *h = 1;
   while ((*y + *h + 1 < bmp_h) &&
      data[*x + 1 + (*y + *h + 1) * pitch] != c) {
      (*h)++;
   }
}



/* import_bitmap_font_color:
 *  Helper for import_bitmap_font, below.
 */
static int import_bitmap_font_color(uint32_t *data, int pitch,
   int bmp_w, int bmp_h,
   A5O_BITMAP **bits, A5O_BITMAP *glyphs, int num,
   int *import_x, int *import_y)
{
   int w, h, i;

   for (i = 0; i < num; i++) {
      font_find_character(data, pitch, bmp_w, bmp_h,
         import_x, import_y, &w, &h);
      if (w <= 0 || h <= 0) {
         A5O_ERROR("Unable to find character %d\n", i);
         return -1;
      }
      else {
         bits[i] = al_create_sub_bitmap(glyphs,
            *import_x + 1, *import_y + 1, w, h);
         *import_x += w;
      }
   }
   return 0;
}




/* bitmap_font_count:
 *  Helper for `import_bitmap_font', below.
 */
static int bitmap_font_count(A5O_BITMAP* bmp)
{
   int x = 0, y = 0, w = 0, h = 0;
   int num = 0;
   A5O_LOCKED_REGION *lock;
   
   lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_RGBA_8888,
      A5O_LOCK_READONLY);

   while (1) {
      font_find_character(lock->data, lock->pitch,
         al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
         &x, &y, &w, &h);
      if (w <= 0 || h <= 0)
         break;
      num++;
      x += w;
   }
   
   al_unlock_bitmap(bmp);

   return num;
}



A5O_FONT *_al_load_bitmap_font(const char *fname, int size, int font_flags)
{
   A5O_BITMAP *import_bmp;
   A5O_FONT *f;
   A5O_STATE backup;
   int range[2];
   int bmp_flags;
   ASSERT(fname);

   (void)size;

   bmp_flags = 0;
   if (font_flags & A5O_NO_PREMULTIPLIED_ALPHA) {
      bmp_flags |= A5O_NO_PREMULTIPLIED_ALPHA;
   }

   al_store_state(&backup, A5O_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(A5O_MEMORY_BITMAP);
   al_set_new_bitmap_format(A5O_PIXEL_FORMAT_ANY_WITH_ALPHA);
   import_bmp = al_load_bitmap_flags(fname, bmp_flags);
   al_restore_state(&backup);

   if (!import_bmp) {
      A5O_ERROR("Couldn't load bitmap from '%s'\n", fname);
      return NULL;
   }

   /* We assume a single unicode range, starting at the space
    * character.
    */
   range[0] = 32;
   range[1] = 32 + bitmap_font_count(import_bmp) - 1;

   f = al_grab_font_from_bitmap(import_bmp, 1, range);

   al_destroy_bitmap(import_bmp);

   return f;
}



/* Function: al_load_bitmap_font
 */
A5O_FONT *al_load_bitmap_font(const char *fname)
{
   int flags = 0;

   /* For backwards compatibility with the 5.0 branch. */
   if (al_get_new_bitmap_flags() & A5O_NO_PREMULTIPLIED_ALPHA) {
      flags |= A5O_NO_PREMULTIPLIED_ALPHA;
   }

   return _al_load_bitmap_font(fname, 0, flags);
}



/* Function: al_load_bitmap_font_flags
 */
A5O_FONT *al_load_bitmap_font_flags(const char *fname, int flags)
{
   return _al_load_bitmap_font(fname, 0, flags);
}



/* Function: al_grab_font_from_bitmap
 */
A5O_FONT *al_grab_font_from_bitmap(A5O_BITMAP *bmp,
   int ranges_n, const int ranges[])
{
   A5O_FONT *f;
   A5O_FONT_COLOR_DATA *cf, *prev = NULL;
   A5O_STATE backup;
   int i;
   A5O_COLOR mask = al_get_pixel(bmp, 0, 0);
   A5O_BITMAP *glyphs = NULL, *unmasked = NULL;
   int import_x = 0, import_y = 0;
   A5O_LOCKED_REGION *lock = NULL;
   int w, h;

   ASSERT(bmp);
   
   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);

   f = al_calloc(1, sizeof *f);
   f->vtable = &_al_font_vtable_color;
   
   al_store_state(&backup, A5O_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(A5O_MEMORY_BITMAP);
   al_set_new_bitmap_format(A5O_PIXEL_FORMAT_ANY_WITH_ALPHA);
   unmasked = al_clone_bitmap(bmp);
   /* At least with OpenGL, texture pixels at the very border of
    * the glyph are sometimes partly sampled from the yellow mask
    * pixels. To work around this, we replace the mask with full
    * transparency.
    * And we best do it on a memory copy to avoid loading back a texture.
    */
   al_convert_mask_to_alpha(unmasked, mask);
   al_restore_state(&backup);   

   al_store_state(&backup, A5O_STATE_BITMAP | A5O_STATE_BLENDER);
   // Use the users preferred format, so don't set this below!
   //al_set_new_bitmap_format(A5O_PIXEL_FORMAT_ANY_WITH_ALPHA);

   for (i = 0; i < ranges_n; i++) {
      int first = ranges[i * 2];
      int last = ranges[i * 2 + 1];
      int n = 1 + last - first;
      cf = al_calloc(1, sizeof(A5O_FONT_COLOR_DATA));

      if (prev)
         prev->next = cf;
      else
         f->data = cf;
      
      cf->bitmaps = al_malloc(sizeof(A5O_BITMAP*) * n);
      cf->bitmaps[0] = NULL;

      if (!glyphs) {
         glyphs = al_clone_bitmap(unmasked);
         if (!glyphs) {
            A5O_ERROR("Unable clone bitmap.\n");
            goto cleanup_and_fail_on_error;
         }

         lock = al_lock_bitmap(bmp,
            A5O_PIXEL_FORMAT_RGBA_8888, A5O_LOCK_READONLY);
      }
      cf->glyphs = glyphs;

      if (import_bitmap_font_color(lock->data, lock->pitch, w, h,
         cf->bitmaps, cf->glyphs, n,
         &import_x, &import_y)) {
         goto cleanup_and_fail_on_error;
      }
      else {
         cf->begin = first;
         cf->end = last + 1;
         prev = cf;
      }
   }
   al_restore_state(&backup);
   
   cf = f->data;
   if (cf && cf->bitmaps[0])
      f->height = al_get_bitmap_height(cf->bitmaps[0]);

   if (lock)
      al_unlock_bitmap(bmp);

   if (unmasked)
       al_destroy_bitmap(unmasked);

   f->dtor_item = _al_register_destructor(_al_dtor_list, "font", f,
      (void (*)(void  *))al_destroy_font);

   return f;

cleanup_and_fail_on_error:

   if (lock)
      al_unlock_bitmap(bmp);
   al_restore_state(&backup);
   al_destroy_font(f);
   if (unmasked)
       al_destroy_bitmap(unmasked);
   return NULL;
}

/* vim: set sts=3 sw=3 et: */
