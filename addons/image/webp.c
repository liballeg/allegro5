/* Allegro wrapper routines for libwebp
 * by Sebastian Krzyszkowiak (dos@dosowisko.net).
 */


#include <webp/decode.h>
#include <webp/encode.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

A5O_DEBUG_CHANNEL("image")


/*****************************************************************************
 * Loading routines
 ****************************************************************************/


static A5O_BITMAP *load_from_buffer(const uint8_t* data, size_t data_size,
   int flags)
{
   A5O_BITMAP *bmp;
   A5O_LOCKED_REGION *lock;
   bool premul = !(flags & A5O_NO_PREMULTIPLIED_ALPHA);

   WebPDecoderConfig config;
   WebPInitDecoderConfig(&config);

   if (WebPGetFeatures(data, data_size, &config.input) != VP8_STATUS_OK) {
      A5O_ERROR("Could not read WebP stream info\n");
      return NULL;
   }

   bmp = al_create_bitmap(config.input.width, config.input.height);
   if (!bmp) {
      A5O_ERROR("al_create_bitmap failed while loading WebP.\n");
      return NULL;
   }

   lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE,
      A5O_LOCK_WRITEONLY);

   config.output.colorspace = premul ? MODE_rgbA : MODE_RGBA;
   config.output.u.RGBA.rgba = (uint8_t*)lock->data;
   config.output.u.RGBA.stride = lock->pitch;
   config.output.u.RGBA.size = lock->pitch * config.input.height;
   config.output.is_external_memory = 1;

   if (WebPDecode(data, data_size, &config) != VP8_STATUS_OK) {
      A5O_ERROR("Could not decode WebP stream\n");
      al_destroy_bitmap(bmp);
      return NULL;
   }

   al_unlock_bitmap(bmp);

   return bmp;
}


A5O_BITMAP *_al_load_webp_f(A5O_FILE *fp, int flags)
{
   A5O_ASSERT(fp);
   A5O_BITMAP *bmp;

   size_t data_size = al_fsize(fp);
   uint8_t *data = al_malloc(data_size * sizeof(uint8_t));

   if (al_fread(fp, data, data_size) != data_size) {
      A5O_ERROR("Could not read WebP file\n");
      free(data);
      return NULL;
   }

   bmp = load_from_buffer(data, data_size, flags);

   free(data);

   return bmp;
}





A5O_BITMAP *_al_load_webp(const char *filename, int flags)
{
   A5O_FILE *fp;
   A5O_BITMAP *bmp;

   A5O_ASSERT(filename);

   fp = al_fopen(filename, "rb");
   if (!fp) {
      A5O_ERROR("Unable to open %s for reading.\n", filename);
      return NULL;
   }

   bmp = _al_load_webp_f(fp, flags);

   al_fclose(fp);

   return bmp;
}




/*****************************************************************************
 * Saving routines
 ****************************************************************************/


bool _al_save_webp_f(A5O_FILE *fp, A5O_BITMAP *bmp)
{

   A5O_LOCKED_REGION *lock;
   bool ret = false;

   lock = al_lock_bitmap(bmp, A5O_PIXEL_FORMAT_ABGR_8888_LE,
      A5O_LOCK_READONLY);
   if (!lock) {
      A5O_ERROR("Failed to lock bitmap.\n");
      return false;
   }

   uint8_t *output = NULL;
   size_t size;

   const char* level = al_get_config_value(al_get_system_config(), "image", "webp_quality_level");
   if (!level || strcmp(level, "lossless") == 0) {
      size = WebPEncodeLosslessRGBA((const uint8_t*)lock->data,
         al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
         lock->pitch, &output);
   } else {
      size = WebPEncodeRGBA((const uint8_t*)lock->data,
         al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
         lock->pitch, level ? strtol(level, NULL, 10) : 75, &output);
   }

   if (al_fwrite(fp, output, size) == size) {
       ret = true;
   }

#if WEBP_DECODER_ABI_VERSION >= 0x0206
   WebPFree(output);
#else
   free(output);
#endif
   al_unlock_bitmap(bmp);

   return ret;

}


bool _al_save_webp(const char *filename, A5O_BITMAP *bmp)
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

   retsave = _al_save_webp_f(fp, bmp);
   if (!retsave) {
      A5O_ERROR("Unable to store WebP bitmap in file %s\n", filename);
   }
   retclose = al_fclose(fp);
   if (!retclose) {
      A5O_ERROR("Unable to close file %s\n", filename);
   }

   return retsave && retclose;
}

/* vim: set sts=3 sw=3 et: */
