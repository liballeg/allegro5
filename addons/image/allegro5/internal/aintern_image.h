#ifdef ALLEGRO_IPHONE
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_iphone_load_image, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_iphone_load_image_f, (ALLEGRO_FILE *f));
#endif

#ifdef ALLEGRO_MACOSX
ALLEGRO_IIO_FUNC(bool, _al_osx_register_image_loader, (void));
#endif
