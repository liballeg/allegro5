/*       ______   ___    ___ 
 *	/\  _  \ /\_ \  /\_ \ 
 *	\ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *	 \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *	  \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *	   \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *	    \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *					   /\____/
 *					   \_/__/
 *
 *      Read font from a bitmap.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"

#include "a5_font.h"


/* state information for the bitmap font importer */
static int import_x = 0;
static int import_y = 0;



/* splits bitmaps into sub-sprites, using regions bounded by col #255 */
static void font_find_character(ALLEGRO_BITMAP *bmp, int *x, int *y, int *w, int *h)
{
   ALLEGRO_COLOR c, c2;
   ALLEGRO_LOCKED_REGION lr;

   al_map_rgb(&c, 255, 255, 0);

   al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_READONLY);

   /* look for top left corner of character */
   while (1) {
      if (*x+1 >= bmp->w) {
         *x = 0;
         (*y)++;
         if (*y+1 >= bmp->h) {
            *w = 0;
            *h = 0;
            al_unlock_bitmap(bmp);
            return;
         }
      }
      if (!(
         memcmp(al_get_pixel(bmp, *x, *y, &c2), &c, sizeof(ALLEGRO_COLOR)) ||
         memcmp(al_get_pixel(bmp, *x+1, *y, &c2), &c, sizeof(ALLEGRO_COLOR)) ||
         memcmp(al_get_pixel(bmp, *x, *y+1, &c2), &c, sizeof(ALLEGRO_COLOR)) ||
         !memcmp(al_get_pixel(bmp, *x+1, *y+1, &c2), &c, sizeof(ALLEGRO_COLOR))
      )) {
         break;
      }
      (*x)++;
   }

   /* look for right edge of character */
   *w = 0;
   while ((*x+*w+1 < bmp->w) && 
      (!memcmp(al_get_pixel(bmp, *x+*w+1, *y, &c2), &c, sizeof(ALLEGRO_COLOR)) &&
      memcmp(al_get_pixel(bmp, *x+*w+1, *y+1, &c2), &c, sizeof(ALLEGRO_COLOR)))) {
         (*w)++;
   }

   /* look for bottom edge of character */
   *h = 0;
   while ((*y+*h+1 < bmp->h) &&
      (!memcmp(al_get_pixel(bmp, *x, *y+*h+1, &c2), &c, sizeof(ALLEGRO_COLOR)) &&
      memcmp(al_get_pixel(bmp, *x+1, *y+*h+1, &c2), &c, sizeof(ALLEGRO_COLOR)))) {
         (*h)++;
   }

   al_unlock_bitmap(bmp);
}



/* import_bitmap_font_color:
 *  Helper for import_bitmap_font, below.
 */
static int import_bitmap_font_color(ALLEGRO_BITMAP *import_bmp, ALLEGRO_BITMAP** bits, int num)
{
   int w = 1, h = 1, i;
   int ret = 0;
   ALLEGRO_COLOR col;
         
   al_map_rgb(&col, 255, 255, 0);

   _al_push_target_bitmap();
   _al_push_new_bitmap_parameters();
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
         al_clear(&col);
      }
      else {
	 bits[i] = al_create_bitmap(w, h);
	 if(!bits[i]) {
            ret = -1;
            goto done;
         }
         al_set_target_bitmap(bits[i]);
         al_draw_bitmap_region(import_bmp, import_x + 1 + PIXEL_OFFSET, import_y + 1 + PIXEL_OFFSET, w, h, PIXEL_OFFSET, PIXEL_OFFSET, 0);
	 import_x += w;
      }
   }

done:
   _al_pop_target_bitmap();
   _al_pop_new_bitmap_parameters();

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



/* import routine for the Allegro .pcx font format */
A5FONT_FONT *a5font_load_bitmap_font(AL_CONST char *fname, void *param)
{
   /* NB: `end' is -1 if we want every glyph */
   int color_conv_mode;
   ALLEGRO_BITMAP *import_bmp;
   A5FONT_FONT *f;
   ASSERT(fname);

   /* Don't change the colourdepth of the bitmap if it is 8 bit */
   (void) color_conv_mode;
   /*
   color_conv_mode = get_color_conversion();
   set_color_conversion(COLORCONV_MOST | COLORCONV_KEEP_TRANS);
   import_bmp = load_bitmap(fname, pal);
   set_color_conversion(color_conv_mode);
   */

   _al_push_new_bitmap_parameters();
   //al_set_new_bitmap_flags(0);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
   import_bmp = al_load_bitmap(fname);
   _al_pop_new_bitmap_parameters();

   if(!import_bmp) 
     return NULL;

   ALLEGRO_COLOR col;
   al_get_pixel(import_bmp, 0, 0, &col);
   unsigned char r,g,b,a;
   al_unmap_rgba(&col, &r, &g, &b, &a);

   f = a5font_grab_font_from_bitmap(import_bmp);

   al_destroy_bitmap(import_bmp);

   return f;
}



/* work horse for grabbing a font from an Allegro bitmap */
A5FONT_FONT *a5font_grab_font_from_bitmap(ALLEGRO_BITMAP *bmp)
{
   int begin = ' ';
   int end = -1;
   A5FONT_FONT *f;
   ASSERT(bmp)

   import_x = 0;
   import_y = 0;

   f = _AL_MALLOC(sizeof *f);
   if (end == -1) end = bitmap_font_count(bmp) + begin;

   A5FONT_FONT_COLOR_DATA* cf = _AL_MALLOC(sizeof(A5FONT_FONT_COLOR_DATA));
   cf->bitmaps = _AL_MALLOC(sizeof(ALLEGRO_BITMAP*) * (end - begin));

   if( import_bitmap_font_color(bmp, cf->bitmaps, end - begin) ) {
      _AL_FREE(cf->bitmaps);
      _AL_FREE(cf);
      _AL_FREE(f);
      f = 0;
   }
   else {
      f->data = cf;
      f->vtable = a5font_font_vtable_color;
      f->height = cf->bitmaps[0]->h;

      cf->begin = begin;
      cf->end = end;
      cf->next = 0;
   }
   
   return f;
}

