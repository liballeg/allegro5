#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"

#include "iio.h"


static ALLEGRO_BITMAP *iio_load_pcx_pf(ALLEGRO_FS_ENTRY *f)
{
   ALLEGRO_BITMAP *b;
   int c;
   int width, height;
   int bpp, bytes_per_line;
   int x, xx, y;
   char ch;
   ALLEGRO_LOCKED_REGION lr;
   ALLEGRO_STATE backup;
   unsigned char *buf;
   PalEntry pal[256];
   ASSERT(f);

   al_fs_entry_getc(f);                    /* skip manufacturer ID */
   al_fs_entry_getc(f);                    /* skip version flag */
   al_fs_entry_getc(f);                    /* skip encoding flag */

   if (al_fs_entry_getc(f) != 8) {         /* we like 8 bit color planes */
      return NULL;
   }

   width = -(al_fs_entry_igetw(f));        /* xmin */
   height = -(al_fs_entry_igetw(f));       /* ymin */
   width += al_fs_entry_igetw(f) + 1;      /* xmax */
   height += al_fs_entry_igetw(f) + 1;     /* ymax */

   al_fs_entry_igetl(f);                   /* skip DPI values */

   for (c = 0; c < 16 * 3; c++) {          /* skip the 16 color palette */
      al_fs_entry_getc(f);
   }

   al_fs_entry_getc(f);

   bpp = al_fs_entry_getc(f) * 8;          /* how many color planes? */

   if ((bpp != 8) && (bpp != 24)) {
      return NULL;
   }

   bytes_per_line = al_fs_entry_igetw(f);

   for (c = 0; c < 60; c++)                /* skip some more junk */
      al_fs_entry_getc(f);

   b = al_create_bitmap(width, height);
   if (!b) {
      return NULL;
   }

   al_set_errno(0);

   if (bpp == 8) {
      /* The palette comes after the image data.  We need to to keep the
       * whole image in a temporary buffer before mapping the final colours.
       */
      buf = (unsigned char *)malloc(bytes_per_line * height);
   }
   else {
      /* We can read one line at a time. */
      buf = (unsigned char *)malloc(bytes_per_line * 3);
   }

   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(b);
   al_lock_bitmap(b, &lr, ALLEGRO_LOCK_WRITEONLY);

   xx = 0;                      /* index into buf, only for bpp = 8 */

   for (y = 0; y < height; y++) {       /* read RLE encoded PCX data */

      x = 0;

      while (x < bytes_per_line * bpp / 8) {
         ch = al_fs_entry_getc(f);
         if ((ch & 0xC0) == 0xC0) { /* a run */
            c = (ch & 0x3F);
            ch = al_fs_entry_getc(f);
         }
         else {
            c = 1;                  /* single pixel */
         }

         if (bpp == 8) {
            while (c--) {
               if (x < width)       /* ignore padding */
                  buf[xx++] = ch;
               x++;
            }
         }
         else {
            while (c--) {
               if (x < width * 3)   /* ignore padding */
                  buf[x] = ch;
               x++;
            }
         }
      }
      if (bpp == 24) {
         int i;
         for (i = 0; i < width; i++) {
            int r = buf[i];
            int g = buf[i + width];
            int b = buf[i + width * 2];
            ALLEGRO_COLOR color = al_map_rgb(r, g, b);
            al_put_pixel(i, y, color);
         }
      }
   }

   if (bpp == 8) {               /* look for a 256 color palette */
      while ((c = al_fs_entry_getc(f)) != EOF) {
         if (c == 12) {
            for (c = 0; c < 256; c++) {
               pal[c].r = al_fs_entry_getc(f);
               pal[c].g = al_fs_entry_getc(f);
               pal[c].b = al_fs_entry_getc(f);
            }
            break;
         }
      }
      for (y = 0; y < height; y++) {
         for (x = 0; x < width; x++) {
            int index = buf[y * width + x];
            ALLEGRO_COLOR color = al_map_rgb(pal[index].r,
                                             pal[index].g,
                                             pal[index].b);
            al_put_pixel(x, y, color);
         }
      }
   }

   al_restore_state(&backup);
   al_unlock_bitmap(b);

   free(buf);

   if (al_get_errno()) {
      al_destroy_bitmap(b);
      return NULL;
   }

   return b;
}


static int iio_save_pcx_pf(ALLEGRO_FS_ENTRY *f, ALLEGRO_BITMAP *bmp)
{
   int c;
   int x, y;
   int i;
   int w, h;
   unsigned char *buf;
   ALLEGRO_LOCKED_REGION lr;
   ASSERT(f);
   ASSERT(bmp);

   al_set_errno(0);

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);

   al_fs_entry_putc(10, f);     /* manufacturer */
   al_fs_entry_putc(5, f);      /* version */
   al_fs_entry_putc(1, f);      /* run length encoding  */
   al_fs_entry_putc(8, f);      /* 8 bits per pixel */
   al_fs_entry_iputw(0, f);     /* xmin */
   al_fs_entry_iputw(0, f);     /* ymin */
   al_fs_entry_iputw(w - 1, f); /* xmax */
   al_fs_entry_iputw(h - 1, f); /* ymax */
   al_fs_entry_iputw(320, f);   /* HDpi */
   al_fs_entry_iputw(200, f);   /* VDpi */

   for (c = 0; c < 16 * 3; c++) {
      al_fs_entry_putc(0, f);
   }

   al_fs_entry_putc(0, f);      /* reserved */
   al_fs_entry_putc(3, f);      /* color planes */
   al_fs_entry_iputw(w, f);     /* number of bytes per scanline */
   al_fs_entry_iputw(1, f);     /* color palette */
   al_fs_entry_iputw(w, f);     /* hscreen size */
   al_fs_entry_iputw(h, f);     /* vscreen size */
   for (c = 0; c < 54; c++)     /* filler */
      al_fs_entry_putc(0, f);

   buf = malloc(w * 3);

   al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_READONLY);

   for (y = 0; y < h; y++) {    /* for each scanline... */
      for (x = 0; x < w; x++) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, x, y);
         unsigned char r, g, b;
         al_unmap_rgb(c, &r, &g, &b);
         buf[x] = r;
         buf[x + w] = g;
         buf[x + w * 2] = b;
      }

      for (i = 0; i < 3; i++) {
         int color;
         int count;
         x = 0;
         for (;;) {
            count = 0;
            color = buf[x + w * i];
            do {
               count++;
               x++;
            } while ((count < 63) && (x < w) && (color == buf[x + w * i]));
            al_fs_entry_putc(count | 0xC0, f);
            al_fs_entry_putc(color, f);
            if (x >= w)
               break;
         }
      }
   }

   free(buf);

   al_unlock_bitmap(bmp);

   if (al_get_errno())
      return -1;
   else
      return 0;
}


/* Function: iio_load_bmp
 * Create a new ALLEGRO_BITMAP from a PCX file. The bitmap is created with
 * <al_create_bitmap>.
 * See Also: <al_iio_load>.
 */
ALLEGRO_BITMAP *iio_load_pcx(AL_CONST char *filename)
{
   ALLEGRO_FS_ENTRY *f;
   ALLEGRO_BITMAP *bmp;
   ASSERT(filename);

   f = al_fs_entry_open(filename, "rb");
   if (!f)
      return NULL;

   bmp = iio_load_pcx_pf(f);

   al_fs_entry_close(f);

   return bmp;
}


/* Function: iio_save_bmp
 * Save an ALLEGRO_BITMAP as a PCX file. 
 * See Also: <al_iio_save>.
 */
int iio_save_pcx(AL_CONST char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FS_ENTRY *f;
   int ret;
   ASSERT(filename);

   f = al_fs_entry_open(filename, "wb");
   if (!f)
      return -1;

   ret = iio_save_pcx_pf(f, bmp);

   al_fs_entry_close(f);

   return ret;
}
