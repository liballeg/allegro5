#ifndef __al_included_allegro_image_h
#define __al_included_allegro_image_h

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_IIO_SRC
         #define _ALLEGRO_IIO_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_IIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_IIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_IIO_FUNC(type, name, args)      _ALLEGRO_IIO_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_IIO_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_IIO_FUNC(type, name, args)      extern _ALLEGRO_IIO_DLL type name args
#else
   #define ALLEGRO_IIO_FUNC      AL_FUNC
#endif


#ifdef __cplusplus
extern "C" {
#endif


ALLEGRO_IIO_FUNC(bool, al_init_image_addon, (void));
ALLEGRO_IIO_FUNC(void, al_shutdown_image_addon, (void));
ALLEGRO_IIO_FUNC(uint32_t, al_get_allegro_image_version, (void));

/* Format specific functions */
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_pcx, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bmp, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_tga, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_png, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_jpg, (const char *filename));


ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_pcx_f, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bmp_f, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_tga_f, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_png_f, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_jpg_f, (ALLEGRO_FILE *fp));


ALLEGRO_IIO_FUNC(bool, al_save_pcx, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_bmp, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_tga, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_png, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_jpg, (const char *filename, ALLEGRO_BITMAP *bmp));


ALLEGRO_IIO_FUNC(bool, al_save_pcx_f, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_bmp_f, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_tga_f, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_png_f, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_jpg_f, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));


#ifdef __cplusplus
}
#endif

#endif

