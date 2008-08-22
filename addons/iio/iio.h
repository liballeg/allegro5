#ifndef IIO_H
#define IIO_H


typedef struct PalEntry {
   int r, g, b, a;
} PalEntry;


ALLEGRO_BITMAP *iio_load_pcx(AL_CONST char *filename);
ALLEGRO_BITMAP *iio_load_bmp(AL_CONST char *filename);
ALLEGRO_BITMAP *iio_load_tga(AL_CONST char *filename);


int iio_save_pcx(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);
int iio_save_bmp(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);
int iio_save_tga(AL_CONST char *filename, ALLEGRO_BITMAP *bmp);


#endif // IIO_H

