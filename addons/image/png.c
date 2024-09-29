/* loadpng, Allegro wrapper routines for libpng
 * by Peter Wang (tjaden@users.sf.net).
 */


#include <png.h>
#include <zlib.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

A5O_DEBUG_CHANNEL("image")


/* get_gamma:
 *  Get screen gamma value one of three ways.
 */
static double get_gamma(void)
{
   double gamma = -1.0;
   const char* config = al_get_config_value(al_get_system_config(), "image", "png_screen_gamma");
   if (config) {
      gamma = strtod(config, NULL);
   }
   if (gamma == -1.0) {
      /* Use the environment variable if available.
       * 2.2 is a good guess for PC monitors.
       * 1.1 is good for my laptop.
       */
      const char *gamma_str = getenv("SCREEN_GAMMA");
      return (gamma_str) ? strtod(gamma_str, NULL) : 2.2;
   }

   return gamma;
}



static void user_error_fn(png_structp png_ptr, png_const_charp message)
{
   jmp_buf *jmpbuf = (jmp_buf *)png_get_error_ptr(png_ptr);
   A5O_DEBUG("PNG error: %s\n", message);
   longjmp(*jmpbuf, 1);
}



/*****************************************************************************
 * Loading routines
 ****************************************************************************/



/* read_data:
 *  Custom read function to use Allegro packfile routines,
 *  rather than C streams (so we can read from datafiles!)
 */
static void read_data(png_structp png_ptr, png_bytep data, size_t length)
{
   A5O_FILE *f = (A5O_FILE *)png_get_io_ptr(png_ptr);
   if ((png_uint_32) al_fread(f, data, length) != length)
      png_error(png_ptr, "read error (loadpng calling al_fs_entry_read)");
}



/* check_if_png:
 *  Check if input file is really PNG format.
 */
#define PNG_BYTES_TO_CHECK 4

static int check_if_png(A5O_FILE *fp)
{
   unsigned char buf[PNG_BYTES_TO_CHECK];

   A5O_ASSERT(fp);

   if (al_fread(fp, buf, PNG_BYTES_TO_CHECK) != PNG_BYTES_TO_CHECK)
      return 0;

   return (png_sig_cmp(buf, (png_size_t) 0, PNG_BYTES_TO_CHECK) == 0);
}



/* really_load_png:
 *  Worker routine, used by load_png and load_memory_png.
 */
static A5O_BITMAP *really_load_png(png_structp png_ptr, png_infop info_ptr,
   int flags)
{
   A5O_BITMAP *bmp;
   png_uint_32 width, height, rowbytes, real_rowbytes;
   int bit_depth, color_type, interlace_type;
   double image_gamma, screen_gamma;
   int intent;
   int bpp;
   int number_passes, pass;
   int num_trans = 0;
   PalEntry pal[256];
   png_bytep trans;
   A5O_LOCKED_REGION *lock;
   unsigned char *buf;
   unsigned char *dest;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);
   bool index_only;

   A5O_ASSERT(png_ptr && info_ptr);

   /* The call to png_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).
    */
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                &color_type, &interlace_type, NULL, NULL);

   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   png_set_packing(png_ptr);

   /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
   if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
      png_set_expand(png_ptr);

   /* Adds a full alpha channel if there is transparency information
    * in a tRNS chunk.
    */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
      if (!(color_type & PNG_COLOR_MASK_PALETTE))
         png_set_tRNS_to_alpha(png_ptr);
      png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, NULL);
   }

   /* Convert 16-bits per colour component to 8-bits per colour component. */
   if (bit_depth == 16)
      png_set_strip_16(png_ptr);

   /* Convert grayscale to RGB triplets */
   if ((color_type == PNG_COLOR_TYPE_GRAY) ||
       (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
      png_set_gray_to_rgb(png_ptr);

   /* Optionally, tell libpng to handle the gamma correction for us. */
   screen_gamma = get_gamma();
   if (screen_gamma != 0.0) {
      if (png_get_sRGB(png_ptr, info_ptr, &intent))
         png_set_gamma(png_ptr, screen_gamma, 0.45455);
      else {
         if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
         else
            png_set_gamma(png_ptr, screen_gamma, 0.45455);
      }
   }

   /* Turn on interlace handling. */
   number_passes = png_set_interlace_handling(png_ptr);

   /* Call to gamma correct and add the background to the palette
    * and update info structure.
    */
   png_read_update_info(png_ptr, info_ptr);

   /* Palettes. */
   if (color_type & PNG_COLOR_MASK_PALETTE) {
      int num_palette, i;
      png_colorp palette;

      if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
         /* We don't actually dither, we just copy the palette. */
         for (i = 0; ((i < num_palette) && (i < 256)); i++) {
            pal[i].r = palette[i].red;
            pal[i].g = palette[i].green;
            pal[i].b = palette[i].blue;
         }

         for (; i < 256; i++)
            pal[i].r = pal[i].g = pal[i].b = 0;
      }
   }

   rowbytes = png_get_rowbytes(png_ptr, info_ptr);

   /* Allocate the memory to hold the image using the fields of info_ptr. */
   bpp = rowbytes * 8 / width;

   /* Allegro cannot handle less than 8 bpp. */
   if (bpp < 8)
      bpp = 8;


   if ((bpp == 24) || (bpp == 32)) {
#ifdef A5O_BIG_ENDIAN
      png_set_bgr(png_ptr);
      png_set_swap_alpha(png_ptr);
#endif
   }

   bmp = al_create_bitmap(width, height);
   if (!bmp) {
      A5O_ERROR("al_create_bitmap failed while loading PNG.\n");
      return NULL;
   }

   // TODO: can this be different from rowbytes?
   real_rowbytes = ((bpp + 7) / 8) * width;
   if (interlace_type == PNG_INTERLACE_ADAM7)
      buf = al_malloc(real_rowbytes * height);
   else
      buf = al_malloc(real_rowbytes);

   if (bpp == 8 && (color_type & PNG_COLOR_MASK_PALETTE) &&
      (flags & A5O_KEEP_INDEX))
   {
      lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8,
         A5O_LOCK_WRITEONLY);
      index_only = true;
   }
   else {
      lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE,
         A5O_LOCK_WRITEONLY);
      index_only = false;
   }

   /* Read the image, one line at a time (easier to debug!) */
   for (pass = 0; pass < number_passes; pass++) {
      png_uint_32 y;
      unsigned int i;
      unsigned char *ptr;
      dest = lock->data;

      for (y = 0; y < height; y++) {
         unsigned char *dest_row_start = dest;
         /* For interlaced pictures, the row needs to be initialized with
          * the contents of the previous pass.
          */
         if (interlace_type == PNG_INTERLACE_ADAM7)
            ptr = buf + y * real_rowbytes;
         else
            ptr = buf;
         png_read_row(png_ptr, NULL, ptr);
   
         switch (bpp) {
            case 8:
               if (index_only) {
                  for (i = 0; i < width; i++) {
                     *(dest++) = *(ptr++);
                  }
               }
               else if (color_type & PNG_COLOR_MASK_PALETTE) {
                  for (i = 0; i < width; i++) {
                     int pix = ptr[0];
                     ptr++;
                     dest[0] = pal[pix].r;
                     dest[1] = pal[pix].g;
                     dest[2] = pal[pix].b;
                     if (pix < num_trans) {
                        int a = trans[pix];
                        dest[3] = a;
                        if (premul) {
                           dest[0] = dest[0] * a / 255;
                           dest[1] = dest[1] * a / 255;
                           dest[2] = dest[2] * a / 255;
                        }
                     } else {
                        dest[3] = 255;
                     }
                     dest += 4;
                  }
               }
               else {
                  for (i = 0; i < width; i++) {
                     int pix = ptr[0];
                     ptr++;
                     *(dest++) = pix;
                     *(dest++) = pix;
                     *(dest++) = pix;
                     *(dest++) = 255;
                  }
               }
               break;

            case 24:
               for (i = 0; i < width; i++) {
                  uint32_t pix = _AL_READ3BYTES(ptr);
                  ptr += 3;
                  *(dest++) = pix & 0xff;
                  *(dest++) = (pix >> 8) & 0xff;
                  *(dest++) = (pix >> 16) & 0xff;
                  *(dest++) = 255;
               }
               break;

            case 32:
               for (i = 0; i < width; i++) {
                  uint32_t pix = *(uint32_t*)ptr;
                  int r = pix & 0xff;
                  int g = (pix >> 8) & 0xff;
                  int b = (pix >> 16) & 0xff;
                  int a = (pix >> 24) & 0xff;
                  ptr += 4;

                  if (premul) {
                     r = r * a / 255;
                     g = g * a / 255;
                     b = b * a / 255;
                  }

                  *(dest++) = r;
                  *(dest++) = g;
                  *(dest++) = b;
                  *(dest++) = a;
               }
               break;

            default:
               A5O_ASSERT(bpp == 8 || bpp == 24 || bpp == 32);
               break;
         }
         dest = dest_row_start + lock->pitch;
      }
   }

   al_unlock_bitmap(bmp);

   al_free(buf);

   /* Read rest of file, and get additional chunks in info_ptr. */
   png_read_end(png_ptr, info_ptr);

   return bmp;
}


/* Load a PNG file from disk, doing colour coversion if required.
 */
A5O_BITMAP *_al_load_png_f(A5O_FILE *fp, int flags)
{
   jmp_buf jmpbuf;
   A5O_BITMAP *bmp;
   png_structp png_ptr;
   png_infop info_ptr;

   A5O_ASSERT(fp);

   if (!check_if_png(fp)) {
      A5O_ERROR("Not a png.\n");
      return NULL;
   }

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                    (void *)NULL, NULL, NULL);
   if (!png_ptr) {
      A5O_ERROR("png_ptr == NULL\n");
      return NULL;
   }

   /* Allocate/initialize the memory for image information. */
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
      A5O_ERROR("png_create_info_struct failed\n");
      return NULL;
   }

   /* Set error handling. */
   if (setjmp(jmpbuf)) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
      /* If we get here, we had a problem reading the file */
      A5O_ERROR("Error reading PNG file\n");
      return NULL;
   }
   png_set_error_fn(png_ptr, jmpbuf, user_error_fn, NULL);

   /* Use Allegro packfile routines. */
   png_set_read_fn(png_ptr, fp, (png_rw_ptr) read_data);

   /* We have already read some of the signature. */
   png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

   /* Really load the image now. */
   bmp = really_load_png(png_ptr, info_ptr, flags);

   /* Clean up after the read, and free any memory allocated. */
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

   return bmp;
}





A5O_BITMAP *_al_load_png(const char *filename, int flags)
{
   A5O_FILE *fp;
   A5O_BITMAP *bmp;

   A5O_ASSERT(filename);

   fp = al_fopen(filename, "rb");
   if (!fp) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_load_png_f(fp, flags);

   al_fclose(fp);

   return bmp;
}




/*****************************************************************************
 * Saving routines
 ****************************************************************************/



/* write_data:
 *  Custom write function to use Allegro packfile routines,
 *  rather than C streams.
 */
static void write_data(png_structp png_ptr, png_bytep data, size_t length)
{
   A5O_FILE *f = (A5O_FILE *)png_get_io_ptr(png_ptr);
   if ((png_uint_32) al_fwrite(f, data, length) != length)
      png_error(png_ptr, "write error (loadpng calling al_fs_entry_write)");
}

/* Don't think Allegro has any problem with buffering
 * (rather, Allegro provides no way to flush packfiles).
 */
static void flush_data(png_structp png_ptr)
{
   (void)png_ptr;
}

/* translate_compression_level:
 *  Translate string with config value into proper zlib's
 *  compression level. Assumes default on NULL.
 */
static int translate_compression_level(const char* value) {
   if (!value || strcmp(value, "default") == 0) {
      return Z_DEFAULT_COMPRESSION;
   }
   if (strcmp(value, "best") == 0) {
      return Z_BEST_COMPRESSION;
   }
   if (strcmp(value, "fastest") == 0) {
      return Z_BEST_SPEED;
   }
   if (strcmp(value, "none") == 0) {
      return Z_NO_COMPRESSION;
   }
   return strtol(value, NULL, 10);
}

/* save_rgba:
 *  Core save routine for 32 bpp images.
 */
static int save_rgba(png_structp png_ptr, A5O_BITMAP *bmp)
{
   const int bmp_h = al_get_bitmap_height(bmp);
   A5O_LOCKED_REGION *lock;
   int y;

   lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE,
      A5O_LOCK_READONLY);
   if (!lock)
      return 0;

   for (y = 0; y < bmp_h; y++) {
      unsigned char *p = (unsigned char *)lock->data + lock->pitch * y;
      png_write_row(png_ptr, p);
   }
   
   al_unlock_bitmap(bmp);

   return 1;
}



/* Writes a non-interlaced, no-frills PNG, taking the usual save_xyz
 *  parameters.  Returns non-zero on error.
 */
bool _al_save_png_f(A5O_FILE *fp, A5O_BITMAP *bmp)
{
   jmp_buf jmpbuf;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   int colour_type;

   /* Create and initialize the png_struct with the
    * desired error handler functions.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                     (void *)NULL, NULL, NULL);
   if (!png_ptr) {
      A5O_ERROR("Unable to create PNG write struct.\n");
      goto Error;
   }

   /* Allocate/initialize the image information data. */
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      A5O_ERROR("Unable to create PNG info struct.\n");
      goto Error;
   }

   /* Set error handling. */
   if (setjmp(jmpbuf)) {
      A5O_ERROR("Failed to call setjmp.\n");
      goto Error;
   }
   png_set_error_fn(png_ptr, jmpbuf, user_error_fn, NULL);

   /* Use packfile routines. */
   png_set_write_fn(png_ptr, fp, (png_rw_ptr) write_data, flush_data);

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE.
    */
   colour_type = PNG_COLOR_TYPE_RGB_ALPHA;

   /* Set compression level. */
   int z_level = translate_compression_level(
      al_get_config_value(al_get_system_config(), "image", "png_compression_level")
   );
   png_set_compression_level(png_ptr, z_level);

   png_set_IHDR(png_ptr, info_ptr,
                al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
                8, colour_type,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

   /* Optionally write comments into the image ... Nah. */

   /* Write the file header information. */
   png_write_info(png_ptr, info_ptr);

   /* Once we write out the header, the compression type on the text
    * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
    * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
    * at the end.
    */
   if (!save_rgba(png_ptr, bmp)) {
      A5O_ERROR("save_rgba failed.\n");
      goto Error;
   }

   png_write_end(png_ptr, info_ptr);

   png_destroy_write_struct(&png_ptr, &info_ptr);

   return true;

 Error:

   if (png_ptr) {
      if (info_ptr)
         png_destroy_write_struct(&png_ptr, &info_ptr);
      else
         png_destroy_write_struct(&png_ptr, NULL);
   }

   return false;
}


bool _al_save_png(const char *filename, A5O_BITMAP *bmp)
{
   A5O_FILE *fp;
   bool retsave;
   bool retclose;

   A5O_ASSERT(filename);
   A5O_ASSERT(bmp);

   fp = al_fopen(filename, "wb");
   if (!fp) {
      A5O_ERROR("Unable to open file %s for writing\n", filename);
      return false;
   }

   retsave = _al_save_png_f(fp, bmp);
   retclose = al_fclose(fp);

   return retsave && retclose;
}

/* vim: set sts=3 sw=3 et: */
