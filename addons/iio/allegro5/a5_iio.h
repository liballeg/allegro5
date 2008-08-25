#ifndef A5_IIO_H
#define A5_IIO_H

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_IIO_SRC
         #define _A5_IIO_DLL __declspec(dllexport)
      #else
         #define _A5_IIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_IIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_IIO_FUNC(type, name, args)      _A5_IIO_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_IIO_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_IIO_FUNC(type, name, args)      extern _A5_IIO_DLL type name args
#else
   #define A5_IIO_FUNC      AL_FUNC
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef ALLEGRO_BITMAP *(*IIO_LOADER_FUNCTION)(AL_CONST char *filename);


A5_IIO_FUNC(bool, al_iio_init, (void));
A5_IIO_FUNC(bool, al_iio_add_loader, (AL_CONST char *ext, IIO_LOADER_FUNCTION function));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_iio_load, (AL_CONST char *filename));

#ifdef __cplusplus
}
#endif

#endif // A5_IIO_H

