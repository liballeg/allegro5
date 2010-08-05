#ifndef __al_included_allegro5_bitmap_io_h
#define __al_included_allegro5_bitmap_io_h

#include "allegro5/bitmap.h"
#include "allegro5/file.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef ALLEGRO_BITMAP *(*ALLEGRO_IIO_LOADER_FUNCTION)(const char *filename);
typedef ALLEGRO_BITMAP *(*ALLEGRO_IIO_FS_LOADER_FUNCTION)(ALLEGRO_FILE *fp);
typedef bool (*ALLEGRO_IIO_SAVER_FUNCTION)(const char *filename, ALLEGRO_BITMAP *bitmap);
typedef bool (*ALLEGRO_IIO_FS_SAVER_FUNCTION)(ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bitmap);

AL_FUNC(bool, al_register_bitmap_loader, (const char *ext, ALLEGRO_IIO_LOADER_FUNCTION loader));
AL_FUNC(bool, al_register_bitmap_saver, (const char *ext, ALLEGRO_IIO_SAVER_FUNCTION saver));
AL_FUNC(bool, al_register_bitmap_loader_f, (const char *ext, ALLEGRO_IIO_FS_LOADER_FUNCTION fs_loader));
AL_FUNC(bool, al_register_bitmap_saver_f, (const char *ext, ALLEGRO_IIO_FS_SAVER_FUNCTION fs_saver));
AL_FUNC(ALLEGRO_BITMAP *, al_load_bitmap, (const char *filename));
AL_FUNC(ALLEGRO_BITMAP *, al_load_bitmap_f, (ALLEGRO_FILE *fp, const char *ident));
AL_FUNC(bool, al_save_bitmap, (const char *filename, ALLEGRO_BITMAP *bitmap));
AL_FUNC(bool, al_save_bitmap_f, (ALLEGRO_FILE *fp, const char *ident, ALLEGRO_BITMAP *bitmap));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
