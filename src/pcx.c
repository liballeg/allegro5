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
 *      PCX reader.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* load_pcx:
 *  Loads a 256 color PCX file, returning a bitmap structure and storing
 *  the palette data in the specified palette (this should be an array of
 *  at least 256 RGB structures).
 */
BITMAP *load_pcx(AL_CONST char *filename, RGB *pal)
{
   PACKFILE *f;
   BITMAP *b;
   PALETTE tmppal;
   int c;
   int width, height;
   int bpp, bytes_per_line;
   int xx, po;
   int x, y;
   char ch;
   int dest_depth;

   if (!pal)
      pal = tmppal;

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   pack_getc(f);                    /* skip manufacturer ID */
   pack_getc(f);                    /* skip version flag */
   pack_getc(f);                    /* skip encoding flag */

   if (pack_getc(f) != 8) {         /* we like 8 bit color planes */
      pack_fclose(f);
      return NULL;
   }

   width = -(pack_igetw(f));        /* xmin */
   height = -(pack_igetw(f));       /* ymin */
   width += pack_igetw(f) + 1;      /* xmax */
   height += pack_igetw(f) + 1;     /* ymax */

   pack_igetl(f);                   /* skip DPI values */

   for (c=0; c<16; c++) {           /* read the 16 color palette */
      pal[c].r = pack_getc(f) / 4;
      pal[c].g = pack_getc(f) / 4;
      pal[c].b = pack_getc(f) / 4;
   }

   pack_getc(f);

   bpp = pack_getc(f) * 8;          /* how many color planes? */
   if ((bpp != 8) && (bpp != 24)) {
      pack_fclose(f);
      return NULL;
   }

   dest_depth = _color_load_depth(bpp, FALSE);
   bytes_per_line = pack_igetw(f);

   for (c=0; c<60; c++)             /* skip some more junk */
      pack_getc(f);

   b = create_bitmap_ex(bpp, width, height);
   if (!b) {
      pack_fclose(f);
      return FALSE;
   }

   for (y=0; y<height; y++) {       /* read RLE encoded PCX data */
      x = xx = 0;
      po = _rgb_r_shift_24/8;

      while (x < bytes_per_line*bpp/8) {
	 ch = pack_getc(f);
	 if ((ch & 0xC0) == 0xC0) {
	    c = (ch & 0x3F);
	    ch = pack_getc(f);
	 }
	 else
	    c = 1;

	 if (bpp == 8) {
	    while (c--) {
	       if (x < b->w)
		  b->line[y][x] = ch;
	       x++;
	    }
	 }
	 else {
	    while (c--) {
	       if (xx < b->w)
		  b->line[y][xx*3+po] = ch;
	       x++;
	       if (x == bytes_per_line) {
		  xx = 0;
		  po = _rgb_g_shift_24/8;
	       }
	       else if (x == bytes_per_line*2) {
		  xx = 0;
		  po = _rgb_b_shift_24/8;
	       }
	       else
		  xx++;
	    }
	 }
      }
   }

   if (bpp == 8) {                  /* look for a 256 color palette */
      while (!pack_feof(f)) { 
	 if (pack_getc(f)==12) {
	    for (c=0; c<256; c++) {
	       pal[c].r = pack_getc(f) / 4;
	       pal[c].g = pack_getc(f) / 4;
	       pal[c].b = pack_getc(f) / 4;
	    }
	    break;
	 }
      }
   }
   else
      generate_332_palette(pal);

   pack_fclose(f);

   if (*allegro_errno) {
      destroy_bitmap(b);
      return FALSE;
   }

   if (dest_depth != bpp)
      b = _fixup_loaded_bitmap(b, pal, dest_depth);

   return b;
}



/* save_pcx:
 *  Writes a bitmap into a PCX file, using the specified palette (this
 *  should be an array of at least 256 RGB structures).
 */
int save_pcx(AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal)
{
   PACKFILE *f;
   PALETTE tmppal;
   int c;
   int x, y;
   int runcount;
   int depth, planes;
   char runchar;
   char ch;

   if (!pal) {
      get_palette(tmppal);
      pal = tmppal;
   }

   f = pack_fopen(filename, F_WRITE);
   if (!f)
      return *allegro_errno;

   depth = bitmap_color_depth(bmp);
   if (depth == 8)
      planes = 1;
   else
      planes = 3;

   pack_putc(10, f);                      /* manufacturer */
   pack_putc(5, f);                       /* version */
   pack_putc(1, f);                       /* run length encoding  */
   pack_putc(8, f);                       /* 8 bits per pixel */
   pack_iputw(0, f);                      /* xmin */
   pack_iputw(0, f);                      /* ymin */
   pack_iputw(bmp->w-1, f);               /* xmax */
   pack_iputw(bmp->h-1, f);               /* ymax */
   pack_iputw(320, f);                    /* HDpi */
   pack_iputw(200, f);                    /* VDpi */

   for (c=0; c<16; c++) {
      pack_putc(_rgb_scale_6[pal[c].r], f);
      pack_putc(_rgb_scale_6[pal[c].g], f);
      pack_putc(_rgb_scale_6[pal[c].b], f);
   }

   pack_putc(0, f);                       /* reserved */
   pack_putc(planes, f);                  /* one or three color planes */
   pack_iputw(bmp->w, f);                 /* number of bytes per scanline */
   pack_iputw(1, f);                      /* color palette */
   pack_iputw(bmp->w, f);                 /* hscreen size */
   pack_iputw(bmp->h, f);                 /* vscreen size */
   for (c=0; c<54; c++)                   /* filler */
      pack_putc(0, f);

   for (y=0; y<bmp->h; y++) {             /* for each scanline... */
      runcount = 0;
      runchar = 0;
      for (x=0; x<bmp->w*planes; x++) {   /* for each pixel... */
	 if (depth == 8) {
	    ch = getpixel(bmp, x, y);
	 }
	 else {
	    if (x<bmp->w) {
	       c = getpixel(bmp, x, y);
	       ch = getr_depth(depth, c);
	    }
	    else if (x<bmp->w*2) {
	       c = getpixel(bmp, x-bmp->w, y);
	       ch = getg_depth(depth, c);
	    }
	    else {
	       c = getpixel(bmp, x-bmp->w*2, y);
	       ch = getb_depth(depth, c);
	    }
	 }
	 if (runcount==0) {
	    runcount = 1;
	    runchar = ch;
	 }
	 else {
	    if ((ch != runchar) || (runcount >= 0x3f)) {
	       if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
		  pack_putc(0xC0 | runcount, f);
	       pack_putc(runchar,f);
	       runcount = 1;
	       runchar = ch;
	    }
	    else
	       runcount++;
	 }
      }
      if ((runcount > 1) || ((runchar & 0xC0) == 0xC0))
	 pack_putc(0xC0 | runcount, f);
      pack_putc(runchar,f);
   }

   if (depth == 8) {                      /* 256 color palette */
      pack_putc(12, f); 

      for (c=0; c<256; c++) {
	 pack_putc(_rgb_scale_6[pal[c].r], f);
	 pack_putc(_rgb_scale_6[pal[c].g], f);
	 pack_putc(_rgb_scale_6[pal[c].b], f);
      }
   }

   pack_fclose(f);
   return *allegro_errno;
}

