#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern.h"
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

   success |= al_register_bitmap_loader(".pcx", al_load_pcx);
   success |= al_register_bitmap_saver(".pcx", al_save_pcx);
   success |= al_register_bitmap_loader_f(".pcx", al_load_pcx_f);
   success |= al_register_bitmap_saver_f(".pcx", al_save_pcx_f);

   success |= al_register_bitmap_loader(".bmp", al_load_bmp);
   success |= al_register_bitmap_saver(".bmp", al_save_bmp);
   success |= al_register_bitmap_loader_f(".bmp", al_load_bmp_f);
   success |= al_register_bitmap_saver_f(".bmp", al_save_bmp_f);

   success |= al_register_bitmap_loader(".tga", al_load_tga);
   success |= al_register_bitmap_saver(".tga", al_save_tga);
   success |= al_register_bitmap_loader_f(".tga", al_load_tga_f);
   success |= al_register_bitmap_saver_f(".tga", al_save_tga_f);

#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
   success |= al_register_bitmap_loader(".png", al_load_png);
   success |= al_register_bitmap_saver(".png", al_save_png);
   success |= al_register_bitmap_loader_f(".png", al_load_png_f);
   success |= al_register_bitmap_saver_f(".png", al_save_png_f);
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
   success |= al_register_bitmap_loader(".jpg", al_load_jpg);
   success |= al_register_bitmap_saver(".jpg", al_save_jpg);
   success |= al_register_bitmap_loader_f(".jpg", al_load_jpg_f);
   success |= al_register_bitmap_saver_f(".jpg", al_save_jpg_f);

   success |= al_register_bitmap_loader(".jpeg", al_load_jpg);
   success |= al_register_bitmap_saver(".jpeg", al_save_jpg);
   success |= al_register_bitmap_loader_f(".jpeg", al_load_jpg_f);
   success |= al_register_bitmap_saver_f(".jpeg", al_save_jpg_f);
#endif

#ifdef ALLEGRO_CFG_WANT_NATIVE_APPLE_IMAGE_LOADER
#ifdef ALLEGRO_IPHONE
   char const *extensions[] = {".tif", ".tiff", ".jpg", ".jpeg", ".gif",
      ".png", ".BMPf", ".ico", ".cur", ".xbm", NULL};
   for (int i = 0; extensions[i]; i++) {
      success |= al_register_bitmap_loader(extensions[i], _al_iphone_load_image);
      success |= al_register_bitmap_loader_f(extensions[i], _al_iphone_load_image_f);
   }
#endif

#ifdef ALLEGRO_MACOSX
   success |= _al_osx_register_image_loader();
#endif
#endif

   if (success)
      iio_inited = true;

   _al_add_exit_func(al_shutdown_image_addon, "al_shutdown_image_addon");

   return success;
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
