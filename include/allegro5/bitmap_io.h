#ifndef __al_included_allegro5_bitmap_io_h
#define __al_included_allegro5_bitmap_io_h

#include "allegro5/bitmap.h"
#include "allegro5/file.h"

#ifdef __cplusplus
   extern "C" {
#endif

/*
 * Bitmap loader flag
 */
enum {
   A5O_KEEP_BITMAP_FORMAT       = 0x0002,   /* was a bitmap flag in 5.0 */
   A5O_NO_PREMULTIPLIED_ALPHA   = 0x0200,   /* was a bitmap flag in 5.0 */
   A5O_KEEP_INDEX               = 0x0800
};

typedef A5O_BITMAP *(*A5O_IIO_LOADER_FUNCTION)(const char *filename, int flags);
typedef A5O_BITMAP *(*A5O_IIO_FS_LOADER_FUNCTION)(A5O_FILE *fp, int flags);
typedef bool (*A5O_IIO_SAVER_FUNCTION)(const char *filename, A5O_BITMAP *bitmap);
typedef bool (*A5O_IIO_FS_SAVER_FUNCTION)(A5O_FILE *fp, A5O_BITMAP *bitmap);
typedef bool (*A5O_IIO_IDENTIFIER_FUNCTION)(A5O_FILE *f);

AL_FUNC(bool, al_register_bitmap_loader, (const char *ext, A5O_IIO_LOADER_FUNCTION loader));
AL_FUNC(bool, al_register_bitmap_saver, (const char *ext, A5O_IIO_SAVER_FUNCTION saver));
AL_FUNC(bool, al_register_bitmap_loader_f, (const char *ext, A5O_IIO_FS_LOADER_FUNCTION fs_loader));
AL_FUNC(bool, al_register_bitmap_saver_f, (const char *ext, A5O_IIO_FS_SAVER_FUNCTION fs_saver));
AL_FUNC(bool, al_register_bitmap_identifier, (const char *ext,
   A5O_IIO_IDENTIFIER_FUNCTION identifier));
AL_FUNC(A5O_BITMAP *, al_load_bitmap, (const char *filename));
AL_FUNC(A5O_BITMAP *, al_load_bitmap_flags, (const char *filename, int flags));
AL_FUNC(A5O_BITMAP *, al_load_bitmap_f, (A5O_FILE *fp, const char *ident));
AL_FUNC(A5O_BITMAP *, al_load_bitmap_flags_f, (A5O_FILE *fp, const char *ident, int flags));
AL_FUNC(bool, al_save_bitmap, (const char *filename, A5O_BITMAP *bitmap));
AL_FUNC(bool, al_save_bitmap_f, (A5O_FILE *fp, const char *ident, A5O_BITMAP *bitmap));
AL_FUNC(char const *, al_identify_bitmap_f, (A5O_FILE *fp));
AL_FUNC(char const *, al_identify_bitmap, (char const *filename));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
