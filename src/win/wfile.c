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


#include "allegro.h"
#include "allegro/aintern.h"

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



/* _al_file_exists:
 *  Checks whether the specified file exists.
 */
int _al_file_exists(AL_CONST char *filename, int attrib, int *aret)
{
   struct _finddata_t info;
   long handle;

   errno = *allegro_errno = 0;

   if ((handle = _findfirst(uconvert_toascii(filename, NULL), &info)) < 0) {
      *allegro_errno = errno;
      return FALSE;
   }

   _findclose(handle);

   if (aret)
      *aret = info.attrib;

   info.attrib &= (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC);

   if ((info.attrib & attrib) != info.attrib)
      return FALSE;

   return TRUE;
}



/* _al_file_size:
 *  Measures the size of the specified file.
 */
long _al_file_size(AL_CONST char *filename)
{
   struct _finddata_t info;
   long handle;

   errno = *allegro_errno = 0;

   if ((handle = _findfirst(uconvert_toascii(filename, NULL), &info)) < 0) {
      *allegro_errno = errno;
      return 0;
   }

   _findclose(handle);

   if (info.attrib & (FA_SYSTEM | FA_LABEL | FA_DIREC))
      return 0;

   return info.size;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
long _al_file_time(AL_CONST char *filename)
{
   struct _finddata_t info;
   long handle;

   errno = *allegro_errno = 0;

   if ((handle = _findfirst(uconvert_toascii(filename, NULL), &info)) < 0) {
      *allegro_errno = errno;
      return 0;
   }

   _findclose(handle);

   if (info.attrib & (FA_SYSTEM | FA_LABEL | FA_DIREC))
      return 0;

   return info.time_write;
}



/* information structure for use by the directory scanning routines */
typedef struct FFIND_INFO {
   struct _finddata_t info;
   long handle;
   int attrib;
} FFIND_INFO;



/* _al_findfirst:
 *  Initiates a directory search.
 */
void *_al_findfirst(AL_CONST char *name, int attrib, char *nameret, int *aret)
{
   FFIND_INFO *info;
   int a;

   info = malloc(sizeof(FFIND_INFO));

   if (!info) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   info->attrib = attrib;

   errno = *allegro_errno = 0;

   if ((info->handle = _findfirst(uconvert_toascii(name, NULL), &info->info)) < 0) {
      *allegro_errno = errno;
      free(info);
      return NULL;
   }

   a = info->info.attrib & (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC);

   if ((a & attrib) != a) {
      if (_al_findnext(info, nameret, aret) != 0) {
	 _findclose(info->handle);
	 free(info);
	 return NULL;
      }
      else
	 return info;
   }

   do_uconvert(info->info.name, U_ASCII, nameret, U_CURRENT, -1);

   if (aret)
      *aret = info->info.attrib;

   return info;
}



/* _al_findnext:
 *  Retrives the next file from a directory search.
 */
int _al_findnext(void *dta, char *nameret, int *aret)
{
   FFIND_INFO *info = (FFIND_INFO *) dta;
   int a;

   do {
      if (_findnext(info->handle, &info->info) != 0) {
	 *allegro_errno = errno;
	 return -1;
      }

      a = info->info.attrib & (FA_HIDDEN | FA_SYSTEM | FA_LABEL | FA_DIREC);

   } while ((a & info->attrib) != a);

   do_uconvert(info->info.name, U_ASCII, nameret, U_CURRENT, -1);

   if (aret)
      *aret = info->info.attrib;

   return 0;
}



/* _al_findclose:
 *  Cleans up after a directory search.
 */
void _al_findclose(void *dta)
{
   FFIND_INFO *info = (FFIND_INFO *) dta;

   _findclose(info->handle);
   free(info);
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive()
{
   return _getdrive() - 1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   char tmp[256];

   if (_getdcwd(drive+1, tmp, sizeof(tmp)))
      do_uconvert(tmp, U_ASCII, buf, U_CURRENT, size);
   else
      usetc(buf, 0);
}
