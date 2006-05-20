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
 *      Helper routines to make file.c work on Windows platforms.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <time.h>
   #include <sys/stat.h>
#endif

#include "allegro.h"
#include "winalleg.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif



/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive. This really only applies to the DOS library, so we don't bother
 *  with it.
 */
int _al_file_isok(AL_CONST char *filename)
{
   return TRUE;
}



/* _al_file_size:
 *  Measures the size of the specified file.
 */
long _al_file_size(AL_CONST char *filename)
{
   struct _stat s;
   char tmp[1024];

   if (_wstat((wchar_t*)uconvert(filename, U_CURRENT, tmp, U_UNICODE,
              sizeof(tmp)), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_size;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
time_t _al_file_time(AL_CONST char *filename)
{
   struct _stat s;
   char tmp[1024];

   if (_wstat((wchar_t*)uconvert(filename, U_CURRENT, tmp, U_UNICODE,
              sizeof(tmp)), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_mtime;
}



/* structure for use by the directory scanning routines */
struct FF_DATA
{
   struct _wfinddata_t data;
   long handle;
   int attrib;
};



/* fill_ffblk:
 *  Helper function to fill in an al_ffblk structure.
 */
static void fill_ffblk(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   info->attrib = ff_data->data.attrib;
   info->time = ff_data->data.time_write;
   info->size = ff_data->data.size;

   do_uconvert((const char*)ff_data->data.name, U_UNICODE, info->name, U_CURRENT,
               sizeof(info->name));
}



/* al_findfirst:
 *  Initiates a directory search.
 */
int al_findfirst(AL_CONST char *pattern, struct al_ffblk *info, int attrib)
{
   struct FF_DATA *ff_data;
   char tmp[1024];

   /* allocate ff_data structure */
   ff_data = _AL_MALLOC(sizeof(struct FF_DATA));

   if (!ff_data) {
      *allegro_errno = ENOMEM;
      return -1;
   }

   /* attach it to the info structure */
   info->ff_data = (void *) ff_data;

   /* Windows defines specific flags for NTFS permissions:
    *   FA_TEMPORARY            0x0100
    *   FA_SPARSE_FILE          0x0200 
    *   FA_REPARSE_POINT        0x0400
    *   FA_COMPRESSED           0x0800
    *   FA_OFFLINE              0x1000
    *   FA_NOT_CONTENT_INDEXED  0x2000
    *   FA_ENCRYPTED            0x4000
    * so we must set them in the mask by default; moreover,
    * in order to avoid problems with flags added in the
    * future, we simply set all bits past the first byte.
    */
   ff_data->attrib = attrib | 0xFFFFFF00;

   /* start the search */
   errno = *allegro_errno = 0;

   ff_data->handle = _wfindfirst((wchar_t*)uconvert(pattern, U_CURRENT, tmp,
                                 U_UNICODE, sizeof(tmp)), &ff_data->data);

   if (ff_data->handle < 0) {
      *allegro_errno = errno;
      _AL_FREE(ff_data);
      info->ff_data = NULL;
      return -1;
   }

   if (ff_data->data.attrib & ~ff_data->attrib) {
      if (al_findnext(info) != 0) {
         al_findclose(info);
         return -1;
      }
      else
         return 0;
   }

   fill_ffblk(info);
   return 0;
}



/* al_findnext:
 *  Retrieves the next file from a directory search.
 */
int al_findnext(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   do {
      if (_wfindnext(ff_data->handle, &ff_data->data) != 0) {
         *allegro_errno = errno;
         return -1;
      }

   } while (ff_data->data.attrib & ~ff_data->attrib);

   fill_ffblk(info);
   return 0;
}



/* al_findclose:
 *  Cleans up after a directory search.
 */
void al_findclose(struct al_ffblk *info)
{
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   if (ff_data) {
      _findclose(ff_data->handle);
      _AL_FREE(ff_data);
      info->ff_data = NULL;
   }
}



/* _al_drive_exists:
 *  Checks whether the specified drive is valid.
 */
int _al_drive_exists(int drive)
{
   return GetLogicalDrives() & (1 << drive);
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive(void)
{
   return _getdrive() - 1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   wchar_t tmp[1024];

   if (_wgetdcwd(drive+1, tmp, sizeof(tmp)/sizeof(tmp[0])))
      do_uconvert((const char*)tmp, U_UNICODE, buf, U_CURRENT, size);
   else
      usetc(buf, 0);
}


/* _al_ffblk_get_size:
 *  Returns the size out of an _al_ffblk structure.
 */
uint64_t al_ffblk_get_size(struct _al_ffblk *info)
{
   ASSERT(info);
   struct FF_DATA *ff_data = (struct FF_DATA *) info->ff_data;

   return ff_data->data.size;
}
