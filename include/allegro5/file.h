#ifndef __al_included_allegro5_file_h
#define __al_included_allegro5_file_h

#include "allegro5/base.h"
#include "allegro5/path.h"
#include "allegro5/utf8.h"

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_FILE
 */
typedef struct A5O_FILE A5O_FILE;


/* Type: A5O_FILE_INTERFACE
 */
typedef struct A5O_FILE_INTERFACE
{
   AL_METHOD(void *,  fi_fopen, (const char *path, const char *mode));
   AL_METHOD(bool,    fi_fclose, (A5O_FILE *handle));
   AL_METHOD(size_t,  fi_fread, (A5O_FILE *f, void *ptr, size_t size));
   AL_METHOD(size_t,  fi_fwrite, (A5O_FILE *f, const void *ptr, size_t size));
   AL_METHOD(bool,    fi_fflush, (A5O_FILE *f));
   AL_METHOD(int64_t, fi_ftell, (A5O_FILE *f));
   AL_METHOD(bool,    fi_fseek, (A5O_FILE *f, int64_t offset, int whence));
   AL_METHOD(bool,    fi_feof, (A5O_FILE *f));
   AL_METHOD(int,     fi_ferror, (A5O_FILE *f));
   AL_METHOD(const char *, fi_ferrmsg, (A5O_FILE *f));
   AL_METHOD(void,    fi_fclearerr, (A5O_FILE *f));
   AL_METHOD(int,     fi_fungetc, (A5O_FILE *f, int c));
   AL_METHOD(off_t,   fi_fsize, (A5O_FILE *f));
} A5O_FILE_INTERFACE;


/* Enum: A5O_SEEK
 */
typedef enum A5O_SEEK
{
   A5O_SEEK_SET = 0,
   A5O_SEEK_CUR,
   A5O_SEEK_END
} A5O_SEEK;


/* The basic operations. */
AL_FUNC(A5O_FILE*, al_fopen, (const char *path, const char *mode));
AL_FUNC(A5O_FILE*, al_fopen_interface, (const A5O_FILE_INTERFACE *vt, const char *path, const char *mode));
AL_FUNC(A5O_FILE*, al_create_file_handle, (const A5O_FILE_INTERFACE *vt, void *userdata));
AL_FUNC(bool, al_fclose, (A5O_FILE *f));
AL_FUNC(size_t, al_fread, (A5O_FILE *f, void *ptr, size_t size));
AL_FUNC(size_t, al_fwrite, (A5O_FILE *f, const void *ptr, size_t size));
AL_FUNC(bool, al_fflush, (A5O_FILE *f));
AL_FUNC(int64_t, al_ftell, (A5O_FILE *f));
AL_FUNC(bool, al_fseek, (A5O_FILE *f, int64_t offset, int whence));
AL_FUNC(bool, al_feof, (A5O_FILE *f));
AL_FUNC(int, al_ferror, (A5O_FILE *f));
AL_FUNC(const char *, al_ferrmsg, (A5O_FILE *f));
AL_FUNC(void, al_fclearerr, (A5O_FILE *f));
AL_FUNC(int, al_fungetc, (A5O_FILE *f, int c));
AL_FUNC(int64_t, al_fsize, (A5O_FILE *f));

/* Convenience functions. */
AL_FUNC(int, al_fgetc, (A5O_FILE *f));
AL_FUNC(int, al_fputc, (A5O_FILE *f, int c));
AL_FUNC(int16_t, al_fread16le, (A5O_FILE *f));
AL_FUNC(int16_t, al_fread16be, (A5O_FILE *f));
AL_FUNC(size_t, al_fwrite16le, (A5O_FILE *f, int16_t w));
AL_FUNC(size_t, al_fwrite16be, (A5O_FILE *f, int16_t w));
AL_FUNC(int32_t, al_fread32le, (A5O_FILE *f));
AL_FUNC(int32_t, al_fread32be, (A5O_FILE *f));
AL_FUNC(size_t, al_fwrite32le, (A5O_FILE *f, int32_t l));
AL_FUNC(size_t, al_fwrite32be, (A5O_FILE *f, int32_t l));
AL_FUNC(char*, al_fgets, (A5O_FILE *f, char * const p, size_t max));
AL_FUNC(A5O_USTR *, al_fget_ustr, (A5O_FILE *f));
AL_FUNC(int, al_fputs, (A5O_FILE *f, const char *p));
AL_FUNC(int, al_fprintf, (A5O_FILE *f, const char *format, ...));
AL_FUNC(int, al_vfprintf, (A5O_FILE *f, const char* format, va_list args));

/* Specific to stdio backend. */
AL_FUNC(A5O_FILE*, al_fopen_fd, (int fd, const char *mode));
AL_FUNC(A5O_FILE*, al_make_temp_file, (const char *tmpl,
      A5O_PATH **ret_path));

/* Specific to slices. */
AL_FUNC(A5O_FILE*, al_fopen_slice, (A5O_FILE *fp,
      size_t initial_size, const char *mode));

/* Thread-local state. */
AL_FUNC(const A5O_FILE_INTERFACE *, al_get_new_file_interface, (void));
AL_FUNC(void, al_set_new_file_interface, (const A5O_FILE_INTERFACE *
      file_interface));
AL_FUNC(void, al_set_standard_file_interface, (void));

/* A5O_FILE field accessors */
AL_FUNC(void *, al_get_file_userdata, (A5O_FILE *f));


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
