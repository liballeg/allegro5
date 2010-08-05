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
 *      List of system pathes for the Unix library.
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

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/fshook.h"
#include "allegro5/path.h"

#ifdef ALLEGRO_HAVE_SYS_UTSNAME_H
   #include <sys/utsname.h>
#endif

#ifdef ALLEGRO_HAVE_SV_PROCFS_H
   #include <sys/procfs.h>
   #include <sys/ioctl.h>
   #include <fcntl.h>
#endif

ALLEGRO_DEBUG_CHANNEL("upath")


#ifndef ALLEGRO_MACOSX
/* _find_executable_file:
 *  Helper function: searches path and current directory for executable.
 *  Returns 1 on succes, 0 on failure.
 */
static ALLEGRO_PATH *_find_executable_file(const char *filename)
{
   char *env;

   /* If filename has an explicit path, search current directory */
   if (strchr(filename, '/')) {
      if (filename[0] == '/') {
         /* Full path; done */
         return al_create_path(filename);
      }
      else {
         struct stat finfo;
         char pathname[1024];
    
         /* Prepend current directory */
         ALLEGRO_PATH *path = al_get_current_directory();
         al_append_path_component(path, filename);

         if ((stat(pathname, &finfo)==0) &&
            (!S_ISDIR (finfo.st_mode))) {
            return path;
         }
      }
   }
   /* If filename has no explicit path, but we do have $PATH, search
    * there
    */
   else if ((env = getenv("PATH"))) {
      struct stat finfo;
      ALLEGRO_USTR *us = al_ustr_new(env);
      int start_pos = 0;
      while (start_pos >= 0) {
         int next_start_pos = al_ustr_find_chr(us, start_pos + 1, ':');
         int end_pos = next_start_pos;
         if (next_start_pos < 0)
            end_pos = al_ustr_size(us);
         ALLEGRO_USTR_INFO info;
         ALLEGRO_USTR *sub = al_ref_ustr(&info, us, start_pos, end_pos);

         ALLEGRO_PATH *path = al_create_path_for_directory(al_cstr(sub));
         al_append_path_component(path, filename);

         if (stat(al_path_cstr(path, '/'), &finfo) == 0 &&
            !S_ISDIR (finfo.st_mode)) {
            return path;
         }
         start_pos = next_start_pos;
      }
   }

   return NULL;
}

/* Return full path to the current executable, use proc fs if
 * available.
 */
static ALLEGRO_PATH *get_executable_name(void)
{
   ALLEGRO_PATH *path;
   #ifdef ALLEGRO_HAVE_GETEXECNAME
   {
      const char *s = getexecname();
      if (s) {
         if (s[0] == '/') {   /* Absolute path */
            return al_create_path(s);
         }
         else {               /* Not an absolute path */
            path = _find_executable_file(s);
            if (path)
               return path;
         }
      }
   }
   #endif

   /* We need the PID in order to query procfs */
   pid_t pid = getpid();

   /* Try a Linux-like procfs */   
   /* get symolic link to executable from proc fs */
   char linkname[1024];
   char filename[1024];
   struct stat finfo;
   sprintf(linkname, "/proc/%d/exe", (int)pid);
   if (stat(linkname, &finfo) == 0) {
      int len = readlink(linkname, filename, sizeof(filename) - 1);
      if (len > -1) {
         filename[len] = '\0';
         return al_create_path(filename);
      }
   }

   /* Use System V procfs calls if available */
#ifdef ALLEGRO_HAVE_SV_PROCFS_H
   struct prpsinfo psinfo;
   int fd;
   sprintf(linkname, "/proc/%d/exe", (int)pid);
   fd = open(linkname, O_RDONLY);
   if (!fd == -1) {
      ioctl(fd, PIOCPSINFO, &psinfo);
      close(fd);
   
   /* Use argv[0] directly if we can */
#ifdef ALLEGRO_HAVE_PROCFS_ARGCV
      if (psinfo.pr_argv && psinfo.pr_argc) {
          path = _find_executable_file(psinfo.pr_argv[0]);
          if (path)
            return path;
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
         if (s)
            s[0] = '\0';
         path = _find_executable_file(psinfo.pr_psargs);
         if (path)
            return path;
      }
   
      /* Try the pr_fname just for completeness' sake if argv[0] fails */
      path = _find_executable_file(psinfo.pr_fname);
      if (path)
         return path;
   }
#endif

   /* Last resort: try using the output of the ps command to at least find */
   /* the name of the file if not the full path */
   char command[1024];
   sprintf(command, "ps -p %d", (int)pid);
   FILE *pipe = popen(command, "r");
   if (pipe) {
      char* ret;
      /* The first line of output is a header */
      ret = fgets(linkname, sizeof(linkname), pipe);
      if(!ret)
         ALLEGRO_ERROR("Failed to read the name of the executable file.\n");
      
      /* The information we want is in the last column; find it */
      int len = strlen(linkname);
      while (linkname[len] != ' ' && linkname[len] != '\t')
         len--;

      /* The second line contains the info we want */
      ret = fgets(linkname, sizeof(linkname), pipe);
      if(!ret)
         ALLEGRO_ERROR("Failed to read the name of the executable file.\n");
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

      path = _find_executable_file(filename);
      if (path)
         return path;

      /* Just return the output from ps... */         
      return al_create_path(filename);
   }

   /* Give up; return empty string */
   return al_create_path("");
}

#endif


static ALLEGRO_PATH *_unix_find_home(void)
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
         return NULL;
      }

      if (pass->pw_dir) {
         /* hey, we got our home directory */
         return al_create_path_for_directory(pass->pw_dir);
      }
      al_set_errno(ENOENT);
      return NULL;
   }
   else {
      return al_create_path_for_directory(home_env);
   }
}

ALLEGRO_PATH *_al_unix_get_path(int id)
{
   switch (id) {
      case ALLEGRO_TEMP_PATH: {
         /* Check: TMP, TMPDIR, TEMP or TEMPDIR */
         char *envs[] = { "TMP", "TMPDIR", "TEMP", "TEMPDIR", NULL};
         uint32_t i = 0;
         for (; envs[i] != NULL; ++i) {
            char *tmp = getenv(envs[i]);
            if (tmp) {
               return al_create_path(tmp);
            }
         }

         /* next try: /tmp /var/tmp /usr/tmp */
         char *paths[] = { "/tmp/", "/var/tmp/", "/usr/tmp/", NULL };
         for (i=0; paths[i] != NULL; ++i) {
            ALLEGRO_FS_ENTRY *fse = al_create_fs_entry(paths[i]);
            bool found = al_fs_entry_is_directory(fse);
            al_destroy_fs_entry(fse);
            if (found) {
               return al_create_path(paths[i]);
            }
         }

         /* Give up? */
         return NULL;
      } break;

      case ALLEGRO_PROGRAM_PATH: {

         ALLEGRO_PATH *exe = get_executable_name();
         al_set_path_filename(exe, NULL);
         return exe;

      } break;

      case ALLEGRO_SYSTEM_DATA_PATH: {
         ALLEGRO_PATH *sys_data_path = NULL;

         /* FIXME: make this a compile time define, or a allegro cfg option? or both */
         sys_data_path = al_create_path("/usr/share/");
         al_append_path_component(sys_data_path, al_get_org_name());
         al_append_path_component(sys_data_path, al_get_app_name());

         return sys_data_path;
      } break;

#if 0
      case ALLEGRO_USER_DATA_PATH: {
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

         get_executable_name(prog, PATH_MAX);

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
         do_uconvert(path, U_ASCII, dir, U_UTF8, strlen(path)+1);

      } break;
#endif

      case ALLEGRO_USER_SETTINGS_PATH:
      case ALLEGRO_USER_DATA_PATH: {
         ALLEGRO_PATH *local_path = NULL;

         local_path = _unix_find_home();
         if (!local_path)
            return NULL;

         al_append_path_component(local_path, ".config");
         al_append_path_component(local_path, al_get_org_name());
         al_append_path_component(local_path, al_get_app_name());

        return local_path;
      } break;

      case ALLEGRO_USER_HOME_PATH:
         return _unix_find_home();

      case ALLEGRO_SYSTEM_SETTINGS_PATH: {
         ALLEGRO_PATH *sys_path;

         /* FIXME: make this a compile time define, or something */
         sys_path = al_create_path("/etc/");
         al_append_path_component(sys_path, al_get_org_name());
         al_append_path_component(sys_path, al_get_app_name());

         return sys_path;
      } break;

      case ALLEGRO_EXENAME_PATH:
         return get_executable_name();
         break;

      default:
         return NULL;
   }

   return NULL;
}
