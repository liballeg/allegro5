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
 *      Helper routines to make file.c work on DOS platforms.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <dos.h>
   #include <time.h>
   #include <string.h>
#endif

#include "allegro.h"
#include "allegro/aintern.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* _al_file_isok:
 *  Helper function to check if it is safe to access a file on a floppy
 *  drive.
 */
int _al_file_isok(AL_CONST char *filename)
{
   char ch = utolower(ugetc(filename));
   __dpmi_regs r;

   if (((ch == 'a') || (ch == 'b')) && (ugetat(filename, 1) == ':')) {
      r.x.ax = 0x440E;
      r.x.bx = 1;
      __dpmi_int(0x21, &r);

      if ((r.h.al != 0) && (r.h.al != (ch - 'a' + 1))) {
	 *allegro_errno = EACCES;
	 return FALSE;
      }
   }

   return TRUE;
}



/* _al_file_exists:
 *  Checks whether the specified file exists.
 */
int _al_file_exists(AL_CONST char *filename, int attrib, int *aret)
{
   struct ffblk dta;

   *allegro_errno = findfirst(uconvert_toascii(filename, NULL), &dta, attrib);

   if (*allegro_errno)
      return FALSE;

   if (aret)
      *aret = dta.ff_attrib;

   return TRUE;
}



/* _al_file_size:
 *  Measures the size of the specified file.
 */
long _al_file_size(AL_CONST char *filename)
{
   struct ffblk dta;

   *allegro_errno = findfirst(uconvert_toascii(filename, NULL), &dta, FA_RDONLY | FA_HIDDEN | FA_ARCH);

   if (*allegro_errno)
      return 0;

   return dta.ff_fsize;
}



/* _al_file_time:
 *  Returns the timestamp of the specified file.
 */
long _al_file_time(AL_CONST char *filename)
{
   struct ffblk dta;
   struct tm t;

   *allegro_errno = findfirst(uconvert_toascii(filename, NULL), &dta, FA_RDONLY | FA_HIDDEN | FA_ARCH);

   if (*allegro_errno)
      return 0;

   memset(&t, 0, sizeof(struct tm));

   t.tm_sec  = (dta.ff_ftime & 0x1F) * 2;
   t.tm_min  = (dta.ff_ftime >> 5) & 0x3F;
   t.tm_hour = (dta.ff_ftime >> 11) & 0x1F;
   t.tm_mday = (dta.ff_fdate & 0x1F);
   t.tm_mon  = ((dta.ff_fdate >> 5) & 0x0F) - 1;
   t.tm_year = (dta.ff_fdate >> 9) + 80;

   t.tm_isdst = -1;

   return mktime(&t);
}



/* _al_findfirst:
 *  Initiates a directory search.
 */
void *_al_findfirst(AL_CONST char *name, int attrib, char *nameret, int *aret)
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
   free(dta);
}



/* _al_getdrive:
 *  Returns the current drive number (0=A, 1=B, etc).
 */
int _al_getdrive()
{
   unsigned int d;

   _dos_getdrive(&d);

   return d-1;
}



/* _al_getdcwd:
 *  Returns the current directory on the specified drive.
 */
void _al_getdcwd(int drive, char *buf, int size)
{
   unsigned int old_drive, tmp_drive;
   char filename[32], tmp[256];
   int pos; 

   pos = usetc(filename, drive+'a');
   pos += usetc(filename+pos, ':');
   pos += usetc(filename+pos, '\\');
   usetc(filename+pos, 0);

   if (!_al_file_isok(filename)) {
      *buf = 0;
      return;
   }

   _dos_getdrive(&old_drive);
   _dos_setdrive(drive+1, &tmp_drive);
   _dos_getdrive(&tmp_drive);

   if (tmp_drive == (unsigned int)drive+1) {
      if (getcwd(tmp, sizeof(tmp)))
	 do_uconvert(tmp, U_ASCII, buf, U_CURRENT, size);
      else
	 usetc(buf, 0);
   }
   else
      usetc(buf, 0);

   _dos_setdrive(old_drive, &tmp_drive); 
}
