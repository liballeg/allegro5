/* Allegro wrapper routines for libwebp
 * by Sebastian Krzyszkowiak (dos@dosowisko.net).
 */


#include <webp/decode.h>
#include <webp/encode.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_image.h"

#include "iio.h"

ALLEGRO_DEBUG_CHANNEL("image")


/*****************************************************************************
 * Loading routines
 ****************************************************************************/


static ALLEGRO_BITMAP *load_from_buffer(const uint8_t* data, size_t data_size,
   int flags)
{
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_LOCKED_REGION *lock;
   bool premul = !(flags & ALLEGRO_NO_PREMULTIPLIED_ALPHA);

   WebPDecoderConfig config;
   WebPInitDecoderConfig(&config);

   if (WebPGetFeatures(data, data_size, &config.input) != VP8_STATUS_OK) {
      ALLEGRO_ERROR("Could not read WebP stream info\n");
      return NULL;
   }

   bmp = al_create_bitmap(config.input.width, config.input.height);
   if (!bmp) {
      ALLEGRO_ERROR("al_create_bitmap failed while loading WebP.\n");
      return NULL;
   }

   lock = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
      ALLEGRO_LOCK_WRITEONLY);

   config.output.colorspace = premul ? MODE_rgbA : MODE_RGBA;
   config.output.u.RGBA.rgba = (uint8_t*)lock->data;
   config.output.u.RGBA.stride = lock->pitch;
   config.output.u.RGBA.size = lock->pitch * config.input.height;
   config.output.is_external_memory = 1;

   if (WebPDecode(data, data_size, &config) != VP8_STATUS_OK) {
      ALLEGRO_ERROR("Could not decode WebP stream\n");
      al_destroy_bitmap(bmp);
      return NULL;
   }

   al_unlock_bitmap(bmp);

   return bmp;
}


ALLEGRO_BITMAP *_al_load_webp_f(ALLEGRO_FILE *fp, int flags)
{
   ALLEGRO_ASSERT(fp);
   ALLEGRO_BITMAP *bmp;

   size_t data_size = al_fsize(fp);
   uint8_t *data = al_malloc(data_size * sizeof(uint8_t));

   if (al_fread(fp, data, data_size) != data_size) {
      ALLEGRO_ERROR("Could not read WebP file\n");
      free(data);
      return NULL;
   }

   bmp = load_from_buffer(data, data_size, flags);

   free(data);

   return bmp;
}





ALLEGRO_BITMAP *_al_load_webp(const char *filename, int flags)
{
   ALLEGRO_FILE *fp;
   ALLEGRO_BITMAP *bmp;

   ALLEGRO_ASSERT(filename);

   fp = al_fopen(filename, "rb");
   if (!fp)
      return NULL;

   bmp = _al_load_webp_f(fp, flags);

   al_fclose(fp);

   return bmp;
}




/*****************************************************************************
 * Saving routines
 ****************************************************************************/


bool _al_save_webp_f(ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp)
{

   ALLEGRO_LOCKED_REGION *lock;
   bool ret = false;

   lock = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
      ALLEGRO_LOCK_READONLY);
   if (!lock)
      return false;

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


bool _al_save_webp(const char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FILE *fp;
   bool retsave;
   bool retclose;

   ALLEGRO_ASSERT(filename);
   ALLEGRO_ASSERT(bmp);

   fp = al_fopen(filename, "wb");
   if (!fp) {
      ALLEGRO_ERROR("Unable to open file %s for writing\n", filename);
      return false;
   }

   retsave = _al_save_webp_f(fp, bmp);
   if (!retsave) {
      ALLEGRO_ERROR("Unable to store WebP bitmap in file %s\n", filename);
   }
   retclose = al_fclose(fp);
   if (!retclose) {
      ALLEGRO_ERROR("Unable to close file %s\n", filename);
   }

   return retsave && retclose;
}

/* vim: set sts=3 sw=3 et: */
