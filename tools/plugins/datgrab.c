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
 *      Grabber plugin for implementing the interactive bitmap grab
 *      command (where you select an image region using the mouse).
 *      This file provides a special version of the "grab" function
 *      for use with bitmap images, and adds the "box grab" and
 *      "ungrab" routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "../datedit.h"



/* handles grabbing data from a bitmap */
static void grabbit(DATAFILE *dat, int box)
{
   int xgrid, ygrid;
   int using_mouse;
   int x, y, w, h;
   int ox = -1, oy = -1, ow = -1, oh = -1;
   char buf[80];
   BITMAP *graphiccopy;
   BITMAP *pattern;
   BITMAP *bmp;
   RLE_SPRITE *spr;

   grabber_get_grid_size(&xgrid, &ygrid);

   show_mouse(NULL);
   grabber_sel_palette(grabber_palette);

   if (bitmap_color_depth(screen) == 8)
      clear_to_color(screen, gui_mg_color);

   graphiccopy = create_bitmap(grabber_graphic->w, grabber_graphic->h);
   blit(grabber_graphic, graphiccopy, 0, 0, 0, 0, grabber_graphic->w, grabber_graphic->h);

   if (bitmap_color_depth(screen) != 8)
      clear_to_color(screen, gui_mg_color);

   blit(graphiccopy, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   pattern = create_bitmap(2, 2);

   putpixel(pattern, 0, 0, gui_bg_color);
   putpixel(pattern, 1, 1, gui_bg_color);
   putpixel(pattern, 0, 1, gui_fg_color);
   putpixel(pattern, 1, 0, gui_fg_color);

   do {
      poll_mouse();
   } while (mouse_b);

   set_mouse_range(0, 0, grabber_graphic->w-1, grabber_graphic->h-1);

   do {
      poll_mouse();

      x = mouse_x;
      y = mouse_y;

      if ((x >= grabber_graphic->w) || (y >= grabber_graphic->h)) {
	 x = CLAMP(0, x, grabber_graphic->w-1);
	 y = CLAMP(0, y, grabber_graphic->h-1);
	 position_mouse(x, y);
      }

      if (!box) {
	 x = (x / xgrid) * xgrid;
	 y = (y / ygrid) * ygrid;
      }

      if ((x != ox) || (y != oy)) {
	 acquire_screen();

	 blit(graphiccopy, screen, 0, 0, 0, 0, grabber_graphic->w, grabber_graphic->h);
	 hline(screen, 0, grabber_graphic->h, grabber_graphic->w, 0);
	 vline(screen, grabber_graphic->w, 0, grabber_graphic->h, 0);

	 drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);

	 if (box) {
	    hline(screen, 0, y-1, grabber_graphic->w, 0);
	    vline(screen, x-1, 0, grabber_graphic->h, 0);
	 }
	 else {
	    hline(screen, x-1, y-1, grabber_graphic->w, 0);
	    vline(screen, x-1, y-1, grabber_graphic->h, 0);
	 }

	 drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);

	 sprintf(buf, "%d, %d", x, y);
	 rectfill(screen, 0, SCREEN_H-8, SCREEN_W, SCREEN_H, gui_fg_color);
	 textout_ex(screen, font, buf, 0, SCREEN_H-8, gui_bg_color, -1);

	 release_screen();

	 ox = x;
	 oy = y;
      }

      if (keypressed()) {
	 switch (readkey() >> 8) {

	    case KEY_UP:
	       position_mouse(mouse_x, MAX(mouse_y-ygrid, 0));
	       break;

	    case KEY_DOWN:
	       position_mouse(mouse_x, mouse_y+ygrid);
	       break;

	    case KEY_RIGHT:
	       position_mouse(mouse_x+xgrid, mouse_y);
	       break;

	    case KEY_LEFT:
	       position_mouse(MAX(mouse_x-xgrid, 0), mouse_y);
	       break;

	    case KEY_ENTER:
	    case KEY_SPACE:
	       goto gottheposition;

	    case KEY_ESC:
	       goto getmeoutofhere;
	 }
      }
   } while (!mouse_b);

   if (mouse_b & 2)
      goto getmeoutofhere;

   gottheposition:

   if (box) {
      int xx, yy, ww, hh;

      xx = 0;
      yy = 0;

      datedit_find_character(grabber_graphic, &xx, &yy, &ww, &hh);

      while ((ww > 0) && (hh > 0)) {
	 if ((x > xx) && (x <= xx+ww) &&
	     (y > yy) && (y <= yy+hh)) {
	    x = xx+1;
	    y = yy+1;
	    w = ww;
	    h = hh;
	    goto gotthesize;
	 }

	 xx += ww;
	 datedit_find_character(grabber_graphic, &xx, &yy, &ww, &hh);
      }

      x = 0;
      y = 0;
      w = grabber_graphic->w;
      h = grabber_graphic->h;
   }
   else {
      using_mouse = (mouse_b & 1);

      do {
	 poll_mouse();

	 w = mouse_x;
	 h = mouse_y;

	 if ((w < x) || (w > grabber_graphic->w) || ( h < y) || (h > grabber_graphic->h)) {
	    w = CLAMP(x, w, grabber_graphic->w);
	    h = CLAMP(y, h, grabber_graphic->h);
	    position_mouse(w, h);
	 }

	 w = ((w - x) / xgrid + 1) * xgrid;
	 if (x+w > grabber_graphic->w)
	    w = grabber_graphic->w - x;

	 h = ((h - y) / ygrid + 1) * ygrid;
	 if (y+h > grabber_graphic->h)
	    h = grabber_graphic->h - y;

	 if ((w != ow) || (h != oh)) {
	    acquire_screen();

	    blit(graphiccopy, screen, 0, 0, 0, 0, grabber_graphic->w, grabber_graphic->h);
	    hline(screen, 0, grabber_graphic->h, grabber_graphic->w, 0);
	    vline(screen, grabber_graphic->w, 0, grabber_graphic->h, 0);

	    drawing_mode(DRAW_MODE_COPY_PATTERN, pattern, 0, 0);
	    hline(screen, x-1, y-1, x+w, 0);
	    hline(screen, x-1, y+h, x+w, 0);
	    vline(screen, x-1, y-1, y+h, 0);
	    vline(screen, x+w, y-1, y+h, 0);
	    drawing_mode(DRAW_MODE_SOLID, NULL, 0, 0);

	    sprintf(buf, "%d, %d (%dx%d)", x, y, w, h);
	    rectfill(screen, 0, SCREEN_H-8, SCREEN_W, SCREEN_H, gui_fg_color);
	    textout_ex(screen, font, buf, 0, SCREEN_H-8, gui_bg_color, -1);

	    release_screen();

	    ow = w;
	    oh = h;
	 }

	 if (keypressed()) {
	    switch (readkey() >> 8) {

	       case KEY_UP:
		  position_mouse(mouse_x, MAX(mouse_y-ygrid, 0));
		  break;

	       case KEY_DOWN:
		  position_mouse(mouse_x, mouse_y+ygrid);
		  break;

	       case KEY_RIGHT:
		  position_mouse(mouse_x+xgrid, mouse_y);
		  break;

	       case KEY_LEFT:
		  position_mouse(MAX(mouse_x-xgrid, 0), mouse_y);
		  break;

	       case KEY_ENTER:
	       case KEY_SPACE:
		  goto gotthesize;

	       case KEY_ESC:
		  goto getmeoutofhere;
	    }
	 }

	 if (mouse_b & 2)
	    goto getmeoutofhere;

      } while (((mouse_b) && (using_mouse)) || ((!mouse_b) && (!using_mouse)));
   }

   gotthesize:

   if ((w > 0) && (h > 0)) {
      bmp = create_bitmap_ex(bitmap_color_depth(grabber_graphic), w, h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      blit(grabber_graphic, bmp, x, y, 0, 0, w, h);

      if (dat->type == DAT_RLE_SPRITE) {
	 spr = get_rle_sprite(bmp);
	 destroy_bitmap(bmp);
	 destroy_rle_sprite(dat->dat);
	 dat->dat = spr;
      }
      else {
	 destroy_bitmap(dat->dat);
	 dat->dat = bmp;
      }

      sprintf(buf, "%d", x);
      datedit_set_property(dat, DAT_XPOS, buf);

      sprintf(buf, "%d", y);
      datedit_set_property(dat, DAT_YPOS, buf);

      sprintf(buf, "%d", w);
      datedit_set_property(dat, DAT_XSIZ, buf);

      sprintf(buf, "%d", h);
      datedit_set_property(dat, DAT_YSIZ, buf);

      datedit_set_property(dat, DAT_ORIG, grabber_graphic_origin);
      datedit_set_property(dat, DAT_DATE, grabber_graphic_date);
   }

   getmeoutofhere:

   if (mouse_b)
      clear_to_color(screen, gui_mg_color);

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);

   destroy_bitmap(graphiccopy);
   destroy_bitmap(pattern);
   show_mouse(screen);

   do {
      poll_mouse();
   } while (mouse_b);
}



/* checks whether our grab command is allowed at the moment */
static int grabber_query(int popup)
{
   DATAFILE *dat = grabber_single_selection();

   if ((!dat) || (!grabber_graphic))
      return FALSE;

   return ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
	   (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE));
}



/* menu hook for our special grab command */
static int grabber(void)
{
   DATAFILE *dat = grabber_single_selection();

   if ((!dat) || (!grabber_graphic))
      return D_O_K;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE))
      return D_O_K;

   grabbit(dat, FALSE);

   datedit_sort_properties(dat->prop);
   grabber_select_property(DAT_NAME);

   grabber_modified(TRUE);

   return D_REDRAW;
}



/* checks whether our box grab command is allowed at the moment */
static int boxgrab_query(int popup)
{
   DATAFILE *dat = grabber_single_selection();

   if (!dat) {
      if (popup)
	 return FALSE;
      else
	 alert("You must create and select a single object to", "contain the data before you can box-grab it", NULL, "OK", NULL, 13, 0);
   }
   else if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
	    (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      if (popup)
	 return FALSE;
      else
	 alert("You can only box-grab to a bitmap object!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (!grabber_graphic) {
      if (popup)
	 return FALSE;
      else
	 alert("You must read in a bitmap file", "before you can box-grab data from it", NULL, "OK", NULL, 13, 0);
   }

   return TRUE;
}



/* menu hook for the box grab command */
static int boxgrab(void)
{
   DATAFILE *dat = grabber_single_selection();

   if ((!dat) || (!grabber_graphic))
      return D_O_K;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE))
      return D_O_K;

   grabbit(dat, TRUE);

   datedit_sort_properties(dat->prop);
   grabber_select_property(DAT_NAME);

   grabber_modified(TRUE);

   return D_REDRAW;
}



/* checks whether our ungrab command is allowed at the moment */
static int ungrab_query(int popup)
{
   DATAFILE *dat = grabber_single_selection();

   if ((!dat) ||
       ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
	(dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE))) {
      if (popup)
	 return FALSE;
      else
	 alert("Only bitmap objects can be ungrabbed!", NULL, NULL, "OK", NULL, 13, 0);
   }

   return TRUE;
}



/* menu hook for the ungrab command */
static int ungrab(void)
{
   DATAFILE *dat = grabber_single_selection();

   if (!dat)
      return D_O_K;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE))
      return D_O_K;

   if (grabber_graphic)
      destroy_bitmap(grabber_graphic);

   if (dat->type == DAT_RLE_SPRITE) {
      RLE_SPRITE *spr = (RLE_SPRITE *)dat->dat;
      grabber_graphic = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_bitmap(grabber_graphic);
      draw_rle_sprite(grabber_graphic, spr, 0, 0);
   }
   else {
      BITMAP *bmp = (BITMAP *)dat->dat;
      grabber_graphic = create_bitmap_ex(bitmap_color_depth(bmp), bmp->w, bmp->h);
      blit(bmp, grabber_graphic, 0, 0, 0, 0, bmp->w, bmp->h);
   }

   if (bitmap_color_depth(grabber_graphic) == 8)
      memcpy(grabber_palette, datedit_current_palette, sizeof(PALETTE));
   else
      generate_optimized_palette(grabber_graphic, grabber_palette, NULL);

   alert("Bitmap data copied to the input buffer", NULL, NULL, "OK", NULL, 13, 0);

   return D_O_K;
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



DATEDIT_MENU_INFO datgrab_grabber_menu =
{
   &grabber_menu,
   grabber_query,
   DATEDIT_MENU_OBJECT,
   0,
   "XPOS;YPOS;XSIZ;YSIZ"
};



static MENU boxgrab_menu =
{
   "Box Grab\t(ctrl+B)",
   boxgrab,
   NULL,
   0,
   NULL
};



DATEDIT_MENU_INFO datgrab_boxgrab_menu =
{
   &boxgrab_menu,
   boxgrab_query,
   DATEDIT_MENU_OBJECT | DATEDIT_MENU_POPUP,
   2,  /* ctrl+B */
   "XPOS;YPOS;XSIZ;YSIZ"
};



static MENU ungrab_menu =
{
   "Ungrab",
   ungrab,
   NULL,
   0,
   NULL
};



DATEDIT_MENU_INFO datgrab_ungrab_menu =
{
   &ungrab_menu,
   ungrab_query,
   DATEDIT_MENU_OBJECT | DATEDIT_MENU_POPUP,
   0,
   NULL
};

