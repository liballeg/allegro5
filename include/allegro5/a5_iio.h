#ifndef A5_IIO_H
#define A5_IIO_H


typedef ALLEGRO_BITMAP *(*IIO_LOADER_FUNCTION)(AL_CONST char *filename);


AL_FUNC(bool, iio_init, (void));
AL_FUNC(bool, iio_add_loader, (AL_CONST char *ext, IIO_LOADER_FUNCTION function));
AL_FUNC(ALLEGRO_BITMAP *, iio_load, (AL_CONST char *filename));


#endif // A5_IIO_H

