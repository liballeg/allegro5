#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_image.h"
#include "allegro5/internal/aintern_image_cfg.h"


/* globals */
static bool iio_inited = false;


/* Function: al_init_image_addon
 */
bool al_init_image_addon(void)
{
   int success;

   if (iio_inited)
      return true;

   success = 0;

   success |= al_register_bitmap_loader(".pcx", _al_load_pcx);
   success |= al_register_bitmap_saver(".pcx", _al_save_pcx);
   success |= al_register_bitmap_loader_f(".pcx", _al_load_pcx_f);
   success |= al_register_bitmap_saver_f(".pcx", _al_save_pcx_f);
   success |= al_register_bitmap_identifier(".pcx", _al_identify_pcx);

   success |= al_register_bitmap_loader(".bmp", _al_load_bmp);
   success |= al_register_bitmap_saver(".bmp", _al_save_bmp);
   success |= al_register_bitmap_loader_f(".bmp", _al_load_bmp_f);
   success |= al_register_bitmap_saver_f(".bmp", _al_save_bmp_f);
   success |= al_register_bitmap_identifier(".bmp", _al_identify_bmp);

   success |= al_register_bitmap_loader(".tga", _al_load_tga);
   success |= al_register_bitmap_saver(".tga", _al_save_tga);
   success |= al_register_bitmap_loader_f(".tga", _al_load_tga_f);
   success |= al_register_bitmap_saver_f(".tga", _al_save_tga_f);
   success |= al_register_bitmap_identifier(".tga", _al_identify_tga);

   success |= al_register_bitmap_loader(".dds", _al_load_dds);
   success |= al_register_bitmap_loader_f(".dds", _al_load_dds_f);
   success |= al_register_bitmap_identifier(".dds", _al_identify_dds);

   /* Even if we don't have libpng or libjpeg we most likely have a
    * native reader for those instead so always identify them.
    */
   success |= al_register_bitmap_identifier(".png", _al_identify_png);
   success |= al_register_bitmap_identifier(".jpg", _al_identify_jpg);

/* ALLEGRO_CFG_IIO_HAVE_* is sufficient to know that the library
   should be used. i.e., ALLEGRO_CFG_IIO_HAVE_GDIPLUS and
   ALLEGRO_CFG_IIO_HAVE_PNG will never both be set. */

#ifdef ALLEGRO_CFG_IIO_HAVE_GDIPLUS
   {
      char const *extensions[] = {".tif", ".tiff", ".jpg", ".jpeg", ".gif",
         ".png", NULL};
      int i;

      if (_al_init_gdiplus()) {
         for (i = 0; extensions[i]; i++) {
            success |= al_register_bitmap_loader(extensions[i], _al_load_gdiplus_bitmap);
            success |= al_register_bitmap_loader_f(extensions[i], _al_load_gdiplus_bitmap_f);
            success |= al_register_bitmap_saver(extensions[i], _al_save_gdiplus_bitmap);
         }

         success |= al_register_bitmap_saver_f(".tif", _al_save_gdiplus_tif_f);
         success |= al_register_bitmap_saver_f(".tiff", _al_save_gdiplus_tif_f);
         success |= al_register_bitmap_saver_f(".gif", _al_save_gdiplus_gif_f);
         success |= al_register_bitmap_saver_f(".png", _al_save_gdiplus_png_f);
         success |= al_register_bitmap_saver_f(".jpg", _al_save_gdiplus_jpg_f);
         success |= al_register_bitmap_saver_f(".jpeg", _al_save_gdiplus_jpg_f);
      }
   }
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
   success |= al_register_bitmap_loader(".png", _al_load_png);
   success |= al_register_bitmap_saver(".png", _al_save_png);
   success |= al_register_bitmap_loader_f(".png", _al_load_png_f);
   success |= al_register_bitmap_saver_f(".png", _al_save_png_f);
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
   success |= al_register_bitmap_loader(".jpg", _al_load_jpg);
   success |= al_register_bitmap_saver(".jpg", _al_save_jpg);
   success |= al_register_bitmap_loader_f(".jpg", _al_load_jpg_f);
   success |= al_register_bitmap_saver_f(".jpg", _al_save_jpg_f);

   success |= al_register_bitmap_loader(".jpeg", _al_load_jpg);
   success |= al_register_bitmap_saver(".jpeg", _al_save_jpg);
   success |= al_register_bitmap_loader_f(".jpeg", _al_load_jpg_f);
   success |= al_register_bitmap_saver_f(".jpeg", _al_save_jpg_f);
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_WEBP
   success |= al_register_bitmap_loader(".webp", _al_load_webp);
   success |= al_register_bitmap_saver(".webp", _al_save_webp);
   success |= al_register_bitmap_loader_f(".webp", _al_load_webp_f);
   success |= al_register_bitmap_saver_f(".webp", _al_save_webp_f);
   success |= al_register_bitmap_identifier(".webp", _al_identify_webp);
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_FREEIMAGE
   {
      char const *extensions[] = {".ico", ".jng", ".koala", ".lbm",
         ".iff", ".mng", ".pbm", ".pcd", ".pgm", ".ppm", ".ras", ".wbmp",
         ".psd", ".cut", ".xpm", ".hdr", ".fax", ".sgi", ".exr", ".raw",
         ".j2k", ".jp2", ".jpf", ".jpm", ".jpx", ".mj2", ".pfm", ".pict",
         ".jxr", ".jbg", NULL};
      int i;

      if (_al_init_fi()) {
         for (i = 0; extensions[i]; i++) {
            success |= al_register_bitmap_loader(extensions[i], _al_load_fi_bitmap);
            success |= al_register_bitmap_loader_f(extensions[i], _al_load_fi_bitmap_f);
            success |= al_register_bitmap_identifier(extensions[i], _al_identify_fi);
         }
      }
   }
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_ANDROID
   {
      char const *extensions[] = {".webp", ".jpg", ".jpeg", ".ico", ".gif",
         ".wbmp", ".png", NULL};
      int i;


      for (i = 0; extensions[i]; i++) {
         success |= al_register_bitmap_loader(extensions[i], _al_load_android_bitmap);
         success |= al_register_bitmap_loader_f(extensions[i], _al_load_android_bitmap_f);
         //success |= al_register_bitmap_saver(extensions[i], _al_save_android_bitmap);
      }
      success |= al_register_bitmap_identifier(".webp", _al_identify_webp);
   }
#endif

#ifdef ALLEGRO_CFG_WANT_NATIVE_IMAGE_LOADER
#ifdef ALLEGRO_IPHONE
   {
      char const *extensions[] = {".tif", ".tiff", ".jpg", ".jpeg", ".gif",
         ".png", ".BMPf", ".ico", ".cur", ".xbm", NULL};
      int i;
      for (i = 0; extensions[i]; i++) {
         success |= al_register_bitmap_loader(extensions[i], _al_iphone_load_image);
         success |= al_register_bitmap_loader_f(extensions[i], _al_iphone_load_image_f);
      }
   }
#endif
#endif

   if (success)
      iio_inited = true;

   _al_add_exit_func(al_shutdown_image_addon, "al_shutdown_image_addon");

   return success;
}


/* Function: al_is_image_addon_initialized
 */
bool al_is_image_addon_initialized(void)
{
   return iio_inited;
}


/* Function: al_shutdown_image_addon
 */
void al_shutdown_image_addon(void)
{
   iio_inited = false;
}


/* Function: al_get_allegro_image_version
 */
uint32_t al_get_allegro_image_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* vim: set sts=3 sw=3 et: */
