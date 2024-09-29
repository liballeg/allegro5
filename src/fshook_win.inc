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
 *      Helper routines for fshook implementation on Windows.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <windows.h>

#if defined(A5O_MSVC)

#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)


/*
 * MSVC is missing the whole dirent.h so we implement the bits we need here.
 * The following block is copied from dirent.c from MinGW Runtime sources
 * version 3.15.1 with minor modifications.  The code was public domain.
 */

#ifdef _UNICODE
   #define _TDIR        _WDIR
   #define _tdirent     _wdirent
   #define _topendir    _wopendir
   #define _tclosedir   _wclosedir
   #define _treaddir    _wreaddir
#else
   #error _UNICODE not defined
#endif

#define  SUFFIX _T("*")
#define  SLASH _T("\\")

struct _wdirent
{
   long           d_ino;            /* Always zero. */
   unsigned short d_reclen;         /* Always zero. */
   unsigned short d_namlen;         /* Length of name in d_name. */
   wchar_t        d_name[FILENAME_MAX]; /* File name. */
};

/*
 * This is an internal data structure. Good programmers will not use it
 * except as an argument to one of the functions below.
 * dd_stat field is now int (was short in older versions).
 */
typedef struct
{
   /* disk transfer area for this dir */
   struct _wfinddata_t dd_dta;

   /* dirent struct to return from dir (NOTE: this makes this thread
    * safe as long as only one thread uses a particular DIR struct at
    * a time) */
   struct _wdirent dd_dir;

   /* _findnext handle */
   intptr_t dd_handle;

   /*
    * Status of search:
    *   0 = not started yet (next entry to read is first entry)
    *  -1 = off the end
    *   positive = 0 based index of next entry
    */
   int dd_stat;

   /* given path for dir with search pattern (struct is extended) */
   wchar_t dd_name[1];
} _WDIR;


/*
 * opendir
 *
 * Returns a pointer to a DIR structure appropriately filled in to begin
 * searching a directory.
 */
static _TDIR *_topendir(const _TCHAR *szPath)
{
   _TDIR *nd;
   unsigned int rc;
   _TCHAR szFullPath[MAX_PATH];

   errno = 0;

   if (!szPath) {
      errno = EFAULT;
      return (_TDIR *) 0;
   }

   if (szPath[0] == _T('\0')) {
      errno = ENOTDIR;
      return (_TDIR*) 0;
   }

   /* Attempt to determine if the given path really is a directory. */
   rc = GetFileAttributes(szPath);
   if (rc == (unsigned int)-1) {
      /* call GetLastError for more error info */
      errno = ENOENT;
      return (_TDIR*) 0;
   }
   if (!(rc & FILE_ATTRIBUTE_DIRECTORY)) {
      /* Error, entry exists but not a directory. */
      errno = ENOTDIR;
      return (_TDIR*) 0;
   }

   /* Make an absolute pathname.  */
   _tfullpath(szFullPath, szPath, MAX_PATH);

   /* Allocate enough space to store DIR structure and the complete
    * directory path given. */
   nd = (_TDIR *) al_malloc(sizeof (_TDIR) + (_tcslen(szFullPath) + _tcslen(SLASH)
      + _tcslen(SUFFIX) + 1) * sizeof(_TCHAR));

   if (!nd) {
      /* Error, out of memory. */
      errno = ENOMEM;
      return (_TDIR*) 0;
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
static struct _tdirent* _treaddir(_TDIR* dirp)
{
   errno = 0;

   /* Check for valid DIR struct. */
   if (!dirp) {
     errno = EFAULT;
     return (struct _tdirent*) 0;
   }

   if (dirp->dd_stat < 0) {
      /* We have already returned all files in the directory
       * (or the structure has an invalid dd_stat). */
      return (struct _tdirent*) 0;
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
      dirp->dd_dir.d_namlen = (unsigned short)_tcslen(dirp->dd_dta.name);
      _tcscpy(dirp->dd_dir.d_name, dirp->dd_dta.name);
      return &dirp->dd_dir;
   }

   return (struct _tdirent*) 0;
}


/*
 * closedir
 *
 * Frees up resources allocated by opendir.
 */
static int _tclosedir(_TDIR* dirp)
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

#endif /* A5O_MSVC */

/* vim: set ft=c sts=3 sw=3 et: */
