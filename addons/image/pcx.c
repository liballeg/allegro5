#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

ALLEGRO_BITMAP *_al_load_pcx_f(ALLEGRO_FILE *f)
{
   ALLEGRO_BITMAP *b;
   int c;
   int width, height;
   int bpp, bytes_per_line;
   int x, xx, y;
   char ch;
   ALLEGRO_LOCKED_REGION *lr;
   unsigned char *buf;
   PalEntry pal[256];
   ASSERT(f);

   al_fgetc(f);                    /* skip manufacturer ID */
   al_fgetc(f);                    /* skip version flag */
   al_fgetc(f);                    /* skip encoding flag */

   if (al_fgetc(f) != 8) {         /* we like 8 bit color planes */
      return NULL;
   }

   width = -(al_fread16le(f));        /* xmin */
   height = -(al_fread16le(f));       /* ymin */
   width += al_fread16le(f) + 1;      /* xmax */
   height += al_fread16le(f) + 1;     /* ymax */

   al_fread32le(f);		   /* skip DPI values */

   for (c = 0; c < 16 * 3; c++) {          /* skip the 16 color palette */
      al_fgetc(f);
   }

   al_fgetc(f);

   bpp = al_fgetc(f) * 8;          /* how many color planes? */

   if ((bpp != 8) && (bpp != 24)) {
      return NULL;
   }

   bytes_per_line = al_fread16le(f);

   for (c = 0; c < 60; c++)                /* skip some more junk */
      al_fgetc(f);

   if (al_feof(f) || al_ferror(f)) {
      return NULL;
   }

   b = al_create_bitmap(width, height);
   if (!b) {
      return NULL;
   }

   al_set_errno(0);

   if (bpp == 8) {
      /* The palette comes after the image data.  We need to to keep the
       * whole image in a temporary buffer before mapping the final colours.
       */
      buf = (unsigned char *)al_malloc(bytes_per_line * height);
   }
   else {
      /* We can read one line at a time. */
      buf = (unsigned char *)al_malloc(bytes_per_line * 3);
   }

   lr = al_lock_bitmap(b, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
   if (!lr) {
      al_free(buf);
      return NULL;
   }

   xx = 0;                      /* index into buf, only for bpp = 8 */

   for (y = 0; y < height; y++) {       /* read RLE encoded PCX data */

      x = 0;

      while (x < bytes_per_line * bpp / 8) {
         ch = al_fgetc(f);
         if ((ch & 0xC0) == 0xC0) { /* a run */
            c = (ch & 0x3F);
            ch = al_fgetc(f);
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
         char *dest = (char*)lr->data + y*lr->pitch;
         for (x = 0; x < width; x++) {
            dest[x*4    ] = buf[x];
            dest[x*4 + 1] = buf[x + width];
            dest[x*4 + 2] = buf[x + width * 2];
            dest[x*4 + 3] = 255;
         }
      }
   }

   if (bpp == 8) {               /* look for a 256 color palette */
      while ((c = al_fgetc(f)) != EOF) {
         if (c == 12) {
            for (c = 0; c < 256; c++) {
               pal[c].r = al_fgetc(f);
               pal[c].g = al_fgetc(f);
               pal[c].b = al_fgetc(f);
            }
            break;
         }
      }
      for (y = 0; y < height; y++) {
         char *dest = (char*)lr->data + y*lr->pitch;
         for (x = 0; x < width; x++) {
            int index = buf[y * width + x];
            dest[x*4    ] = pal[index].r;
            dest[x*4 + 1] = pal[index].g;
            dest[x*4 + 2] = pal[index].b;
            dest[x*4 + 3] = 255;
         }
      }
   }

   al_unlock_bitmap(b);

   al_free(buf);

   if (al_get_errno()) {
      al_destroy_bitmap(b);
      return NULL;
   }

   return b;
}

/* Function: al_save_pcx_f
 */
bool _al_save_pcx_f(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   int c;
   int x, y;
   int i;
   int w, h;
   unsigned char *buf;
   ASSERT(f);
   ASSERT(bmp);

   al_set_errno(0);

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);

   al_fputc(f, 10);     /* manufacturer */
   al_fputc(f, 5);      /* version */
   al_fputc(f, 1);      /* run length encoding  */
   al_fputc(f, 8);      /* 8 bits per pixel */
   al_fwrite16le(f, 0);     /* xmin */
   al_fwrite16le(f, 0);     /* ymin */
   al_fwrite16le(f, w - 1); /* xmax */
   al_fwrite16le(f, h - 1); /* ymax */
   al_fwrite16le(f, 320);   /* HDpi */
   al_fwrite16le(f, 200);   /* VDpi */

   for (c = 0; c < 16 * 3; c++) {
      al_fputc(f, 0);
   }

   al_fputc(f, 0);      /* reserved */
   al_fputc(f, 3);      /* color planes */
   al_fwrite16le(f, w);     /* number of bytes per scanline */
   al_fwrite16le(f, 1);     /* color palette */
   al_fwrite16le(f, w);     /* hscreen size */
   al_fwrite16le(f, h);     /* vscreen size */
   for (c = 0; c < 54; c++)     /* filler */
      al_fputc(f, 0);

   buf = al_malloc(w * 3);

   al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

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
            al_fputc(f, count | 0xC0);
            al_fputc(f, color);
            if (x >= w)
               break;
         }
      }
   }

   al_free(buf);

   al_unlock_bitmap(bmp);

   if (al_get_errno())
      return false;
   else
      return true;
}

ALLEGRO_BITMAP *_al_load_pcx(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_BITMAP *bmp;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   bmp = _al_load_pcx_f(f);

   al_fclose(f);

   return bmp;
}

bool _al_save_pcx(const char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FILE *f;
   bool ret;
   ASSERT(filename);

   f = al_fopen(filename, "wb");
   if (!f)
      return false;

   ret = _al_save_pcx_f(f, bmp);

   al_fclose(f);

   return ret;
}


/* vim: set sts=3 sw=3 et: */
