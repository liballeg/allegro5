/* libjpeg wrapper for Allegro 5 iio addon.
 * by Elias Pschernig
 */

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <jerror.h>

#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"
#include "allegro5/internal/aintern_memory.h"

#include "iio.h"

#define BUFFER_SIZE 4096

struct my_src_mgr
{
   struct jpeg_source_mgr pub;
   unsigned char *buffer;
   ALLEGRO_FS_ENTRY *pf;
};

struct my_dest_mgr
{
   struct jpeg_destination_mgr pub;
   unsigned char *buffer;
   ALLEGRO_FS_ENTRY *pf;
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
   src->pub.bytes_in_buffer = al_fread(src->pf, BUFFER_SIZE, src->buffer);
   return 1;
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
   struct my_dest_mgr *dest = (void *)cinfo->dest;
   al_fwrite(dest->pf, BUFFER_SIZE, dest->buffer);
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
      al_fseek(src->pf, skip, ALLEGRO_SEEK_CUR);
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
   al_fwrite(dest->pf, BUFFER_SIZE - dest->pub.free_in_buffer, dest->buffer);
}


static void jpeg_packfile_src(j_decompress_ptr cinfo, ALLEGRO_FS_ENTRY *pf,
                              unsigned char *buffer)
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
   src->pf = pf;
}

static void jpeg_packfile_dest(j_compress_ptr cinfo, ALLEGRO_FS_ENTRY *pf,
                               unsigned char *buffer)
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
   dest->pf = pf;
}

static ALLEGRO_BITMAP *load_jpg_pf(ALLEGRO_FS_ENTRY *pf)
{
   struct jpeg_decompress_struct cinfo;
   struct jpeg_error_mgr jerr;
   ALLEGRO_BITMAP *bmp = NULL;
   ALLEGRO_LOCKED_REGION lock;
   ALLEGRO_STATE backup;
   unsigned char *buffer = NULL;
   unsigned char *rows[1] = { NULL };
   int w, h, s;

   cinfo.err = jpeg_std_error(&jerr);

   buffer = _AL_MALLOC(BUFFER_SIZE);

   jpeg_create_decompress(&cinfo);
   jpeg_packfile_src(&cinfo, pf, buffer);
   jpeg_read_header(&cinfo, TRUE);
   jpeg_start_decompress(&cinfo);

   w = cinfo.output_width;
   h = cinfo.output_height;
   s = cinfo.output_components;

   /* Only one and three components make sense in a JPG file. */
   if (s != 1 && s != 3) {
      goto error;
   }

   bmp = al_create_bitmap(w, h);
   al_lock_bitmap(bmp, &lock, ALLEGRO_LOCK_WRITEONLY);
   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(bmp);

   rows[0] = _AL_MALLOC(w * s);

   while ((int)cinfo.output_scanline < h) {
      int x, y = cinfo.output_scanline;
      jpeg_read_scanlines(&cinfo, (void *)rows, 1);
      for (x = 0; x < w; x++) {
         if (s == 1) {
            unsigned char c = rows[0][x];
            al_put_pixel(x, y, al_map_rgb(c, c, c));
         }
         else if (s == 3) {
            unsigned char r = rows[0][x * s + 0];
            unsigned char g = rows[0][x * s + 1];
            unsigned char b = rows[0][x * s + 2];
            al_put_pixel(x, y, al_map_rgb(r, g, b));
         }
      }
   }

   al_unlock_bitmap(bmp);
   al_restore_state(&backup);

 error:
   _AL_FREE(buffer);
   _AL_FREE(rows[0]);
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);

   return bmp;
}

static int save_jpg_pf(ALLEGRO_FS_ENTRY *pf, ALLEGRO_BITMAP *bmp)
{
   struct jpeg_compress_struct cinfo;
   struct jpeg_error_mgr jerr;
   ALLEGRO_LOCKED_REGION lock;
   ALLEGRO_STATE backup;
   unsigned char *buffer = NULL;
   unsigned char *rows[1] = { NULL };

   cinfo.err = jpeg_std_error(&jerr);

   buffer = _AL_MALLOC(BUFFER_SIZE);

   jpeg_create_compress(&cinfo);
   jpeg_packfile_dest(&cinfo, pf, buffer);

   cinfo.image_width = al_get_bitmap_width(bmp);
   cinfo.image_height = al_get_bitmap_height(bmp);
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;
   jpeg_set_defaults(&cinfo);

   jpeg_start_compress(&cinfo, 1);

   al_lock_bitmap(bmp, &lock, ALLEGRO_LOCK_READONLY);
   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(bmp);

   rows[0] = _AL_MALLOC(cinfo.image_width * 3);

   while (cinfo.next_scanline < cinfo.image_height) {
      int x, y = cinfo.next_scanline;
      for (x = 0; x < (int)cinfo.image_width; x++) {
         unsigned char r, g, b;
         al_unmap_rgb(al_get_pixel(bmp, x, y), &r, &g, &b);
         rows[0][x * 3 + 0] = r;
         rows[0][x * 3 + 1] = g;
         rows[0][x * 3 + 2] = b;
      }
      jpeg_write_scanlines(&cinfo, (void *)rows, 1);
   }

   al_unlock_bitmap(bmp);
   al_restore_state(&backup);

   _AL_FREE(buffer);
   _AL_FREE(rows[0]);
   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);

   return 1;
}

/* Function: iio_load_jpg
 * Create a new ALLEGRO_BITMAP from a JPEG file. The bitmap is created with
 * <al_create_bitmap>.
 * See Also: <al_iio_load>.
 */
ALLEGRO_BITMAP *iio_load_jpg(char const *filename)
{
   ALLEGRO_FS_ENTRY *pf;
   ALLEGRO_BITMAP *bmp;

   ASSERT(filename);

   pf = al_fopen(filename, "rb");
   if (!pf)
      return NULL;

   bmp = load_jpg_pf(pf);

   al_fclose(pf);

   return bmp;
}

/* Function: iio_save_jpg
 * Save an ALLEGRO_BITMAP as a JPEG file. 
 * FIXME: This function is 100% untested right now and not expected to work yet.
 * See Also: <al_iio_save>.
 */
int iio_save_jpg(char const *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FS_ENTRY *pf;
   int result;

   ASSERT(filename);
   ASSERT(bmp);

   pf = al_fopen(filename, "wb");
   if (!pf) {
      TRACE("Unable to open file %s for writing\n", filename);
      return -1;
   }

   result = save_jpg_pf(pf, bmp);

   al_fclose(pf);

   return result;
}
