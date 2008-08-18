#include <allegro5/allegro5.h>
#include <allegro5/internal/aintern.h>


typedef struct PalEntry {
   int r, g, b;
} PalEntry;


static ALLEGRO_BITMAP *iio_load_pcx_pf(PACKFILE *f)
{
   ALLEGRO_BITMAP *b;
   int c;
   int width, height;
   int bpp, bytes_per_line;
   int x, xx = 0, y;
   char ch;
   int dest_depth;
   ALLEGRO_LOCKED_REGION lr;
   unsigned char *buf;
   PalEntry pal[256];
   ASSERT(f);

   pack_getc(f);                    /* skip manufacturer ID */
   pack_getc(f);                    /* skip version flag */
   pack_getc(f);                    /* skip encoding flag */

   if (pack_getc(f) != 8) {         /* we like 8 bit color planes */
      return NULL;
   }

   width = -(pack_igetw(f));        /* xmin */
   height = -(pack_igetw(f));       /* ymin */
   width += pack_igetw(f) + 1;      /* xmax */
   height += pack_igetw(f) + 1;     /* ymax */

   pack_igetl(f);                   /* skip DPI values */

   for (c=0; c<16*3; c++) {         /* skip the 16 color palette */
      pack_getc(f);
   }

   pack_getc(f);

   bpp = pack_getc(f) * 8;          /* how many color planes? */
   if ((bpp != 8) && (bpp != 24)) {
      return NULL;
   }

   bytes_per_line = pack_igetw(f);

   for (c=0; c<60; c++)             /* skip some more junk */
      pack_getc(f);

   b = al_create_bitmap(width, height);
   if (!b) {
      return NULL;
   }

   *allegro_errno = 0;

   if (bpp == 8) {
      buf = (unsigned char *)_AL_MALLOC(bytes_per_line*height);
   }
   else {
      buf = (unsigned char *)_AL_MALLOC(bytes_per_line*3);
   }

   _al_push_target_bitmap();
   al_set_target_bitmap(b);
   al_lock_bitmap(b, &lr, ALLEGRO_LOCK_WRITEONLY);

   for (y=0; y<height; y++) {       /* read RLE encoded PCX data */

      x = 0;

      while (x < bytes_per_line*bpp/8) {
	 ch = pack_getc(f);
	 if ((ch & 0xC0) == 0xC0) {
	    c = (ch & 0x3F);
	    ch = pack_getc(f);
	 }
	 else
	    c = 1;

         
         while (c--) {
            buf[xx] = ch;
	    x++;
            xx++;
	 }
      }
      if (bpp == 24) {
         int i;
         for (i = 0; i < width; i++) {
            int r = buf[i];
            int g = buf[i+width];
            int b = buf[i+width*2];
            ALLEGRO_COLOR color = al_map_rgb(r, g, b);
            al_put_pixel(i, y, color);
         }
         xx = 0;
      }
   }

   if (bpp == 8) {                  /* look for a 256 color palette */
      while ((c = pack_getc(f)) != EOF) { 
	 if (c == 12) {
	    for (c=0; c<256; c++) {
	       pal[c].r = pack_getc(f);
	       pal[c].g = pack_getc(f);
	       pal[c].b = pack_getc(f);
	    }
	    break;
	 }
      }
      for (y = 0; y < height; y++) {
         for (x = 0; x < width; x++) {
            int index = buf[y*width+x];
            ALLEGRO_COLOR color = al_map_rgb(
               pal[index].r,
               pal[index].g,
               pal[index].b);
            al_put_pixel(x, y, color);
         }
      }
   }

   _al_pop_target_bitmap();
   al_unlock_bitmap(b);
   
   _AL_FREE(buf);

   if (*allegro_errno) {
      al_destroy_bitmap(b);
      return NULL;
   }

   return b;
}


static int iio_save_pcx_pf(PACKFILE *f, ALLEGRO_BITMAP *bmp)
{
   int c;
   int x, y;
   int i, j;
   int runcount;
   int depth;
   char runchar;
   char ch;
   int w, h;
   unsigned char *buf;
   ALLEGRO_LOCKED_REGION lr;
   ASSERT(f);
   ASSERT(bmp);

   *allegro_errno = 0;
   
   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);

   pack_putc(10, f);                      /* manufacturer */
   pack_putc(5, f);                       /* version */
   pack_putc(1, f);                       /* run length encoding  */
   pack_putc(8, f);                       /* 8 bits per pixel */
   pack_iputw(0, f);                      /* xmin */
   pack_iputw(0, f);                      /* ymin */
   pack_iputw(w-1, f);                    /* xmax */
   pack_iputw(h-1, f);                    /* ymax */
   pack_iputw(320, f);                    /* HDpi */
   pack_iputw(200, f);                    /* VDpi */

   for (c=0; c<16*3; c++) {
      pack_putc(0, f);
   }

   pack_putc(0, f);                       /* reserved */
   pack_putc(3, f);                       /* color planes */
   pack_iputw(w, f);                      /* number of bytes per scanline */
   pack_iputw(1, f);                      /* color palette */
   pack_iputw(w, f);                      /* hscreen size */
   pack_iputw(h, f);                      /* vscreen size */
   for (c=0; c<54; c++)                   /* filler */
      pack_putc(0, f);

   buf = _AL_MALLOC(w*3);

   al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_READONLY);

   for (y=0; y<h; y++) {             /* for each scanline... */
      for (x = 0; x < w; x++) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, x,  y);
         unsigned char r, g, b;
         al_unmap_rgb(c, &r, &g, &b);
         buf[x] = r;
         buf[x+w] = g;
         buf[x+w*2] = b;
      }

      for (i = 0; i < 3; i++) {
         int color;
         int count;
         x = 0;
         for (;;) {
            count = 0;
            color = buf[x+w*i];
            do {
               count++;
               x++;
            } while ((count < 63) && (x < w) && (color == buf[x+w*i]));
            pack_putc(count | 0xC0, f);
            pack_putc(color, f);
            if (x >= w)
               break;
         }
      }
   }

   _AL_FREE(buf);

   al_unlock_bitmap(bmp);

   if (*allegro_errno)
      return -1;
   else
      return 0;
}


ALLEGRO_BITMAP *iio_load_pcx(AL_CONST char *filename)
{
   PACKFILE *f;
   ALLEGRO_BITMAP *bmp;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   bmp = iio_load_pcx_pf(f);

   pack_fclose(f);

   return bmp;
}


int iio_save_pcx(AL_CONST char *filename, ALLEGRO_BITMAP *bmp)
{
   PACKFILE *f;
   int ret;
   ASSERT(filename);

   f = pack_fopen(filename, F_WRITE);
   if (!f)
      return -1;

   ret = iio_save_pcx_pf(f, bmp);

   pack_fclose(f);
   
   return ret;
}


