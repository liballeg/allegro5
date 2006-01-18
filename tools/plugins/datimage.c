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
 *      Grabber plugin for managing bitmapped image objects. Regular 
 *      bitmaps, RLE sprites, and compiled sprites are all handled in 
 *      this one file, because they can share a lot of common code.
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



/* checks whether this RLE sprite has an alpha channel */
static int rle_has_alpha(AL_CONST RLE_SPRITE *spr)
{
   int32_t *p32;
   int x, y, c;

   if (spr->color_depth != 32)
      return FALSE;

   p32 = (int32_t *)spr->dat;

   for (y=0; y<spr->h; y++) {
      while ((uint32_t)*p32 != MASK_COLOR_32) {
	 if (*p32 < 0) {
	    p32++;
	 }
	 else {
	    x = *p32;
	    p32++;

	    while (x-- > 0) {
	       c = (uint32_t)*p32;
	       if (geta32(c))
		  return TRUE;
	       p32++;
	    }
	 }
      }

      p32++;
   }

   return FALSE;
}



/* creates a new bitmap object */
static void *makenew_bitmap(long *size)
{
   BITMAP *bmp = create_bitmap_ex(8, 32, 32);

   clear_bitmap(bmp);
   textout_centre_ex(bmp, font, "Hi!", 16, 12, 1, -1);

   return bmp;
}



/* returns a bitmap description string */
static void get_bitmap_desc(AL_CONST DATAFILE *dat, char *s)
{
   BITMAP *bmp = (BITMAP *)dat->dat;

   sprintf(s, "bitmap (%dx%d, %d bit)", bmp->w, bmp->h, bitmap_color_depth(bmp));

   if (_bitmap_has_alpha(bmp))
      strcat(s, " +alpha");
}



/* exports a bitmap into an external file */
static int export_bitmap(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   return (save_bitmap(filename, (BITMAP *)dat->dat, datedit_current_palette) == 0);
}



/* draws a bitmap onto the grabber object view window */
static void plot_bitmap(AL_CONST DATAFILE *dat, int x, int y)
{
   BITMAP *b = dat->dat;
   int w = b->w;
   int h = b->h;
   fixed scale;

   if (w > SCREEN_W-x-8) {
      scale = itofix(SCREEN_W-x-8) / w;
      w = (w * scale) >> 16;
      h = (h * scale) >> 16;
   }

   if (h > SCREEN_H-y-40) {
      scale = itofix(SCREEN_H-y-40) / h;
      w = (w * scale) >> 16;
      h = (h * scale) >> 16;
   }

   rect(screen, x, y+32, x+w+1, y+32+h+1, gui_fg_color);

   if ((bitmap_color_depth(screen) == 8) && (bitmap_color_depth(b) > 8)) {
      if ((w != b->w) || (h != b->h))
	 textout_ex(screen, font, "<reduced color scaled preview>", x, y+18, gui_fg_color, gui_bg_color);
      else
	 textout_ex(screen, font, "<reduced color preview>", x, y+18, gui_fg_color, gui_bg_color);
   }
   else if ((w != b->w) || (h != b->h))
      textout_ex(screen, font, "<scaled preview>", x, y+18, gui_fg_color, gui_bg_color);

   if (bitmap_color_depth(b) == bitmap_color_depth(screen)) {
      if (dat->type != DAT_BITMAP)
	 stretch_sprite(screen, b, x+1, y+33, w, h);
      else
	 stretch_blit(b, screen, 0, 0, b->w, b->h, x+1, y+33, w, h);
   }
   else {
      BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(b), w, h);
      clear_to_color(b2, makecol_depth(bitmap_color_depth(b2), 0x80, 0x80, 0x80));

      if (dat->type != DAT_BITMAP) {
	 RGB tmprgb = datedit_current_palette[0];

	 datedit_current_palette[0].r = 63;
	 datedit_current_palette[0].g = 0;
	 datedit_current_palette[0].b = 63;
	 select_palette(datedit_current_palette);

	 stretch_sprite(b2, b, 0, 0, w, h);
	 blit(b2, screen, 0, 0, x+1, y+33, w, h);

	 unselect_palette();
	 datedit_current_palette[0] = tmprgb;
      }
      else {
	 stretch_blit(b, b2, 0, 0, b->w, b->h, 0, 0, w, h);
	 blit(b2, screen, 0, 0, x+1, y+33, w, h);
      }

      destroy_bitmap(b2);
   }
}



/* handles double-clicking on a bitmap in the grabber */
static int view_bitmap(DATAFILE *dat)
{
   BITMAP *b = dat->dat;
   fixed scale = itofix(1);
   fixed prevscale = itofix(1);
   int x = 0;
   int y = 0;
   int prevx = 0;
   int prevy = 0;
   BITMAP *bc = NULL;
   int done = FALSE;
   int c;

   show_mouse(NULL);
   clear_to_color(screen, gui_mg_color);
   blit(b, screen, 0, 0, 0, 0, b->w, b->h);

   clear_keybuf();
   do {
      poll_mouse();
   } while (mouse_b);

   do {
      if ((x != prevx) || (y != prevy) || (scale != prevscale)) {
	 prevx = itofix(SCREEN_W) / scale;
	 prevy = itofix(SCREEN_H) / scale;

	 if ((b->w >= prevx) && (x+prevx > b->w))
	    x = b->w-prevx;
	 else if ((b->w < prevx) && (x > 0))
	    x = 0;

	 if ((b->h >= prevy) && (y+prevy > b->h))
	    y = b->h-prevy;
	 else if ((b->h < prevy) && (y > 0))
	    y = 0;

	 if (x < 0)
	    x = 0;

	 if (y < 0)
	    y = 0;

	 if (scale != prevscale)
	    clear_to_color(screen, gui_mg_color);

	 if (!bc) {
	    bc = create_bitmap(b->w, b->h);
	    blit(b, bc, 0, 0, 0, 0, b->w, b->h);
	 }

	 stretch_blit(bc, screen, x, y, b->w-x, b->h-y, 0, 0, ((b->w-x)*scale)>>16, ((b->h-y)*scale)>>16);

	 prevx = x;
	 prevy = y;
	 prevscale = scale;
      }

      while (keypressed()) {
	 c = readkey();

	 switch (c >> 8) {

	    case KEY_DOWN:
	       y += 4;
	       break;

	    case KEY_RIGHT:
	       x += 4;
	       break;

	    case KEY_UP:
	       y -= 4;
	       break;

	    case KEY_LEFT:
	       x -= 4;
	       break;

	    case KEY_HOME:
	       x = 0;
	       y = 0;
	       break;

	    case KEY_END:
	       x = 65536;
	       y = 65536;
	       break;

	    case KEY_PGUP:
	       if (scale > itofix(1)/16)
		  scale /= 2;
	       break;

	    case KEY_PGDN:
	       if (scale < itofix(16))
		  scale *= 2;
	       break;

	    default:
	       switch (c & 0xFF) {

		  case '+':
		     if (scale < itofix(16))
			scale *= 2;
		     break;

		  case '-':
		     if (scale > itofix(1)/16)
			scale /= 2;
		     break;

		  default:
		     done = TRUE;
		     break;
	       }
	       break;
	 }
      }

      poll_mouse();

      if (mouse_b)
	 done = TRUE;

   } while (!done);

   if (bc)
      destroy_bitmap(bc);

   clear_keybuf();

   do {
      poll_mouse();
   } while (mouse_b);

   clear_bitmap(screen);
   show_mouse(screen);

   return D_REDRAW;
}



/* reads a bitmap from an external file */
static DATAFILE *grab_bitmap(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   BITMAP *bmp;
   int x, y, w, h;

   if (depth > 0) {
      int oldcolordepth = _color_depth;

      _color_depth = depth;
      set_color_conversion(COLORCONV_TOTAL);

      bmp = load_bitmap(filename, datedit_last_read_pal);

      _color_depth = oldcolordepth;
      set_color_conversion(COLORCONV_NONE);
   }
   else
      bmp = load_bitmap(filename, datedit_last_read_pal);

   if (!bmp)
      return NULL;

   x = datedit_numprop(prop, DAT_XPOS);
   y = datedit_numprop(prop, DAT_YPOS);
   w = datedit_numprop(prop, DAT_XSIZ);
   h = datedit_numprop(prop, DAT_YSIZ);

   if ((x >= 0) && (y >= 0) && (w >= 0) && (h >= 0)) {
      BITMAP *b2 = create_bitmap_ex(bitmap_color_depth(bmp), w, h);
      clear_to_color(b2, bitmap_mask_color(b2));
      blit(bmp, b2, x, y, 0, 0, w, h);
      destroy_bitmap(bmp);
      bmp = b2;
   }
   
   return datedit_construct(type, bmp, 0, prop);
}



/* saves a bitmap into the datafile format */
static int save_datafile_bitmap(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
   BITMAP *bmp = (BITMAP *)dat->dat;
   int x, y, c, r, g, b, a;
   uint16_t *p16;
   uint8_t *p24;
   uint32_t *p32;
   int depth;

   if (_bitmap_has_alpha(bmp))
      depth = -32;
   else
      depth = bitmap_color_depth(bmp);

   *allegro_errno = 0;

   pack_mputw(depth, f);
   pack_mputw(bmp->w, f);
   pack_mputw(bmp->h, f);

   switch (depth) {

      case 8:
	 /* 256 colors */
	 for (y=0; y<bmp->h; y++)
	    for (x=0; x<bmp->w; x++)
	       pack_putc(bmp->line[y][x], f);
	 break;

      case 15:
      case 16:
	 /* hicolor */
	 for (y=0; y<bmp->h; y++) {
	    p16 = (uint16_t *)bmp->line[y];

	    for (x=0; x<bmp->w; x++) {
	       c = p16[x];
	       r = getr_depth(depth, c);
	       g = getg_depth(depth, c);
	       b = getb_depth(depth, c);
	       c = ((r<<8)&0xF800) | ((g<<3)&0x7E0) | ((b>>3)&0x1F);
	       pack_iputw(c, f);
	    }
	 }
	 break;

      case 24:
	 /* 24 bit truecolor */
	 for (y=0; y<bmp->h; y++) {
	    p24 = (uint8_t *)bmp->line[y];

	    for (x=0; x<bmp->w; x++) {
	       c = READ3BYTES(p24);
	       r = getr24(c);
	       g = getg24(c);
	       b = getb24(c);
	       pack_putc(r, f);
	       pack_putc(g, f);
	       pack_putc(b, f);
	       p24 += 3;
	    }
	 }
	 break;

      case 32:
	 /* 32 bit truecolor */
	 for (y=0; y<bmp->h; y++) {
	    p32 = (uint32_t *)bmp->line[y];

	    for (x=0; x<bmp->w; x++) {
	       c = p32[x];
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       pack_putc(r, f);
	       pack_putc(g, f);
	       pack_putc(b, f);
	    }
	 }
	 break;

      case -32:
	 /* 32 bit truecolor with alpha channel */
	 for (y=0; y<bmp->h; y++) {
	    p32 = (uint32_t *)bmp->line[y];

	    for (x=0; x<bmp->w; x++) {
	       c = p32[x];
	       r = getr32(c);
	       g = getg32(c);
	       b = getb32(c);
	       a = geta32(c);
	       pack_putc(r, f);
	       pack_putc(g, f);
	       pack_putc(b, f);
	       pack_putc(a, f);
	    }
	 }
	 break;
   }

   if (*allegro_errno)
      return FALSE;
   else
      return TRUE;
}



/* creates a new RLE sprite object */
static void *makenew_rle_sprite(long *size)
{
   BITMAP *bmp = makenew_bitmap(size);
   RLE_SPRITE *spr = get_rle_sprite(bmp);

   destroy_bitmap(bmp);

   return spr;
}



/* draws an RLE sprite onto the grabber object view window */
static void plot_rle_sprite(AL_CONST DATAFILE *dat, int x, int y)
{
   RLE_SPRITE *s = dat->dat;
   BITMAP *b = create_bitmap_ex(s->color_depth, s->w, s->h);
   DATAFILE tmpdat;

   tmpdat.dat = b;
   tmpdat.type = DAT_RLE_SPRITE;
   tmpdat.size = 0;
   tmpdat.prop = NULL;

   clear_to_color(b, b->vtable->mask_color);
   draw_rle_sprite(b, s, 0, 0);
   plot_bitmap(&tmpdat, x, y);
   destroy_bitmap(b);
}



/* handles double-clicking on an RLE sprite in the grabber */
static int view_rle_sprite(DATAFILE *dat)
{
   RLE_SPRITE *spr = dat->dat;
   BITMAP *bmp = create_bitmap_ex(spr->color_depth, spr->w, spr->h);
   DATAFILE tmpdat;

   tmpdat.dat = bmp;
   tmpdat.type = DAT_RLE_SPRITE;
   tmpdat.size = 0;
   tmpdat.prop = NULL;

   clear_to_color(bmp, bmp->vtable->mask_color);
   draw_rle_sprite(bmp, spr, 0, 0);
   view_bitmap(&tmpdat);
   destroy_bitmap(bmp);
   return D_REDRAW;
}



/* returns a description string for an RLE sprite */
static void get_rle_sprite_desc(AL_CONST DATAFILE *dat, char *s)
{
   RLE_SPRITE *spr = (RLE_SPRITE *)dat->dat;

   sprintf(s, "RLE sprite (%dx%d, %d bit)", spr->w, spr->h, spr->color_depth);

   if (rle_has_alpha(spr))
      strcat(s, " +alpha");
}



/* exports an RLE sprite into an external file */
static int export_rle_sprite(AL_CONST DATAFILE *dat, AL_CONST char *filename)
{
   BITMAP *bmp;
   RLE_SPRITE *rle = (RLE_SPRITE *)dat->dat;
   int ret;

   bmp = create_bitmap_ex(rle->color_depth, rle->w, rle->h);
   clear_to_color(bmp, bmp->vtable->mask_color);
   draw_rle_sprite(bmp, rle, 0, 0);

   ret = (save_bitmap(filename, bmp, datedit_current_palette) == 0);

   destroy_bitmap(bmp);
   return ret;
}



/* reads an RLE sprite from an external file */
static DATAFILE *grab_rle_sprite(int type, AL_CONST char *filename, DATAFILE_PROPERTY **prop, int depth)
{
   DATAFILE *dat;
   BITMAP *bmp;

   dat = grab_bitmap(type, filename, prop, depth);
   if (!dat)
      return NULL;

   bmp = dat->dat;
   dat->dat = get_rle_sprite(bmp);
   destroy_bitmap(bmp);

   return dat;
}



/* saves an RLE sprite into the datafile format */
static int save_rle_sprite(DATAFILE *dat, AL_CONST int *fixed_prop, int pack, int pack_kids, int strip, int sort, int verbose, int extra, PACKFILE *f)
{
   RLE_SPRITE *spr = (RLE_SPRITE *)dat->dat;
   int x, y, c, r, g, b, a;
   int16_t *p16;
   int32_t *p32;
   unsigned long eol_marker;
   int depth;

   if (rle_has_alpha(spr))
      depth = -32;
   else
      depth = spr->color_depth;

   *allegro_errno = 0;

   pack_mputw(depth, f);
   pack_mputw(spr->w, f);
   pack_mputw(spr->h, f);
   pack_mputl(spr->size, f);

   switch (depth) {

      case 8:
	 /* 256 colors, easy! */
	 pack_fwrite(spr->dat, spr->size, f);
	 break;

      case 15:
      case 16:
	 /* hicolor */
	 p16 = (int16_t *)spr->dat;
	 eol_marker = (depth == 15) ? MASK_COLOR_15 : MASK_COLOR_16;

	 for (y=0; y<spr->h; y++) {
	    while ((uint16_t)*p16 != (uint16_t)eol_marker) {
	       if (*p16 < 0) {
		  /* skip count */
		  pack_iputw(*p16, f);
		  p16++;
	       }
	       else {
		  /* solid run */
		  x = *p16;
		  p16++;

		  pack_iputw(x, f);

		  while (x-- > 0) {
		     c = (uint16_t)*p16;
		     r = getr_depth(depth, c);
		     g = getg_depth(depth, c);
		     b = getb_depth(depth, c);
		     c = ((r<<8)&0xF800) | ((g<<3)&0x7E0) | ((b>>3)&0x1F);
		     pack_iputw(c, f);
		     p16++;
		  }
	       }
	    }

	    /* end of line */
	    pack_iputw(MASK_COLOR_16, f);
	    p16++;
	 }
	 break;

      case 24:
      case 32:
	 /* truecolor */
	 p32 = (int32_t *)spr->dat;
	 eol_marker = (depth == 24) ? MASK_COLOR_24 : MASK_COLOR_32;

	 for (y=0; y<spr->h; y++) {
	    while ((uint32_t)*p32 != eol_marker) {
	       if (*p32 < 0) {
		  /* skip count */
		  pack_iputl(*p32, f);
		  p32++;
	       }
	       else {
		  /* solid run */
		  x = *p32;
		  p32++;

		  pack_iputl(x, f);

		  while (x-- > 0) {
		     c = (uint32_t)*p32;
		     r = getr_depth(depth, c);
		     g = getg_depth(depth, c);
		     b = getb_depth(depth, c);
		     pack_putc(r, f);
		     pack_putc(g, f);
		     pack_putc(b, f);
		     p32++;
		  }
	       }
	    }

	    /* end of line */
	    pack_iputl(MASK_COLOR_32, f);
	    p32++;
	 }
	 break;

      case -32:
	 /* truecolor with alpha channel */
	 p32 = (int32_t *)spr->dat;

	 for (y=0; y<spr->h; y++) {
	    while ((uint32_t)*p32 != MASK_COLOR_32) {
	       if (*p32 < 0) {
		  /* skip count */
		  pack_iputl(*p32, f);
		  p32++;
	       }
	       else {
		  /* solid run */
		  x = *p32;
		  p32++;

		  pack_iputl(x, f);

		  while (x-- > 0) {
		     c = (uint32_t)*p32;
		     r = getr32(c);
		     g = getg32(c);
		     b = getb32(c);
		     a = geta32(c);
		     pack_putc(r, f);
		     pack_putc(g, f);
		     pack_putc(b, f);
		     pack_putc(a, f);
		     p32++;
		  }
	       }
	    }

	    /* end of line */
	    pack_iputl(MASK_COLOR_32, f);
	    p32++;
	 }
	 break;
   }

   if (*allegro_errno)
      return FALSE;
   else
      return TRUE;
}



/* returns a description string for a compiled sprite object */
static void get_c_sprite_desc(AL_CONST DATAFILE *dat, char *s)
{
   BITMAP *bmp = (BITMAP *)dat->dat;

   sprintf(s, "compiled sprite (%dx%d, %d bit)", bmp->w, bmp->h, bitmap_color_depth(bmp));
}



/* returns a description string for a mode-X compiled sprite object */
static void get_xc_sprite_desc(AL_CONST DATAFILE *dat, char *s)
{
   BITMAP *bmp = (BITMAP *)dat->dat;

   if (bitmap_color_depth(bmp) == 8)
      sprintf(s, "mode-X compiled sprite (%dx%d)", bmp->w, bmp->h);
   else
      sprintf(s, "!!! %d bit XC sprite not possible !!!", bitmap_color_depth(bmp));
}



/* plugin interface header */
DATEDIT_OBJECT_INFO datbitmap_info =
{
   DAT_BITMAP, 
   "Bitmap", 
   get_bitmap_desc,
   makenew_bitmap,
   save_datafile_bitmap,
   plot_bitmap,
   view_bitmap,
   NULL
};



DATEDIT_GRABBER_INFO datbitmap_grabber =
{
   DAT_BITMAP, 
   "bmp;lbm;pcx;tga",
   "bmp;pcx;tga",
   grab_bitmap,
   export_bitmap,
   "XPOS;YPOS;XSIZ;YSIZ"
};



/* plugin interface header */
DATEDIT_OBJECT_INFO datrlesprite_info =
{
   DAT_RLE_SPRITE, 
   "RLE sprite", 
   get_rle_sprite_desc,
   makenew_rle_sprite,
   save_rle_sprite,
   plot_rle_sprite,
   view_rle_sprite,
   NULL
};



DATEDIT_GRABBER_INFO datrlesprite_grabber =
{
   DAT_RLE_SPRITE, 
   "bmp;lbm;pcx;tga",
   "bmp;pcx;tga",
   grab_rle_sprite,
   export_rle_sprite,
   "XPOS;YPOS;XSIZ;YSIZ"
};



/* plugin interface header */
DATEDIT_OBJECT_INFO datcsprite_info =
{
   DAT_C_SPRITE, 
   "Compiled sprite", 
   get_c_sprite_desc,
   makenew_bitmap,
   save_datafile_bitmap,
   plot_bitmap,
   view_bitmap,
   NULL
};



DATEDIT_GRABBER_INFO datcsprite_grabber =
{
   DAT_C_SPRITE, 
   "bmp;lbm;pcx;tga",
   "bmp;pcx;tga",
   grab_bitmap,
   export_bitmap,
   "XPOS;YPOS;XSIZ;YSIZ"
};



/* plugin interface header */
DATEDIT_OBJECT_INFO datxcsprite_info =
{ 
   DAT_XC_SPRITE, 
   "Compiled X-sprite", 
   get_xc_sprite_desc,
   makenew_bitmap,
   save_datafile_bitmap,
   plot_bitmap,
   view_bitmap,
   NULL
};



DATEDIT_GRABBER_INFO datxcsprite_grabber =
{ 
   DAT_XC_SPRITE, 
   "bmp;lbm;pcx;tga",
   "bmp;pcx;tga",
   grab_bitmap,
   export_bitmap,
   "XPOS;YPOS;XSIZ;YSIZ"
};

