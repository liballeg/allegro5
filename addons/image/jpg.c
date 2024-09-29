/* libjpeg wrapper for Allegro 5 iio addon.
 * by Elias Pschernig
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#ifdef A5O_HAVE_STDINT_H
#include <stdint.h>
#endif

#define BUFFER_SIZE 4096

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

#include <jpeglib.h>
#include <jerror.h>

A5O_DEBUG_CHANNEL("image")


struct my_src_mgr
{
   struct jpeg_source_mgr pub;
   JOCTET *buffer;
   A5O_FILE *fp;
};

struct my_dest_mgr
{
   struct jpeg_destination_mgr pub;
   JOCTET *buffer;
   A5O_FILE *fp;
};

struct my_err_mgr
{
   struct jpeg_error_mgr pub;
   jmp_buf jmpenv;
};

static void init_source(j_decompress_ptr cinfo)
{
   (void)cinfo;
}

static void init_destination(j_compress_ptr cinfo)
{
   struct my_dest_mgr *dest = (void *)cinfo->dest;
   dest->pub.next_output_byte = dest->buffer;
   dest->pub.free_in_buffer = BUFFER_SIZE;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
   struct my_src_mgr *src = (void *)cinfo->src;
   src->pub.next_input_byte = src->buffer;
   src->pub.bytes_in_buffer = al_fread(src->fp, src->buffer, BUFFER_SIZE);
   return 1;
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
   struct my_dest_mgr *dest = (void *)cinfo->dest;
   al_fwrite(dest->fp, dest->buffer, BUFFER_SIZE);
   dest->pub.next_output_byte = dest->buffer;
   dest->pub.free_in_buffer = BUFFER_SIZE;
   return 1;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
   struct my_src_mgr *src = (void *)cinfo->src;
   if (num_bytes <= (long)src->pub.bytes_in_buffer) {
      src->pub.next_input_byte += num_bytes;
      src->pub.bytes_in_buffer -= num_bytes;
   }
   else {
      long skip = num_bytes - src->pub.bytes_in_buffer;
      al_fseek(src->fp, skip, A5O_SEEK_CUR);
      src->pub.bytes_in_buffer = 0;
   }
}

static void term_source(j_decompress_ptr cinfo)
{
   (void)cinfo;
}

static void term_destination(j_compress_ptr cinfo)
{
   struct my_dest_mgr *dest = (void *)cinfo->dest;
   al_fwrite(dest->fp, dest->buffer, BUFFER_SIZE - dest->pub.free_in_buffer);
}


static void jpeg_packfile_src(j_decompress_ptr cinfo, A5O_FILE *fp,
                              JOCTET *buffer)
{
   struct my_src_mgr *src;

   if (!cinfo->src)
      cinfo->src =
          (*cinfo->mem->alloc_small) ((void *)cinfo, JPOOL_PERMANENT,
                                      sizeof *src);

   src = (void *)cinfo->src;
   src->pub.init_source = init_source;
   src->pub.fill_input_buffer = fill_input_buffer;
   src->pub.skip_input_data = skip_input_data;
   src->pub.resync_to_restart = jpeg_resync_to_restart;
   src->pub.term_source = term_source;
   src->pub.bytes_in_buffer = 0;
   src->buffer = buffer;
   src->fp = fp;
}

static void jpeg_packfile_dest(j_compress_ptr cinfo, A5O_FILE *fp,
                               JOCTET *buffer)
{
   struct my_dest_mgr *dest;

   if (!cinfo->dest)
      cinfo->dest =
          (*cinfo->mem->alloc_small) ((void *)cinfo, JPOOL_PERMANENT,
                                      sizeof *dest);

   dest = (void *)cinfo->dest;
   dest->pub.init_destination = init_destination;
   dest->pub.empty_output_buffer = empty_output_buffer;
   dest->pub.term_destination = term_destination;
   dest->pub.free_in_buffer = 0;
   dest->buffer = buffer;
   dest->fp = fp;
}

static void my_error_exit(j_common_ptr cinfo)
{
   char buffer[JMSG_LENGTH_MAX];
   struct my_err_mgr *jerr = (void *)cinfo->err;
   jerr->pub.format_message(cinfo, buffer);
   A5O_ERROR("jpeg error: %s\n", buffer);
   longjmp(jerr->jmpenv, 1);
}

/* We keep data for load_jpg_entry_helper in a structure allocated in the
 * caller's stack frame to avoid problems with automatic variables being
 * undefined after a longjmp.
 */
struct load_jpg_entry_helper_data {
   bool error;
   A5O_BITMAP *bmp;
   JOCTET *buffer;
   unsigned char *row;
};

static void load_jpg_entry_helper(A5O_FILE *fp,
   struct load_jpg_entry_helper_data *data, int flags)
{
   struct jpeg_decompress_struct cinfo;
   struct my_err_mgr jerr;
   A5O_LOCKED_REGION *lock;
   int w, h, s;

   /* A5O_NO_PREMULTIPLIED_ALPHA does not apply.
    * A5O_KEEP_INDEX does not apply.
    */
   (void)flags;

   data->error = false;

   cinfo.err = jpeg_std_error(&jerr.pub);
   jerr.pub.error_exit = my_error_exit;
   if (setjmp(jerr.jmpenv) != 0) {
      /* Longjmp'd. */
      data->error = true;
      goto longjmp_error;
   }

   data->buffer = al_malloc(BUFFER_SIZE);
   if (!data->buffer) {
      data->error = true;
      goto error;
   }

   jpeg_create_decompress(&cinfo);
   jpeg_packfile_src(&cinfo, fp, data->buffer);
   jpeg_read_header(&cinfo, true);
   jpeg_start_decompress(&cinfo);

   w = cinfo.output_width;
   h = cinfo.output_height;
   s = cinfo.output_components;

   /* Only one and three components make sense in a JPG file. */
   if (s != 1 && s != 3) {
      data->error = true;
      A5O_ERROR("%d components makes no sense\n", s);
      goto error;
   }

   data->bmp = al_create_bitmap(w, h);
   if (!data->bmp) {
      data->error = true;
      A5O_ERROR("%dx%d bitmap creation failed\n", w, h);
      goto error;
   }

   /* Allegro's pixel format is endian independent, so that in
    * A5O_PIXEL_FORMAT_RGB_888 the lower 8 bits always hold the Blue
    * component.  On a little endian system this is in byte 0.  On a big
    * endian system this is in byte 2.
    *
    * libjpeg expects byte 0 to hold the Red component, byte 1 to hold the
    * Green component, byte 2 to hold the Blue component.  Hence on little
    * endian systems we need the opposite format, A5O_PIXEL_FORMAT_BGR_888.
    */
#ifdef A5O_BIG_ENDIAN
   lock = al_lock_bitmap(data->bmp, A5O_PIXEL_FORMAT_RGB_888,
       A5O_LOCK_WRITEONLY);
#else
   lock = al_lock_bitmap(data->bmp, A5O_PIXEL_FORMAT_BGR_888,
       A5O_LOCK_WRITEONLY);
#endif

   if (s == 3) {
      /* Colour. */
      int y;

      for (y = cinfo.output_scanline; y < h; y = cinfo.output_scanline) {
         unsigned char *out[1];
         out[0] = ((unsigned char *)lock->data) + y * lock->pitch;
         jpeg_read_scanlines(&cinfo, (void *)out, 1);
      }
   }
   else if (s == 1) {
      /* Greyscale. */
      unsigned char *in;
      unsigned char *out;
      int x, y;

      data->row = al_malloc(w);
      for (y = cinfo.output_scanline; y < h; y = cinfo.output_scanline) {
         jpeg_read_scanlines(&cinfo, (void *)&data->row, 1);
         in = data->row;
         out = ((unsigned char *)lock->data) + y * lock->pitch;
         for (x = 0; x < w; x++) {
            *out++ = *in;
            *out++ = *in;
            *out++ = *in;
            in++;
         }
      }
   }

 error:
   jpeg_finish_decompress(&cinfo);

 longjmp_error:
   jpeg_destroy_decompress(&cinfo);

   if (data->bmp) {
      if (al_is_bitmap_locked(data->bmp)) {
         al_unlock_bitmap(data->bmp);
      }
      if (data->error) {
         al_destroy_bitmap(data->bmp);
         data->bmp = NULL;
      }
   }

   al_free(data->buffer);
   al_free(data->row);
}

A5O_BITMAP *_al_load_jpg_f(A5O_FILE *fp, int flags)
{
   struct load_jpg_entry_helper_data data;

   memset(&data, 0, sizeof(data));
   load_jpg_entry_helper(fp, &data, flags);

   return data.bmp;
}

/* See comment about load_jpg_entry_helper_data. */
struct save_jpg_entry_helper_data {
   bool error;
   JOCTET *buffer;
};

static void save_jpg_entry_helper(A5O_FILE *fp, A5O_BITMAP *bmp,
   struct save_jpg_entry_helper_data *data)
{
   struct jpeg_compress_struct cinfo;
   struct my_err_mgr jerr;
   A5O_LOCKED_REGION *lock;

   data->error = false;

   cinfo.err = jpeg_std_error(&jerr.pub);
   jerr.pub.error_exit = my_error_exit;
   if (setjmp(jerr.jmpenv)) {
      /* Longjmp'd. */
      data->error = true;
      goto longjmp_error;
   }

   data->buffer = al_malloc(BUFFER_SIZE);
   if (!data->buffer) {
      data->error = true;
      goto error;
   }

   jpeg_create_compress(&cinfo);
   jpeg_packfile_dest(&cinfo, fp, data->buffer);

   cinfo.image_width = al_get_bitmap_width(bmp);
   cinfo.image_height = al_get_bitmap_height(bmp);
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;
   jpeg_set_defaults(&cinfo);

   const char* level = al_get_config_value(al_get_system_config(), "image", "jpeg_quality_level");
   jpeg_set_quality(&cinfo, level ? strtol(level, NULL, 10) : 75, true);

   jpeg_start_compress(&cinfo, 1);

   /* See comment in load_jpg_entry_helper. */
#ifdef A5O_BIG_ENDIAN
   lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_RGB_888,
      A5O_LOCK_READONLY);
#else
   lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_BGR_888,
      A5O_LOCK_READONLY);
#endif

   while (cinfo.next_scanline < cinfo.image_height) {
      unsigned char *row[1];
      row[0] = ((unsigned char *)lock->data)
         + (signed)cinfo.next_scanline * lock->pitch;
      jpeg_write_scanlines(&cinfo, (void *)row, 1);
   }

 error:
   jpeg_finish_compress(&cinfo);

 longjmp_error:
   jpeg_destroy_compress(&cinfo);

   if (al_is_bitmap_locked(bmp)) {
      al_unlock_bitmap(bmp);
   }

   al_free(data->buffer);
}

bool _al_save_jpg_f(A5O_FILE *fp, A5O_BITMAP *bmp)
{
   struct save_jpg_entry_helper_data data;

   memset(&data, 0, sizeof(data));
   save_jpg_entry_helper(fp, bmp, &data);

   return !data.error;
}

A5O_BITMAP *_al_load_jpg(char const *filename, int flags)
{
   A5O_FILE *fp;
   A5O_BITMAP *bmp;

   A5O_ASSERT(filename);

   fp = al_fopen(filename, "rb");
   if (!fp) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_load_jpg_f(fp, flags);

   al_fclose(fp);

   return bmp;
}

bool _al_save_jpg(char const *filename, A5O_BITMAP *bmp)
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

   retsave = _al_save_jpg_f(fp, bmp);

   retclose = al_fclose(fp);

   return retsave && retclose;
}

/* vim: set sts=3 sw=3 et: */
