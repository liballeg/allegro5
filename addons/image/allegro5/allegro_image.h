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

typedef ALLEGRO_BITMAP *(*ALLEGRO_IIO_LOADER_FUNCTION)(const char *filename);
typedef ALLEGRO_BITMAP *(*ALLEGRO_IIO_FS_LOADER_FUNCTION)(ALLEGRO_FILE *fp);
typedef bool (*ALLEGRO_IIO_SAVER_FUNCTION)(const char *filename, ALLEGRO_BITMAP *bitmap);
typedef bool (*ALLEGRO_IIO_FS_SAVER_FUNCTION)(ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bitmap);


ALLEGRO_IIO_FUNC(bool, al_init_image_addon, (void));
ALLEGRO_IIO_FUNC(void, al_shutdown_image_addon, (void));
ALLEGRO_IIO_FUNC(bool, al_register_bitmap_loader, (const char *ext, ALLEGRO_IIO_LOADER_FUNCTION loader));
ALLEGRO_IIO_FUNC(bool, al_register_bitmap_saver, (const char *ext, ALLEGRO_IIO_SAVER_FUNCTION saver));
ALLEGRO_IIO_FUNC(bool, al_register_bitmap_loader_fp, (const char *ext, ALLEGRO_IIO_FS_LOADER_FUNCTION fs_loader));
ALLEGRO_IIO_FUNC(bool, al_register_bitmap_saver_fp, (const char *ext, ALLEGRO_IIO_FS_SAVER_FUNCTION fs_saver));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bitmap, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bitmap_fp, (ALLEGRO_FILE *fp, const char *ident));
ALLEGRO_IIO_FUNC(bool, al_save_bitmap, (const char *filename, ALLEGRO_BITMAP *bitmap));
ALLEGRO_IIO_FUNC(bool, al_save_bitmap_fp, (ALLEGRO_FILE *fp, const char *ident, ALLEGRO_BITMAP *bitmap));
ALLEGRO_IIO_FUNC(uint32_t, al_get_allegro_image_version, (void));

/* Format specific functions */
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_pcx, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bmp, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_tga, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_png, (const char *filename));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_jpg, (const char *filename));


ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_pcx_fp, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_bmp_fp, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_tga_fp, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_png_fp, (ALLEGRO_FILE *fp));
ALLEGRO_IIO_FUNC(ALLEGRO_BITMAP *, al_load_jpg_fp, (ALLEGRO_FILE *fp));


ALLEGRO_IIO_FUNC(bool, al_save_pcx, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_bmp, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_tga, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_png, (const char *filename, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_jpg, (const char *filename, ALLEGRO_BITMAP *bmp));


ALLEGRO_IIO_FUNC(bool, al_save_pcx_fp, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_bmp_fp, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_tga_fp, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_png_fp, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));
ALLEGRO_IIO_FUNC(bool, al_save_jpg_fp, (ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp));


#ifdef __cplusplus
}
#endif

#endif

