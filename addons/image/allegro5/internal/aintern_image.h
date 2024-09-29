#ifndef __al_included_allegro_aintern_image_h
#define __al_included_allegro_aintern_image_h

#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_image_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif 

#ifdef A5O_CFG_WANT_NATIVE_IMAGE_LOADER

#ifdef A5O_IPHONE
A5O_IIO_FUNC(A5O_BITMAP *, _al_iphone_load_image, (const char *filename, int flags));
A5O_IIO_FUNC(A5O_BITMAP *, _al_iphone_load_image_f, (A5O_FILE *f, int flags));
#endif

#ifdef A5O_MACOSX
A5O_IIO_FUNC(bool, _al_osx_register_image_loader, (void));
#endif

#endif

A5O_IIO_FUNC(A5O_BITMAP *, _al_load_pcx, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_pcx, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_pcx_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_pcx_f, (A5O_FILE *f, A5O_BITMAP *bmp));
A5O_IIO_FUNC(bool, _al_identify_pcx, (A5O_FILE *f));

A5O_IIO_FUNC(A5O_BITMAP *, _al_load_bmp, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_bmp, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_bmp_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_bmp_f, (A5O_FILE *f, A5O_BITMAP *bmp));
A5O_IIO_FUNC(bool, _al_identify_bmp, (A5O_FILE *f));


A5O_IIO_FUNC(A5O_BITMAP *, _al_load_tga, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_tga, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_tga_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_tga_f, (A5O_FILE *f, A5O_BITMAP *bmp));
A5O_IIO_FUNC(bool, _al_identify_tga, (A5O_FILE *f));

A5O_IIO_FUNC(A5O_BITMAP *, _al_load_dds, (const char *filename, int flags));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_dds_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_identify_dds, (A5O_FILE *f));

A5O_IIO_FUNC(bool, _al_identify_png, (A5O_FILE *f));
A5O_IIO_FUNC(bool, _al_identify_jpg, (A5O_FILE *f));
A5O_IIO_FUNC(bool, _al_identify_webp, (A5O_FILE *f));

#ifdef A5O_CFG_IIO_HAVE_FREEIMAGE
A5O_IIO_FUNC(bool, _al_init_fi, (void));
A5O_IIO_FUNC(void, _al_shutdown_fi, (void));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_fi_bitmap, (const char *filename, int flags));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_fi_bitmap_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_identify_fi, (A5O_FILE *f));
#endif

#ifdef A5O_CFG_IIO_HAVE_GDIPLUS
A5O_IIO_FUNC(bool, _al_init_gdiplus, (void));
A5O_IIO_FUNC(void, _al_shutdown_gdiplus, (void));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_gdiplus_bitmap, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_gdiplus_bitmap, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_gdiplus_bitmap_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_gdiplus_png_f, (A5O_FILE *f, A5O_BITMAP *bmp));
A5O_IIO_FUNC(bool, _al_save_gdiplus_jpg_f, (A5O_FILE *f, A5O_BITMAP *bmp));
A5O_IIO_FUNC(bool, _al_save_gdiplus_tif_f, (A5O_FILE *f, A5O_BITMAP *bmp));
A5O_IIO_FUNC(bool, _al_save_gdiplus_gif_f, (A5O_FILE *f, A5O_BITMAP *bmp));
#endif

#ifdef A5O_CFG_IIO_HAVE_ANDROID
A5O_BITMAP *_al_load_android_bitmap_f(A5O_FILE *fp, int flags);
A5O_BITMAP *_al_load_android_bitmap(const char *filename, int flags);
#endif

/* A5O_CFG_IIO_HAVE_PNG/JPG implies that "native" loaders aren't available. */

#ifdef A5O_CFG_IIO_HAVE_PNG
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_png, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_png, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_png_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_png_f, (A5O_FILE *f, A5O_BITMAP *bmp));
#endif

#ifdef A5O_CFG_IIO_HAVE_JPG
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_jpg, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_jpg, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_jpg_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_jpg_f, (A5O_FILE *f, A5O_BITMAP *bmp));
#endif

#ifdef A5O_CFG_IIO_HAVE_WEBP
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_webp, (const char *filename, int flags));
A5O_IIO_FUNC(bool, _al_save_webp, (const char *filename, A5O_BITMAP *bmp));
A5O_IIO_FUNC(A5O_BITMAP *, _al_load_webp_f, (A5O_FILE *f, int flags));
A5O_IIO_FUNC(bool, _al_save_webp_f, (A5O_FILE *f, A5O_BITMAP *bmp));
#endif

#ifdef __cplusplus
}
#endif 


#endif
