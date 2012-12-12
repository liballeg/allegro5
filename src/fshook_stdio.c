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
 *      File System Hook, stdio "driver".
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"

ALLEGRO_DEBUG_CHANNEL("fshook")

/* Enable large file support in gcc/glibc. */
#if defined ALLEGRO_HAVE_FTELLO && defined ALLEGRO_HAVE_FSEEKO
   #define _LARGEFILE_SOURCE
   #define _LARGEFILE_SOURCE64
   #define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
   #define _POSIX_
   #include <limits.h>
   #undef _POSIX_

   #include <tchar.h>
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

#if defined ALLEGRO_HAVE_TIME_H && defined ALLEGRO_HAVE_SYS_TIME
   #include <sys/time.h>
   #include <time.h>
#else
   #ifdef ALLEGRO_HAVE_SYS_TIME_H
      #include <sys/time.h>
   #else
      #include <time.h>
   #endif
#endif

#ifdef ALLEGRO_WINDOWS
   #include "fshook_win.inc"
#endif


/* Windows' stat() doesn't like the slash at the end of the path when the path
 * is pointing to a directory.  This function should be called instead of
 * stat() if a path may have a trailing slash.  fs_stdio_create_entry() already
 * removes trailing slashes from ALLEGRO_FS_ENTRY paths.
 */
static int stat_unslash(const char *s, struct stat *st)
{
#ifdef ALLEGRO_WINDOWS
   size_t len;
   char buf[PATH_MAX];

   len = strlen(s);
   if (len + 1 > PATH_MAX) {
      errno = EINVAL;
      return -1;
   }
   if (len > 1 && (s[len-1] == '\\' || s[len-1] == '/')) {
      _al_sane_strncpy(buf, s, len);
      return stat(buf, st);
   }
#endif

   return stat(s, st);
}


typedef struct ALLEGRO_FS_ENTRY_STDIO ALLEGRO_FS_ENTRY_STDIO;

struct ALLEGRO_FS_ENTRY_STDIO
{
   ALLEGRO_FS_ENTRY fs_entry; /* must be first */
   char *abs_path;
   uint32_t stat_mode;
   struct stat st;
   DIR *dir;
};


static void fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio);
static bool fs_stdio_update_entry(ALLEGRO_FS_ENTRY *fp);


/* Make an absolute path given a potentially relative path.
 * The result must be freed with free(), NOT al_free().
 */
static char *make_absolute_path(const char *tail)
{
#ifdef ALLEGRO_WINDOWS
   /* We use _fullpath to get the proper drive letter semantics on Windows. */
   return _fullpath(NULL, tail, MAX_PATH);
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


/* At least under Windows 7, a trailing slash or backslash makes
 * all calls to stat() with the given filename fail - making the
 * filesystem entry useless.
 * But don't trim the root path "/" to nothing.
 */
static void trim_unnecessary_trailing_slashes(char *s)
{
   char *p = s + strlen(s) - 1;

   for (; p > s; p--) {
      if (*p != '/' && *p != ALLEGRO_NATIVE_PATH_SEP) {
         break;
      }
      *p = '\0';
   }
}


static bool unix_hidden_file(const char *path)
{
#if defined(ALLEGRO_UNIX) || defined(ALLEGRO_MACOSX)
   /* Filenames beginning with dot are considered hidden. */
   const char *p = strrchr(path, ALLEGRO_NATIVE_PATH_SEP);
   if (p)
      p++;
   else
      p = path;
   return (p[0] == '.');
#else
   (void)path;
   return false;
#endif
}


static ALLEGRO_FS_ENTRY *fs_stdio_create_entry(const char *orig_path)
{
   ALLEGRO_FS_ENTRY_STDIO *fh;

   fh = al_calloc(1, sizeof(*fh));
   if (!fh) {
      al_set_errno(errno);
      return NULL;
   }

   fh->fs_entry.vtable = &_al_fs_interface_stdio;

   fh->abs_path = make_absolute_path(orig_path);
   if (!fh->abs_path) {
      al_free(fh);
      return NULL;
   }

   trim_unnecessary_trailing_slashes(fh->abs_path);

   ALLEGRO_DEBUG("Creating entry for %s\n", fh->abs_path);

   fs_stdio_update_entry((ALLEGRO_FS_ENTRY *) fh);

   return (ALLEGRO_FS_ENTRY *) fh;
}

static void fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio)
{
   if (S_ISDIR(fp_stdio->st.st_mode))
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_ISDIR;
   else /* marks special unix files as files... might want to add enum items for symlink, CHAR, BLOCK and SOCKET files. */
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_ISFILE;

   /*
   if (S_ISREG(fh->st.st_mode))
      fh->stat_mode |= ALLEGRO_FILEMODE_ISFILE;
   */

#ifndef ALLEGRO_WINDOWS
   if (fp_stdio->st.st_mode & S_IRUSR || fp_stdio->st.st_mode & S_IRGRP)
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_READ;

   if (fp_stdio->st.st_mode & S_IWUSR || fp_stdio->st.st_mode & S_IWGRP)
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_WRITE;

   if (fp_stdio->st.st_mode & S_IXUSR || fp_stdio->st.st_mode & S_IXGRP)
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_EXECUTE;
#else
   if (fp_stdio->st.st_mode & S_IRUSR)
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_READ;

   if (fp_stdio->st.st_mode & S_IWUSR)
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_WRITE;

   if (fp_stdio->st.st_mode & S_IXUSR)
      fp_stdio->stat_mode |= ALLEGRO_FILEMODE_EXECUTE;
#endif

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
   if (0 == (fp_stdio->stat_mode & ALLEGRO_FILEMODE_HIDDEN)) {
      if (unix_hidden_file(fp_stdio->abs_path)) {
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
      }
   }
}

static bool fs_stdio_update_entry(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int ret;

   ret = stat(fp_stdio->abs_path, &(fp_stdio->st));
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

   fp_stdio->dir = opendir(fp_stdio->abs_path);
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

   rc = closedir(fp_stdio->dir);
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
   struct dirent *ent;
   int abs_path_len;
   int ent_name_len;
   char *buf;
   ALLEGRO_FS_ENTRY *ret;
   
   ASSERT(fp_stdio->dir);

   do {
      ent = readdir(fp_stdio->dir);
      if (!ent) {
         al_set_errno(errno);
         return NULL;
      }
      /* Don't bother the user with these entries. */
   } while (0 == strcmp(ent->d_name, ".") || 0 == strcmp(ent->d_name, ".."));

   abs_path_len = strlen(fp_stdio->abs_path);
   ent_name_len = strlen(ent->d_name);
   buf = al_malloc(abs_path_len + 1 + ent_name_len + 1);
   if (!buf) {
      al_set_errno(ENOMEM);
      return NULL;
   }
   memcpy(buf, fp_stdio->abs_path, abs_path_len);
   buf[abs_path_len] = ALLEGRO_NATIVE_PATH_SEP;
   memcpy(buf + abs_path_len + 1, ent->d_name, ent_name_len);
   buf[abs_path_len + 1 + ent_name_len] = '\0';
   ret = fs_stdio_create_entry(buf);
   al_free(buf);
   return ret;
}

static void fs_stdio_destroy_entry(ALLEGRO_FS_ENTRY *fh_)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;

   /* abs_path is created by make_absolute_path. free() is correct here. */
   free(fh->abs_path);

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
}

static bool fs_stdio_change_directory(const char *path)
{
   int ret = chdir(path);
   if (ret == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}

static int mkdir_perm(const char *path)
{
#ifdef ALLEGRO_WINDOWS
   return mkdir(path);
#else
   return mkdir(path, 0755);
#endif
}

static bool fs_stdio_make_directory(const char *path)
{
   ALLEGRO_PATH *path1, *path2;
   const char *s;
   struct stat st;
   int i, n;
   bool success = true;

   path1 = al_create_path_for_directory(path);
   path2 = al_create_path(NULL);
   al_set_path_drive(path2, al_get_path_drive(path1));

   n = al_get_path_num_components(path1);
   for (i = 0; i < n; i++) {
      const char *component = al_get_path_component(path1, i);
      al_append_path_component(path2, component);

      /* Skip empty components. Windows mkdir will fail otherwise. */
      if (*component == '\0')
         continue;

      s = al_path_cstr(path2, ALLEGRO_NATIVE_PATH_SEP);
      if (stat_unslash(s, &st) == 0) {
         if (S_ISDIR(st.st_mode))
            continue;
         al_set_errno(ENOTDIR);
         success = false;
         break;
      }
      if (errno != ENOENT) {
         al_set_errno(errno);
         success = false;
         break;
      }
      if (mkdir_perm(s) != 0) {
         ALLEGRO_WARN("mkdir_perm(\"%s\") failed (%s)\n", s, strerror(errno));
         al_set_errno(errno);
         success = false;
         break;
      }
   }

   al_destroy_path(path1);
   al_destroy_path(path2);

   return success;
}

static bool fs_stdio_entry_exists(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   struct stat st;

   if (stat(fp_stdio->abs_path, &st) != 0) {
      if (errno != ENOENT) {
         al_set_errno(errno);
      }
      return false;
   }

   return true;
}

static bool fs_stdio_filename_exists(const char *path)
{
   struct stat st;
   ASSERT(path);

   if (stat_unslash(path, &st) == 0) {
      return true;
   }
   else {
      if (errno != ENOENT)
         al_set_errno(errno);
      return false;
   }
}

static bool fs_stdio_remove_entry(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int err;

   ASSERT(fp);

   if (fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISDIR) {
      err = rmdir(fp_stdio->abs_path);
   }
   else if (fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISFILE) {
      err = unlink(fp_stdio->abs_path);
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
   if (!fp)
      return false;

   rc = fs_stdio_remove_entry(fp);
   fs_stdio_destroy_entry(fp);
   return rc;
}

static const char *fs_stdio_name(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   return fp_stdio->abs_path;
}

static ALLEGRO_FILE *fs_stdio_open_file(ALLEGRO_FS_ENTRY *fp, const char *mode)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   return al_fopen_interface(&_al_file_interface_stdio,
      fp_stdio->abs_path, mode);
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

/* Function: al_set_standard_fs_interface
 */
void al_set_standard_fs_interface(void)
{
   al_set_fs_interface(&_al_fs_interface_stdio);
}

/* vim: set sts=3 sw=3 et: */
