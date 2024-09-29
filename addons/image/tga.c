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

A5O_DEBUG_CHANNEL("image")


/* raw_tga_read8:
 *  Helper for reading 256-color raw data from TGA files.
 *  Returns pointer past the end.
 */
static INLINE unsigned char *raw_tga_read8(unsigned char *b, int w, A5O_FILE *f)
{
   return b + al_fread(f, b, w);
}



/* rle_tga_read8:
 *  Helper for reading 256-color RLE data from TGA files.
 *  Returns pointer past the end or NULL for error.
 */
static unsigned char *rle_tga_read8(unsigned char *b, int w, A5O_FILE *f)
{
   int value, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         value = al_fgetc(f);
         while (count--)
            *b++ = value;
      }
      else {
         /* raw packet */
         count++;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         b = raw_tga_read8(b, count, f);
      }
   } while (c < w);
   return b;
}



/* single_tga_read32:
 *  Helper for reading a single 32-bit data from TGA files.
 */
static INLINE int32_t single_tga_read32(A5O_FILE *f)
{
   return al_fread32le(f);
}



/* raw_tga_read32:
 *  Helper for reading 32-bit raw data from TGA files.
 *  Returns pointer past the end.
 */
static unsigned int *raw_tga_read32(unsigned int *b, int w, A5O_FILE *f)
{
   while (w--)
      *b++ = single_tga_read32(f);

   return b;
}



/* rle_tga_read32:
 *  Helper for reading 32-bit RLE data from TGA files.
 *  Returns pointer past the end or NULL for error.
 */
static unsigned int *rle_tga_read32(unsigned int *b, int w, A5O_FILE *f)
{
   int color, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         color = single_tga_read32(f);
         while (count--)
            *b++ = color;
      }
      else {
         /* raw packet */
         count++;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         b = raw_tga_read32(b, count, f);
      }
   } while (c < w);
   return b;
}


/* single_tga_read24:
 *  Helper for reading a single 24-bit data from TGA files.
 */
static INLINE void single_tga_read24(A5O_FILE *f, unsigned char color[3])
{
   al_fread(f, color, 3);
}



/* raw_tga_read24:
 *  Helper for reading 24-bit raw data from TGA files.
 *  Returns pointer past the end.
 */
static unsigned char *raw_tga_read24(unsigned char *b, int w, A5O_FILE *f)
{
   while (w--) {
      single_tga_read24(f, b);
      b += 3;
   }

   return b;
}



/* rle_tga_read24:
 *  Helper for reading 24-bit RLE data from TGA files.
 *  Returns pointer past the end or NULL for error.
 */
static unsigned char *rle_tga_read24(unsigned char *b, int w, A5O_FILE *f)
{
   int count, c = 0;
   unsigned char color[3];

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
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
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         b = raw_tga_read24(b, count, f);
      }
   } while (c < w);
   return b;
}



/* single_tga_read16:
 *  Helper for reading a single 16-bit data from TGA files.
 */
static INLINE int single_tga_read16(A5O_FILE *f)
{
   return al_fread16le(f);
}



/* raw_tga_read16:
 *  Helper for reading 16-bit raw data from TGA files.
 *  Returns pointer past the end.
 */
static unsigned short *raw_tga_read16(unsigned short *b, int w, A5O_FILE *f)
{
   while (w--)
      *b++ = single_tga_read16(f);

   return b;
}



/* rle_tga_read16:
 *  Helper for reading 16-bit RLE data from TGA files.
 *  Returns pointer past the end or NULL for error.
 */
static unsigned short *rle_tga_read16(unsigned short *b, int w, A5O_FILE *f)
{
   int color, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         color = single_tga_read16(f);
         while (count--)
            *b++ = color;
      }
      else {
         /* raw packet */
         count++;
         c += count;
         if (c > w) {
            /* Stepped past the end of the line, error */
            return NULL;
         }
         b = raw_tga_read16(b, count, f);
      }
   } while (c < w);
   return b;
}

typedef unsigned char palette_entry[3];

/* Like load_tga, but starts loading from the current place in the A5O_FILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
A5O_BITMAP *_al_load_tga_f(A5O_FILE *f, int flags)
{
   unsigned char image_id[256];
   palette_entry image_palette[256];
   unsigned char id_length, palette_type, image_type, palette_entry_size;
   unsigned char bpp, descriptor_bits;
   short unsigned int palette_colors;
   short unsigned int palette_start;
   short unsigned int image_width, image_height;
   bool left_to_right;
   bool top_to_bottom;
   unsigned int c, i;
   int y;
   int compressed;
   A5O_BITMAP *bmp;
   A5O_LOCKED_REGION *lr;
   unsigned char *buf;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);
   ASSERT(f);

   id_length = al_fgetc(f);
   palette_type = al_fgetc(f);
   image_type = al_fgetc(f);
   palette_start = al_fread16le(f); /* first_color */
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
      A5O_ERROR("Invalid image type %d.\n", image_type);
      return NULL;
   }

   switch (image_type) {

      case 1:
         /* paletted image */
         /* Only support 8 bit palettes (up to 256 entries) though in principle the file format could have more(?)*/
         if ((palette_type != 1) || (bpp != 8) || (palette_start + palette_colors) > 256) {
            A5O_ERROR("Invalid image/palette/bpp combination %d/%d/%d.\n", image_type, palette_type, bpp);
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
            A5O_ERROR("Invalid image/palette/bpp combination %d/%d/%d.\n", image_type, palette_type, bpp);
            return NULL;
         }
         break;

      case 3:
         /* grayscale image */
         if ((palette_type != 0) || (bpp != 8)) {
            A5O_ERROR("Invalid image/palette/bpp combination %d/%d/%d.\n", image_type, palette_type, bpp);
            return NULL;
         }

         for (i=0; i<256; i++) {
            image_palette[i][0] = i;
            image_palette[i][1] = i;
            image_palette[i][2] = i;
         }
         break;

      default:
         A5O_ERROR("Invalid image type %d.\n", image_type);
         return NULL;
   }

   if (palette_type == 0) {
      /* No-op */
   } else if (palette_type == 1) {
      for (i = 0; i < palette_colors; i++) {
         palette_entry *entry = image_palette + palette_start + i;
         switch (palette_entry_size) {
            case 16:
               c = al_fread16le(f);
               (*entry)[0] = (c & 0x1F) << 3;
               (*entry)[1] = ((c >> 5) & 0x1F) << 3;
               (*entry)[2] = ((c >> 10) & 0x1F) << 3;
               break;
            case 24:
               (*entry)[0] = al_fgetc(f);
               (*entry)[1] = al_fgetc(f);
               (*entry)[2] = al_fgetc(f);
               break;
            case 32:
               (*entry)[0] = al_fgetc(f);
               (*entry)[1] = al_fgetc(f);
               (*entry)[2] = al_fgetc(f);
               al_fgetc(f); /* Ignore 4th byte (alpha) */
               break;
         }
      }
   }
   else  {
      A5O_ERROR("Invalid palette type %d.\n", palette_type);
      return NULL;
   }


   bmp = al_create_bitmap(image_width, image_height);
   if (!bmp) {
      A5O_ERROR("Failed to create bitmap.\n");
      return NULL;
   }

   al_set_errno(0);

   lr = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE, A5O_LOCK_WRITEONLY);
   if (!lr) {
      A5O_ERROR("Failed to lock bitmap.\n");
      al_destroy_bitmap(bmp);
      return NULL;
   }

   /* bpp + 1 accounts for 15 bpp. */
   buf = al_malloc(image_width * ((bpp + 1) / 8));
   if (!buf) {
      al_unlock_bitmap(bmp);
      al_destroy_bitmap(bmp);
      A5O_ERROR("Failed to allocate enough memory.\n");
      return NULL;
   }

   for (y = 0; y < image_height; y++) {
      int true_y = (top_to_bottom) ? y : (image_height - 1 - y);

      switch (image_type) {

         case 1:
         case 3: {
            unsigned char *ptr;
            if (compressed)
               ptr = rle_tga_read8(buf, image_width, f);
            else
               ptr = raw_tga_read8(buf, image_width, f);
            if (!ptr) {
               al_free(buf);
               al_unlock_bitmap(bmp);
               al_destroy_bitmap(bmp);
               A5O_ERROR("Invalid image data.\n");
               return NULL;
            }
         }
            for (i = 0; i < image_width; i++) {
               int true_x = (left_to_right) ? i : (image_width - 1 - i);
               int pix = buf[i];

               unsigned char *dest = (unsigned char *)lr->data +
                  lr->pitch*true_y + true_x*4;
               if (pix < palette_start || pix >= (palette_start + palette_colors)) {
                  al_free(buf);
                  al_unlock_bitmap(bmp);
                  al_destroy_bitmap(bmp);
                  A5O_ERROR("Invalid image data.\n");
                  return NULL;
               }
               palette_entry* entry = image_palette + pix;
               dest[0] = (*entry)[2];
               dest[1] = (*entry)[1];
               dest[2] = (*entry)[0];
               dest[3] = 255;
            }

            break;

         case 2:
            if (bpp == 32) {
               unsigned int *ptr;
               if (compressed)
                  ptr = rle_tga_read32((unsigned int *)buf, image_width, f);
               else
                  ptr = raw_tga_read32((unsigned int *)buf, image_width, f);
               if (!ptr) {
                  al_free(buf);
                  al_unlock_bitmap(bmp);
                  al_destroy_bitmap(bmp);
                  A5O_ERROR("Invalid image data.\n");
                  return NULL;
               }
               for (i = 0; i < image_width; i++) {
                  int true_x = (left_to_right) ? i : (image_width - 1 - i);
                  unsigned char *dest = (unsigned char *)lr->data +
                     lr->pitch*true_y + true_x*4;

#ifdef A5O_BIG_ENDIAN
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
               unsigned char *ptr;
               if (compressed)
                  ptr = rle_tga_read24(buf, image_width, f);
               else
                  ptr = raw_tga_read24(buf, image_width, f);
               if (!ptr) {
                  al_free(buf);
                  al_unlock_bitmap(bmp);
                  al_destroy_bitmap(bmp);
                  A5O_ERROR("Invalid image data.\n");
                  return NULL;
               }
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
               unsigned short *ptr;
               if (compressed)
                  ptr = rle_tga_read16((unsigned short *)buf, image_width, f);
               else
                  ptr = raw_tga_read16((unsigned short *)buf, image_width, f);
               if (!ptr) {
                  al_free(buf);
                  al_unlock_bitmap(bmp);
                  al_destroy_bitmap(bmp);
                  A5O_ERROR("Invalid image data.\n");
                  return NULL;
               }
               for (i = 0; i < image_width; i++) {
                  int true_x = (left_to_right) ? i : (image_width - 1 - i);
                  int pix = *((unsigned short *)(buf + i * 2));
                  /* TODO - do something with the 1-bit A value (alpha?) */
                  int r = _al_rgb_scale_5[(pix >> 10) & 0x1F];
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
      A5O_ERROR("Error detected: %d.\n", al_get_errno());
      al_destroy_bitmap(bmp);
      return NULL;
   }

   return bmp;
}



/* Like save_tga but writes into the A5O_FILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
bool _al_save_tga_f(A5O_FILE *f, A5O_BITMAP *bmp)
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

   al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ANY, A5O_LOCK_READONLY);

   for (y = h - 1; y >= 0; y--) {
      for (x = 0; x < w; x++) {
         A5O_COLOR c = al_get_pixel(bmp, x, y);
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


A5O_BITMAP *_al_load_tga(const char *filename, int flags)
{
   A5O_FILE *f;
   A5O_BITMAP *bmp;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_load_tga_f(f, flags);

   al_fclose(f);

   return bmp;
}



bool _al_save_tga(const char *filename, A5O_BITMAP *bmp)
{
   A5O_FILE *f;
   bool retsave;
   bool retclose;
   ASSERT(filename);

   f = al_fopen(filename, "wb");
   if (!f) {
      A5O_ERROR("Unable to open %s for writing.\n", filename);
      return false;
   }

   retsave = _al_save_tga_f(f, bmp);

   retclose = al_fclose(f);

   return retsave && retclose;
}


bool _al_identify_tga(A5O_FILE *f)
{
   uint8_t x[4];
   al_fgetc(f); // skip id length
   al_fread(f, x, 4);

   if (x[0] > 1) // TGA colormap must be 0 or 1
      return false;
   if ((x[1] & 0xf7) == 0) // type must be 1, 2, 3, 9, 10 or 11
      return false;
   if (x[2] != 0 || x[3] != 0) // color map must start at 0
      return false;
   return true;
}


/* vim: set sts=3 sw=3 et: */
