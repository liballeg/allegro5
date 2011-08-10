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
   #define  SUFFIX _T("*")
   #define  SLASH _T("\\")
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


/*
 * MSVC is missing the whole dirent.h so we implement the bits we need here.
 * The following block is coppied from dirent.c from MinGW Runtime sources
 * version 3.15.1 with minor modifications.
 */
#ifdef ALLEGRO_MSVC

#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)

struct dirent
{
   long d_ino;                /* Always zero. */
   unsigned short	d_reclen;   /* Always zero. */
   unsigned short	d_namlen;   /* Length of name in d_name. */
   char d_name[FILENAME_MAX]; /* File name. */
};

/*
 * This is an internal data structure. Good programmers will not use it
 * except as an argument to one of the functions below.
 * dd_stat field is now int (was short in older versions).
 */
typedef struct
{
   /* disk transfer area for this dir */
   struct _finddata_t dd_dta;

   /* dirent struct to return from dir (NOTE: this makes this thread
    * safe as long as only one thread uses a particular DIR struct at
    * a time) */
   struct dirent dd_dir;

   /* _findnext handle */
   long dd_handle;

   /*
    * Status of search:
    *   0 = not started yet (next entry to read is first entry)
    *  -1 = off the end
    *   positive = 0 based index of next entry
    */
   int dd_stat;

   /* given path for dir with search pattern (struct is extended) */
   char dd_name[1];
} DIR;

/* Helper for opendir().  */
static unsigned __inline _tGetFileAttributes(const _TCHAR * tPath)
{
#ifdef _UNICODE
  /* GetFileAttributesW does not work on W9x, so convert to ANSI */
  if (_osver & 0x8000)
    {
      char aPath [MAX_PATH];
      WideCharToMultiByte(CP_ACP, 0, tPath, -1, aPath, MAX_PATH, NULL,
         NULL);
      return GetFileAttributesA(aPath);
    }
  return GetFileAttributesW(tPath);
#else
  return GetFileAttributesA(tPath);
#endif
}

/*
 * opendir
 *
 * Returns a pointer to a DIR structure appropriately filled in to begin
 * searching a directory.
 */
static DIR *opendir(const _TCHAR *szPath)
{
   DIR *nd;
   unsigned int rc;
   _TCHAR szFullPath[MAX_PATH];

   errno = 0;

   if (!szPath) {
      errno = EFAULT;
      return (DIR *) 0;
   }

   if (szPath[0] == _T('\0')) {
      errno = ENOTDIR;
      return (DIR*) 0;
   }

   /* Attempt to determine if the given path really is a directory. */
   rc = _tGetFileAttributes(szPath);
   if (rc == (unsigned int)-1) {
      /* call GetLastError for more error info */
      errno = ENOENT;
      return (DIR*) 0;
   }
   if (!(rc & FILE_ATTRIBUTE_DIRECTORY)) {
      /* Error, entry exists but not a directory. */
      errno = ENOTDIR;
      return (DIR*) 0;
   }

   /* Make an absolute pathname.  */
   _tfullpath(szFullPath, szPath, MAX_PATH);

   /* Allocate enough space to store DIR structure and the complete
   * directory path given. */
   nd = (DIR *) al_malloc(sizeof (DIR) + (_tcslen(szFullPath) + _tcslen(SLASH)
      + _tcslen(SUFFIX) + 1) * sizeof(_TCHAR));

   if (!nd) {
      /* Error, out of memory. */
      errno = ENOMEM;
      return (DIR*) 0;
   }

   /* Create the search expression. */
   _tcscpy(nd->dd_name, szFullPath);

   /* Add on a slash if the path does not end with one. */
   if (nd->dd_name[0] != _T('\0')
      && _tcsrchr(nd->dd_name, _T('/')) !=
         nd->dd_name + _tcslen(nd->dd_name) - 1
      && _tcsrchr(nd->dd_name, _T('\\')) !=
         nd->dd_name + _tcslen(nd->dd_name) - 1)
   {
      _tcscat(nd->dd_name, SLASH);
   }

   /* Add on the search pattern */
   _tcscat(nd->dd_name, SUFFIX);

   /* Initialize handle to -1 so that a premature closedir doesn't try
   * to call _findclose on it. */
   nd->dd_handle = -1;

   /* Initialize the status. */
   nd->dd_stat = 0;

   /* Initialize the dirent structure. ino and reclen are invalid under
   * Win32, and name simply points at the appropriate part of the
   * findfirst_t structure. */
   nd->dd_dir.d_ino = 0;
   nd->dd_dir.d_reclen = 0;
   nd->dd_dir.d_namlen = 0;
   memset(nd->dd_dir.d_name, 0, FILENAME_MAX);

   return nd;
}


/*
 * readdir
 *
 * Return a pointer to a dirent structure filled with the information on the
 * next entry in the directory.
 */
static struct dirent* readdir(DIR* dirp)
{
   errno = 0;

   /* Check for valid DIR struct. */
   if (!dirp) {
     errno = EFAULT;
     return (struct dirent*) 0;
   }

   if (dirp->dd_stat < 0) {
      /* We have already returned all files in the directory
       * (or the structure has an invalid dd_stat). */
      return (struct dirent*) 0;
   }
   else if (dirp->dd_stat == 0) {
      /* We haven't started the search yet. */
      /* Start the search */
      dirp->dd_handle = _tfindfirst(dirp->dd_name, &(dirp->dd_dta));

      if (dirp->dd_handle == -1) {
         /* Whoops! Seems there are no files in that
          * directory. */
         dirp->dd_stat = -1;
      }
      else {
         dirp->dd_stat = 1;
      }
   }
   else {
      /* Get the next search entry. */
      if (_tfindnext(dirp->dd_handle, &(dirp->dd_dta))) {
         /* We are off the end or otherwise error.	
            _findnext sets errno to ENOENT if no more file
            Undo this. */ 
         DWORD winerr = GetLastError();
         if (winerr == ERROR_NO_MORE_FILES)
            errno = 0;	
         _findclose(dirp->dd_handle);
         dirp->dd_handle = -1;
         dirp->dd_stat = -1;
      }
      else {
         /* Update the status to indicate the correct
          * number. */
         dirp->dd_stat++;
      }
   }

   if (dirp->dd_stat > 0) {
      /* Successfully got an entry. Everything about the file is
       * already appropriately filled in except the length of the
       * file name. */
      dirp->dd_dir.d_namlen = _tcslen(dirp->dd_dta.name);
      _tcscpy(dirp->dd_dir.d_name, dirp->dd_dta.name);
      return &dirp->dd_dir;
   }

   return (struct dirent*) 0;
}


/*
 * closedir
 *
 * Frees up resources allocated by opendir.
 */
static int closedir(DIR* dirp)
{
  int rc;

  errno = 0;
  rc = 0;

  if (!dirp) {
     errno = EFAULT;
     return -1;
  }

  if (dirp->dd_handle != -1) {
     rc = _findclose(dirp->dd_handle);
  }

  /* Delete the dir structure. */
  al_free(dirp);

  return rc;
}

#endif /* ALLEGRO_MSVC */


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
struct ALLEGRO_FS_ENTRY_STDIO {
   ALLEGRO_FS_ENTRY fs_entry; /* must be first */

   bool isdir;
   DIR *dir;

   struct stat st;
   char *path;                /* stores the path given by the user */
   ALLEGRO_PATH *apath;       /* for al_get_fs_entry_name */
   uint32_t ulink;
   uint32_t stat_mode;
};


static void fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio);
static bool fs_stdio_update_entry(ALLEGRO_FS_ENTRY *fp);


static ALLEGRO_FS_ENTRY *fs_stdio_create_entry(const char *path)
{
   ALLEGRO_FS_ENTRY_STDIO *fh;
   int len;
   int trailing_slashes;
   char c;

   fh = al_calloc(1, sizeof(*fh));
   if (!fh) {
      al_set_errno(errno);
      return NULL;
   }

   fh->fs_entry.vtable = &_al_fs_interface_stdio;

   len = strlen(path);
   trailing_slashes = 0;

   /* At least under Windows 7, a trailing slash or backslash makes
    * all calls to stat() with the given filename fail - making the
    * filesystem entry useless.
    * But don't trim the root path "/" to nothing.
    */
   while (len - trailing_slashes > 1) {
      c = path[len - 1 - trailing_slashes];
      if (c != '/' && c != '\\')
         break;
      trailing_slashes++;
   }

   fh->path = al_malloc(len + 1 - trailing_slashes);
   if (!fh->path) {
      al_set_errno(errno);
      al_free(fh);
      return NULL;
   }

   memcpy(fh->path, path, len - trailing_slashes);
   fh->path[len - trailing_slashes] = 0;

   ALLEGRO_DEBUG("Creating entry for %s\n", fh->path);

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

#ifdef ALLEGRO_WINDOWS
   {
      DWORD attrib = GetFileAttributes(fp_stdio->path);
      if (attrib & FILE_ATTRIBUTE_HIDDEN)
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
   }
#else
#if defined ALLEGRO_MACOSX && defined UF_HIDDEN
   {
      /* OSX hidden files can both start with the dot as well as having this flag set...
       * Note that this flag does not exist on all versions of OS X (Tiger
       * doesn't seem to have it) so we need to test for it.
       */
      if (fp_stdio->st.st_flags & UF_HIDDEN)
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
   }
#endif
   fp_stdio->stat_mode |= (fp_stdio->path[0] == '.' ? ALLEGRO_FILEMODE_HIDDEN : 0);
#endif

   return;
}

static bool fs_stdio_update_entry(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int ret;

   ret = stat(fp_stdio->path, &(fp_stdio->st));
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

   fp_stdio->dir = opendir(fp_stdio->path);
   if (!fp_stdio->dir) {
      al_set_errno(errno);
      return false;
   }

   fp_stdio->isdir = true;
   return true;
}

static bool fs_stdio_close_directory(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   bool ret;

   fp_stdio->isdir = false;

   if (closedir(fp_stdio->dir) == -1) {
      al_set_errno(errno);
      ret = false;
   }
   else {
      ret = true;
   }

   fp_stdio->dir = NULL;

   return ret;
}

static ALLEGRO_FS_ENTRY *fs_stdio_read_directory(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   // FIXME: Must use readdir_r as Allegro allows file functions being
   // called from different threads.
   struct dirent *ent;
   ALLEGRO_PATH *path;
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

   /* TODO: Maybe we should keep an ALLEGRO_PATH for each entry in
    * the first place?
    */
   path = al_create_path_for_directory(fp_stdio->path);
   al_set_path_filename(path, ent->d_name);
   ret = fs_stdio_create_entry(al_path_cstr(path, '/'));
   al_destroy_path(path);
   return ret;
}

static void fs_stdio_destroy_entry(ALLEGRO_FS_ENTRY *fh_)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;

   if (fh->ulink)
      unlink(fh->path);

   if (fh->path)
      al_free(fh->path);

   if (fh->apath)
      al_destroy_path(fh->apath);

   if (fh->isdir)
      fs_stdio_close_directory(fh_);

   memset(fh, 0, sizeof(*fh));
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

   if (stat(fp_stdio->path, &st) != 0) {
      if (errno == ENOENT) {
         return false;
      }
      else {
         al_set_errno(errno);
         return false;
      }
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
      err = rmdir(fp_stdio->path);
   }
   else if (fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISFILE) {
      err = unlink(fp_stdio->path);
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

   if (!fp_stdio->apath) {
      if (al_get_fs_entry_mode(fp) & ALLEGRO_FILEMODE_ISDIR) {
         fp_stdio->apath = al_create_path_for_directory(fp_stdio->path);
      }
      else {
         fp_stdio->apath = al_create_path(fp_stdio->path);
      }
   }

   return al_path_cstr(fp_stdio->apath, ALLEGRO_NATIVE_PATH_SEP);
}

static ALLEGRO_FILE *fs_stdio_open_file(ALLEGRO_FS_ENTRY *fp, const char *mode)
{
   return al_fopen_interface(&_al_file_interface_stdio, fs_stdio_name(fp), mode);
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
