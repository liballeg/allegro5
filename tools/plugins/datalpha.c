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
 *      Grabber plugin for editing alpha channels.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "../datedit.h"



/* creates a monochrome image containing the alpha channel of this bitmap */
static BITMAP *get_alpha_bitmap(BITMAP *bmp)
{
   BITMAP *alpha;
   int x, y, c, a;
   int got = FALSE;

   if (bitmap_color_depth(bmp) != 32)
      return NULL;

   alpha = create_bitmap_ex(24, bmp->w, bmp->h);

   for (y=0; y<bmp->h; y++) {
      for (x=0; x<bmp->w; x++) {
	 c = getpixel(bmp, x, y);
	 a = geta32(c);
	 putpixel(alpha, x, y, makecol24(a, a, a));
	 if (a)
	    got = TRUE;
      }
   }

   if (!got) {
      destroy_bitmap(alpha);
      return NULL;
   }

   return alpha;
}



/* checks whether the selection is capable of having an alpha channel */
static int alpha_query(int popup)
{
   if (popup) {
      DATAFILE *dat = grabber_single_selection();
      return ((dat) && ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE)));
   }

   return TRUE;
}



/* views the current alpha channel */
static int view_alpha(void)
{
   DATAFILE *dat = grabber_single_selection();
   DATAFILE tmpdat;
   RLE_SPRITE *rle;
   BITMAP *bmp;
   int ret = 0;
   int i;

   if ((!dat) || ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE))) {
      alert("You must select a single bitmap or RLE sprite",
	    "object before you can view the alpha channel", 
	    NULL, "Sorry", NULL, 13, 0);
      return 0;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      rle = dat->dat;
      bmp = create_bitmap_ex(rle->color_depth, rle->w, rle->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, rle, 0, 0);
   }
   else
      bmp = dat->dat;

   tmpdat.dat = get_alpha_bitmap(bmp);

   if (tmpdat.dat) {
      tmpdat.type = DAT_BITMAP;
      tmpdat.size = 0;
      tmpdat.prop = NULL;

      for (i=0; datedit_object_info[i]->type != DAT_END; i++) {
	 if ((datedit_object_info[i]->type == DAT_BITMAP) && (datedit_object_info[i]->dclick)) {
	    ret = datedit_object_info[i]->dclick(&tmpdat);
	    break;
	 }
      }

      destroy_bitmap(tmpdat.dat);
   }
   else
      alert("There is no alpha channel in this image", NULL, NULL, "Sorry", NULL, 13, 0);

   if (dat->type == DAT_RLE_SPRITE)
      destroy_bitmap(bmp);

   return ret;
}



/* exports the current alpha channel */
static int export_alpha(void)
{
   DATAFILE *dat = grabber_single_selection();
   DATAFILE tmpdat;
   RLE_SPRITE *rle;
   BITMAP *bmp;
   char buf[256], name[FILENAME_LENGTH] = EMPTY_STRING;
   AL_CONST char *ext;
   int ret = 0;

   if ((!dat) || ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE))) {
      alert("You must select a single bitmap or RLE sprite",
	    "object before you can export the alpha channel", 
	    NULL, "Sorry", NULL, 13, 0);
      return 0;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      rle = dat->dat;
      bmp = create_bitmap_ex(rle->color_depth, rle->w, rle->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, rle, 0, 0);
   }
   else
      bmp = dat->dat;

   tmpdat.dat = get_alpha_bitmap(bmp);

   if (tmpdat.dat) {
      tmpdat.type = DAT_BITMAP;
      tmpdat.size = 0;
      tmpdat.prop = NULL;

      ext = datedit_export_ext(DAT_BITMAP);
      sprintf(buf, "Export alpha (%s)", ext);

      strcpy(name, grabber_import_file);
      *get_filename(name) = 0;

      if (file_select_ex(buf, name, ext, sizeof(name), 0, 0)) {
	 fix_filename_case(name);
	 strcpy(grabber_import_file, name);
	 grabber_busy_mouse(TRUE);
	 datedit_export(&tmpdat, name);
	 grabber_busy_mouse(FALSE);
      }

      destroy_bitmap(tmpdat.dat);
      ret = D_REDRAW;
   }
   else
      alert("There is no alpha channel in this image", NULL, NULL, "Sorry", NULL, 13, 0);

   if (dat->type == DAT_RLE_SPRITE)
      destroy_bitmap(bmp);

   return ret;
}



/* deletes the current alpha channel */
static int delete_alpha(void)
{
   DATAFILE *dat = grabber_single_selection();
   RLE_SPRITE *rle;
   BITMAP *bmp;
   int got = FALSE;
   int ret = 0;
   int x, y, c, r, g, b, a;

   if ((!dat) || ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE))) {
      alert("You must select a single bitmap or RLE sprite",
	    "object before you can delete the alpha channel", 
	    NULL, "Sorry", NULL, 13, 0);
      return 0;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      rle = dat->dat;
      bmp = create_bitmap_ex(rle->color_depth, rle->w, rle->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, rle, 0, 0);
   }
   else
      bmp = dat->dat;

   if (bitmap_color_depth(bmp) == 32) {
      for (y=0; y<bmp->h; y++) {
	 for (x=0; x<bmp->w; x++) {
	    c = getpixel(bmp, x, y);
	    r = getr32(c);
	    g = getg32(c);
	    b = getb32(c);
	    a = geta32(c);
	    putpixel(bmp, x, y, makecol32(r, g, b));
	    if (a)
	       got = TRUE;
	 }
      }
   }

   if (got) {
      if (dat->type == DAT_RLE_SPRITE) {
	 destroy_rle_sprite(dat->dat);
	 dat->dat = get_rle_sprite(bmp);
      }

      alert("Success: alpha channel moved to /dev/null", NULL, NULL, "Cool", NULL, 13, 0);
      ret = D_REDRAW;
   }
   else
      alert("There is no alpha channel in this image", NULL, NULL, "Sorry", NULL, 13, 0);

   if (dat->type == DAT_RLE_SPRITE)
      destroy_bitmap(bmp);

   return ret;
}



/* worker function for importing alpha channels */
static BITMAP *do_alpha_import(BITMAP *bmp, int *changed, RGB *pal)
{
   BITMAP *newbmp;
   DATAFILE *alpha;
   char buf[256], name[FILENAME_LENGTH];
   DATEDIT_GRAB_PARAMETERS params;
   AL_CONST char *ext;
   int x, y, c, r, g, b, a;

   *changed = FALSE;

   ext = datedit_grab_ext(DAT_BITMAP);
   sprintf(buf, "Import alpha (%s)", ext);

   strcpy(name, grabber_import_file);
   *get_filename(name) = 0;

   if (file_select_ex(buf, name, ext, sizeof(name), 0, 0)) {
      fix_filename_case(name);
      strcpy(grabber_import_file, name);
      grabber_busy_mouse(TRUE);

      params.datafile = NULL;  /* only with absolute filenames */
      params.filename = name;
      params.name = name;
      params.type = DAT_BITMAP;
      params.x = -1;
      params.y = -1;
      params.w = -1;
      params.h = -1;
      params.colordepth = -1;
      params.relative = FALSE;  /* required (see above) */

      alpha = datedit_grab(NULL, &params);

      if ((alpha) && (alpha->dat)) {
	 if (pal)
	    select_palette(pal);

	 newbmp = create_bitmap_ex(32, bmp->w, bmp->h);
	 blit(bmp, newbmp, 0, 0, 0, 0, bmp->w, bmp->h);
	 destroy_bitmap(bmp);
	 bmp = newbmp;

	 if (pal)
	    unselect_palette();

	 select_palette(datedit_last_read_pal);

	 for (y=0; y<bmp->h; y++) {
	    for (x=0; x<bmp->w; x++) {
	       if (getpixel(bmp, x, y) != bitmap_mask_color(bmp)) {
		  c = getpixel(alpha->dat, x, y);
		  r = getr_depth(bitmap_color_depth(alpha->dat), c);
		  g = getg_depth(bitmap_color_depth(alpha->dat), c);
		  b = getb_depth(bitmap_color_depth(alpha->dat), c);
		  a = (r+g+b)/3;

		  bmp->line[y][x*4+_rgb_a_shift_32/8] = a;
	       }
	    }
	 }

	 unselect_palette();
	 _unload_datafile_object(alpha);

	 *changed = TRUE;
      }

      grabber_busy_mouse(FALSE);
   }

   return bmp;
}



/* imports an alpha channel over the top of the current selection */
static int import_alpha(void)
{
   DATAFILE *dat = grabber_single_selection();
   RLE_SPRITE *rle;
   BITMAP *bmp;
   int changed;

   if ((!dat) || ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE))) {
      alert("You must select a single bitmap or RLE sprite",
	    "object before you can import an alpha channel", 
	    NULL, "Sorry", NULL, 13, 0);
      return 0;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      rle = dat->dat;
      bmp = create_bitmap_ex(rle->color_depth, rle->w, rle->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, rle, 0, 0);
      bmp = do_alpha_import(bmp, &changed, NULL);
      destroy_rle_sprite(rle);
      dat->dat = get_rle_sprite(bmp);
      destroy_bitmap(bmp);
   }
   else
      dat->dat = do_alpha_import(dat->dat, &changed, NULL);

   if (changed)
      view_alpha();

   return D_REDRAW;
}



/* reads an alpha channel over the top of the grab source bitmap */
static int read_alpha(void)
{
   int changed;

   if (!grabber_graphic) {
      alert("You must read in a bitmap file before", "you can add an alpha channel to it", NULL, "OK", NULL, 13, 0);
      return 0;
   }

   grabber_graphic = do_alpha_import(grabber_graphic, &changed, grabber_palette);

   if (changed)
      alert("Alpha channel imported successfully: this will be", "used when you next grab a bitmap or RLE sprite", NULL, "OK", NULL, 13, 0);

   return D_REDRAW;
}



/* menu commands for doing stuff to the alpha channel */
static MENU alpha_sub_menu[] =
{
   { "&View Alpha",     view_alpha,    NULL,    0,    NULL  },
   { "&Import Alpha",   import_alpha,  NULL,    0,    NULL  },
   { "&Export Alpha",   export_alpha,  NULL,    0,    NULL  },
   { "&Delete Alpha",   delete_alpha,  NULL,    0,    NULL  },
   { NULL,              NULL,          NULL,    0,    NULL  }
};



/* per-object alpha channel menu */
static MENU alpha_menu =
{
   "Alpha Channel",
   NULL,
   alpha_sub_menu,
   0,
   NULL
};



/* alpha channel command for the file menu */
static MENU read_alpha_menu =
{
   "Read Alpha Channel",
   read_alpha,
   NULL,
   0,
   NULL
};



/* plugin interface header */
DATEDIT_MENU_INFO datalpha_menu1 =
{
   &alpha_menu,
   alpha_query,
   DATEDIT_MENU_POPUP | DATEDIT_MENU_OBJECT,
   0,
   NULL
};



DATEDIT_MENU_INFO datalpha_menu2 =
{
   &read_alpha_menu,
   NULL,
   DATEDIT_MENU_FILE,
   0,
   NULL
};

