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
 *      File System Hook, POSIX "driver" (not stdio).
 *
 *      By Thomas Fjellstrom.
 *
 *      Modified by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

/* Eventually we will make Allegro use the Unicode (UTF-16) Windows API
 * globally but not yet.
 */
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_wunicode.h"
#include ALLEGRO_INTERNAL_HEADER

ALLEGRO_DEBUG_CHANNEL("fshook")

/* Enable large file support in gcc/glibc. */
#if defined ALLEGRO_HAVE_FTELLO && defined ALLEGRO_HAVE_FSEEKO
   #define _LARGEFILE_SOURCE
   #define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
   #define _POSIX_
   #include <limits.h>
   #undef _POSIX_
   #include <io.h>
#endif

#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_fshook.h"

#ifdef ALLEGRO_HAVE_SYS_STAT_H
   #include <sys/stat.h>
#endif

#ifdef ALLEGRO_HAVE_DIRENT_H
   #include <sys/types.h>
   #include <dirent.h>
   #define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
   #define dirent direct
   #define NAMLEN(dirent) ((dirent)->d_namlen)
   #ifdef ALLEGRO_HAVE_SYS_NDIR_H
      #include <sys/ndir.h>
   #endif
   #ifdef ALLEGRO_HAVE_SYS_DIR_H
      #include <sys/dir.h>
   #endif
   #ifdef ALLEGRO_HAVE_NDIR_H
      #include <ndir.h>
   #endif
#endif

#ifndef S_IRGRP
   #define S_IRGRP   (0)
#endif
#ifndef S_IWGRP
   #define S_IWGRP   (0)
#endif
#ifndef S_IXGRP
   #define S_IXGRP   (0)
#endif

#ifdef ALLEGRO_HAVE_SYS_TIME
   #include <sys/time.h>
#endif
#ifdef ALLEGRO_HAVE_TIME_H
   #include <time.h>
#endif

#ifdef ALLEGRO_WINDOWS
   #include <tchar.h>
   #include "fshook_win.inc"

   typedef wchar_t                  WRAP_CHAR;
   typedef struct _stat             WRAP_STAT_TYPE;
   typedef _WDIR                    WRAP_DIR_TYPE;
   typedef struct _wdirent          WRAP_DIRENT_TYPE;
   #define WRAP_LIT(s)              _TEXT(s)
   #define WRAP_STRLEN(s)           (wcslen(s))
   #define WRAP_STRCMP(s1, s2)      (wcscmp((s1), (s2)))
   #define WRAP_STAT(p, b)          (_wstat((p), (b)))
   #define WRAP_MKDIR(p)            (_wmkdir(p))
   #define WRAP_RMDIR(p)            (_wrmdir(p))
   #define WRAP_UNLINK(p)           (_wunlink(p))
   #define WRAP_OPENDIR(p)          (_wopendir(p))
   #define WRAP_CLOSEDIR(d)         (_wclosedir(d))
   #define WRAP_READDIR(d)          (_wreaddir(d))
#else
   typedef char                     WRAP_CHAR;
   typedef struct stat              WRAP_STAT_TYPE;
   typedef DIR                      WRAP_DIR_TYPE;
   typedef struct dirent            WRAP_DIRENT_TYPE;
   #define WRAP_LIT(s)              (s)
   #define WRAP_STRLEN(s)           (strlen(s))
   #define WRAP_STRCMP(s1, s2)      (strcmp((s1), (s2)))
   #define WRAP_STAT(p, b)          (stat((p), (b)))
   #define WRAP_STAT_UNSLASH(p, b)  (stat((p), (b)))
   #define WRAP_MKDIR(p)            (mkdir(p, 0755))
   #define WRAP_RMDIR(p)            (rmdir(p))
   #define WRAP_UNLINK(p)           (unlink(p))
   #define WRAP_OPENDIR(p)          (opendir(p))
   #define WRAP_CLOSEDIR(d)         (closedir(d))
   #define WRAP_READDIR(d)          (readdir(d))
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


typedef struct ALLEGRO_FS_ENTRY_STDIO ALLEGRO_FS_ENTRY_STDIO;

struct ALLEGRO_FS_ENTRY_STDIO
{
   ALLEGRO_FS_ENTRY fs_entry; /* must be first */
   WRAP_CHAR *abs_path;
#ifdef ALLEGRO_WINDOWS
   char *abs_path_utf8;
   #define ABS_PATH_UTF8   abs_path_utf8
#else
   #define ABS_PATH_UTF8   abs_path
#endif
   uint32_t stat_mode;
   WRAP_STAT_TYPE st;
   WRAP_DIR_TYPE *dir;
};


static void fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio);
static bool fs_stdio_update_entry(ALLEGRO_FS_ENTRY *fp);


/* Make an absolute path given a potentially relative path.
 * The result must be freed with free(), NOT al_free().
 */
static WRAP_CHAR *make_absolute_path_inner(const WRAP_CHAR *tail)
{
#ifdef ALLEGRO_WINDOWS
   /* We use _wfullpath to get the proper drive letter semantics on Windows. */
   wchar_t *abs_path = _wfullpath(NULL, tail, 0);

   /* Remove trailing backslash (_wstat fails otherwise), but NOT directly
    * following the drive letter.
    */
   if (abs_path) {
      const size_t abs_len = WRAP_STRLEN(abs_path);
      if (abs_len == 3 && abs_path[1] == ':' && abs_path[2] == '\\') {
         /* Do not strip "C:\" */
      }
      else if (abs_len > 1 && abs_path[abs_len - 1] == '\\') {
         abs_path[abs_len - 1] = '\0';
      }
   }

   return abs_path;
#else
   char cwd[PATH_MAX];
   ALLEGRO_PATH *cwdpath = NULL;
   ALLEGRO_PATH *tailpath = NULL;
   char *ret = NULL;

   if (!getcwd(cwd, sizeof(cwd))) {
      ALLEGRO_WARN("Unable to get current working directory.\n");
      al_set_errno(errno);
      goto Error;
   }

   cwdpath = al_create_path_for_directory(cwd);
   if (!cwdpath) {
      goto Error;
   }

   tailpath = al_create_path(tail);
   if (!tailpath) {
      goto Error;
   }

   if (al_rebase_path(cwdpath, tailpath)) {
      al_make_path_canonical(tailpath);
   }

   ret = strdup(al_path_cstr(tailpath, ALLEGRO_NATIVE_PATH_SEP));

Error:

   al_destroy_path(cwdpath);
   al_destroy_path(tailpath);
   return ret;
#endif
}


static WRAP_CHAR *make_absolute_path(const char *tail)
{
   WRAP_CHAR *abs_path = NULL;
#ifdef ALLEGRO_WINDOWS
   wchar_t *wtail = _al_win_utf8_to_utf16(tail);
   if (wtail) {
      abs_path = make_absolute_path_inner(wtail);
      al_free(wtail);
   }
#else
   abs_path = make_absolute_path_inner(tail);
#endif
   return abs_path;
}


static ALLEGRO_FS_ENTRY *create_abs_path_entry(const WRAP_CHAR *abs_path)
{
   ALLEGRO_FS_ENTRY_STDIO *fh;
   size_t len;

   fh = al_calloc(1, sizeof(*fh));
   if (!fh) {
      al_set_errno(errno);
      return NULL;
   }

   fh->fs_entry.vtable = &_al_fs_interface_stdio;

   len = WRAP_STRLEN(abs_path) + 1; /* including terminator */
   fh->abs_path = al_malloc(len * sizeof(WRAP_CHAR));
   if (!fh->abs_path) {
      al_free(fh);
      return NULL;
   }
   memcpy(fh->abs_path, abs_path, len * sizeof(WRAP_CHAR));

#ifdef ALLEGRO_WINDOWS
   fh->abs_path_utf8 = _al_win_utf16_to_utf8(fh->abs_path);
   if (!fh->abs_path_utf8) {
      al_free(fh->abs_path);
      al_free(fh);
      return NULL;
   }
#endif

   ALLEGRO_DEBUG("Creating entry for %s\n", fh->ABS_PATH_UTF8);

   fs_stdio_update_entry((ALLEGRO_FS_ENTRY *) fh);

   return (ALLEGRO_FS_ENTRY *) fh;
}


static ALLEGRO_FS_ENTRY *fs_stdio_create_entry(const char *orig_path)
{
   ALLEGRO_FS_ENTRY *ret = NULL;
   WRAP_CHAR *abs_path;

   abs_path = make_absolute_path(orig_path);
   if (abs_path) {
      ret = create_abs_path_entry(abs_path);
      free(abs_path);
   }
   return ret;
}


#if defined(ALLEGRO_UNIX) || defined(ALLEGRO_MACOSX)
static bool unix_hidden_file(const char *path)
{
   /* Filenames beginning with dot are considered hidden. */
   const char *p = strrchr(path, ALLEGRO_NATIVE_PATH_SEP);
   if (p)
      p++;
   else
      p = path;
   return (p[0] == '.');
}
#endif


static void fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio)
{
   fp_stdio->stat_mode = 0;

   if (S_ISDIR(fp_stdio->st.st_mode))
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_ISDIR;
   else /* marks special unix files as files... might want to add enum items for symlink, CHAR, BLOCK and SOCKET files. */
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_ISFILE;

   /*
   if (S_ISREG(fh->st.st_mode))
      fh->stat_mode |= ALLEGRO_FILEMODE_ISFILE;
   */

   if (fp_stdio->st.st_mode & (S_IRUSR | S_IRGRP))
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_READ;

   if (fp_stdio->st.st_mode & (S_IWUSR | S_IWGRP))
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_WRITE;

   if (fp_stdio->st.st_mode & (S_IXUSR | S_IXGRP))
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_EXECUTE;

#if defined(ALLEGRO_WINDOWS)
   {
      DWORD attrib = GetFileAttributes(fp_stdio->abs_path);
      if (attrib & FILE_ATTRIBUTE_HIDDEN)
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
   }
#endif
#if defined(ALLEGRO_MACOSX) && defined(UF_HIDDEN)
   {
      /* OSX hidden files can both start with the dot as well as having this flag set...
       * Note that this flag does not exist on all versions of OS X (Tiger
       * doesn't seem to have it) so we need to test for it.
       */
      if (fp_stdio->st.st_flags & UF_HIDDEN)
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
   }
#endif
#if defined(ALLEGRO_UNIX) || defined(ALLEGRO_MACOSX)
   if (0 == (fp_stdio->stat_mode & ALLEGRO_FILEMODE_HIDDEN)) {
      if (unix_hidden_file(fp_stdio->abs_path)) {
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
      }
   }
#endif
}


static bool fs_stdio_update_entry(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int ret;

   ret = WRAP_STAT(fp_stdio->abs_path, &(fp_stdio->st));
   if (ret == -1) {
      al_set_errno(errno);
      return false;
   }

   fs_update_stat_mode(fp_stdio);

   return true;
}


static bool fs_stdio_open_directory(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   if (!(fp_stdio->stat_mode & ALLEGRO_FILEMODE_ISDIR))
      return false;

   fp_stdio->dir = WRAP_OPENDIR(fp_stdio->abs_path);
   if (!fp_stdio->dir) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static bool fs_stdio_close_directory(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int rc;

   if (!fp_stdio->dir) {
      al_set_errno(ENOTDIR);
      return false;
   }

   rc = WRAP_CLOSEDIR(fp_stdio->dir);
   fp_stdio->dir = NULL;
   if (rc == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static ALLEGRO_FS_ENTRY *fs_stdio_read_directory(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   // FIXME: Must use readdir_r as Allegro allows file functions being
   // called from different threads.
   WRAP_DIRENT_TYPE *ent;
   ALLEGRO_FS_ENTRY *ret;

   ASSERT(fp_stdio->dir);

   do {
      ent = WRAP_READDIR(fp_stdio->dir);
      if (!ent) {
         al_set_errno(errno);
         return NULL;
      }
      /* Don't bother the user with these entries. */
   } while (0 == WRAP_STRCMP(ent->d_name, WRAP_LIT("."))
         || 0 == WRAP_STRCMP(ent->d_name, WRAP_LIT("..")));

#ifdef ALLEGRO_WINDOWS
   {
      wchar_t buf[MAX_PATH];
      int buflen;

      buflen = _snwprintf(buf, MAX_PATH, L"%s\\%s",
         fp_stdio->abs_path, ent->d_name);
      if (buflen >= MAX_PATH) {
         al_set_errno(ERANGE);
         return NULL;
      }
      ret = create_abs_path_entry(buf);
   }
#else
   {
      int abs_path_len = strlen(fp_stdio->abs_path);
      int ent_name_len = strlen(ent->d_name);
      char *buf = al_malloc(abs_path_len + 1 + ent_name_len + 1);
      if (!buf) {
         al_set_errno(ENOMEM);
         return NULL;
      }
      memcpy(buf, fp_stdio->abs_path, abs_path_len);
      if (  (abs_path_len >= 1) &&
            buf[abs_path_len - 1] == ALLEGRO_NATIVE_PATH_SEP)
      {
         /* do NOT add a new separator if we have one already */
         memcpy(buf + abs_path_len, ent->d_name, ent_name_len);
         buf[abs_path_len + ent_name_len] = '\0';
      }
      else {
         /* append separator */
         buf[abs_path_len] = ALLEGRO_NATIVE_PATH_SEP;
         memcpy(buf + abs_path_len + 1, ent->d_name, ent_name_len);
         buf[abs_path_len + 1 + ent_name_len] = '\0';
      }
      ret = create_abs_path_entry(buf);
      al_free(buf);
   }
#endif
   return ret;
}


static void fs_stdio_destroy_entry(ALLEGRO_FS_ENTRY *fh_)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;

   al_free(fh->abs_path);
#ifdef ALLEGRO_WINDOWS
   al_free(fh->abs_path_utf8);
#endif

   if (fh->dir)
      fs_stdio_close_directory(fh_);

   al_free(fh);
}


static off_t fs_stdio_entry_size(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_size;
}


static uint32_t fs_stdio_entry_mode(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->stat_mode;
}


static time_t fs_stdio_entry_atime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_atime;
}


static time_t fs_stdio_entry_mtime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_mtime;
}


static time_t fs_stdio_entry_ctime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_ctime;
}


static char *fs_stdio_get_current_directory(void)
{
#ifdef ALLEGRO_WINDOWS
   wchar_t *wcwd;
   char *cwd;

   wcwd = _wgetcwd(NULL, 1);
   if (!wcwd) {
      al_set_errno(errno);
      return NULL;
   }
   cwd = _al_win_utf16_to_utf8(wcwd);
   free(wcwd);
   return cwd;
#else
   char tmpdir[PATH_MAX];
   char *cwd;

   if (!getcwd(tmpdir, PATH_MAX)) {
      al_set_errno(errno);
      return NULL;
   }

   cwd = al_malloc(strlen(tmpdir) + 1);
   if (!cwd) {
      al_set_errno(ENOMEM);
      return NULL;
   }
   return strcpy(cwd, tmpdir);
#endif
}


static bool fs_stdio_change_directory(const char *path)
{
   int ret = -1;

#ifdef ALLEGRO_WINDOWS
   wchar_t *wpath = _al_win_utf8_to_utf16(path);
   if (wpath) {
      ret = _wchdir(wpath);
      al_free(wpath);
   }
#else
   ret = chdir(path);
#endif

   if (ret == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static bool mkdir_exists(const WRAP_CHAR *path)
{
   WRAP_STAT_TYPE st;

   if (WRAP_STAT(path, &st) == 0) {
      return S_ISDIR(st.st_mode);
   }

   return WRAP_MKDIR(path) == 0;
}


static bool do_make_directory(WRAP_CHAR *abs_path)
{
   const WRAP_CHAR * const end = abs_path + WRAP_STRLEN(abs_path);
   WRAP_CHAR *p;
   bool ret;

   p = abs_path + 1;

#ifdef ALLEGRO_WINDOWS
   /* Skip drive letter. */
   if (end - abs_path >= 3
      && abs_path[1] == ':'
      && (abs_path[2] == '\\' || abs_path[2] == '/'))
   {
      p = abs_path + 3;
   }
#endif

   for ( ; p < end; p++) {
      const WRAP_CHAR c = *p;
      if (c == ALLEGRO_NATIVE_PATH_SEP || c == '/') {
         *p = '\0';
         ret = mkdir_exists(abs_path);
         *p = c;
         if (!ret)
            return false;
      }
   }

   return mkdir_exists(abs_path);
}


static bool fs_stdio_make_directory(const char *tail)
{
   bool ret = false;
   WRAP_CHAR *abs_path = make_absolute_path(tail);
   if (abs_path) {
      ret = do_make_directory(abs_path);
      free(abs_path);
   }
   return ret;
}


static bool fs_stdio_entry_exists(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   WRAP_STAT_TYPE st;

   if (WRAP_STAT(fp_stdio->abs_path, &st) != 0) {
      if (errno != ENOENT) {
         al_set_errno(errno);
      }
      return false;
   }

   return true;
}


static bool fs_stdio_filename_exists(const char *path)
{
   WRAP_STAT_TYPE st;
   bool ret = false;
   ASSERT(path);

#ifdef ALLEGRO_WINDOWS
   {
      /* Pass an path created by _wfullpath() to avoid issues
       * with stat() failing when there is a trailing slash.
       */
      wchar_t *abs_path = make_absolute_path(path);
      if (abs_path) {
         ret = (0 == WRAP_STAT(abs_path, &st));
         if (!ret && errno != ENOENT) {
            al_set_errno(errno);
         }
         free(abs_path);
      }
   }
#else
   ret = (0 == WRAP_STAT(path, &st));
   if (!ret && errno != ENOENT) {
      al_set_errno(errno);
   }
#endif

   return ret;
}


static bool fs_stdio_remove_entry(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int err;

   ASSERT(fp);

   if (fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISDIR) {
      err = WRAP_RMDIR(fp_stdio->abs_path);
   }
   else if (fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISFILE) {
      err = WRAP_UNLINK(fp_stdio->abs_path);
   }
   else {
      al_set_errno(ENOENT);
      return false;
   }

   if (err != 0) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static bool fs_stdio_remove_filename(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   bool rc;

   fp = fs_stdio_create_entry(path);
   if (!fp) {
      ALLEGRO_WARN("Cannot remove %s.", path);
      return false;
   }

   rc = fs_stdio_remove_entry(fp);
   fs_stdio_destroy_entry(fp);
   return rc;
}


static const char *fs_stdio_name(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   return fp_stdio->ABS_PATH_UTF8;
}


static ALLEGRO_FILE *fs_stdio_open_file(ALLEGRO_FS_ENTRY *fp, const char *mode)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   /* XXX on Windows it would be nicer to use the UTF-16 abs_path field
    * directly
    */
   return al_fopen_interface(&_al_file_interface_stdio,
      fp_stdio->ABS_PATH_UTF8, mode);
}


struct ALLEGRO_FS_INTERFACE _al_fs_interface_stdio = {
   fs_stdio_create_entry,
   fs_stdio_destroy_entry,
   fs_stdio_name,
   fs_stdio_update_entry,
   fs_stdio_entry_mode,
   fs_stdio_entry_atime,
   fs_stdio_entry_mtime,
   fs_stdio_entry_ctime,
   fs_stdio_entry_size,
   fs_stdio_entry_exists,
   fs_stdio_remove_entry,

   fs_stdio_open_directory,
   fs_stdio_read_directory,
   fs_stdio_close_directory,

   fs_stdio_filename_exists,
   fs_stdio_remove_filename,
   fs_stdio_get_current_directory,
   fs_stdio_change_directory,
   fs_stdio_make_directory,

   fs_stdio_open_file
};

/* vim: set sts=3 sw=3 et: */
