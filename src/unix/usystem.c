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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


#include "allegro.h"
#include "allegro/platform/aintunix.h"

#if defined(ALLEGRO_USE_SCHED_YIELD) && defined(_POSIX_PRIORITY_SCHEDULING)
   /* ALLEGRO_USE_SCHED_YIELD is set by configure */
   /* Manpages say systems providing sched_yield() define
    * _POSIX_PRIORITY_SCHEDULING in unistd.h
    */
   #include <sched.h>
#endif

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
#ifdef ALLEGRO_QNX
   {  SYSTEM_QNX,       &system_qnx,      TRUE  },
#endif
   {  SYSTEM_NONE,      &system_none,     FALSE },
   {  0,                NULL,             0     }
};



/* background function manager */
struct bg_manager *_unix_bg_man;



/* _unix_find_resource:
 *  Helper for locating a Unix config file. Looks in the home directory
 *  of the current user, and in /etc.
 */
int _unix_find_resource(char *dest, AL_CONST char *resource, int size)
{
   char buf[256], tmp[256], *last;
   char *home = getenv("HOME");

   if (home) {
      /* look for ~/file */
      append_filename(buf, uconvert_ascii(home, tmp), resource, sizeof(buf));
      if (exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }

      /* if it is a .cfg, look for ~/.filerc */
      if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
	 ustrzcpy(buf, sizeof(buf) - ucwidth(OTHER_PATH_SEPARATOR), uconvert_ascii(home, tmp));
	 put_backslash(buf);
	 ustrzcat(buf, sizeof(buf), uconvert_ascii(".", tmp));
	 ustrzcpy(tmp, sizeof(tmp), resource);
	 ustrzcat(buf, sizeof(buf), ustrtok_r(tmp, ".", &last));
	 ustrzcat(buf, sizeof(buf), uconvert_ascii("rc", tmp));
	 if (file_exists(buf, FA_ARCH | FA_RDONLY | FA_HIDDEN, NULL)) {
	    ustrzcpy(dest, size, buf);
	    return 0;
	 }
      }
   }

   /* look for /etc/file */
   append_filename(buf, uconvert_ascii("/etc/", tmp), resource, sizeof(buf));
   if (exists(buf)) {
      ustrzcpy(dest, size, buf);
      return 0;
   }

   /* if it is a .cfg, look for /etc/filerc */
   if (ustricmp(get_extension(resource), uconvert_ascii("cfg", tmp)) == 0) {
      ustrzcpy(buf, sizeof(buf), uconvert_ascii("/etc/", tmp));
      ustrzcpy(tmp, sizeof(tmp), resource);
      ustrzcat(buf, sizeof(buf), ustrtok_r(tmp, ".", &last));
      ustrzcat(buf, sizeof(buf), uconvert_ascii("rc", tmp));
      if (exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }
   }

   /* if it is a .dat, look in /usr/share/ and /usr/local/share/ */
   if (ustricmp(get_extension(resource), uconvert_ascii("dat", tmp)) == 0) {
      ustrzcpy(buf, sizeof(buf), uconvert_ascii("/usr/share/allegro/", tmp));
      ustrzcat(buf, sizeof(buf), resource);
      if (exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }
      ustrzcpy(buf, sizeof(buf), uconvert_ascii("/usr/local/share/allegro/", tmp));
      ustrzcat(buf, sizeof(buf), resource);
      if (exists(buf)) {
	 ustrzcpy(dest, size, buf);
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
      char *tmpstr, *tmpstr2;
      size_t pos;
      
      uname(&utsn);

      /* fetch OS version and revision */
      tmpstr = malloc(strlen(utsn.release)+1);
      strcpy(tmpstr, utsn.release);
      tmpstr2 = NULL;

      for (pos = 0; pos <= strlen(utsn.release); pos++) {
         if (tmpstr[pos] == '.') {
	    tmpstr[pos] = '\0';
	    if (!tmpstr2)
	       tmpstr2 = tmpstr + pos + 1;
	 }
      }

      os_version = atoi(tmpstr);
      os_revision = atoi(tmpstr2);

      free(tmpstr);

      /* try to detect Unix systems we know of */
      if (!strcmp(utsn.sysname, "Linux")) {
	 os_type = OSTYPE_LINUX;
      }
      else if (!strcmp(utsn.sysname, "FreeBSD")) {
	 os_type = OSTYPE_FREEBSD;
      }
      else if (!strcmp(utsn.sysname, "QNX")) {
	 os_type = OSTYPE_QNX;
      }
      else {
	 os_type = OSTYPE_UNIX;     /* that's all we can say for now */
      }

   #else

      os_type = OSTYPE_UNIX;

   #endif
   
   os_multitasking = TRUE;
}



/* _unix_sysdrv_yield_timeslice:
 *  Yields remaining timeslice portion to the system
 */
void _unix_yield_timeslice(void)
{
   #if defined(ALLEGRO_USE_SCHED_YIELD) && defined(_POSIX_PRIORITY_SCHEDULING)

      sched_yield();

   #else

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 1;
      select(0, NULL, NULL, NULL, &timeout);

   #endif
}



/* _unix_get_executable_name:
 *  Return full path to the current executable.
 */
void _unix_get_executable_name(char *output, int size)
{
   char *path;

   /* If argv[0] has no explicit path, but we do have $PATH, search there */
   if (!strchr (__crt0_argv[0], '/') && (path = getenv("PATH"))) {
      char *start = path, *end = path, *buffer = NULL, *temp;
      struct stat finfo;

      while (*end) {
	 end = strchr (start, ':');
	 if (!end) end = strchr (start, '\0');

	 /* Resize `buffer' for path component, slash, argv[0] and a '\0' */
	 temp = realloc (buffer, end - start + 1 + strlen (__crt0_argv[0]) + 1);
	 if (temp) {
	    buffer = temp;

	    strncpy (buffer, start, end - start);
	    *(buffer + (end - start)) = '/';
	    strcpy (buffer + (end - start) + 1, __crt0_argv[0]);

	    if ((stat(buffer, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
	       do_uconvert (buffer, U_ASCII, output, U_CURRENT, size);
	       free (buffer);
	       return;
	    }
	 } /* else... ignore the failure; `buffer' is still valid anyway. */

	 start = end + 1;
      }
      /* Path search failed */
      free (buffer);
   }

   /* If argv[0] had a slash, or the path search failed, just return argv[0] */
   do_uconvert (__crt0_argv[0], U_ASCII, output, U_CURRENT, size);
}

