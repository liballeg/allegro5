#ifndef __al_included_allegro_aintern_image_h
#define __al_included_allegro_aintern_image_h

#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_image_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef ALLEGRO_CFG_WANT_NATIVE_IMAGE_LOADER

#ifdef ALLEGRO_IPHONE
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_iphone_load_image, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_iphone_load_image_f, (ALLEGRO_FILE *f, int flags));
#endif

#endif

ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_pcx, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_pcx, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_pcx_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_pcx_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, _al_identify_pcx, (ALLEGRO_FILE *f));

ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_bmp, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_bmp, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_bmp_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_bmp_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, _al_identify_bmp, (ALLEGRO_FILE *f));


ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_tga, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_tga, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_tga_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_tga_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, _al_identify_tga, (ALLEGRO_FILE *f));

ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_dds, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_dds_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_identify_dds, (ALLEGRO_FILE *f));

ALLEGRO_IIO_FUNC(bool, _al_identify_png, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_identify_jpg, (ALLEGRO_FILE *f));
ALLEGRO_IIO_FUNC(bool, _al_identify_webp, (ALLEGRO_FILE *f));

#ifdef ALLEGRO_CFG_IIO_HAVE_FREEIMAGE
ALLEGRO_IIO_FUNC(bool, _al_init_fi, (void));
ALLEGRO_IIO_FUNC(void, _al_shutdown_fi, (void));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_fi_bitmap, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_fi_bitmap_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_identify_fi, (ALLEGRO_FILE *f));
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_GDIPLUS
ALLEGRO_IIO_FUNC(bool, _al_init_gdiplus, (void));
ALLEGRO_IIO_FUNC(void, _al_shutdown_gdiplus, (void));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_gdiplus_bitmap, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_gdiplus_bitmap, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_gdiplus_bitmap_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_gdiplus_png_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, _al_save_gdiplus_jpg_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, _al_save_gdiplus_tif_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, _al_save_gdiplus_gif_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_ANDROID
ALLEGRO_BITMAP *_al_load_android_bitmap_f(ALLEGRO_FILE *fp, int flags);
ALLEGRO_BITMAP *_al_load_android_bitmap(const char *filename, int flags);
#endif

/* ALLEGRO_CFG_IIO_HAVE_PNG/JPG implies that "native" loaders aren't available. */

#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_png, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_png, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_png_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_png_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_jpg, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_jpg, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_jpg_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_jpg_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_WEBP
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_webp, (const char *filename, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_webp, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, _al_load_webp_f, (ALLEGRO_FILE *f, int flags));
ALLEGRO_IIO_FUNC(bool, _al_save_webp_f, (ALLEGRO_FILE *f, ALLEGRO_BITMAP *bmp));
#endif

#ifdef __cplusplus
}
#endif


#endif
