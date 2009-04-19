/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      File I/O.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro5.h"

/* enable large file support in gcc/glibc */
#if defined ALLEGRO_HAVE_FTELLO && defined ALLEGRO_HAVE_FSEEKO
   #define _LARGEFILE_SOURCE
   #define _LARGEFILE_SOURCE64
   #define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_memory.h"

#ifdef ALLEGRO_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif


typedef struct ALLEGRO_FILE_STDIO
{
   ALLEGRO_FILE file;               /* must be first */
   FILE *fp;
   ALLEGRO_PATH *remove_on_close;   /* may be NULL */
} ALLEGRO_FILE_STDIO;


/* forward declaration */
const struct ALLEGRO_FILE_INTERFACE _al_file_interface_stdio;


static FILE *get_fp(ALLEGRO_FILE *f)
{
   if (f)
      return ((ALLEGRO_FILE_STDIO *)f)->fp;
   else
      return NULL;
}


/* Function: al_fopen_fd
 */
ALLEGRO_FILE *al_fopen_fd(int fd, const char *mode)
{
   ALLEGRO_FILE_STDIO *f;
   FILE *fp;

   /* The fd should remain open if this function fails in either way. */

   f = _AL_MALLOC(sizeof(*f));
   if (!f)
      return NULL;

   fp = fdopen(fd, mode);
   if (!fp) {
      _AL_FREE(f);
      return NULL;
   }

   f->file.vtable = &_al_file_interface_stdio;
   f->fp = fp;
   f->remove_on_close = NULL;
   return (ALLEGRO_FILE *)f;
}


static ALLEGRO_FILE *file_stdio_fopen(const char *path, const char *mode)
{
   FILE *fp;
   ALLEGRO_FILE_STDIO *f;

   fp = fopen(path, mode);
   if (!fp)
      return NULL;

   f = _AL_MALLOC(sizeof(*f));
   if (!f) {
      fclose(fp);
      return NULL;
   }

   f->file.vtable = &_al_file_interface_stdio;
   f->fp = fp;
   f->remove_on_close = NULL;
   return (ALLEGRO_FILE *)f;
}


static void file_stdio_fclose(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_STDIO *fstd = (ALLEGRO_FILE_STDIO *)f;

   if (f) {
      fclose(get_fp(f));

      /* A temporary file may need to be automatically removed. */
      if (fstd->remove_on_close) {
         unlink(al_path_to_string(fstd->remove_on_close,
            ALLEGRO_NATIVE_PATH_SEP));
         al_path_free(fstd->remove_on_close);
      }

      _AL_FREE(f);
   }
}


static size_t file_stdio_fread(ALLEGRO_FILE *f, void *ptr, size_t size)
{
   size_t ret;

   ret = fread(ptr, 1, size, get_fp(f));
   if (ret < size) {
      al_set_errno(errno);
   }

   return ret;
}


static size_t file_stdio_fwrite(ALLEGRO_FILE *f, const void *ptr, size_t size)
{
   size_t ret;

   ret = fwrite(ptr, 1, size, get_fp(f));
   if (ret < size) {
      al_set_errno(errno);
   }

   return ret;
}


static bool file_stdio_fflush(ALLEGRO_FILE *f)
{
   FILE *fp = get_fp(f);

   if (fflush(fp) == EOF) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static int64_t file_stdio_ftell(ALLEGRO_FILE *f)
{
   FILE *fp = get_fp(f);
   int64_t ret;

#ifdef ALLEGRO_HAVE_FTELLO
   ret = ftello(fp);
#else
   ret = ftell(fp);
#endif
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}


static bool file_stdio_fseek(ALLEGRO_FILE *f, int64_t offset,
   int whence)
{
   FILE *fp = get_fp(f);
   int rc;

   switch (whence) {
      case ALLEGRO_SEEK_SET: whence = SEEK_SET; break;
      case ALLEGRO_SEEK_CUR: whence = SEEK_CUR; break;
      case ALLEGRO_SEEK_END: whence = SEEK_END; break;
   }

#ifdef ALLEGRO_HAVE_FSEEKO
   rc = fseeko(fp, offset, whence);
#else
   rc = fseek(fp, offset, whence);
#endif

   if (rc == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static bool file_stdio_feof(ALLEGRO_FILE *f)
{
   return feof(get_fp(f));
}


static bool file_stdio_ferror(ALLEGRO_FILE *f)
{
   return ferror(get_fp(f));
}


static int file_stdio_fungetc(ALLEGRO_FILE *f, int c)
{
   return ungetc(c, get_fp(f));
}


static off_t file_stdio_fsize(ALLEGRO_FILE *f)
{
   int64_t old_pos;
   int64_t new_pos;

   old_pos = file_stdio_ftell(f);
   if (old_pos == -1)
      return -1;

   if (!file_stdio_fseek(f, 0, ALLEGRO_SEEK_END))
      return -1;

   new_pos = file_stdio_ftell(f);
   if (new_pos == -1)
      return -1;

   if (!file_stdio_fseek(f, old_pos, ALLEGRO_SEEK_SET))
      return -1;

   return new_pos;
}


const struct ALLEGRO_FILE_INTERFACE _al_file_interface_stdio =
{
   file_stdio_fopen,
   file_stdio_fclose,
   file_stdio_fread,
   file_stdio_fwrite,
   file_stdio_fflush,
   file_stdio_ftell,
   file_stdio_fseek,
   file_stdio_feof,
   file_stdio_ferror,
   file_stdio_fungetc,
   file_stdio_fsize
};


#define MAX_MKTEMP_TRIES 1000

static void mktemp_replace_XX(const char *template, char *dst)
{
   static const char chars[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
   size_t len = strlen(template);
   unsigned i;

   for (i=0; i<len; i++) {
      if (template[i] != 'X') {
         dst[i] = template[i];
      }
      else {
         /* XXX _al_rand is not seed properly yet */
         /* -1 to avoid the NUL terminator. */
         dst[i] = chars[_al_rand() % (sizeof(chars) - 1)];
      }
   }

   dst[i] = '\0';
}


/* Function: al_make_temp_file
 */
ALLEGRO_FILE *al_make_temp_file(const char *template, int remove_when,
   ALLEGRO_PATH **ret_path)
{
   ALLEGRO_PATH *path;
   ALLEGRO_FILE *f;
   char filename[PATH_MAX];
   int fd;
   int i;

   path = al_get_standard_path(ALLEGRO_TEMP_PATH);
   if (!path) {
      al_path_free(path);
      return NULL;
   }

   /* Ensure that the path is absolute, otherwise if
    * ALLEGRO_MAKE_TEMP_REMOVE_ON_CLOSE is set, and the working directory is
    * changed, we could end up removing the wrong file when al_fclose() is
    * called.
    */
   if (!al_path_make_absolute(path)) {
      al_path_free(path);
      return NULL;
   }

   for (i=0; i<MAX_MKTEMP_TRIES; ++i) {
      mktemp_replace_XX(template, filename);
      al_path_set_filename(path, filename);

#ifndef ALLEGRO_MSVC
      fd = open(al_path_to_string(path, ALLEGRO_NATIVE_PATH_SEP),
         O_EXCL | O_CREAT | O_RDWR, S_IRWXU);
#else
      fd = open(al_path_to_string(path, ALLEGRO_NATIVE_PATH_SEP),
         O_EXCL | O_CREAT | O_RDWR, _S_IWRITE | _S_IREAD);
#endif

      if (fd != -1)
         break;
   }

   if (fd == -1) {
      al_path_free(path);
      return NULL;
   }

   f = al_fopen_fd(fd, "rb+");
   if (!f) {
      close(fd);
      unlink(al_path_to_string(path, ALLEGRO_NATIVE_PATH_SEP));
      al_path_free(path);
      return NULL;
   }

   switch (remove_when) {
      case ALLEGRO_MAKE_TEMP_REMOVE_NEVER:
         if (ret_path)
            *ret_path = path;
         else
            al_path_free(path);
         break;

      case ALLEGRO_MAKE_TEMP_REMOVE_ON_OPEN:
         unlink(al_path_to_string(path, ALLEGRO_NATIVE_PATH_SEP));
         al_path_free(path);
         if (ret_path)
            *ret_path = NULL;
         break;

      case ALLEGRO_MAKE_TEMP_REMOVE_ON_CLOSE:
         /* f takes ownership of path so don't free it. */
         ((ALLEGRO_FILE_STDIO *)f)->remove_on_close = path;
         if (ret_path)
            *ret_path = al_path_clone(path);
         break;
   }

   return f;
}


/* vim: set sts=3 sw=3 et: */
