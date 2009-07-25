/* libjpeg wrapper for Allegro 5 iio addon.
 * by Elias Pschernig
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#define boolean A5_BOOLEAN_HACK
#include <jpeglib.h>
#undef boolean
#include <jerror.h>

#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/a5_iio.h"

#include "iio.h"

#define BUFFER_SIZE 4096

struct my_src_mgr
{
   struct jpeg_source_mgr pub;
   unsigned char *buffer;
   ALLEGRO_FILE *pf;
};

struct my_dest_mgr
{
   struct jpeg_destination_mgr pub;
   unsigned char *buffer;
   ALLEGRO_FILE *pf;
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

static A5_BOOLEAN_HACK fill_input_buffer(j_decompress_ptr cinfo)
{
   struct my_src_mgr *src = (void *)cinfo->src;
   src->pub.next_input_byte = src->buffer;
   src->pub.bytes_in_buffer = al_fread(src->pf, src->buffer, BUFFER_SIZE);
   return 1;
}

static A5_BOOLEAN_HACK empty_output_buffer(j_compress_ptr cinfo)
{
   struct my_dest_mgr *dest = (void *)cinfo->dest;
   al_fwrite(dest->pf, dest->buffer, BUFFER_SIZE);
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
   al_fwrite(dest->pf, dest->buffer, BUFFER_SIZE - dest->pub.free_in_buffer);
}


static void jpeg_packfile_src(j_decompress_ptr cinfo, ALLEGRO_FILE *pf,
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

static void jpeg_packfile_dest(j_compress_ptr cinfo, ALLEGRO_FILE *pf,
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

static void my_error_exit(j_common_ptr cinfo)
{
   struct my_err_mgr *jerr = (void *)cinfo->err;

   longjmp(jerr->jmpenv, 1);
}

static ALLEGRO_BITMAP *load_jpg_entry_helper(ALLEGRO_FILE *pf,
   unsigned char **pbuffer, unsigned char **prow)
{
   struct jpeg_decompress_struct cinfo;
   struct my_err_mgr jerr;
   ALLEGRO_BITMAP *bmp = NULL;
   ALLEGRO_LOCKED_REGION *lock;
   int w, h, s;

   cinfo.err = jpeg_std_error(&jerr.pub);
   jerr.pub.error_exit = my_error_exit;
   if (setjmp(jerr.jmpenv) != 0) {
      /* Longjmp'd. */
      goto longjmp_error;
   }

   *pbuffer = _AL_MALLOC(BUFFER_SIZE);

   jpeg_create_decompress(&cinfo);
   jpeg_packfile_src(&cinfo, pf, *pbuffer);
   jpeg_read_header(&cinfo, true);
   jpeg_start_decompress(&cinfo);

   w = cinfo.output_width;
   h = cinfo.output_height;
   s = cinfo.output_components;

   /* Only one and three components make sense in a JPG file. */
   if (s != 1 && s != 3) {
      goto error;
   }

   bmp = al_create_bitmap(w, h);
   if (!bmp) {
      goto error;
   }

   lock = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
   al_set_target_bitmap(bmp);

   *prow = _AL_MALLOC(w * s);

   while ((int)cinfo.output_scanline < h) {
      int x, y = cinfo.output_scanline;
      jpeg_read_scanlines(&cinfo, (void *)prow, 1);
      for (x = 0; x < w; x++) {
         if (s == 1) {
            unsigned char c = (*prow)[x];
            al_put_pixel(x, y, al_map_rgb(c, c, c));
         }
         else if (s == 3) {
            unsigned char r = (*prow)[x * s + 0];
            unsigned char g = (*prow)[x * s + 1];
            unsigned char b = (*prow)[x * s + 2];
            al_put_pixel(x, y, al_map_rgb(r, g, b));
         }
      }
   }

   al_unlock_bitmap(bmp);

 error:
   jpeg_finish_decompress(&cinfo);

 longjmp_error:
   jpeg_destroy_decompress(&cinfo);

   return bmp;
}

ALLEGRO_BITMAP *al_load_jpg_entry(ALLEGRO_FILE *pf)
{
   unsigned char *buffer = NULL;
   unsigned char *row = NULL;
   ALLEGRO_STATE state;
   ALLEGRO_BITMAP *bmp;

   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);

   bmp = load_jpg_entry_helper(pf, &buffer, &row);

   /* Automatic variables in load_jpg_entry_helper might lose their values
    * after a longjmp so we do the cleanup here.
    */
   _AL_FREE((unsigned char *)buffer);
   _AL_FREE(row);

   al_restore_state(&state);

   return bmp;
}

static bool save_jpg_entry_helper(ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp,
   unsigned char **pbuffer, unsigned char **prow)
{
   struct jpeg_compress_struct cinfo;
   struct my_err_mgr jerr;
   ALLEGRO_LOCKED_REGION *lock;
   bool rc = true;

   cinfo.err = jpeg_std_error(&jerr.pub);
   jerr.pub.error_exit = my_error_exit;
   if (setjmp(jerr.jmpenv)) {
      /* Longjmp'd. */
      rc = false;
      goto longjmp_error;
   }

   *pbuffer = _AL_MALLOC(BUFFER_SIZE);

   jpeg_create_compress(&cinfo);
   jpeg_packfile_dest(&cinfo, pf, *pbuffer);

   cinfo.image_width = al_get_bitmap_width(bmp);
   cinfo.image_height = al_get_bitmap_height(bmp);
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;
   jpeg_set_defaults(&cinfo);

   jpeg_start_compress(&cinfo, 1);

   lock = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
   al_set_target_bitmap(bmp);

   *prow = _AL_MALLOC(cinfo.image_width * 3);

   while (cinfo.next_scanline < cinfo.image_height) {
      int x, y = cinfo.next_scanline;
      for (x = 0; x < (int)cinfo.image_width; x++) {
         unsigned char r, g, b;
         al_unmap_rgb(al_get_pixel(bmp, x, y), &r, &g, &b);
         (*prow)[x * 3 + 0] = r;
         (*prow)[x * 3 + 1] = g;
         (*prow)[x * 3 + 2] = b;
      }
      jpeg_write_scanlines(&cinfo, (void *)prow, 1);
   }

   al_unlock_bitmap(bmp);

   jpeg_finish_compress(&cinfo);

 longjmp_error:
   jpeg_destroy_compress(&cinfo);

   return rc;
}

int al_save_jpg_entry(ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_STATE state;
   unsigned char *buffer = NULL;
   unsigned char *row = NULL;
   int rc;

   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);

   rc = save_jpg_entry_helper(pf, bmp, &buffer, &row);

   al_restore_state(&state);

   /* Automatic variables in save_jpg_entry_helper might lose their values
    * after a longjmp so we do the cleanup here.
    */
   _AL_FREE(buffer);
   _AL_FREE(row);

   return rc ? 0 : -1;
}

/* Function: al_load_jpg
 */
ALLEGRO_BITMAP *al_load_jpg(char const *filename)
{
   ALLEGRO_FILE *pf;
   ALLEGRO_BITMAP *bmp;

   ASSERT(filename);

   pf = al_fopen(filename, "rb");
   if (!pf)
      return NULL;

   bmp = al_load_jpg_entry(pf);

   al_fclose(pf);

   return bmp;
}

/* Function: al_save_jpg
 */
int al_save_jpg(char const *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FILE *pf;
   int result;

   ASSERT(filename);
   ASSERT(bmp);

   pf = al_fopen(filename, "wb");
   if (!pf) {
      TRACE("Unable to open file %s for writing\n", filename);
      return -1;
   }

   result = al_save_jpg_entry(pf, bmp);

   al_fclose(pf);

   return result;
}
