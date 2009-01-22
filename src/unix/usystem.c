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


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/fshook.h"

#if defined(ALLEGRO_HAVE_SCHED_YIELD) && defined(_POSIX_PRIORITY_SCHEDULING)
   /* ALLEGRO_HAVE_SCHED_YIELD is set by configure */
   /* Manpages say systems providing sched_yield() define
    * _POSIX_PRIORITY_SCHEDULING in unistd.h
    */
   #include <sched.h>
#else
   #include <sys/time.h>
#endif

#ifdef ALLEGRO_HAVE_SYS_UTSNAME_H
   #include <sys/utsname.h>
#endif

#ifdef ALLEGRO_HAVE_SV_PROCFS_H
   #include <sys/procfs.h>
   #include <sys/ioctl.h>
   #include <fcntl.h>
#endif

#include "allegro5/path.h"

/* _unix_find_resource:
 *  Helper for locating a Unix config file. Looks in the home directory
 *  of the current user, and in /etc.
 */
int _unix_find_resource(char *dest, AL_CONST char *resource, int size)
{
   char buf[256], tmp[256];
   char home[256], ext[256];
   ALLEGRO_PATH *path;

   _unix_get_path(AL_USER_HOME_PATH, home, sizeof(home));

   if (ustrlen(home)) {
      ALLEGRO_PATH *local_path;

      local_path = al_path_create(home);
      if (!local_path) {
         return -1;
      }

      al_path_append(local_path, resource);
      al_path_to_string(local_path, buf, sizeof(buf), '/');

      if (al_is_present_str(buf)) {
         ustrzcpy(dest, size, buf);
         return 0;
      }

      /* if it is a .cfg, look for ~/.filerc */
      if (ustricmp(al_path_get_extension(local_path, ext, sizeof(ext)), uconvert_ascii("cfg", tmp)) == 0) {

         al_path_get_basename(local_path, buf, sizeof(buf));
         ustrncat(buf, "rc", 2);
         al_path_set_filename(local_path, buf);
         al_path_to_string(local_path, buf, sizeof(buf), '/');

         /* FIXME: we're temporarily forgetting about these permission flags
	 if (file_exists(buf, FA_ARCH | FA_RDONLY | FA_HIDDEN, NULL)) {
         */
         if (al_fs_exists(buf)) {
	    ustrzcpy(dest, size, buf);
	    return 0;
	 }
      }

      al_path_free(local_path);
   }

   /* look for /etc/file */
   path = al_path_create("/etc");
   al_path_set_filename(path, resource);
   al_path_to_string(path, buf, sizeof(buf), '/');

   if (al_fs_exists(buf)) {
      ustrzcpy(dest, size, buf);
      return 0;
   }

   /* if it is a .cfg, look for /etc/filerc */
   if (ustricmp(al_path_get_extension(path, ext, sizeof(ext)), uconvert_ascii("cfg", tmp)) == 0) {

      al_path_get_basename(path, buf, sizeof(buf));
      ustrncat(buf, "rc", 2);
      al_path_set_filename(path, buf);
      al_path_to_string(path, buf, sizeof(buf), '/');

      if (al_fs_exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }
   }

   al_path_free(path);

   /* if it is a .dat, look in AL_SYSTEM_DATA_PATH */
   path = al_path_create(al_get_path(AL_SYSTEM_DATA_PATH, buf, sizeof(buf)));
   al_path_set_filename(path, resource);

   if (ustricmp(al_path_get_extension(path, ext, sizeof(ext)), uconvert_ascii("dat", tmp)) == 0) {
      al_path_append(path, "allegro");
      al_path_to_string(path, buf, sizeof(buf), '/');
      if (al_fs_exists(buf)) {
	 ustrzcpy(dest, size, buf);
	 return 0;
      }
   }

   al_path_free(path);

   return -1;
}



#if 0

/* _unix_read_os_type:
 *  Set the os_type variable to something sensible.
 */
void _unix_read_os_type(void)
{
   #ifdef ALLEGRO_HAVE_SYS_UTSNAME_H

      struct utsname utsn;
      char *tmpstr, *tmpstr2;
      size_t pos;
      
      uname(&utsn);

      /* fetch OS version and revision */
      tmpstr = _AL_MALLOC_ATOMIC(strlen(utsn.release)+1);
      _al_sane_strncpy(tmpstr, utsn.release, strlen(utsn.release)+1);
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

      _AL_FREE(tmpstr);

      /* try to detect Unix systems we know of */
      if (!strcmp(utsn.sysname, "Linux")) {
	 os_type = OSTYPE_LINUX;
      }
      else if (!strcmp(utsn.sysname, "SunOS")) {
	 os_type = OSTYPE_SUNOS;
      }
      else if (!strcmp(utsn.sysname, "FreeBSD")) {
	 os_type = OSTYPE_FREEBSD;
      }
      else if (!strcmp(utsn.sysname, "NetBSD")) {
	 os_type = OSTYPE_NETBSD;
      }
      else if (!strcmp(utsn.sysname, "OpenBSD")) {
	 os_type = OSTYPE_OPENBSD;
      }
      else if ((!strcmp(utsn.sysname, "IRIX"))
	       || (!strcmp(utsn.sysname, "IRIX64"))) {
	 os_type = OSTYPE_IRIX;
      }
      else if (!strcmp(utsn.sysname, "Darwin")) {
	 os_type = OSTYPE_DARWIN;
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

#endif



/* _unix_sysdrv_yield_timeslice:
 *  Yields remaining timeslice portion to the system
 */
void _unix_yield_timeslice(void)
{
   #if defined(ALLEGRO_HAVE_SCHED_YIELD) && defined(_POSIX_PRIORITY_SCHEDULING)

      sched_yield();

   #else

      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      select(0, NULL, NULL, NULL, &timeout);

   #endif
}



#ifndef ALLEGRO_MACOSX
/* _find_executable_file:
 *  Helper function: searches path and current directory for executable.
 *  Returns 1 on succes, 0 on failure.
 */
static int _find_executable_file(const char *filename, char *output, int size)
{
   char *path;

   /* If filename has an explicit path, search current directory */
   if (strchr(filename, '/')) {
      if (filename[0] == '/') {
         /* Full path; done */
         do_uconvert(filename, U_ASCII, output, U_CURRENT, size);
         return 1;
      }
      else {
         struct stat finfo;
         char pathname[1024];
         int len;
            
	 /* Prepend current directory */
	 getcwd(pathname, sizeof(pathname));
	 len = strlen(pathname);
	 pathname[len] = '/';
	 _al_sane_strncpy(pathname+len+1, filename, strlen(filename));
            
	 if ((stat(pathname, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
	    do_uconvert(pathname, U_ASCII, output, U_CURRENT, size);
	    return 1;
	 }
      }
   }
   /* If filename has no explicit path, but we do have $PATH, search there */
   else if ((path = getenv("PATH"))) {
      char *start = path, *end = path, *buffer = NULL, *temp;
      struct stat finfo;

      while (*end) {
	 end = strchr(start, ':');
	 if (!end) end = strchr(start, '\0');

	 /* Resize `buffer' for path component, slash, filename and a '\0' */
	 temp = _AL_REALLOC (buffer, end - start + 1 + strlen (filename) + 1);
	 if (temp) {
	    buffer = temp;

	    _al_sane_strncpy(buffer, start, end - start);
	    *(buffer + (end - start)) = '/';
	    _al_sane_strncpy(buffer + (end - start) + 1, filename, end - start + 1 + strlen (filename) + 1);

	    if ((stat(buffer, &finfo)==0) && (!S_ISDIR (finfo.st_mode))) {
	       do_uconvert(buffer, U_ASCII, output, U_CURRENT, size);
	       _AL_FREE (buffer);
	       return 1;
	    }
	 } /* else... ignore the failure; `buffer' is still valid anyway. */

	 start = end + 1;
      }
      /* Path search failed */
      _AL_FREE (buffer);
   }
   
   return 0;
}

/* _unix_get_executable_name:
 *  Return full path to the current executable, use proc fs if available.
 */
void _unix_get_executable_name(char *output, int size)
{
   #ifdef ALLEGRO_HAVE_SV_PROCFS_H
      struct prpsinfo psinfo;
      int fd;
   #endif
   char linkname[1024];
   char filename[1024];
   struct stat finfo;
   FILE *pipe;
   pid_t pid;
   int len;
   
   #ifdef ALLEGRO_HAVE_GETEXECNAME
   {
      const char *s = getexecname();
      if (s) {
         if (s[0] == '/') {   /* Absolute path */
            do_uconvert(s, U_ASCII, output, U_CURRENT, size);
            return;
         }
         else {               /* Not an absolute path */
            if (_find_executable_file(s, output, size))
               return;
         }
      }
   }
   #endif

   /* We need the PID in order to query procfs */
   pid = getpid();

   /* Try a Linux-like procfs */   
   /* get symolic link to executable from proc fs */
   sprintf (linkname, "/proc/%d/exe", (int)pid);
   if (stat (linkname, &finfo) == 0) {
      len = readlink (linkname, filename, sizeof(filename)-1);
      if (len>-1) {
	 filename[len] = '\0';
         
	 do_uconvert(filename, U_ASCII, output, U_CURRENT, size);
	 return;
      }
   }
   
   /* Use System V procfs calls if available */
   #ifdef ALLEGRO_HAVE_SV_PROCFS_H
      sprintf (linkname, "/proc/%d/exe", (int)pid);
      fd = open(linkname, O_RDONLY);
      if (!fd == -1) {
         ioctl(fd, PIOCPSINFO, &psinfo);
         close(fd);
   
         /* Use argv[0] directly if we can */
      #ifdef ALLEGRO_HAVE_PROCFS_ARGCV
	 if (psinfo.pr_argv && psinfo.pr_argc) {
	    if (_find_executable_file(psinfo.pr_argv[0], output, size))
	       return;
	 }
	 else
      #endif
	 {
	    /* Emulate it */
	    /* We use the pr_psargs field to find argv[0]
	     * This is better than using the pr_fname field because we need
	     * the additional path information that may be present in argv[0]
	     */
	 
	    /* Skip other args */
	    char *s = strchr(psinfo.pr_psargs, ' ');
	    if (s) s[0] = '\0';
	    if (_find_executable_file(psinfo.pr_psargs, output, size))
	       return;
	 }

         /* Try the pr_fname just for completeness' sake if argv[0] fails */
         if (_find_executable_file(psinfo.pr_fname, output, size))
            return;
      }
   #endif
   
   /* Last resort: try using the output of the ps command to at least find */
   /* the name of the file if not the full path */
   uszprintf (linkname, sizeof(linkname), "ps -p %d", (int)pid);
   do_uconvert(linkname, U_CURRENT, filename, U_ASCII, size);
   pipe = popen(filename, "r");
   if (pipe) {
      /* The first line of output is a header */
      fgets(linkname, sizeof(linkname), pipe);
      
      /* The information we want is in the last column; find it */
      len = strlen(linkname);
      while (linkname[len] != ' ' && linkname[len] != '\t')
         len--;
      
      /* The second line contains the info we want */
      fgets(linkname, sizeof(linkname), pipe);
      pclose(pipe);
      
      /* Treat special cases: filename between [] and - for login shell */
      if (linkname[len] == '-')
         len++;

      if (linkname[len] == '[' && linkname[strlen(linkname)] == ']') {
         len++;
         linkname[strlen(linkname)] = '\0';
      }         
      
      /* Now, the filename should be in the last column */
      _al_sane_strncpy(filename, linkname+len+1, strlen(linkname)-len+1);
            
      if (_find_executable_file(filename, output, size))
         return;

      /* Just return the output from ps... */         
      do_uconvert(filename, U_ASCII, output, U_CURRENT, size);
      return;
   }

#ifdef ALLEGRO_WITH_MAGIC_MAIN
   /* Try the captured argv[0] */   
   if (_find_executable_file(__crt0_argv[0], output, size))
      return;
#endif

   /* Give up; return empty string */
   do_uconvert("", U_ASCII, output, U_CURRENT, size);
}

#endif


static int32_t _unix_find_home(char *dir, uint32_t len)
{
   char *home_env = getenv("HOME");

   if (!home_env || home_env[0] == '\0') {
      /* since HOME isn't set, we have to ask libc for the info */

      /* get user id */
      uid_t uid = getuid();

      /* grab user information */
      struct passwd *pass = getpwuid(uid);
      if (!pass) {
         al_set_errno(errno);
         return -1;
      }

      if (pass->pw_dir) {
         /* hey, we got our home directory */
         do_uconvert(pass->pw_dir, U_ASCII, dir, U_CURRENT, len);
         return 0;
      }

      al_set_errno(ENOENT);
      return -1;
   }
   else {
      do_uconvert(home_env, U_ASCII, dir, U_CURRENT, len);
      return 0;
   }

   /* should not reach here */
   return -1;
}

AL_CONST char *_unix_get_path(uint32_t id, char *dir, size_t size)
{
   switch (id) {
      case AL_TEMP_PATH: {
         /* Check: TMP, TMPDIR, TEMP or TEMPDIR */
         char *envs[] = { "TMP", "TMPDIR", "TEMP", "TEMPDIR", NULL};
         uint32_t i = 0;
         for (; envs[i] != NULL; ++i) {
            char *tmp = getenv(envs[i]);
            if (tmp) {
               /* this may truncate paths, not likely in unix */
               do_uconvert(tmp, U_ASCII, dir, U_CURRENT, strlen(tmp)+1);
               return dir;
            }
         }

         /* next try: /tmp /var/tmp /usr/tmp */
         char *paths[] = { "/tmp/", "/var/tmp/", "/usr/tmp/", NULL };
         for (i=0; paths[i] != NULL; ++i) {
            if (al_get_entry_mode_str(paths[i]) & AL_FM_ISDIR) {
               do_uconvert(paths[i], U_ASCII, dir, U_CURRENT, strlen(paths[i])+1);
               return dir;
            }
         }

         /* Give up? */
         return NULL;
      } break;

      case AL_PROGRAM_PATH: {
         char *ptr = NULL;

         _unix_get_executable_name(dir, size);

         ptr = ustrrchr(dir, '/');
         if (!ptr) {
            al_set_errno(EINVAL);
            return NULL;
         }

         ptr+=ucwidth(*ptr);
         usetc(ptr, 0);
         //ustrcat(ptr, "/");

      } break;

      case AL_SYSTEM_DATA_PATH: {
         char tmp[PATH_MAX] = "";
         ALLEGRO_PATH *sys_data_path = NULL;

         /* FIXME: make this a compile time define, or a allegro cfg option? or both */
         sys_data_path = al_path_create("/usr/share/");
         al_path_append(sys_data_path, al_get_orgname());
         al_path_append(sys_data_path, al_get_appname());
         al_path_to_string(sys_data_path, tmp, PATH_MAX, '/');
         if((size_t)(ustrlen(tmp)+1) > size) {
            al_path_free(sys_data_path);
            al_set_errno(ERANGE);
            return NULL;
         }

         al_path_free(sys_data_path);
         _al_sane_strncpy(dir, tmp, size);
      } break;

#if 0
      case AL_USER_DATA_PATH: {
         int32_t ret = 0;
         uint32_t path_len = 0, ptr_len = 0, prog_len = 0;
         char path[PATH_MAX] = "", *ptr = NULL;
         char prog[PATH_MAX] = "";

         if (_unix_find_home(path, PATH_MAX) != 0) {
            return NULL;
         }

         strncat(path, "/.", 2);

         path_len = strlen(path);

         /* get exe name */
         /* FIXME:
            This really aught to get the "Program" name from somewhere, say a config var? Or a function al_set_program_name()?
            making a ~/.test_program dir for a exe named "test_program" might not be what people have in mind.
         */

         _unix_get_executable_name(prog, PATH_MAX);

         ptr = strrchr(prog, '/');
         if (!ptr) {
            al_set_errno(EINVAL);
            return NULL;
         }

         *ptr = '\0';
         ptr++;
         ptr_len = strlen(ptr);
         //
         strncat(path, ptr, ptr_len+1);
         //*(ptr-1) = '/';
         do_uconvert(path, U_ASCII, dir, U_CURRENT, strlen(path)+1);

      } break;
#endif

      case AL_USER_SETTINGS_PATH:
      case AL_USER_DATA_PATH: {
         ALLEGRO_PATH *local_path = NULL;
         char tmp[PATH_MAX] = "";
         if (_unix_find_home(tmp, PATH_MAX) != 0) {
            return NULL;
         }

         if (tmp[strlen(tmp)-1] != '/')
            ustrcat(tmp, "/");

         local_path = al_path_create(tmp);
         al_path_append(local_path, ".config");
         al_path_append(local_path, al_get_orgname());
         al_path_append(local_path, al_get_appname());

         al_path_to_string(local_path, tmp, PATH_MAX, '/');

         if((size_t)(ustrlen(tmp)+1) > size)
            return NULL;

         ustrzcpy(dir, size, tmp);
      } break;

      case AL_USER_HOME_PATH: {
         char tmp[PATH_MAX] = "";
         if (_unix_find_home(tmp, PATH_MAX) != 0) {
            return NULL;
         }

         if (tmp[strlen(tmp)-1] != '/')
            ustrcat(tmp, "/");

         ustrcpy(dir, tmp);
         //printf("userhome/datapath: '%s'\n", tmp);
//         do_uconvert(tmp, U_ASCII, dir, U_CURRENT, strlen(tmp)+1);
      } break;

      case AL_SYSTEM_SETTINGS_PATH:
         /* FIXME: make this a compile time define, or a allegro cfg option? or both */
         _al_sane_strncpy(dir, "/etc/", strlen("/etc/")+1);
         break;

      case AL_EXENAME_PATH:
         _unix_get_executable_name(dir, size);
         break;

      default:
         return NULL;
   }

   return dir;
}

/* _unix_get_page_size:
 *  Get size of a memory page in bytes.  If we can't do it, we make a guess.
 */
size_t _unix_get_page_size(void)
{
#if defined(ALLEGRO_HAVE_SYSCONF) && defined(_SC_PAGESIZE)
   long page_size = sysconf(_SC_PAGESIZE); 
#else
   long page_size = -1;
#endif

   return (page_size == -1) ? 4096 : page_size;
}
