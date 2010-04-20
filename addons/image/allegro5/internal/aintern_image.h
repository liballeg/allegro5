#ifdef ALLEGRO_CFG_WANT_NATIVE_APPLE_IMAGE_LOADER
#ifdef ALLEGRO_IPHONE
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_iphone_load_image, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_iphone_load_image_f, (ALLEGRO_FILE *f));
#endif

#ifdef ALLEGRO_MACOSX
ALLEGRO_IIO_FUNC(bool, _al_osx_register_image_loader, (void));
#endif
#endif

ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_pcx, (const char *filename));
ALLEGRO_IIO_FUNC(bool, _al_save_pcx, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_pcx_f, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_save_pcx_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));

ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_bmp, (const char *filename));
ALLEGRO_IIO_FUNC(bool, _al_save_bmp, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_bmp_f, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_save_bmp_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));

ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_tga, (const char *filename));
ALLEGRO_IIO_FUNC(bool, _al_save_tga, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_tga_f, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_save_tga_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
   
#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_png, (const char *filename));
ALLEGRO_IIO_FUNC(bool, _al_save_png, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_png_f, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_save_png_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_jpg, (const char *filename));
ALLEGRO_IIO_FUNC(bool, _al_save_jpg, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_jpg_f, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_save_jpg_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
#endif

