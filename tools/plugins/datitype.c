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
 *      Grabber plugin for converting between different image formats
 *      and color depths.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>

#include "allegro.h"
#include "../datedit.h"



/* worker function for counting bitmap objects */
static int do_bitmap_check(DATAFILE *dat, int *param, int param2)
{
   if ((dat->type == DAT_BITMAP) || (dat->type == DAT_RLE_SPRITE) ||
       (dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) 
      (*param)++;

   return D_O_K;
}



/* checks whether our image changing commands are allowed at the moment */
static int change_query(int popup)
{
   int n, p;

   if (popup) {
      p = 0;
      grabber_foreach_selection(do_bitmap_check, &n, &p, 0);
      return (p > 0);
   }

   return TRUE;
}



/* worker function for changing the type of an image */
static int do_changetype(DATAFILE *dat, int *param, int type)
{
   BITMAP *bmp;
   RLE_SPRITE *spr;
   int x, y, c, r, g, b;

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      (*param)++;
      return D_O_K;
   }

   if (dat->type == type)
      return D_O_K;

   if (dat->type == DAT_RLE_SPRITE) {
      spr = (RLE_SPRITE *)dat->dat;
      bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, spr, 0, 0);
      dat->dat = bmp;
      destroy_rle_sprite(spr);
   }
   else if (type == DAT_RLE_SPRITE) {
      bmp = (BITMAP *)dat->dat;
      spr = get_rle_sprite(bmp);
      dat->dat = spr;
      destroy_bitmap(bmp);
   }
   else if ((type == DAT_C_SPRITE) || (type == DAT_XC_SPRITE)) {
      bmp = (BITMAP *)dat->dat;
      if (bitmap_color_depth(bmp) == 32) {
	 for (y=0; y<bmp->h; y++) {
	    for (x=0; x<bmp->w; x++) {
	       c = getpixel(bmp, x, y);
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       putpixel(bmp, x, y, makecol32(r, g, b));
	    }
	 }
      }
   }

   dat->type = type;

   return D_REDRAW;
}



/* changes the type of bitmap data */
static int changetype(void)
{
   int type = (int)((unsigned long)active_menu->dp);
   char buf[80];
   int ret, n;
   int p = 0;

   ret = grabber_foreach_selection(do_changetype, &n, &p, type);

   if (n <= 0) {
      alert ("Nothing to change!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d non-bitmap object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   if (n > p) {
      grabber_modified(TRUE);
      grabber_rebuild_list(NULL, FALSE);
   }

   return ret;
}



/* worker function for changing the color depth of bitmap data */
static int do_changedepth(DATAFILE *dat, int *param, int depth)
{
   BITMAP *bmp, *bmp2;
   RLE_SPRITE *spr;
   RGB tmprgb = datedit_current_palette[0];

   if ((dat->type != DAT_BITMAP) && (dat->type != DAT_RLE_SPRITE) &&
       (dat->type != DAT_C_SPRITE) && (dat->type != DAT_XC_SPRITE)) {
      (*param)++;
      return D_O_K;
   }

   if (dat->type == DAT_RLE_SPRITE) {
      spr = (RLE_SPRITE *)dat->dat;
      if (spr->color_depth == depth)
	 return D_O_K;

      bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
      clear_to_color(bmp, bmp->vtable->mask_color);
      draw_rle_sprite(bmp, spr, 0, 0);
      bmp2 = create_bitmap_ex(depth, bmp->w, bmp->h);

      datedit_current_palette[0].r = 63;
      datedit_current_palette[0].g = 0;
      datedit_current_palette[0].b = 63;
      select_palette(datedit_current_palette);

      blit(bmp, bmp2, 0, 0, 0, 0, bmp->w, bmp->h);

      unselect_palette();
      datedit_current_palette[0] = tmprgb;

      dat->dat = get_rle_sprite(bmp2);
      destroy_bitmap(bmp);
      destroy_bitmap(bmp2);
      destroy_rle_sprite(spr);
   }
   else {
      bmp = (BITMAP *)dat->dat;
      if (bitmap_color_depth(bmp) == depth)
	 return D_O_K;
      bmp2 = create_bitmap_ex(depth, bmp->w, bmp->h);

      if ((dat->type == DAT_C_SPRITE) || (dat->type == DAT_XC_SPRITE)) {
	 datedit_current_palette[0].r = 63;
	 datedit_current_palette[0].g = 0;
	 datedit_current_palette[0].b = 63;
      }
      select_palette(datedit_current_palette);

      blit(bmp, bmp2, 0, 0, 0, 0, bmp->w, bmp->h);

      unselect_palette();
      datedit_current_palette[0] = tmprgb;

      dat->dat = bmp2;
      destroy_bitmap(bmp);
   }

   return D_REDRAW;
}



/* changes the color depth of bitmap data */
static int changedepth(void)
{
   int depth = (int)((unsigned long)active_menu->dp);
   char buf[80];
   int ret, n;
   int p = 0;

   grabber_busy_mouse(TRUE);

   ret = grabber_foreach_selection(do_changedepth, &n, &p, depth);

   grabber_busy_mouse(FALSE);

   if (n <= 0) {
      alert ("Nothing to change!", NULL, NULL, "OK", NULL, 13, 0);
   }
   else if (p > 0) {
      sprintf(buf, "%d non-bitmap object%s ignored", p, (p==1) ? " was" : "s were");
      alert(buf, NULL, NULL, "OK", NULL, 13, 0);
   }

   if (n > p)
      grabber_modified(TRUE);

   return ret;
}



static MENU type_menu[] =
{
   { "To &Bitmap",               changetype,    NULL,    0,    (void *)DAT_BITMAP      },
   { "To &RLE Sprite",           changetype,    NULL,    0,    (void *)DAT_RLE_SPRITE  },
   { "To &Compiled Sprite",      changetype,    NULL,    0,    (void *)DAT_C_SPRITE    },
   { "To &X-Compiled Sprite",    changetype,    NULL,    0,    (void *)DAT_XC_SPRITE   },
   { NULL,                       NULL,          NULL,    0,    NULL                    }
};



static MENU depth_menu[] =
{
   { "To &256 color palette",    changedepth,   NULL,    0,    (void *)8   },
   { "To 1&5 bit hicolor",       changedepth,   NULL,    0,    (void *)15  },
   { "To 1&6 bit hicolor",       changedepth,   NULL,    0,    (void *)16  },
   { "To 2&4 bit truecolor",     changedepth,   NULL,    0,    (void *)24  },
   { "To &32 bit truecolor",     changedepth,   NULL,    0,    (void *)32  },
   { NULL,                       NULL,          NULL,    0,    NULL        }
};



/* hook ourselves into the grabber menu system */
static MENU change_type_menu =
{
   "Change Type",
   NULL,
   type_menu,
   0,
   NULL
};



DATEDIT_MENU_INFO datitype_type_menu =
{
   &change_type_menu,
   change_query,
   DATEDIT_MENU_OBJECT | DATEDIT_MENU_POPUP,
   0,
   NULL
};



static MENU change_depth_menu =
{
   "Change Depth",
   NULL,
   depth_menu,
   0,
   NULL
};



DATEDIT_MENU_INFO datitype_depth_menu =
{
   &change_depth_menu,
   change_query,
   DATEDIT_MENU_OBJECT | DATEDIT_MENU_POPUP,
   0,
   NULL
};


