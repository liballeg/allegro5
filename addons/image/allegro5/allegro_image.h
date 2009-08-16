#ifndef __al_included_allegro_image_h
#define __al_included_allegro_image_h

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

typedef ALLEGRO_BITMAP *(*ALLEGRO_IIO_LOADER_FUNCTION)(const char *filename);
typedef ALLEGRO_BITMAP *(*ALLEGRO_IIO_FS_LOADER_FUNCTION)(ALLEGRO_FILE *pf);
typedef bool (*ALLEGRO_IIO_SAVER_FUNCTION)(const char *filename, ALLEGRO_BITMAP *bitmap);
typedef bool (*ALLEGRO_IIO_FS_SAVER_FUNCTION)(ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bitmap);


/* XXX the *_entry names are terrible */
A5_IIO_FUNC(bool, al_init_image_addon, (void));
A5_IIO_FUNC(void, al_shutdown_image_addon, (void));
A5_IIO_FUNC(bool, al_register_bitmap_loader, (const char *ext, ALLEGRO_IIO_LOADER_FUNCTION loader));
A5_IIO_FUNC(bool, al_register_bitmap_saver, (const char *ext, ALLEGRO_IIO_SAVER_FUNCTION saver));
A5_IIO_FUNC(bool, al_register_bitmap_loader_entry, (const char *ext, ALLEGRO_IIO_FS_LOADER_FUNCTION fs_loader));
A5_IIO_FUNC(bool, al_register_bitmap_saver_entry, (const char *ext, ALLEGRO_IIO_FS_SAVER_FUNCTION fs_saver));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bitmap, (const char *filename));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bitmap_entry, (ALLEGRO_FILE *pf, const char *ident));
A5_IIO_FUNC(bool, al_save_bitmap, (const char *filename, ALLEGRO_BITMAP *bitmap));
A5_IIO_FUNC(bool, al_save_bitmap_entry, (ALLEGRO_FILE *pf, const char *ident, ALLEGRO_BITMAP *bitmap));

/* Format specific functions */
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_pcx, (const char *filename));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bmp, (const char *filename));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_tga, (const char *filename));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_png, (const char *filename));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_jpg, (const char *filename));


A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_pcx_entry, (ALLEGRO_FILE *pf));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bmp_entry, (ALLEGRO_FILE *pf));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_tga_entry, (ALLEGRO_FILE *pf));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_png_entry, (ALLEGRO_FILE *pf));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_load_jpg_entry, (ALLEGRO_FILE *pf));


A5_IIO_FUNC(bool, al_save_pcx, (const char *filename, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_bmp, (const char *filename, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_tga, (const char *filename, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_png, (const char *filename, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_jpg, (const char *filename, ALLEGRO_BITMAP *bmp));


A5_IIO_FUNC(bool, al_save_pcx_entry, (ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_bmp_entry, (ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_tga_entry, (ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_png_entry, (ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp));
A5_IIO_FUNC(bool, al_save_jpg_entry, (ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp));


#ifdef __cplusplus
}
#endif

#endif

