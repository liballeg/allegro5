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


#define PREFIX_I "al-fs-stdio INFO: "


typedef struct ALLEGRO_FS_ENTRY_STDIO ALLEGRO_FS_ENTRY_STDIO;
struct ALLEGRO_FS_ENTRY_STDIO {
   ALLEGRO_FS_ENTRY fs_entry;   /* must be first */

   uint32_t isdir;
   union {
      FILE *handle;
      DIR *dir;
   } hd;

   struct stat st;
   char *path;  // stores the path given by the user.
   char *found; // used to store the proper path to a file opened and found via the search path.
   char mode[6];
   uint32_t free_on_fclose;
   uint32_t ulink;
   uint32_t stat_mode;
};


static char **search_path = NULL;
static uint32_t search_path_count = 0;


static ALLEGRO_FS_ENTRY *al_fs_stdio_create_handle(const char *path)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = NULL;
   uint32_t len = 0;
   uint32_t fnd = 0;
   char *tmp = NULL;

   fh = _AL_MALLOC(sizeof(*fh));
   if (!fh) {
      al_set_errno(errno);
      return NULL;
   }

   memset(fh, 0, sizeof(*fh));

   fh->fs_entry.vtable = &_al_stdio_entry_fshooks;

   len = strlen(path);
   fh->path = _AL_MALLOC(len+1);
   if (!fh->path) {
      al_set_errno(errno);
      _AL_FREE(fh);
      return NULL;
   }

   memcpy(fh->path, path, len+1);

   /* lookup real file path if given non abs path */
   if (fh->path[0] != '/') {
      uint32_t spi = 0;
      for (spi = 0; spi < search_path_count; ++spi) {
         uint32_t splen = strlen(search_path[spi]);
         struct stat st;

         tmp = _AL_REALLOC(tmp, splen + len + 1);
         memcpy(tmp, search_path[spi], MIN(splen, PATH_MAX));
         if (tmp[splen-1] == '/') {
            tmp[splen] = '/';
            splen++;
         }

         memcpy(tmp+splen, fh->path, MIN(len + splen, PATH_MAX));
         tmp[splen+len] = '\0';

         if (stat(tmp, &st) != 0) {
            _AL_FREE(tmp);
            break;
         }

         fnd = 1;
         fh->st = st;
         fh->found = tmp;
         break;
      }
   }

   if (!fnd) {
      /* FIXME: I'm not sure this check is needed, it might even be broken, if fh->found is set here, it could be stale info */
      if (fh->found)
         stat(fh->found, &(fh->st));
      else
         stat(fh->path, &(fh->st));
   }

   fh->stat_mode = 0;

   if (S_ISDIR(fh->st.st_mode))
      fh->stat_mode |= AL_FM_ISDIR;
   else /* marks special unix files as files... might want to add enum items for symlink, CHAR, BLOCK and SOCKET files. */
      fh->stat_mode |= AL_FM_ISFILE;

   /*
   if (S_ISREG(fh->st.st_mode))
      fh->stat_mode |= AL_FM_ISFILE;
   */

#ifndef ALLEGRO_WINDOWS
   if (fh->st.st_mode & S_IRUSR || fh->st.st_mode & S_IRGRP)
      fh->stat_mode |= AL_FM_READ;

   if (fh->st.st_mode & S_IWUSR || fh->st.st_mode & S_IWGRP)
      fh->stat_mode |= AL_FM_WRITE;

   if (fh->st.st_mode & S_IXUSR || fh->st.st_mode & S_IXGRP)
      fh->stat_mode |= AL_FM_EXECUTE;
#else
   if (fh->st.st_mode & S_IRUSR)
      fh->stat_mode |= AL_FM_READ;

   if (fh->st.st_mode & S_IWUSR)
      fh->stat_mode |= AL_FM_WRITE;

   if (fh->st.st_mode & S_IXUSR)
      fh->stat_mode |= AL_FM_EXECUTE;
#endif

/* TODO: do we need a special OSX section here? or are . (dot) files "proper" under osx? */
#ifdef ALLEGRO_WINDOWS
   {
      DWORD attrib = GetFileAttributes(fh->found ? fh->found : fh->path);
      if (attrib & FILE_ATTRIBUTE_HIDDEN)
         fh->stat_mode |= AL_FM_HIDDEN;
   }
#else
   if (fh->found)
      fh->stat_mode |= (fh->found[0] == '.' ? AL_FM_HIDDEN : 0);
   else
      fh->stat_mode |= (fh->path[0] == '.' ? AL_FM_HIDDEN : 0);
#endif

   return (ALLEGRO_FS_ENTRY *) fh;
}

static void al_fs_stdio_destroy_handle(ALLEGRO_FS_ENTRY *fh_)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;

   if (fh->found) {
      if (fh->ulink)
         unlink(fh->found);

      _AL_FREE(fh->found);
   }
   else {
      if (fh->ulink)
         unlink(fh->path);
   }

   if (fh->path)
      _AL_FREE(fh->path);

   if (fh->isdir)
      closedir(fh->hd.dir);
   else if (fh->hd.handle)
      fclose(fh->hd.handle);

   memset(fh, 0, sizeof(*fh));
   _AL_FREE(fh);
}

static int32_t al_fs_stdio_open_handle(ALLEGRO_FS_ENTRY *fh_, const char *mode)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;
   char *tmp = NULL;

   tmp = fh->found ? fh->found : fh->path;
   if (fh->stat_mode & AL_FM_ISDIR) {
      fh->hd.dir = opendir(tmp);
      if (!fh->hd.dir) {
         al_set_errno(errno);
         return -1;
      }
      fh->isdir = 1;
   }
   else {
      fh->hd.handle = fopen(tmp, mode);
      if (!fh->hd.handle) {
         al_set_errno(errno);
         return -1;
      }
      fh->isdir = 0;
   }

   return 0;
}

static void al_fs_stdio_close_handle(ALLEGRO_FS_ENTRY *fh_)
{
   ALLEGRO_FS_ENTRY_STDIO *fh = (ALLEGRO_FS_ENTRY_STDIO *) fh_;
   int32_t ret = 0;

   if (fh->isdir) {
      /* think about handling the unlink on close.. may only be usefull if mktemp can do dirs as well, or a mkdtemp is provided */
      closedir(fh->hd.dir);
      if (fh->found) {
         _AL_FREE(fh->found);
         fh->found = NULL;
      }
      fh->hd.dir = NULL;
      fh->isdir = 0;
   }
   else if (fh->hd.handle) {
      ret = fclose(fh->hd.handle);

      /* unlink on close */
      if (fh->found && fh->ulink) {
         unlink(fh->found);
         _AL_FREE(fh->found);
         fh->found = NULL;
      }
      else if (fh->path && fh->ulink) {
         unlink(fh->found);
      }

      fh->hd.handle = NULL;
   }
}

static ALLEGRO_FS_ENTRY *al_fs_stdio_fopen(const char *path, const char *mode)
{
   ALLEGRO_FS_ENTRY *fp;
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio;

   fp = al_fs_stdio_create_handle(path);
   if (!fp) {
      return NULL;
   }
   fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;

   if (al_fs_stdio_open_handle(fp, mode) != 0 ||
         fp_stdio->stat_mode & AL_FM_ISDIR) {
      al_fs_stdio_destroy_handle(fp);
      return NULL;
   }

   fp_stdio->free_on_fclose = 1;
   return fp;
}

static void al_fs_stdio_fclose(ALLEGRO_FS_ENTRY *fh)
{
   ALLEGRO_FS_ENTRY_STDIO *fh_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fh;
   ASSERT(!fh_stdio->isdir);

   al_fs_stdio_close_handle(fh);

   if (fh_stdio->free_on_fclose) {
      al_fs_stdio_destroy_handle(fh);
   }
}

static size_t al_fs_stdio_fread(void *ptr, size_t size, ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   size_t ret;
   ASSERT(!fp_stdio->isdir);

   ret = fread(ptr, 1, size, fp_stdio->hd.handle);
   if (ret == 0) {
      al_set_errno(errno);
   }

   return ret;
}

static size_t al_fs_stdio_fwrite(const void *ptr, size_t size,
   ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   size_t ret;
   ASSERT(!fp_stdio->isdir);

   ret = fwrite(ptr, 1, size, fp_stdio->hd.handle);
   if (ret == 0) {
      al_set_errno(errno);
   }

   return ret;
}

static int32_t al_fs_stdio_fflush(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t ret;
   ASSERT(!fp_stdio->isdir);

   ret = fflush(fp_stdio->hd.handle);
   if (ret == EOF) {
      al_set_errno(errno);
   }

   return ret;
}

static int32_t al_fs_stdio_fseek(ALLEGRO_FS_ENTRY *fp, uint32_t offset,
   uint32_t whence)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t ret = 0;
   ASSERT(!fp_stdio->isdir);

   switch (whence) {
      case ALLEGRO_SEEK_SET: whence = SEEK_SET; break;
      case ALLEGRO_SEEK_CUR: whence = SEEK_CUR; break;
      case ALLEGRO_SEEK_END: whence = SEEK_END; break;
   }

   ret = fseek(fp_stdio->hd.handle, offset, whence);
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}

static int32_t al_fs_stdio_ftell(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t ret;
   ASSERT(!fp_stdio->isdir);

   ret = ftell(fp_stdio->hd.handle);
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}

static int32_t al_fs_stdio_ferror(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(!fp_stdio->isdir);
   return ferror(fp_stdio->hd.handle);
}

static int32_t al_fs_stdio_feof(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(!fp_stdio->isdir);
   return feof(fp_stdio->hd.handle);
}

static int32_t al_fs_stdio_ungetc(int32_t c, ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   ASSERT(!fp_stdio->isdir);
   return ungetc(c, fp_stdio->hd.handle);
}

static int32_t al_fs_stdio_fstat(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t ret = 0;

   if (fp_stdio->found) {
      ret = stat(fp_stdio->found, &(fp_stdio->st));
   }
   else {
      ret = stat(fp_stdio->path, &(fp_stdio->st));
   }
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}

static ALLEGRO_FS_ENTRY *al_fs_stdio_opendir(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return NULL;

   fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   if (fp_stdio->stat_mode & AL_FM_ISDIR) {
      if (al_fs_stdio_open_handle(fp, 0) != 0) {
         al_fs_stdio_destroy_handle(fp);
         return NULL;
      }
   }
   else {
      al_fs_stdio_destroy_handle(fp);
      return NULL;
   }

   return fp;
}

static int32_t al_fs_stdio_closedir(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int ret = closedir(fp_stdio->hd.dir);
   if (ret == -1) {
      al_set_errno(errno);
   }

   fp_stdio->hd.dir = NULL;
   fp_stdio->isdir = 0;
   _AL_FREE(fp_stdio);

   return ret;
}

static int32_t al_fs_stdio_readdir(ALLEGRO_FS_ENTRY *fp, size_t size,
   char *buf)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   struct dirent *ent = readdir(fp_stdio->hd.dir);
   uint32_t ent_len = 0;

   if (!ent) {
      al_set_errno(errno);
      return -1;
   }

   ent_len = strlen(ent->d_name);
   memcpy(buf, ent->d_name, MIN(ent_len+1, size));

   return 0;
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

#define MAX_MKTEMP_TRIES 1000
static const char mktemp_ok_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static void _al_fs_mktemp_replace_XX(const char *template, char *dst)
{
   size_t len = strlen(template);
   uint32_t i = 0;

   for (i=0; i<len; ++i) {
      if (template[i] != 'X') {
         dst[i] = template[i];
      }
      else {
         dst[i] = mktemp_ok_chars[_al_rand() % (sizeof(mktemp_ok_chars)-1)];
      }
   }
}

/* FIXME: provide the filename created. */
/* might have to make ALLEGRO_FS_ENTRY a strust to provide the filename, and unlink hint. */
/* by default, temp file is NOT unlink'ed automatically */

static ALLEGRO_FS_ENTRY *al_fs_stdio_mktemp(const char *template,
   uint32_t ulink)
{
   int32_t fd = -1, i = 0;
   int32_t template_len = 0, tmpdir_len = 0;
   ALLEGRO_FS_ENTRY *fh = NULL;
   ALLEGRO_FS_ENTRY_STDIO *fh_stdio = NULL;
   char *dest = NULL;
   char tmpdir[PATH_MAX];

   template_len = strlen(template);

   if (al_get_path(AL_TEMP_PATH, tmpdir, PATH_MAX) != 0) {
      /* allegro_error disappeared on me :(
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to find temp directory"));
      */
      return NULL;
   }

   tmpdir_len = strlen(tmpdir);

   dest = _AL_MALLOC(template_len + tmpdir_len + 2);
   if (!dest) {
      al_set_errno(errno);
      return NULL;
   }

   memset(dest, 0, template_len + tmpdir_len + 2);
   memcpy(dest, tmpdir, strlen(tmpdir));

   /* doing this check makes the path prettier, no extra / laying around */
   if (dest[tmpdir_len-1] != '/') {
      dest[tmpdir_len] = '/';
      tmpdir_len++;
   }

   memcpy(dest + tmpdir_len, template, template_len);

   for (i=0; i<MAX_MKTEMP_TRIES; ++i) {
      _al_fs_mktemp_replace_XX(template, dest + tmpdir_len);
#ifndef ALLEGRO_MSVC
      fd = open(dest, O_EXCL | O_CREAT | O_RDWR, S_IRWXU);
#else
      fd = open(dest, O_EXCL | O_CREAT | O_RDWR, _S_IWRITE | _S_IREAD);
#endif
      if (fd == -1)
         continue;

      // changing the hook for create handle in a separate thread will cause some nice errors here,
      //    if you expect it to return a stdio handle ;)
      fh = al_fs_create_handle(dest);
      if (!fh) {
         close(fd);
         free(dest);
         return NULL;
      }

      fh_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fh;

      fh_stdio->hd.handle = fdopen(fd, "r+");
      if (!fh_stdio->hd.handle) {
         al_fs_stdio_destroy_handle(fh);
         close(fd);
         free(dest);
         return NULL;
      }

      if (ulink == ALLEGRO_FS_MKTEMP_UNLINK_NOW)
         unlink(dest);
      else if (ulink == ALLEGRO_FS_MKTEMP_UNLINK_ON_CLOSE)
         fh_stdio->ulink = 1;

      fh_stdio->free_on_fclose = 1;
      fh_stdio->path = dest;

      return fh;
   }

   free(dest);
   /* allegro_error disappeared on me :(
   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to create a uniqe temporary file"));
   */
   return NULL;
}

static int32_t al_fs_stdio_getcwd(char *buf, size_t len)
{
   char *cwd = getcwd(buf, len);
   char sep[2] = { ALLEGRO_NATIVE_PATH_SEP, 0 };
   if (!cwd) {
      al_set_errno(errno);
      return -1;
   }

   ustrcat(buf, sep);
   return 0;
}

static int32_t al_fs_stdio_chdir(const char *path)
{
   int32_t ret = chdir(path);
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}

static int32_t al_fs_stdio_mkdir(const char *path)
{
#ifdef ALLEGRO_WINDOWS
   int32_t ret = mkdir(path);
#else
   int32_t ret = mkdir(path, 0755);
#endif
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}

static int32_t al_fs_stdio_add_search_path(const char *path)
{
   char **new_search_path = NULL;
   char *new_path = NULL;

   /* dup path first to elimiate need to re-resize search_path if dup fails */
   new_path = ustrdup(path);
   if (!new_path) {
      return -1;
   }

   /* extend search_path, store temporarily so original var isn't overwritten with NULL on failure */
   new_search_path = (char **)_AL_REALLOC(search_path, sizeof(char *) * (search_path_count + 1));
   if (!new_search_path) {
      free(new_path);
      al_set_errno(errno);
      return -1;
   }

   search_path = new_search_path;
   search_path[search_path_count] = new_path;
   search_path_count++;

   return 0;
}

static uint32_t al_fs_stdio_search_path_count(void)
{
   return search_path_count;
}

/* FIXME: is this the best way to handle the "search path" ? */
static int32_t al_fs_stdio_get_search_path(uint32_t idx, char *dest,
   uint32_t len)
{
   if (idx < search_path_count) {
      uint32_t slen = strlen(search_path[idx]);

      memcpy(dest, search_path[idx], MIN(slen, len-1));
      dest[len] = '\0';
   }

   return -1;
}

static int32_t al_fs_stdio_drive_sep(size_t len, char *sep)
{
#ifdef ALLEGRO_WINDOWS
   char *s = ":";
   _al_sane_strncpy(sep, s, MIN(ustrlen(sep), ustrlen(s)));
   return 0;
#else
   if (len >= 1)
      sep[0] = '\0';

   return 0;
#endif
}

/* XXX what is this and what does it return? */
static int32_t al_fs_stdio_path_sep(size_t len, char *sep)
{
   char c = '/';

   if (len >= 2) {
      sep[0] = c;
      sep[1] = '\0';
   }

   return 0;
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup. */
static int32_t al_fs_stdio_path_to_sys(const char *orig, uint32_t len,
   char *path)
{
#ifdef ALLEGRO_WINDOWS

#else

#endif
   /* XXX what does this do? */
   return 0;
}

static int32_t al_fs_stdio_path_to_uni(const char *orig, uint32_t len,
   char *path)
{
   /* XXX what does this do? */
   return 0;
}


static int32_t al_fs_stdio_entry_exists(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   struct stat st;
   if (stat(fp_stdio->found ? fp_stdio->found : fp_stdio->path, &st) != 0) {
      if (errno == ENOENT) {
         return 0;
      }
      else {
         return -1;
      }
   }

   return 1;
}

static int32_t al_fs_stdio_file_exists(const char *path)
{
   struct stat st;
   if (stat(path, &st) != 0) {
      if (errno == ENOENT) {
         return 0;
      }
      else {
         return -1;
      }
   }

   return 1;
}

static int32_t al_fs_stdio_entry_unlink(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   int32_t err;

   ASSERT(fp);

   if (al_fs_stdio_entry_mode(fp) & AL_FM_ISDIR) {
      err = rmdir(fp_stdio->found ? fp_stdio->found : fp_stdio->path);
   }
   else if (al_fs_stdio_entry_mode(fp) & AL_FM_ISFILE) {
      err = unlink(fp_stdio->found ? fp_stdio->found : fp_stdio->path);
   }
   else {
      al_set_errno(ENOENT);
      return -1;
   }

   if (err != 0) {
      al_set_errno(errno);
      return -1;
   }

   return 0;
}


static int32_t al_fs_stdio_file_unlink(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   int32_t err;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return 0;

   err = al_fs_stdio_entry_unlink(fp);

   al_fs_stdio_destroy_handle(fp);

   return err;
}

static void al_fs_stdio_fname(ALLEGRO_FS_ENTRY *fp, size_t size, char *buf)
{
   ALLEGRO_FS_ENTRY_STDIO *fp_stdio = (ALLEGRO_FS_ENTRY_STDIO *) fp;
   uint32_t len = strlen(fp_stdio->path);

   memcpy(buf, fp_stdio->path, MIN(len+1, size));
}

static off_t al_fs_stdio_file_size(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   off_t size;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return 0;

   size = al_fs_stdio_entry_size(fp);
   al_fs_stdio_destroy_handle(fp);

   return size;
}

static uint32_t al_fs_stdio_file_mode(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   uint32_t mode;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return 0;

   mode = al_fs_stdio_entry_mode(fp);
   al_fs_stdio_destroy_handle(fp);

   return mode;
}

static time_t al_fs_stdio_file_atime(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   time_t atime;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return 0;

   atime = al_fs_stdio_entry_atime(fp);
   al_fs_stdio_destroy_handle(fp);

   return atime;
}

static time_t al_fs_stdio_file_mtime(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   time_t mtime;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return 0;

   mtime = al_fs_stdio_entry_mtime(fp);
   al_fs_stdio_destroy_handle(fp);

   return mtime;
}

static time_t al_fs_stdio_file_ctime(const char *path)
{
   ALLEGRO_FS_ENTRY *fp;
   time_t ctime;

   fp = al_fs_stdio_create_handle(path);
   if (!fp)
      return 0;

   ctime = al_fs_stdio_entry_ctime(fp);
   al_fs_stdio_destroy_handle(fp);

   return ctime;
}


struct ALLEGRO_FS_HOOK_SYS_INTERFACE _al_stdio_sys_fshooks = {
   al_fs_stdio_create_handle,
   al_fs_stdio_opendir,
   al_fs_stdio_fopen,
   al_fs_stdio_mktemp,

   al_fs_stdio_getcwd,
   al_fs_stdio_chdir,

   al_fs_stdio_add_search_path,
   al_fs_stdio_search_path_count,
   al_fs_stdio_get_search_path,

   al_fs_stdio_path_sep,
   al_fs_stdio_drive_sep,
   al_fs_stdio_path_to_sys,
   al_fs_stdio_path_to_uni,

   al_fs_stdio_file_exists,
   al_fs_stdio_file_unlink,

   al_fs_stdio_mkdir,

   al_fs_stdio_file_size,
   al_fs_stdio_file_mode,
   al_fs_stdio_file_atime,
   al_fs_stdio_file_mtime,
   al_fs_stdio_file_ctime
};

struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE _al_stdio_entry_fshooks = {
   al_fs_stdio_destroy_handle,
   al_fs_stdio_open_handle,
   al_fs_stdio_close_handle,

   al_fs_stdio_fname,
   al_fs_stdio_fclose,
   al_fs_stdio_fread,
   al_fs_stdio_fwrite,
   al_fs_stdio_fflush,
   al_fs_stdio_fseek,
   al_fs_stdio_ftell,
   al_fs_stdio_ferror,
   al_fs_stdio_feof,
   al_fs_stdio_fstat,
   al_fs_stdio_ungetc,

   al_fs_stdio_entry_size,
   al_fs_stdio_entry_mode,
   al_fs_stdio_entry_atime,
   al_fs_stdio_entry_mtime,
   al_fs_stdio_entry_ctime,

   al_fs_stdio_entry_exists,
   al_fs_stdio_entry_unlink,

   al_fs_stdio_readdir,
   al_fs_stdio_closedir
};

/* vim: set sts=3 sw=3 et: */
