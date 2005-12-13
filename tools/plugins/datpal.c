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
 *      Grabber plugin for managing palette objects.
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



/* creates a new palette object */
static void *makenew_palette(long *size)
{
   RGB *pal = _AL_MALLOC(sizeof(PALETTE));

   memcpy(pal, default_palette, sizeof(PALETTE));
   *size = sizeof(PALETTE);

   return pal;
}



/* tests whether two palettes are identical */
static int compare_palettes(RGB *p1, RGB *p2)
{
   int c;

   for (c=0; c<PAL_SIZE; c++) {
      if ((p1[c].r != p2[c].r) || 
	  (p1[c].g != p2[c].g) || 
	  (p1[c].b != p2[c].b))
	 return TRUE;
   }

   return FALSE;
}



/* draws a palette onto the grabber object view window */
static void plot_palette(AL_CONST DATAFILE *dat, int x, int y)
{
   int c;

   if (compare_palettes(dat->dat, datedit_current_palette)) {
      if (bitmap_color_depth(screen) > 8) {
	 select_palette(dat->dat);
	 for (c=0; c<PAL_SIZE; c++)
	    rectfill(screen, x+(c&15)*8, y+(c/16)*8+32, 
		     x+(c&15)*8+7, y+(c/16)*8+39, palette_color[c]);
	 unselect_palette();
	 textout_ex(screen, font, "A different palette", x+160, y+32, gui_fg_color, gui_bg_color);
	 textout_ex(screen, font, "is currently in use.", x+160, y+40, gui_fg_color, gui_bg_color);
	 textout_ex(screen, font, "To select this one,", x+160, y+48, gui_fg_color, gui_bg_color);
	 textout_ex(screen, font, "double-click on it", x+160, y+56, gui_fg_color, gui_bg_color);
	 textout_ex(screen, font, "in the item list.", x+160, y+64, gui_fg_color, gui_bg_color);
      }
      else {
	 textout_ex(screen, font, "A different palette is currently in use.", x, y+32, gui_fg_color, gui_bg_color);
	 textout_ex(screen, font, "To select this one, double-click on it", x, y+40, gui_fg_color, gui_bg_color);
	 textout_ex(screen, font, "in the item list.", x, y+48, gui_fg_color, gui_bg_color);
      }
   }
   else {
      for (c=0; c<PAL_SIZE; c++)
	 rectfill(screen, x+(c&15)*8, y+(c/16)*8+32, 
		  x+(c&15)*8+7, y+(c/16)*8+39, palette_color[c]);
   }
}



/* handles double-clicking on a palette object in the grabber */
static int use_palette(DATAFILE *dat)
{
   grabber_sel_palette(dat->dat);
   return D_REDRAW;
}



/* exports a palette into an external file */
static int export_palette(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   BITMAP *bmp;
   int ret;

   bmp = create_bitmap_ex(8, 32, 8);
   clear_bitmap(bmp);
   textout_ex(bmp, font, "PAL.", 0, 0, 255, 0);

   ret = (save_bitmap(filename, bmp, (RGB *)dat->dat) == 0);

   destroy_bitmap(bmp);
   return ret;
}



/* grabs a palette from an external file */
static DATAFILE *grab_palette(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   int oldcolordepth = _color_depth;
   RGB *pal;
   BITMAP *bmp;

   _color_depth = 8;
   set_color_conversion(COLORCONV_TOTAL);

   pal = _AL_MALLOC(sizeof(PALETTE));
   bmp = load_bitmap(filename, pal);

   _color_depth = oldcolordepth;
   set_color_conversion(COLORCONV_NONE);

   if (!bmp) {
      _AL_FREE(pal);
      return NULL;
   }

   destroy_bitmap(bmp);

   return datedit_construct(type, pal, sizeof(PALETTE), prop);
}



/* checks whether our grab command is allowed at the moment */
static int grabber_query(int popup)
{
   DATAFILE *dat = grabber_single_selection();

   return ((dat) && (dat->type == DAT_PALETTE) && (grabber_graphic));
}



/* menu hook for our special grab command */
static int grabber(void)
{
   DATAFILE *dat = grabber_single_selection();

   if ((!dat) || (dat->type != DAT_PALETTE) || (!grabber_graphic))
      return D_O_K;

   memcpy(dat->dat, grabber_palette, sizeof(PALETTE));

   datedit_set_property(dat, DAT_ORIG, grabber_graphic_origin);
   datedit_set_property(dat, DAT_DATE, grabber_graphic_date);

   alert("Palette data grabbed from the bitmap file",
	 NULL, NULL, "OK", NULL, 13, 0);

   grabber_sel_palette(dat->dat);

   datedit_sort_properties(dat->prop);
   grabber_select_property(DAT_NAME);

   return D_REDRAW;
}



/* hook ourselves into the grabber menu system */
static MENU grabber_menu =
{
   "Grab",
   grabber,
   NULL,
   0,
   NULL
};



DATEDIT_MENU_INFO datpal_grabber_menu =
{
   &grabber_menu,
   grabber_query,
   DATEDIT_MENU_OBJECT,
   0,
   NULL
};



/* plugin interface header */
DATEDIT_OBJECT_INFO datpal_info =
{
   DAT_PALETTE, 
   "Palette", 
   NULL,
   makenew_palette,
   NULL,
   plot_palette,
   use_palette,
   NULL
};



DATEDIT_GRABBER_INFO datpal_grabber =
{
   DAT_PALETTE, 
   "bmp;lbm;pcx;tga",
   "bmp;pcx;tga",
   grab_palette,
   export_palette,
   NULL
};

