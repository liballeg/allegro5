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
 *      Helper routines to make file.c work on Unix (Posix) platforms.
 *      The findfirst() emulation is in libc.c
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include "allegro.h"
#include "allegro/aintern.h"



/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive.
 */
int _al_file_isok(const char *filename)
{
   return TRUE;
}



/* _al_file_exists:
 *  Checks whether the specified file exists.
 */
int _al_file_exists(const char *filename, int attrib, int *aret)
{
   struct ffblk dta;

   *allegro_errno = findfirst(uconvert_toascii(filename, NULL), &dta, attrib);

   if (*allegro_errno)
      return FALSE;

   if (aret)
      *aret = dta.ff_attrib;

   #ifdef findclose
      findclose(&dta);
   #endif

   return TRUE;
}



/* _al_file_size:
 *  Measures the size of the specified file.
 */
long _al_file_size(const char *filename)
{
   struct stat s;

   if (stat(uconvert_toascii(filename, NULL), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_size;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
long _al_file_time(const char *filename)
{
   struct stat s;

   if (stat(uconvert_toascii(filename, NULL), &s) != 0) {
      *allegro_errno = errno;
      return 0;
   }

   return s.st_mtime;
}



/* _al_findfirst:
 *  Initiates a directory search.
 */
void *_al_findfirst(const char *name, int attrib, char *nameret, int *aret)
{
   struct ffblk *info;

   info = malloc(sizeof(struct ffblk));

   if (!info) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   *allegro_errno = findfirst(uconvert_toascii(name, NULL), info, attrib);

   if (*allegro_errno) {
      free(info);
      return NULL;
   }

   do_uconvert(info->ff_name, U_ASCII, nameret, U_CURRENT, -1);

   if (aret)
      *aret = info->ff_attrib;

   return info;
}



/* _al_findnext:
 *  Retrives the next file from a directory search.
 */
int _al_findnext(void *dta, char *nameret, int *aret)
{
   struct ffblk *info = (struct ffblk *)dta;

   *allegro_errno = findnext(info);

   if (*allegro_errno)
      return -1;

   do_uconvert(info->ff_name, U_ASCII, nameret, U_CURRENT, -1);

   if (aret)
      *aret = info->ff_attrib;

   return 0;
}



/* _al_findclose:
 *  Cleans up after a directory search.
 */
void _al_findclose(void *dta)
{
   #ifdef findclose
      findclose(dta);
   #endif

   free(dta);

}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   char tmp[256];

   if (getcwd(tmp, sizeof(tmp)))
      do_uconvert(tmp, U_ASCII, buf, U_CURRENT, size);
   else
      usetc(buf, 0);
}

