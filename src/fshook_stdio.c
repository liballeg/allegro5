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

/* enable large file support in gcc/glibc */
#if defined ALLEGRO_HAVE_FTELLO && defined ALLEGRO_HAVE_FSEEKO
   #define _LARGEFILE_SOURCE
   #define _LARGEFILE_SOURCE64
   #define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>

#ifdef _MSC_VER
   #define _POSIX_
   #include <limits.h>
   #undef _POSIX_

   #include <tchar.h>
   #include <stdlib.h>
   #include <io.h>
   #define  SUFFIX _T("*")
   #define	SLASH	 _T("\\")
#endif

#include "allegro5/allegro5.h"
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/path.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_memory.h"
#include ALLEGRO_INTERNAL_HEADER

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
   nd = (DIR *) malloc(sizeof (DIR) + (_tcslen(szFullPath) + _tcslen(SLASH)
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
  free(dirp);

  return rc;
}

#endif /* ALLEGRO_MSVC */


typedef struct ALLEGRO_FS_ENTRY_STDIO ALLEGRO_FS_ENTRY_STDIO;
struct ALLEGRO_FS_ENTRY_STDIO {
   ALLEGRO_FS_ENTRY fs_entry;   /* must be first */

   bool isdir;
   union {
      DIR *dir;
   } hd;

   struct stat st;
   char *path;  // stores the path given by the user.
   char mode[6];
   uint32_t free_on_fclose;
   uint32_t ulink;
   uint32_t stat_mode;
};


static void _al_fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio);
static bool al_fs_stdio_fstat(ALLEGRO_FS_ENTRY *fp);

static ALLEGRO_FS_ENTRY *al_fs_stdio_create_handle(const char *path)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = NULL;
   uint32_t len = 0;

   fh = _AL_MALLOC(sizeof(*fh));
   if (!fh) {
      al_set_errno(errno);
      return NULL;
   }

   memset(fh, 0, sizeof(*fh));

   fh->fs_entry.vtable = &_al_fs_interface_stdio;

   len = strlen(path);
   fh->path = _AL_MALLOC(len+1);
   if (!fh->path) {
      al_set_errno(errno);
      _AL_FREE(fh);
      return NULL;
   }

   memcpy(fh->path, path, len+1);

   al_fs_stdio_fstat((ALLEGRO_FS_ENTRY *) fh);

   return (ALLEGRO_FS_ENTRY *) fh;
}

static void _al_fs_update_stat_mode(ALLEGRO_FS_ENTRY_STDIO *fp_stdio)
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

/* TODO: do we need a special OSX section here? or are . (dot) files "proper" under osx? */
#ifdef ALLEGRO_WINDOWS
   {
      DWORD attrib = GetFileAttributes(fp_stdio->path);
      if (attrib & FILE_ATTRIBUTE_HIDDEN)
         fp_stdio->stat_mode |= ALLEGRO_FILEMODE_HIDDEN;
   }
#else
   fp_stdio->stat_mode |= (fp_stdio->path[0] == '.' ? ALLEGRO_FILEMODE_HIDDEN : 0);
#endif

   return;
}

static bool al_fs_stdio_fstat(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t ret = 0;

   ret = stat(fp_stdio->path, &(fp_stdio->st));

   if (ret == -1) {
      al_set_errno(errno);
      return false;
   }

   _al_fs_update_stat_mode(fp_stdio);
   
   return true;
}

static bool al_fs_stdio_opendir(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   if (!(fp_stdio->stat_mode & ALLEGRO_FILEMODE_ISDIR))
      return false;

   fp_stdio->hd.dir = opendir(fp_stdio->path);
   if (!fp_stdio->hd.dir) {
      al_set_errno(errno);
      return false;
   }

   fp_stdio->isdir = true;
   return true;
}

static bool al_fs_stdio_closedir(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   bool ret;

   fp_stdio->isdir = false;

   if (closedir(fp_stdio->hd.dir) == -1) {
      al_set_errno(errno);
      ret = false;
   }
   else {
      ret = true;
   }

   fp_stdio->hd.dir = NULL;

   return ret;
}

static ALLEGRO_FS_ENTRY *al_fs_stdio_readdir(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   // FIXME: Must use readdir_r as Allegro allows file functions being
   // called from different threads.
   struct dirent *ent;
   ALLEGRO_PATH *path;
   ALLEGRO_FS_ENTRY *ret;

   do {
      ent = readdir(fp_stdio->hd.dir);
      if (!ent) {
         al_set_errno(errno);
         return NULL;
      }
      /* Don't bother the user with these entries. */
   } while (0 == strcmp(ent->d_name, ".") || 0 == strcmp(ent->d_name, ".."));

   /* TODO: Maybe we should keep an ALLEGRO_PATH for each entry in
    * the first place?
    */
   path = al_create_path_for_dir(fp_stdio->path);
   al_set_path_filename(path, ent->d_name);
   ret = al_fs_stdio_create_handle(al_path_cstr(path, '/'));
   al_free_path(path);
   return ret;
}

static void al_fs_stdio_destroy_handle(ALLEGRO_FS_ENTRY *fh_)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;

   if (fh->ulink)
      unlink(fh->path);

   if (fh->path)
      _AL_FREE(fh->path);

   if (fh->isdir)
      al_fs_stdio_closedir(fh_);

   memset(fh, 0, sizeof(*fh));
   _AL_FREE(fh);
}

static off_t al_fs_stdio_entry_size(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_size;
}

static uint32_t al_fs_stdio_entry_mode(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->stat_mode;
}

static time_t al_fs_stdio_entry_atime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_atime;
}

static time_t al_fs_stdio_entry_mtime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_mtime;
}

static time_t al_fs_stdio_entry_ctime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *ent = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(ent);
   return ent->st.st_ctime;
}

static ALLEGRO_PATH *al_fs_stdio_getcwd(void)
{
   char tmpdir[PATH_MAX];
   char *cwd = getcwd(tmpdir, PATH_MAX);
   size_t len;
   if (!cwd) {
      al_set_errno(errno);
      return NULL;
   }
   len = strlen(cwd);

   return al_create_path_for_dir(tmpdir);
}

static bool al_fs_stdio_chdir(const char *path)
{
   int32_t ret = chdir(path);
   if (ret == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}

static bool al_fs_stdio_mkdir(const char *path)
{
#ifdef ALLEGRO_WINDOWS
   int32_t ret = mkdir(path);
#else
   int32_t ret = mkdir(path, 0755);
#endif
   if (ret == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static bool al_fs_stdio_entry_exists(ALLEGRO_FS_ENTRY *fp)
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

      /* or just: (but ENOENT isn't a fatal error condition for this function...)
         al_set_errno(errno);
         return false;
      */
   }

   return true;
}

static bool al_fs_stdio_file_exists(const char *path)
{
   struct stat st;
   if (stat(path, &st) != 0) {
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

static bool al_fs_stdio_entry_remove(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t err = 0;

   ASSERT(fp);

   if (al_fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISDIR) {
      err = rmdir(fp_stdio->path);
   }
   else if (al_fs_stdio_entry_mode(fp) & ALLEGRO_FILEMODE_ISFILE) {
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


static bool al_fs_stdio_file_remove(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   int err = 0;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return false;

   err = al_fs_stdio_entry_remove(fp);
   if(err != 0) {
      al_set_errno(errno);
      al_fs_stdio_destroy_handle(fp);
      return false;
   }
   
   al_fs_stdio_destroy_handle(fp);

   return true;
}

static ALLEGRO_PATH *al_fs_stdio_fname(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   if (al_is_directory(fp))
      return al_create_path_for_dir(fp_stdio->path);
   else
      return al_create_path(fp_stdio->path);
}

struct ALLEGRO_FS_INTERFACE _al_fs_interface_stdio = {
   al_fs_stdio_create_handle,

   al_fs_stdio_getcwd,
   al_fs_stdio_chdir,

   al_fs_stdio_file_exists,
   al_fs_stdio_file_remove,

   al_fs_stdio_mkdir,

   al_fs_stdio_destroy_handle,

   al_fs_stdio_fname,
   al_fs_stdio_fstat,

   al_fs_stdio_entry_size,
   al_fs_stdio_entry_mode,
   al_fs_stdio_entry_atime,
   al_fs_stdio_entry_mtime,
   al_fs_stdio_entry_ctime,

   al_fs_stdio_entry_exists,
   al_fs_stdio_entry_remove,

   al_fs_stdio_opendir,
   al_fs_stdio_readdir,
   al_fs_stdio_closedir
};

/* vim: set sts=3 sw=3 et: */
