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
 *      List of system drivers for the Unix library.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include <unistd.h>

#ifdef _POSIX_PRIORITY_SCHEDULING
   /* Manpages say systems providing sched_yield() define
    * _POSIX_PRIORITY_SCHEDULING in unistd.h
    */
   #include <sched.h>
#endif

#include "allegro.h"
#include "allegro/aintunix.h"

#ifdef HAVE_SYS_UTSNAME_H
   #include <sys/utsname.h>
#endif



/* list the available drivers */
_DRIVER_INFO _system_driver_list[] =
{
#ifdef ALLEGRO_WITH_XWINDOWS
   {  SYSTEM_XWINDOWS,  &system_xwin,     TRUE  },
#endif
#ifdef ALLEGRO_LINUX
   {  SYSTEM_LINUX,     &system_linux,    TRUE  },
#endif
   {  SYSTEM_NONE,      &system_none,     FALSE },
   {  0,                NULL,             0     }
};



/* _unix_find_resource:
 *  Helper for locating a Unix config file. Looks in the home directory
 *  of the current user, and in /etc.
 */
int _unix_find_resource(char *dest, char *resource, int size)
{
   char buf[256], tmp[256];
   char *home = getenv("HOME");

   if (home) {
      /* look for ~/file */
      append_filename(buf, uconvert_ascii(home, tmp), resource, sizeof(buf));
      if (exists(buf)) {
	 ustrncpy(dest, buf, size-ucwidth(0));
	 return 0;
      }

      /* if it is a .cfg, look for ~/.filerc */
      if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
	 ustrncpy(buf, uconvert_ascii(home, tmp), sizeof(buf)-ucwidth(0));
	 put_backslash(buf);
	 ustrncat(buf, uconvert_ascii(".", tmp), sizeof(buf)-ucwidth(0));
	 ustrncpy(tmp, resource, sizeof(tmp)-ucwidth(0));
	 ustrncat(buf, ustrtok(tmp, "."), sizeof(buf)-ucwidth(0));
	 ustrncat(buf, uconvert_ascii("rc", tmp), sizeof(buf)-ucwidth(0));
	 if (file_exists(buf, FA_ARCH | FA_RDONLY | FA_HIDDEN, NULL)) {
	    ustrncpy(dest, buf, size-ucwidth(0));
	    return 0;
	 }
      }
   }

   /* look for /etc/file */
   append_filename(buf, uconvert_ascii("/etc/", tmp), resource, sizeof(buf));
   if (exists(buf)) {
      ustrncpy(dest, buf, size-ucwidth(0));
      return 0;
   }

   /* if it is a .cfg, look for /etc/filerc */
   if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
      ustrncpy(buf, uconvert_ascii("/etc/", tmp), sizeof(buf)-ucwidth(0));
      ustrncpy(tmp, resource, sizeof(tmp)-ucwidth(0));
      ustrncat(buf, ustrtok(tmp, "."), sizeof(buf)-ucwidth(0));
      ustrncat(buf, uconvert_ascii("rc", tmp), sizeof(buf)-ucwidth(0));
      if (exists(buf)) {
	 ustrncpy(dest, buf, size-ucwidth(0));
	 return 0;
      }
   }

   return -1;
}



/* _read_os_type:
 *  Set the os_type variable to something sensible.
 */
void _read_os_type()
{
   #ifdef HAVE_SYS_UTSNAME_H

      struct utsname utsn;
      uname(&utsn);

      /* try to detect Unix systems we know of */
      if (!strcmp(utsn.sysname, "Linux")) {
	 os_type = OSTYPE_LINUX;
      }
      else {
	 os_type = OSTYPE_UNIX;     /* that's all we can say for now */
      }

   #else

      os_type = OSTYPE_UNIX;

   #endif
}



/* _unix_sysdrv_yield_timeslice:
 *  Yields remaining timeslice portion to the system
 */
void _unix_yield_timeslice(void)
{
   #ifdef _POSIX_PRIORITY_SCHEDULING

      sched_yield();

   #else

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 1;
      select(0, NULL, NULL, NULL, &timeout);

   #endif
}


