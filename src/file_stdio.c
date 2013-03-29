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

#include "allegro5/allegro.h"

/* enable large file support in gcc/glibc */
#if defined ALLEGRO_HAVE_FTELLO && defined ALLEGRO_HAVE_FSEEKO
#ifndef _LARGEFILE_SOURCE
   #define _LARGEFILE_SOURCE
#endif
#ifndef _LARGEFILE_SOURCE64
   #define _LARGEFILE_SOURCE64
#endif
#ifndef _FILE_OFFSET_BITS
   #define _FILE_OFFSET_BITS 64
#endif
#endif

#include <stdio.h>

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_wunicode.h"

#ifdef ALLEGRO_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif


/* forward declaration */
const struct ALLEGRO_FILE_INTERFACE _al_file_interface_stdio;


static FILE *get_fp(ALLEGRO_FILE *f)
{
   if (f)
      return (FILE *)al_get_file_userdata(f);
   else
      return NULL;
}


/* Function: al_fopen_fd
 */
ALLEGRO_FILE *al_fopen_fd(int fd, const char *mode)
{
   ALLEGRO_FILE *f;
   FILE *fp;

   /* The fd should remain open if this function fails in either way. */
   fp = fdopen(fd, mode);
   if (!fp) {
      al_set_errno(errno);
      return NULL;
   }
   
   f = al_create_file_handle(&_al_file_interface_stdio, fp);
   if (!f) {
      al_set_errno(errno);
      return NULL;
   }

   return f;
}


static void *file_stdio_fopen(const char *path, const char *mode)
{
   FILE *fp;

#ifdef ALLEGRO_WINDOWS
   {
      wchar_t *wpath = _al_win_utf16(path);
      wchar_t *wmode = _al_win_utf16(mode);
      fp = _wfopen(wpath, wmode);
      al_free(wpath);
      al_free(wmode);
   }
#else
   fp = fopen(path, mode);
#endif

   if (!fp) {
      al_set_errno(errno);
      return NULL;
   }

   return fp;
}


static void file_stdio_fclose(ALLEGRO_FILE *f)
{
   fclose(get_fp(f));
}


static size_t file_stdio_fread(ALLEGRO_FILE *f, void *ptr, size_t size)
{
   if (size == 1) {
      /* Optimise common case. */
      int c = fgetc(get_fp(f));
      if (c == EOF) {
         al_set_errno(errno);
         return 0;
      }
      *((char *)ptr) = (char)c;
      return 1;
   }
   else {
      size_t ret = fread(ptr, 1, size, get_fp(f));
      if (ret < size) {
         al_set_errno(errno);
      }
      return ret;
   }
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


static void file_stdio_fclearerr(ALLEGRO_FILE *f)
{
   clearerr(get_fp(f));
}


static int file_stdio_fungetc(ALLEGRO_FILE *f, int c)
{
   int rc = ungetc(c, get_fp(f));

   if (rc == EOF) {
      al_set_errno(errno);
   }

   return rc;
}


static off_t file_stdio_fsize(ALLEGRO_FILE *f)
{
   int64_t old_pos;
   int64_t new_pos;

   old_pos = file_stdio_ftell(f);
   if (old_pos == -1)
      goto Error;

   if (!file_stdio_fseek(f, 0, ALLEGRO_SEEK_END))
      goto Error;

   new_pos = file_stdio_ftell(f);
   if (new_pos == -1)
      goto Error;

   if (!file_stdio_fseek(f, old_pos, ALLEGRO_SEEK_SET))
      goto Error;

   return new_pos;

Error:

   al_set_errno(errno);
   return -1;
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
   file_stdio_fclearerr,
   file_stdio_fungetc,
   file_stdio_fsize
};


/* Function: al_set_standard_file_interface
 */
void al_set_standard_file_interface(void)
{
   al_set_new_file_interface(&_al_file_interface_stdio);
}


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
         /* -1 to avoid the NUL terminator. */
         dst[i] = chars[_al_rand() % (sizeof(chars) - 1)];
      }
   }

   dst[i] = '\0';
}


/* Function: al_make_temp_file
 */
ALLEGRO_FILE *al_make_temp_file(const char *template, ALLEGRO_PATH **ret_path)
{
   ALLEGRO_PATH *path;
   ALLEGRO_FILE *f;
   char filename[PATH_MAX];
   int fd;
   int i;

   path = al_get_standard_path(ALLEGRO_TEMP_PATH);
   if (!path) {
      return NULL;
   }

   /* Note: the path should be absolute.  The user is likely to want to remove
    * the file later.  If we return a relative path, the user might change the
    * working directory in the mean time and then try to remove the wrong file.
    * Mostly likely there won't be any such file, but the temporary file
    * wouldn't be removed.
    */

   for (i=0; i<MAX_MKTEMP_TRIES; ++i) {
      mktemp_replace_XX(template, filename);
      al_set_path_filename(path, filename);

#ifndef ALLEGRO_MSVC
      fd = open(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP),
         O_EXCL | O_CREAT | O_RDWR, S_IRWXU);
#else
      fd = open(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP),
         O_EXCL | O_CREAT | O_RDWR, _S_IWRITE | _S_IREAD);
#endif

      if (fd != -1)
         break;
   }

   if (fd == -1) {
      al_set_errno(errno);
      al_destroy_path(path);
      return NULL;
   }

   f = al_fopen_fd(fd, "rb+");
   if (!f) {
      al_set_errno(errno);
      close(fd);
      unlink(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
      al_destroy_path(path);
      return NULL;
   }

   if (ret_path)
      *ret_path = path;
   else
      al_destroy_path(path);

   return f;
}


/* vim: set sts=3 sw=3 et: */
