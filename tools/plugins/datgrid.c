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
 *      Grabber plugin for the grab-from-grid and autocrop functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "../datedit.h"



static int gg_text_proc(int msg, DIALOG *d, int c);
static int gg_edit_proc(int msg, DIALOG *d, int c);



static void *added_item[1024];
static int added_count;



/* helper for cropping bitmaps */
static BITMAP *crop_bitmap(BITMAP *bmp, int *tx, int *ty)
{
   int tw, th, i, j, c;
   int changed = FALSE;

   *tx = 0;
   *ty = 0;
   tw = bmp->w;
   th = bmp->h;

   if ((tw > 0) && (th > 0)) {
      c = getpixel(bmp, 0, 0);

      for (j=*ty; j<(*ty)+th; j++) {       /* top of image */
	 for (i=*tx; i<(*tx)+tw; i++) {
	    if (getpixel(bmp, i, j) != c)
	       goto finishedtop;
	 }
	 (*ty)++;
	 th--;
	 changed = TRUE;
      }

      finishedtop:

      for (j=(*ty)+th-1; j>*ty; j--) {     /* bottom of image */
	 for (i=*tx; i<(*tx)+tw; i++) {
	    if (getpixel(bmp, i, j) != c)
	       goto finishedbottom;
	 }
	 th--;
	 changed = TRUE;
      }

      finishedbottom:

      for (j=*tx; j<*(tx)+tw; j++) {       /* left of image */
	 for (i=*ty; i<(*ty)+th; i++) {
	    if (getpixel(bmp, j, i) != c)
	       goto finishedleft;
	 }
	 (*tx)++;
	 tw--;
	 changed = TRUE;
      }

      finishedleft:

      for (j=*(tx)+tw-1; j>*tx; j--) {     /* right of image */
	 for (i=*ty; i<(*ty)+th; i++) {
	    if (getpixel(bmp, j, i) != c)
	       goto finishedright;
	 }
	 tw--;
	 changed = TRUE;
      }

      finishedright:
	 ;
   }

   if ((tw != 0) && (th != 0) && (changed)) {
      BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(bmp), tw, th);
      clear_to_color(b2, b2->vtable->mask_color);
      blit(bmp, b2, *tx, *ty, 0, 0, tw, th);
      destroy_bitmap(bmp);
      return b2;
   }
   else
      return bmp;
}



/* worker function for counting bitmap objects */
static int do_bitmap_check(DATAFILE *dat, int *param, int param2)
{
   if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
       (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) 
      (*param)++;

   return D_O_K;
}



/* checks whether our grab-from-grid command is allowed at the moment */
static int griddler_query(int popup)
{
   DATAFILE *dat;
   AL_CONST char *s;

   if (popup) {
      dat = grabber_single_selection();

      if (!dat)
	 return FALSE;

      if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
	  (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) 
	 return FALSE;

      s = get_datafile_property(dat, DAT_NAME);

      if ((s[0]) && (uisdigit(s[strlen(s)-1])))
	 return TRUE;
      else
	 return FALSE;
   }

   return TRUE;
}



/* checks whether a bitmap contains any data */
static int bitmap_is_empty(BITMAP *bmp)
{
   int x, y;
   int c = getpixel(bmp, 0, 0);

   for (y=0; y<bmp->h; y++)
      for (x=0; x<bmp->w; x++)
	 if (getpixel(bmp, x, y) != c)
	    return FALSE;

   return TRUE;
}



/* helper for grabbing images from a grid */
static void *griddlit(DATAFILE **parent, AL_CONST char *name, int c, int type, int skipempty, int autocrop, int depth, int x, int y, int w, int h)
{
   DATAFILE *dat;
   BITMAP *bmp;
   void *v;
   char buf[256];
   RGB tmprgb = datedit_current_palette[0];
   int tx = 0, ty = 0;

   if ((type == DAT_RLE_SPRITE) || (type == DAT_C_SPRITE) || (type == DAT_XC_SPRITE)) {
      datedit_current_palette[0].r = 63;
      datedit_current_palette[0].g = 0;
      datedit_current_palette[0].b = 63;
   }
   select_palette(datedit_current_palette);

   bmp = create_bitmap_ex(depth, w, h);
   clear_to_color(bmp, bmp->vtable->mask_color);
   blit(grabber_graphic, bmp, x, y, 0, 0, w, h);

   unselect_palette();
   datedit_current_palette[0] = tmprgb;

   if ((skipempty) && (bitmap_is_empty(bmp))) {
      destroy_bitmap(bmp);
      return NULL;
   }

   if (autocrop)
      bmp = crop_bitmap(bmp, &tx, &ty);

   if (type == DAT_RLE_SPRITE) {
      v = get_rle_sprite(bmp);
      destroy_bitmap(bmp);
   }
   else
      v = bmp;

   sprintf(buf, "%s%03d", name, c);

   *parent = datedit_insert(*parent, &dat, buf, type, v, 0);

   sprintf(buf, "%d", x);
   datedit_set_property(dat, DAT_XPOS, buf);

   sprintf(buf, "%d", y);
   datedit_set_property(dat, DAT_YPOS, buf);

   sprintf(buf, "%d", w);
   datedit_set_property(dat, DAT_XSIZ, buf);

   sprintf(buf, "%d", h);
   datedit_set_property(dat, DAT_YSIZ, buf);

   if (tx || ty) {
      sprintf(buf, "%d", tx);
      datedit_set_property(dat, DAT_XCRP, buf);

      sprintf(buf, "%d", ty);
      datedit_set_property(dat, DAT_YCRP, buf);
   }

   datedit_set_property(dat, DAT_ORIG, grabber_graphic_origin);
   datedit_set_property(dat, DAT_DATE, grabber_graphic_date);

   datedit_sort_properties(dat->prop);

   if (added_count < (int)(sizeof(added_item)/sizeof(added_item[0])))
      added_item[added_count++] = v;

   return v;
}



/* grabs images from a grid, using boxes of color #255 */
static void *box_griddle(DATAFILE **parent, AL_CONST char *name, int type, int skipempty, int autocrop, int depth)
{
   void *ret = NULL;
   void *item;
   int x, y, w, h;
   int c = 0;

   x = 0;
   y = 0;

   datedit_find_character(grabber_graphic, &x, &y, &w, &h);

   while ((w > 0) && (h > 0)) {
      item = griddlit(parent, name, c, type, skipempty, autocrop, depth, x+1, y+1, w, h);
      if (item) {
	 c++;
	 if (!ret)
	    ret = item;
      }

      x += w;
      datedit_find_character(grabber_graphic, &x, &y, &w, &h);
   }

   return ret;
}



/* grabs images from a regular grid */
static void *grid_griddle(DATAFILE **parent, AL_CONST char *name, int type, int skipempty, int autocrop, int depth, int xgrid, int ygrid)
{
   void *ret = NULL;
   void *item;
   int x, y;
   int c = 0;

   for (y=0; y+ygrid<=grabber_graphic->h; y+=ygrid) {
      for (x=0; x+xgrid<=grabber_graphic->w; x+=xgrid) {
	 item = griddlit(parent, name, c, type, skipempty, autocrop, depth, x, y, xgrid, ygrid);
	 if (item) {
	    c++;
	    if (!ret)
	       ret = item;
	 }
      } 
   }

   return ret;
}



static char griddle_xgrid[8] = "32";
static char griddle_ygrid[8] = "32";
static char griddle_name[256] = "";



/* dialog callback for retrieving the contents of the object type list */
static AL_CONST char *typelist_getter(int index, int *list_size)
{
   static char *str[] =
   {
      "Bitmap",
      "RLE Sprite",
      "Compiled Sprite",
      "Mode-X Compiled Sprite"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = 4;
      return NULL;
   }

   return str[index];
}



/* dialog callback for retrieving the contents of the color depth list */
static AL_CONST char *depthlist_getter(int index, int *list_size)
{
   static char *str[] =
   {
      "256 color palette",
      "15 bit hicolor",
      "16 bit hicolor",
      "24 bit truecolor",
      "32 bit truecolor"
   };

   if (index < 0) {
      if (list_size)
	 *list_size = 5;
      return NULL;
   }

   return str[index];
}



static DIALOG griddle_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                     (dp2) (dp3) */
   { d_shadow_box_proc, 0,    0,    277,  305,  0,    0,    0,       0,          0,             0,       NULL,                    NULL, NULL  },
   { d_ctext_proc,      138,  8,    0,    0,    0,    0,    0,       0,          0,             0,       "Grab from Grid",        NULL, NULL  },
   { d_radio_proc,      16,   32,   121,  13,   0,    0,    0,       D_SELECTED, 0,             0,       "Use col #255",          NULL, NULL  },
   { d_radio_proc,      16,   56,   121,  13,   0,    0,    0,       0,          0,             0,       "Regular grid",          NULL, NULL  },
   { gg_text_proc,      160,  58,   0,    0,    0,    0,    0,       0,          0,             0,       "X-grid:",               NULL, NULL  },
   { gg_edit_proc,      224,  58,   40,   8,    0,    0,    0,       0,          4,             0,       griddle_xgrid,           NULL, NULL  },
   { gg_text_proc,      160,  80,   0,    0,    0,    0,    0,       0,          0,             0,       "Y-grid:",               NULL, NULL  },
   { gg_edit_proc,      224,  82,   40,   8,    0,    0,    0,       0,          4,             0,       griddle_ygrid,           NULL, NULL  },
   { d_check_proc,      16,   82,   123,  13,   0,    0,    0,       0,          0,             0,       "Skip empties:",         NULL, NULL  },
   { d_check_proc,      16,   106,  91,   13,   0,    0,    0,       0,          0,             0,       "Autocrop:",             NULL, NULL  },
   { d_text_proc,       16,   138,  0,    0,    0,    0,    0,       0,          0,             0,       "Name:",                 NULL, NULL  },
   { d_edit_proc,       64,   138,  204,  8,    0,    0,    0,       0,          255,           0,       griddle_name,            NULL, NULL  },
   { d_text_proc,       16,   160,  0,    0,    0,    0,    0,       0,          0,             0,       "Type:",                 NULL, NULL  },
   { d_list_proc,       64,   160,  197,  36,   0,    0,    0,       0,          0,             0,       (void*)typelist_getter,  NULL, NULL  },
   { d_text_proc,       16,   208,  0,    0,    0,    0,    0,       0,          0,             0,       "Cols:",                 NULL, NULL  },
   { d_list_proc,       64,   208,  197,  44,   0,    0,    0,       0,          0,             0,       (void*)depthlist_getter, NULL, NULL  },
   { d_button_proc,     50,   272,  81,   17,   0,    0,    13,      D_EXIT,     0,             0,       "OK",                    NULL, NULL  }, 
   { d_button_proc,     146,  272,  81,   17,   0,    0,    27,      D_EXIT,     0,             0,       "Cancel",                NULL, NULL  }, 
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                    NULL, NULL  }
};


#define GRIDDLE_DLG_BOXES        2
#define GRIDDLE_DLG_GRID         3
#define GRIDDLE_DLG_XGRID        5
#define GRIDDLE_DLG_YGRID        7
#define GRIDDLE_DLG_EMPTIES      8
#define GRIDDLE_DLG_AUTOCROP     9
#define GRIDDLE_DLG_NAME         11
#define GRIDDLE_DLG_TYPE         13
#define GRIDDLE_DLG_DEPTH        15
#define GRIDDLE_DLG_OK           16
#define GRIDDLE_DLG_CANCEL       17



/* wrapper for d_text_proc, that greys out when invalid */
static int gg_text_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_IDLE) {
      int valid = (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED) ? 0 : 1;
      int enabled = (d->flags & D_DISABLED) ? 0 : 1;

      if (valid != enabled) {
	 if (valid)
	    d->flags &= ~D_DISABLED;
	 else
	    d->flags |= D_DISABLED;

	 object_message(d, MSG_DRAW, 0);
      }

      return D_O_K;
   }
   else
      return d_text_proc(msg, d, c);
}



/* wrapper for d_edit_proc, that greys out when invalid */
static int gg_edit_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_IDLE) {
      int valid = (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED) ? 0 : 1;
      int enabled = (d->flags & D_DISABLED) ? 0 : 1;

      if (valid != enabled) {
	 if (valid)
	    d->flags &= ~D_DISABLED;
	 else
	    d->flags |= D_DISABLED;

	 object_message(d, MSG_DRAW, 0);
      }

      return D_O_K;
   }
   else
      return d_edit_proc(msg, d, c);
}



/* checks whether an object name matches the grid grab base name */
static int grid_name_matches(AL_CONST char *gn, AL_CONST char *n)
{
   AL_CONST char *s;

   if (strncmp(gn, n, strlen(gn)) != 0)
      return FALSE;

   s = n + strlen(gn);
   if (*s == 0)
      return FALSE;

   while (*s) {
      if (!uisdigit(*s))
	 return FALSE;
      s++;
   }

   return TRUE;
}



/* handle the griddle command */
static int griddler(void)
{
   DATAFILE *dat;
   DATAFILE **parent;
   void *selitem;
   char *s;
   int c;
   int xgrid, ygrid;
   int done, type, skipempty, autocrop;
   int depth = 8;

   if (!grabber_graphic) {
      alert("You must read in a bitmap file",
	    "before you can grab data from it",
	    NULL, "OK", NULL, 13, 0);
      return D_O_K;
   }

   grabber_get_selection_info(&dat, &parent);

   if (!dat) {
      griddle_name[0] = 0;
      depth = bitmap_color_depth(grabber_graphic);
   }
   else {
      strcpy(griddle_name, get_datafile_property(dat, DAT_NAME));
      s = griddle_name + strlen(griddle_name) - 1;
      while ((s >= griddle_name) && (uisdigit(*s))) {
	 *s = 0;
	 s--;
      }

      if (dat->type == DAT_BITMAP) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 0;
	 depth = bitmap_color_depth(dat->dat);
      }
      else if (dat->type == DAT_RLE_SPRITE) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 1;
	 depth = ((RLE_SPRITE *)dat->dat)->color_depth;
      }
      else if (dat->type == DAT_C_SPRITE) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 2;
	 depth = bitmap_color_depth(dat->dat);
      }
      else if (dat->type == DAT_XC_SPRITE) {
	 griddle_dlg[GRIDDLE_DLG_TYPE].d1 = 3;
	 depth = 8;
      }
   }

   if (depth == 8)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 0;
   else if (depth == 15)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 1;
   else if (depth == 16)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 2;
   else if (depth == 24)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 3;
   else if (depth == 32)
      griddle_dlg[GRIDDLE_DLG_DEPTH].d1 = 4;

   centre_dialog(griddle_dlg);
   set_dialog_color(griddle_dlg, gui_fg_color, gui_bg_color);

   if (do_dialog(griddle_dlg, GRIDDLE_DLG_NAME) == GRIDDLE_DLG_CANCEL)
      return D_REDRAW;

   grabber_sel_palette(grabber_palette);

   do {
      done = TRUE;

      for (c=0; (*parent)[c].type != DAT_END; c++) {
	 if (grid_name_matches(griddle_name, get_datafile_property((*parent)+c, DAT_NAME))) {
	    *parent = datedit_delete(*parent, c);
	    done = FALSE;
	    break;
	 }
      }
   } while (!done);

   switch (griddle_dlg[GRIDDLE_DLG_TYPE].d1) {

      case 0:
      default:
	 type = DAT_BITMAP;
	 break;

      case 1:
	 type = DAT_RLE_SPRITE;
	 break;

      case 2:
	 type = DAT_C_SPRITE;
	 break;

      case 3:
	 type = DAT_XC_SPRITE;
	 break;
   }

   switch (griddle_dlg[GRIDDLE_DLG_DEPTH].d1) {

      case 0:
      default:
	 depth = 8;
	 break;

      case 1:
	 depth = 15;
	 break;

      case 2:
	 depth = 16;
	 break;

      case 3:
	 depth = 24;
	 break;

      case 4:
	 depth = 32;
	 break;
   }

   skipempty = griddle_dlg[GRIDDLE_DLG_EMPTIES].flags & D_SELECTED;
   autocrop = griddle_dlg[GRIDDLE_DLG_AUTOCROP].flags & D_SELECTED;

   added_count = 0;

   if (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED)
      selitem = box_griddle(parent, griddle_name, type, skipempty, autocrop, depth);
   else {
      xgrid = CLAMP(1, atoi(griddle_xgrid), 0xFFFF);
      ygrid = CLAMP(1, atoi(griddle_ygrid), 0xFFFF);
      selitem = grid_griddle(parent, griddle_name, type, skipempty, autocrop, depth, xgrid, ygrid);
   }

   datedit_sort_datafile(*parent);
   grabber_rebuild_list(selitem, TRUE);
   grabber_select_property(DAT_NAME);

   for (c=0; c<added_count; c++)
      grabber_set_selection(added_item[c]);

   if (selitem) {
      grabber_sel_palette(grabber_palette);
      grabber_modified(TRUE);
   }
   else
      alert("No grid found - nothing grabbed!", NULL, NULL, "Hmm...", NULL, 13, 0);

   set_config_string("grabber", "griddle_xgrid", griddle_xgrid);
   set_config_string("grabber", "griddle_ygrid", griddle_ygrid);

   if (griddle_dlg[GRIDDLE_DLG_BOXES].flags & D_SELECTED)
      set_config_string("grabber", "griddle_mode", "boxes");
   else
      set_config_string("grabber", "griddle_mode", "grid");

   if (griddle_dlg[GRIDDLE_DLG_EMPTIES].flags & D_SELECTED)
      set_config_string("grabber", "griddle_empties", "y");
   else
      set_config_string("grabber", "griddle_empties", "n");

   if (griddle_dlg[GRIDDLE_DLG_AUTOCROP].flags & D_SELECTED)
      set_config_string("grabber", "griddle_autocrop", "y");
   else
      set_config_string("grabber", "griddle_autocrop", "n");

   set_config_int("grabber", "griddle_type", griddle_dlg[GRIDDLE_DLG_TYPE].d1);

   return D_REDRAW;
}



/* worker function for cropping an image */
static int do_autocropper(DATAFILE *dat, int *param, int param2)
{
   BITMAP *bmp;
   RLE_SPRITE *spr;
   char buf[256];
   int tx, ty;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      (*param)++;
      return D_O_K;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      spr = (RLE_SPRITE *)dat->dat;
      bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, spr, 0, 0);
      destroy_rle_sprite(spr);
      bmp = crop_bitmap(bmp, &tx, &ty);
      dat->dat = get_rle_sprite(bmp);
      destroy_bitmap(bmp);
   }
   else
      dat->dat = crop_bitmap((BITMAP *)dat->dat, &tx, &ty);

   if (tx || ty) {
      sprintf(buf, "%d", tx);
      datedit_set_property(dat, DAT_XCRP, buf);

      sprintf(buf, "%d", ty);
      datedit_set_property(dat, DAT_YCRP, buf);
   }
   
   grabber_modified(TRUE);

   return D_REDRAW;
}



/* checks whether our autocrop command is allowed at the moment */
static int autocrop_query(int popup)
{
   int n, p;

   if (popup) {
      p = 0;
      grabber_foreach_selection(do_bitmap_check, &n, &p, 0);
      return (p > 0);
   }

   return TRUE;
}



/* menu hook for the autocrop command */
static int autocrop(void)
{
   char buf[80];
   int ret, n;
   int p = 0;

   ret = grabber_foreach_selection(do_autocropper, &n, &p, 0);

   if (n <= 0) {
      alert ("Nothing to crop!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d non-bitmap object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   return ret;
}



/* initialisation code for the grab-from-grid plugin */
void datgrid_init(void)
{
   int i;

   if (screen) {
      if (SCREEN_H < 400) {
	 griddle_dlg[0].h = griddle_dlg[0].h * 2/3;
	 for (i=1; griddle_dlg[i].proc; i++) {
	    griddle_dlg[i].y = griddle_dlg[i].y * 2/3;
	    if (griddle_dlg[i].h > 32)
	       griddle_dlg[i].h -= 8;
	 }
      }
   }

   sprintf(griddle_xgrid, "%d", get_config_int("grabber", "griddle_xgrid", 32));
   sprintf(griddle_ygrid, "%d", get_config_int("grabber", "griddle_ygrid", 32));

   if (stricmp(get_config_string("grabber", "griddle_mode", ""), "grid") == 0) {
      griddle_dlg[GRIDDLE_DLG_BOXES].flags &= ~D_SELECTED;
      griddle_dlg[GRIDDLE_DLG_GRID].flags |= D_SELECTED;
   }
   else {
      griddle_dlg[GRIDDLE_DLG_BOXES].flags |= D_SELECTED;
      griddle_dlg[GRIDDLE_DLG_GRID].flags &= ~D_SELECTED;
   }

   if (strpbrk(get_config_string("grabber", "griddle_empties", ""), "yY1"))
      griddle_dlg[GRIDDLE_DLG_EMPTIES].flags |= D_SELECTED;
   else
      griddle_dlg[GRIDDLE_DLG_EMPTIES].flags &= ~D_SELECTED;

   if (strpbrk(get_config_string("grabber", "griddle_autocrop", ""), "yY1"))
      griddle_dlg[GRIDDLE_DLG_AUTOCROP].flags |= D_SELECTED;
   else
      griddle_dlg[GRIDDLE_DLG_AUTOCROP].flags &= ~D_SELECTED;

   griddle_dlg[GRIDDLE_DLG_TYPE].d1 = get_config_int("grabber", "griddle_type", 0);
}



/* hook ourselves into the grabber menu system */
static MENU griddler_menu =
{
   "Grab from Grid",
   griddler,
   NULL,
   0,
   NULL
};



DATEDIT_MENU_INFO datgrid_griddler_menu =
{
   &griddler_menu,
   griddler_query,
   DATEDIT_MENU_FILE | DATEDIT_MENU_POPUP,
   0,
   "XPOS;YPOS;XSIZ;YSIZ"
};



static MENU autocrop_menu =
{
   "Autocrop",
   autocrop,
   NULL,
   0,
   NULL
};



DATEDIT_MENU_INFO datgrid_autocrop_menu =
{
   &autocrop_menu,
   autocrop_query,
   DATEDIT_MENU_OBJECT | DATEDIT_MENU_POPUP,
   0,
   "XCRP;YCRP"
};

