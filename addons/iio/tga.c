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


#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"
#include "allegro5/a5_iio.h"

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
static INLINE int single_tga_read32(ALLEGRO_FILE *f)
{
   PalEntry value;
   int alpha;

   value.b = al_fgetc(f);
   value.g = al_fgetc(f);
   value.r = al_fgetc(f);
   alpha = al_fgetc(f);

#ifdef ALLEGRO_LITTLE_ENDIAN
   return (alpha << 24) | (value.r << 16) | (value.g << 8) | value.b;
#elif defined ALLEGRO_BIG_ENDIAN
   return (value.b << 24) | (value.g << 16) | (value.r << 8) | alpha;
#else
#error "Endianesse not defined!"
#endif
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
static INLINE int single_tga_read24(ALLEGRO_FILE *f)
{
   PalEntry value;

   value.b = al_fgetc(f);
   value.g = al_fgetc(f);
   value.r = al_fgetc(f);

#ifdef ALLEGRO_LITTLE_ENDIAN
   return (value.r << 16) | (value.g << 8) | value.b;
#elif defined ALLEGRO_BIG_ENDIAN
   return (value.b << 16) | (value.g << 8) | value.r;
#else
#error "Endianesse not defined!"
#endif
}



/* raw_tga_read24:
 *  Helper for reading 24-bit raw data from TGA files.
 */
static unsigned char *raw_tga_read24(unsigned char *b, int w, ALLEGRO_FILE *f)
{
   int color;

   while (w--) {
      color = single_tga_read24(f);
      WRITE3BYTES(b, color);
      b += 3;
   }

   return b;
}



/* rle_tga_read24:
 *  Helper for reading 24-bit RLE data from TGA files.
 */
static void rle_tga_read24(unsigned char *b, int w, ALLEGRO_FILE *f)
{
   int color, count, c = 0;

   do {
      count = al_fgetc(f);
      if (count & 0x80) {
         /* run-length packet */
         count = (count & 0x7F) + 1;
         c += count;
         color = single_tga_read24(f);
         while (count--) {
            WRITE3BYTES(b, color);
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
   int value;

   value = al_fread16le(f);

   return value;
   /*
      (((value >> 10) & 0x1F) << _rgb_r_shift_15) |
      (((value >> 5) & 0x1F) << _rgb_g_shift_15)  |
      ((value & 0x1F) << _rgb_b_shift_15);
    */
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


/* al_load_tga_entry:
 *  Like load_tga, but starts loading from the current place in the ALLEGRO_FILE
 *  specified. If successful the offset into the file will be left just after
 *  the image data. If unsuccessful the offset into the file is unspecified,
 *  i.e. you must either reset the offset to some known place or close the
 *  packfile. The packfile is not closed by this function.
 */
ALLEGRO_BITMAP *al_load_tga_entry(ALLEGRO_FILE *f)
{
   unsigned char image_id[256], image_palette[256][3];
   unsigned char id_length, palette_type, image_type, palette_entry_size;
   unsigned char bpp, descriptor_bits;
   short unsigned int first_color, palette_colors;
   short unsigned int left, top, image_width, image_height;
   bool left_to_right;
   bool top_to_bottom;
   unsigned int c, i;
   int y;
   int compressed;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_LOCKED_REGION *lr;
   ALLEGRO_STATE backup;
   unsigned char *buf;
   ASSERT(f);

   id_length = al_fgetc(f);
   palette_type = al_fgetc(f);
   image_type = al_fgetc(f);
   first_color = al_fread16le(f);
   palette_colors  = al_fread16le(f);
   palette_entry_size = al_fgetc(f);
   left = al_fread16le(f);
   top = al_fread16le(f);
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

         break;

      default:
         return NULL;
   }

   bmp = al_create_bitmap(image_width, image_height);
   if (!bmp) {
      return NULL;
   }

   al_set_errno(0);

   lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
   buf = malloc(image_width * ((bpp + 1 / 8)));

   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(bmp);

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
               ALLEGRO_COLOR color = al_map_rgb(image_palette[pix][2],
                                                image_palette[pix][1],
                                                image_palette[pix][0]);
               al_put_pixel(true_x, true_y, color);
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
                  int b = buf[i * 4 + 0];
                  int g = buf[i * 4 + 1];
                  int r = buf[i * 4 + 2];
                  int a = buf[i * 4 + 3];
                  ALLEGRO_COLOR color = al_map_rgba(r, g, b, a);
                  al_put_pixel(true_x, true_y, color);
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
                  ALLEGRO_COLOR color = al_map_rgb(r, g, b);
                  al_put_pixel(true_x, true_y, color);
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
                  int r = _rgb_scale_5[(pix >> 10)];
                  int g = _rgb_scale_5[(pix >> 5) & 0x1F];
                  int b = _rgb_scale_5[(pix & 0x1F)];
                  ALLEGRO_COLOR color = al_map_rgb(r, g, b);
                  al_put_pixel(true_x, true_y, color);
               }
            }
            break;
      }
   }

   free(buf);
   al_unlock_bitmap(bmp);
   al_restore_state(&backup);

   if (al_get_errno()) {
      al_destroy_bitmap(bmp);
      return NULL;
   }

   return bmp;
}



/* al_save_tga_entry:
 *  Like save_tga but writes into the ALLEGRO_FILE given instead of a new file.
 *  The packfile is not closed after writing is completed. On success the
 *  offset into the file is left after the TGA file just written. On failure
 *  the offset is left at the end of whatever incomplete data was written.
 */
int al_save_tga_entry(ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp)
{
   int x, y;
   int w, h;
   ALLEGRO_LOCKED_REGION *lr;
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

   lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

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

   if (al_get_errno())
      return -1;
   else
      return 0;
}


/* Function: al_load_tga
 */
ALLEGRO_BITMAP *al_load_tga(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_BITMAP *bmp;
   ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   bmp = al_load_tga_entry(f);

   al_fclose(f);

   return bmp;
}



/* Function: al_save_tga
 */
int al_save_tga(const char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FILE *f;
   int ret;
   ASSERT(filename);

   f = al_fopen(filename, "wb");
   if (!f)
      return -1;

   ret = al_save_tga_entry(f, bmp);

   al_fclose(f);

   return ret;
}
