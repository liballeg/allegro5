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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"

#include "allegro5/a5_iio.h"

#include "allegro5/a5_font.h"

#include "font.h"


/* state information for the bitmap font importer */
static int import_x = 0;
static int import_y = 0;



static bool color_compare(ALLEGRO_COLOR c1, ALLEGRO_COLOR c2)
{
   return c1.r != c2.r || c1.g != c2.g || c1.b != c2.b || c1.a != c2.a;
}

/* splits bitmaps into sub-sprites, using regions bounded by col #255 */
static void font_find_character(ALLEGRO_BITMAP *bmp, int *x, int *y, int *w, int *h)
{
   const int bmp_w = al_get_bitmap_width(bmp);
   const int bmp_h = al_get_bitmap_height(bmp);
   ALLEGRO_COLOR c;
   ALLEGRO_LOCKED_REGION lr;

   c = al_map_rgb(255, 255, 0);

   al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_READONLY);

   /* look for top left corner of character */
   while (1) {
      if (*x+1 >= bmp_w) {
         *x = 0;
         (*y)++;
         if (*y+1 >= bmp_h) {
            *w = 0;
            *h = 0;
            al_unlock_bitmap(bmp);
            return;
         }
      }
      if (!(
         color_compare(al_get_pixel(bmp, *x, *y), c) ||
         color_compare(al_get_pixel(bmp, *x+1, *y), c) ||
         color_compare(al_get_pixel(bmp, *x, *y+1), c) ||
         !color_compare(al_get_pixel(bmp, *x+1, *y+1), c)
      )) {
         break;
      }
      (*x)++;
   }

   /* look for right edge of character */
   *w = 0;
   while ((*x+*w+1 < bmp_w) &&
      (!color_compare(al_get_pixel(bmp, *x+*w+1, *y), c) &&
      color_compare(al_get_pixel(bmp, *x+*w+1, *y+1), c))) {
      (*w)++;
   }

   /* look for bottom edge of character */
   *h = 0;
   while ((*y+*h+1 < bmp_h) &&
      (!color_compare(al_get_pixel(bmp, *x, *y+*h+1), c) &&
      color_compare(al_get_pixel(bmp, *x+1, *y+*h+1), c))) {
         (*h)++;
   }

   al_unlock_bitmap(bmp);
}



/* import_bitmap_font_color:
 *  Helper for import_bitmap_font, below.
 */
static int import_bitmap_font_color(ALLEGRO_BITMAP *import_bmp, ALLEGRO_BITMAP** bits, ALLEGRO_BITMAP* glyphs, int num)
{
   int w = 1, h = 1, i;
   int ret = 0;
   ALLEGRO_COLOR col;
   ALLEGRO_STATE backup;
         
   col = al_map_rgb(255, 255, 0);

   al_store_state(&backup, ALLEGRO_STATE_BITMAP);
   //al_set_new_bitmap_flags(0);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
   
   for(i = 0; i < num; i++) {
      if(w > 0 && h > 0) font_find_character(import_bmp, &import_x, &import_y, &w, &h);
      if(w <= 0 || h <= 0) {
	 bits[i] = al_create_bitmap(8, 8);
	 if(!bits[i]) {
            ret = -1;
            goto done;
         }
         al_set_target_bitmap(bits[i]);
         al_clear(col);
      }
      else {
         bits[i] = al_create_sub_bitmap(glyphs, import_x + 1, import_y + 1, w, h);
	 if(!bits[i]) {
            ret = -1;
            goto done;
         }
	 import_x += w;
      }
   }

done:
   al_restore_state(&backup);

   return ret;
}




/* bitmap_font_count:
 *  Helper for `import_bitmap_font', below.
 */
static int bitmap_font_count(ALLEGRO_BITMAP* bmp)
{
   int x = 0, y = 0, w = 0, h = 0;
   int num = 0;

   while (1) {
      font_find_character(bmp, &x, &y, &w, &h);
      if (w <= 0 || h <= 0)
         break;
      num++;
      x += w;
   }

   return num;
}



/* Function: al_font_load_bitmap_font
 * Import routine for the Allegro bitmap font format.
 */
ALLEGRO_FONT *al_font_load_bitmap_font(const char *fname, void *param)
{
   /* NB: `end' is -1 if we want every glyph */
   int color_conv_mode;
   ALLEGRO_BITMAP *import_bmp;
   ALLEGRO_FONT *f;
   ALLEGRO_COLOR col;
   unsigned char r,g,b,a;
   ALLEGRO_STATE backup;
   ASSERT(fname);

   (void) param;

   /* Don't change the colourdepth of the bitmap if it is 8 bit */
   (void) color_conv_mode;
   /*
   color_conv_mode = get_color_conversion();
   set_color_conversion(COLORCONV_MOST | COLORCONV_KEEP_TRANS);
   import_bmp = load_bitmap(fname, pal);
   set_color_conversion(color_conv_mode);
   */

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
   import_bmp = al_iio_load(fname);
   al_restore_state(&backup);

   if(!import_bmp) 
     return NULL;

   col = al_get_pixel(import_bmp, 0, 0);
   al_unmap_rgba(col, &r, &g, &b, &a);

   /* We assume a single unicode range, starting at the space
    * character.
    */
   f = al_font_grab_font_from_bitmap(import_bmp, 1, 32,
      32 + bitmap_font_count(import_bmp) - 1);

   al_destroy_bitmap(import_bmp);

   return f;
}



/* Function: al_font_grab_font_from_bitmap
 */
ALLEGRO_FONT *al_font_grab_font_from_bitmap(
   ALLEGRO_BITMAP *bmp,
   int ranges, ...)
{
   va_list args;
   ALLEGRO_FONT *f;
   ALLEGRO_FONT_COLOR_DATA *cf, *prev = NULL;
   ALLEGRO_STATE backup;
   int i;
   ALLEGRO_COLOR white = al_map_rgb(255, 255, 255);

   ASSERT(bmp)
   
   va_start(args, ranges);

   import_x = 0;
   import_y = 0;

   f = _AL_MALLOC(sizeof *f);
   memset(f, 0, sizeof *f);
   f->vtable = al_font_vtable_color;

   al_store_state(&backup, ALLEGRO_STATE_BITMAP | ALLEGRO_STATE_BLENDER);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);

   for (i = 0; i < ranges; i++) {
      int first = va_arg(args, int);
      int last = va_arg(args, int);
      int n = 1 + last - first;
      cf = _AL_MALLOC(sizeof(ALLEGRO_FONT_COLOR_DATA));
      memset(cf, 0, sizeof *cf);

      if (prev)
         prev->next = cf;
      else
         f->data = cf;
      
      cf->bitmaps = _AL_MALLOC(sizeof(ALLEGRO_BITMAP*) * n);

      cf->glyphs = al_create_bitmap(
         al_get_bitmap_width(bmp), al_get_bitmap_height(bmp));
      if (!cf->glyphs)
         goto cleanup_and_fail_on_error;

      al_set_target_bitmap(cf->glyphs);
      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, white);
      al_draw_bitmap(bmp, 0, 0, 0);

      if(import_bitmap_font_color(bmp, cf->bitmaps, cf->glyphs, n)) {
         goto cleanup_and_fail_on_error;
      }
      else {
         cf->begin = first;
         cf->end = last + 1;
         prev = cf;
      }
   }
   va_end(args);
   al_restore_state(&backup);
   
   cf = f->data;
   if (cf)
      f->height = al_get_bitmap_height(cf->bitmaps[0]);

   return f;
cleanup_and_fail_on_error:
   va_end(args);
   al_restore_state(&backup);
   al_font_destroy_font(f);
   return NULL;
}

