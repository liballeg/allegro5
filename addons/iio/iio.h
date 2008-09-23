#ifndef IIO_H
#define IIO_H



#include "allegro5/internal/aintern_iio_cfg.h"



typedef struct PalEntry {
   int r, g, b, a;
} PalEntry;


ALLEGRO_BITMAP *iio_load_pcx(AL_CONST char *filename);
ALLEGRO_BITMAP *iio_load_bmp(AL_CONST char *filename);
ALLEGRO_BITMAP *iio_load_tga(AL_CONST char *filename);
ALLEGRO_BITMAP *iio_load_png(AL_CONST char *filename);
ALLEGRO_BITMAP *iio_load_jpg(AL_CONST char *filename);


int iio_save_pcx(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);
int iio_save_bmp(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);
int iio_save_tga(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);
int iio_save_png(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);
int iio_save_jpg(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);



/* FIXME: Not sure if these should be made accessible. Hide them for now. */

/* _png_screen_gamma is slightly overloaded (sorry):
 *
 * A value of 0.0 means: Don't do any gamma correction in load_png()
 * and load_memory_png().  This meaning was introduced in v1.4.
 *
 * A value of -1.0 means: Use the value from the environment variable
 * SCREEN_GAMMA (if available), otherwise fallback to a value of 2.2
 * (a good guess for PC monitors, and the value for sRGB colourspace).
 * This is the default.
 *
 * Otherwise, the value of _png_screen_gamma is taken as-is.
 */
extern double _png_screen_gamma;

/* Choose zlib compression level for saving file.
 * Default is Z_BEST_COMPRESSION.
 */
extern int _png_compression_level;



#endif // IIO_H

