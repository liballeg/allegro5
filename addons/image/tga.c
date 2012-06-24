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
 *      TGA reader.
 *
 *      By Tim Gunn.
 *
 *      RLE support added by Michal Mertl and Salvador Eduardo Tropea.
 * 
 *      Palette reading improved by Peter Wang.
 *
 *      Big-endian support added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"
#include "allegro5/internal/aintern_pixels.h"

#include "iio.h"



/* raw_tga_read8:
 *  Helper for reading 256-color raw data from TGA files.
 */
static INLINE unsigned char *raw_tga_read8(unsigned char *b, int w, ALLEGRO_FILE *f)
{
   return b + al_fread(f, b, w);
}



/* rle_tga_read8:
 *  Helper for reading 256-color RLE data from TGA files.
 */
static void rle_tga_read8(unsigned char *b, int w, ALLEGRO_FILE *f)
{
   int value, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         value = al_fgetc(f);
         while (count--)
            *b++ = value;
      }
      else {
         /* raw packet */
         count++;
         c += count;
         b = raw_tga_read8(b, count, f);
      }
   } while (c < w);
}



/* single_tga_read32:
 *  Helper for reading a single 32-bit data from TGA files.
 */
static INLINE int32_t single_tga_read32(ALLEGRO_FILE *f)
{
   return al_fread32le(f);
}



/* raw_tga_read32:
 *  Helper for reading 32-bit raw data from TGA files.
 */
static unsigned int *raw_tga_read32(unsigned int *b, int w, ALLEGRO_FILE *f)
{
   while (w--)
      *b++ = single_tga_read32(f);

   return b;
}



/* rle_tga_read32:
 *  Helper for reading 32-bit RLE data from TGA files.
 */
static void rle_tga_read32(unsigned int *b, int w, ALLEGRO_FILE *f)
{
   int color, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         color = single_tga_read32(f);
         while (count--)
            *b++ = color;
      }
      else {
         /* raw packet */
         count++;
         c += count;
         b = raw_tga_read32(b, count, f);
      }
   } while (c < w);
}


/* single_tga_read24:
 *  Helper for reading a single 24-bit data from TGA files.
 */
static INLINE void single_tga_read24(ALLEGRO_FILE *f, unsigned char color[3])
{
   al_fread(f, color, 3);
}



/* raw_tga_read24:
 *  Helper for reading 24-bit raw data from TGA files.
 */
static unsigned char *raw_tga_read24(unsigned char *b, int w, ALLEGRO_FILE *f)
{
   while (w--) {
      single_tga_read24(f, b);
      b += 3;
   }

   return b;
}



/* rle_tga_read24:
 *  Helper for reading 24-bit RLE data from TGA files.
 */
static void rle_tga_read24(unsigned char *b, int w, ALLEGRO_FILE *f)
{
   int count, c = 0;
   unsigned char color[3];

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         single_tga_read24(f, color);
         while (count--) {
            b[0] = color[0];
            b[1] = color[1];
            b[2] = color[2];
            b += 3;
         }
      }
      else {
         /* raw packet */
         count++;
         c += count;
         b = raw_tga_read24(b, count, f);
      }
   } while (c < w);
}



/* single_tga_read16:
 *  Helper for reading a single 16-bit data from TGA files.
 */
static INLINE int single_tga_read16(ALLEGRO_FILE *f)
{
   return al_fread16le(f);
}



/* raw_tga_read16:
 *  Helper for reading 16-bit raw data from TGA files.
 */
static unsigned short *raw_tga_read16(unsigned short *b, int w, ALLEGRO_FILE *f)
{
   while (w--)
      *b++ = single_tga_read16(f);

   return b;
}



/* rle_tga_read16:
 *  Helper for reading 16-bit RLE data from TGA files.
 */
static void rle_tga_read16(unsigned short *b, int w, ALLEGRO_FILE *f)
{
   int color, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         color = single_tga_read16(f);
         while (count--)
            *b++ = color;
      }
      else {
         /* raw packet */
         count++;
         c += count;
         b = raw_tga_read16(b, count, f);
      }
   } while (c < w);
}


/* Like load_tga, but starts loading from the current place in the ALLEGRO_FILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
ALLEGRO_BITMAP *_al_load_tga_f(ALLEGRO_FILE *f)
{
   unsigned char image_id[256], image_palette[256][3];
   unsigned char id_length, palette_type, image_type, palette_entry_size;
   unsigned char bpp, descriptor_bits;
   short unsigned int palette_colors;
   short unsigned int image_width, image_height;
   bool left_to_right;
   bool top_to_bottom;
   unsigned int c, i;
   int y;
   int compressed;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_LOCKED_REGION *lr;
   unsigned char *buf;
   bool premul = !(al_get_new_bitmap_flags() & ALLEGRO_NO_PREMULTIPLIED_ALPHA);
   ASSERT(f);

   id_length = al_fgetc(f);
   palette_type = al_fgetc(f);
   image_type = al_fgetc(f);
   al_fread16le(f); /* first_color */
   palette_colors  = al_fread16le(f);
   palette_entry_size = al_fgetc(f);
   al_fread16le(f); /* left */
   al_fread16le(f); /* top */
   image_width = al_fread16le(f);
   image_height = al_fread16le(f);
   bpp = al_fgetc(f);
   descriptor_bits = al_fgetc(f);

   left_to_right = !(descriptor_bits & (1 << 4));
   top_to_bottom = (descriptor_bits & (1 << 5));

   al_fread(f, image_id, id_length);

   if (palette_type == 1) {

      for (i = 0; i < palette_colors; i++) {

         switch (palette_entry_size) {

            case 16:
               c = al_fread16le(f);
               image_palette[i][0] = (c & 0x1F) << 3;
               image_palette[i][1] = ((c >> 5) & 0x1F) << 3;
               image_palette[i][2] = ((c >> 10) & 0x1F) << 3;
               break;

            case 24:
            case 32:
               image_palette[i][0] = al_fgetc(f);
               image_palette[i][1] = al_fgetc(f);
               image_palette[i][2] = al_fgetc(f);
               if (palette_entry_size == 32)
                  al_fgetc(f);
               break;
         }
      }
   }
   else if (palette_type != 0) {
      return NULL;
   }

   /* Image type:
    *    0 = no image data
    *    1 = uncompressed color mapped
    *    2 = uncompressed true color
    *    3 = grayscale
    *    9 = RLE color mapped
    *   10 = RLE true color
    *   11 = RLE grayscale
    */
   compressed = (image_type & 8);
   image_type &= 7;

   if ((image_type < 1) || (image_type > 3)) {
      return NULL;
   }

   switch (image_type) {

      case 1:
         /* paletted image */
         if ((palette_type != 1) || (bpp != 8)) {
            return NULL;
         }

         break;

      case 2:
         /* truecolor image */
         if ((palette_type == 0) && ((bpp == 15) || (bpp == 16))) {
            bpp = 15;
         }
         else if ((palette_type == 0) && ((bpp == 24) || (bpp == 32))) {
         }
         else {
            return NULL;
         }
         break;

      case 3:
         /* grayscale image */
         if ((palette_type != 0) || (bpp != 8)) {
            return NULL;
         }

         for (i=0; i<256; i++) {
            image_palette[i][0] = i;
            image_palette[i][1] = i;
            image_palette[i][2] = i;
         }
         break;

      default:
         return NULL;
   }

   bmp = al_create_bitmap(image_width, image_height);
   if (!bmp) {
      return NULL;
   }

   al_set_errno(0);

   lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
   if (!lr) {
      al_destroy_bitmap(bmp);
      return NULL;
   }

   /* bpp + 1 accounts for 15 bpp. */
   buf = al_malloc(image_width * ((bpp + 1) / 8));
   if (!buf) {
      al_unlock_bitmap(bmp);
      al_destroy_bitmap(bmp);
      return NULL;
   }

   for (y = 0; y < image_height; y++) {
      int true_y = (top_to_bottom) ? y : (image_height - 1 - y);

      switch (image_type) {

         case 1:
         case 3:
            if (compressed)
               rle_tga_read8(buf, image_width, f);
            else
               raw_tga_read8(buf, image_width, f);

            for (i = 0; i < image_width; i++) {
               int true_x = (left_to_right) ? i : (image_width - 1 - i);
               int pix = buf[i];

               unsigned char *dest = (unsigned char *)lr->data +
                  lr->pitch*true_y + true_x*4;
               dest[0] = image_palette[pix][2];
               dest[1] = image_palette[pix][1];
               dest[2] = image_palette[pix][0];
               dest[3] = 255;
            }

            break;

         case 2:
            if (bpp == 32) {
               if (compressed)
                  rle_tga_read32((unsigned int *)buf, image_width, f);
               else
                  raw_tga_read32((unsigned int *)buf, image_width, f);

               for (i = 0; i < image_width; i++) {
                  int true_x = (left_to_right) ? i : (image_width - 1 - i);
                  unsigned char *dest = (unsigned char *)lr->data +
                     lr->pitch*true_y + true_x*4;

#ifdef ALLEGRO_BIG_ENDIAN
                  int a = buf[i * 4 + 0];
                  int r = buf[i * 4 + 1];
                  int g = buf[i * 4 + 2];
                  int b = buf[i * 4 + 3];
#else					 
                  int b = buf[i * 4 + 0];
                  int g = buf[i * 4 + 1];
                  int r = buf[i * 4 + 2];
                  int a = buf[i * 4 + 3];
#endif                  
                  if (premul) {
                     r = r * a / 255;
                     g = g * a / 255;
                     b = b * a / 255;
                  }

                  dest[0] = r;
                  dest[1] = g;
                  dest[2] = b;
                  dest[3] = a;
               }
            }
            else if (bpp == 24) {
               if (compressed)
                  rle_tga_read24(buf, image_width, f);
               else
                  raw_tga_read24(buf, image_width, f);
               for (i = 0; i < image_width; i++) {
                  int true_x = (left_to_right) ? i : (image_width - 1 - i);
                  int b = buf[i * 3 + 0];
                  int g = buf[i * 3 + 1];
                  int r = buf[i * 3 + 2];

                  unsigned char *dest = (unsigned char *)lr->data +
                     lr->pitch*true_y + true_x*4;
                  dest[0] = r;
                  dest[1] = g;
                  dest[2] = b;
                  dest[3] = 255;
               }
            }
            else {
               if (compressed)
                  rle_tga_read16((unsigned short *)buf, image_width, f);
               else
                  raw_tga_read16((unsigned short *)buf, image_width, f);
               for (i = 0; i < image_width; i++) {
                  int true_x = (left_to_right) ? i : (image_width - 1 - i);
                  int pix = *((unsigned short *)(buf + i * 2));
                  int r = _al_rgb_scale_5[(pix >> 10)];
                  int g = _al_rgb_scale_5[(pix >> 5) & 0x1F];
                  int b = _al_rgb_scale_5[(pix & 0x1F)];

                  unsigned char *dest = (unsigned char *)lr->data +
                     lr->pitch*true_y + true_x*4;
                  dest[0] = r;
                  dest[1] = g;
                  dest[2] = b;
                  dest[3] = 255;
               }
            }
            break;
      }
   }

   al_free(buf);
   al_unlock_bitmap(bmp);

   if (al_get_errno()) {
      al_destroy_bitmap(bmp);
      return NULL;
   }

   return bmp;
}



/* Like save_tga but writes into the ALLEGRO_FILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
bool _al_save_tga_f(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   int x, y;
   int w, h;
   ASSERT(f);
   ASSERT(bmp);

   al_set_errno(0);

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);

   al_fputc(f, 0);      /* id length (no id saved) */
   al_fputc(f, 0);      /* palette type */
   al_fputc(f, 2);      /* image type */
   al_fwrite16le(f, 0);     /* first colour */
   al_fwrite16le(f, 0);     /* number of colours */
   al_fputc(f, 0);      /* palette entry size */
   al_fwrite16le(f, 0);     /* left */
   al_fwrite16le(f, 0);     /* top */
   al_fwrite16le(f, w);     /* width */
   al_fwrite16le(f, h);     /* height */
   al_fputc(f, 32);     /* bits per pixel */
   al_fputc(f, 8);      /* descriptor (bottom to top, 8-bit alpha) */

   al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

   for (y = h - 1; y >= 0; y--) {
      for (x = 0; x < w; x++) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, x, y);
         unsigned char r, g, b, a;
         al_unmap_rgba(c, &r, &g, &b, &a);
         al_fputc(f, b);
         al_fputc(f, g);
         al_fputc(f, r);
         al_fputc(f, a);
      }
   }

   al_unlock_bitmap(bmp);

   return al_get_errno() ? false : true;
}


ALLEGRO_BITMAP *_al_load_tga(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_BITMAP *bmp;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   bmp = _al_load_tga_f(f);

   al_fclose(f);

   return bmp;
}



bool _al_save_tga(const char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FILE *f;
   bool ret;
   ASSERT(filename);

   f = al_fopen(filename, "wb");
   if (!f)
      return false;

   ret = _al_save_tga_f(f, bmp);

   al_fclose(f);

   return ret;
}


/* vim: set sts=3 sw=3 et: */
