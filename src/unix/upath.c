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

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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
         char *cwd;

         /* Prepend current directory */
         cwd = al_get_current_directory();
         if (cwd) {
            ALLEGRO_PATH *path = al_create_path_for_directory(cwd);
            al_free(cwd);
            al_set_path_filename(path, filename);

            if (stat(al_path_cstr(path, '/'), &finfo) == 0
                  && !S_ISDIR(finfo.st_mode)) {
               return path;
            }

            al_destroy_path(path);
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
         const ALLEGRO_USTR *sub = al_ref_ustr(&info, us, start_pos, end_pos);

         ALLEGRO_PATH *path = al_create_path_for_directory(al_cstr(sub));
         al_set_path_filename(path, filename);

         if (stat(al_path_cstr(path, '/'), &finfo) == 0 &&
               !S_ISDIR (finfo.st_mode)) {
            al_ustr_free(us);
            return path;
         }

         al_destroy_path(path);

         start_pos = next_start_pos;
      }
      al_ustr_free(us);
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
   if (fd != -1) {
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
      if (!ret)
         ALLEGRO_ERROR("Failed to read the name of the executable file.\n");

      /* The information we want is in the last column; find it */
      int len = strlen(linkname);
      while (linkname[len] != ' ' && linkname[len] != '\t')
         len--;

      /* The second line contains the info we want */
      ret = fgets(linkname, sizeof(linkname), pipe);
      if (!ret)
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

static ALLEGRO_PATH *follow_symlinks(ALLEGRO_PATH *path)
{
   for (;;) {
      const char *path_str = al_path_cstr(path, '/');
      char buf[PATH_MAX];
      int len;

      len = readlink(path_str, buf, sizeof(buf) - 1);
      if (len <= 0)
         break;
      buf[len] = '\0';
      al_destroy_path(path);
      path = al_create_path(buf);
   }

   /* Make absolute path. */
   {
      const char *cwd = al_get_current_directory();
      ALLEGRO_PATH *cwd_path = al_create_path_for_directory(cwd);
      if (al_rebase_path(cwd_path, path))
         al_make_path_canonical(path);
      al_destroy_path(cwd_path);
      al_free((void *) cwd);
   }

   return path;
}

#endif

#define XDG_MAX_PATH_LEN 1000

/* get_xdg_path - locate an XDG user dir
 */
static ALLEGRO_PATH *_get_xdg_path(const char *location)
{
   ALLEGRO_PATH *location_path = NULL;
   ALLEGRO_PATH *xdg_config_path = NULL;
   ALLEGRO_FILE *xdg_config_file = NULL;
   const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
   int fd;

   if (xdg_config_home) {
      /* use $XDG_CONFIG_HOME since it exists */
      xdg_config_path = al_create_path_for_directory(xdg_config_home);
   }
   else {
      /* the default XDG location is ~/.config */
      xdg_config_path = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
      if (!xdg_config_path) return NULL;
      al_append_path_component(xdg_config_path, ".config");
   }

   al_set_path_filename(xdg_config_path, "user-dirs.dirs");
   fd = open(al_path_cstr(xdg_config_path, '/'), O_RDONLY);
   if (fd != -1) {
     xdg_config_file = al_fopen_fd(fd, "r");
   }
   al_destroy_path(xdg_config_path);

   if (!xdg_config_file) return NULL;

   while (!al_feof(xdg_config_file)) {
      char line[XDG_MAX_PATH_LEN];      /* one line of the config file */
      const char *p = line;             /* where we're at in the line */
      char component[XDG_MAX_PATH_LEN]; /* the path component being parsed */
      int i = 0;                        /* how long the current component is */

      al_fgets(xdg_config_file, line, XDG_MAX_PATH_LEN);

      /* skip leading white space */
      while (*p == ' ' || *p == '\t') p++;

      /* skip the line if it does not begin with XDG_location_DIR */
      if (strncmp(p, "XDG_", 4)) continue;
      p += 4;

      if (strncmp(p, location, strlen(location))) continue;
      p += strlen(location);

      if (strncmp(p, "_DIR", 4)) continue;
      p += 4;

      /* skip past the =", allowing for white space */
      while (*p == ' ' || *p == '\t') p++;
      if (*p++ != '=') continue;
      while (*p == ' ' || *p == '\t') p++;
      if (*p++ != '"') continue;

      /* We've found the right line. Now parse it, basically assuming
         that it is in a sane format.
       */
      if (!strncmp(p, "$HOME", 5)) {
         /* $HOME is the only environment variable that the path is
            allowed to use, and it must be first, by specification. */
         location_path = al_get_standard_path(ALLEGRO_USER_HOME_PATH);
         p += 5;
      }
      else {
         location_path = al_create_path("/");
      }

      while (*p) {
         if (*p == '"' || *p == '/') {
            /* add the component (if non-empty) to the path */
            if (i > 0) {
               component[i] = 0;
               al_append_path_component(location_path, component);
               i = 0;
            }
            if (*p == '"') break;
         }
         else {
            if (*p == '\\') {
               /* treat any escaped character as a literal */
               p++;
               if (!*p) break;
            }
            component[i++] = *p;
         }

         p++;
      }

      /* Finished parsing the path. */
      break;
   }

   al_fclose(xdg_config_file);

   return location_path;
}

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
               return al_create_path_for_directory(tmp);
            }
         }

         /* next try: /tmp /var/tmp /usr/tmp */
         char *paths[] = { "/tmp/", "/var/tmp/", "/usr/tmp/", NULL };
         for (i=0; paths[i] != NULL; ++i) {
            ALLEGRO_FS_ENTRY *fse = al_create_fs_entry(paths[i]);
            bool found = (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR) != 0;
            al_destroy_fs_entry(fse);
            if (found) {
               return al_create_path_for_directory(paths[i]);
            }
         }

         /* Give up? */
         return NULL;
      } break;

      case ALLEGRO_RESOURCES_PATH: {
         ALLEGRO_PATH *exe = get_executable_name();
         exe = follow_symlinks(exe);
         al_set_path_filename(exe, NULL);
         return exe;

      } break;

      case ALLEGRO_USER_DATA_PATH:
      case ALLEGRO_USER_SETTINGS_PATH: {
         ALLEGRO_PATH *local_path = NULL;
         const char *org_name = al_get_org_name();
         const char *app_name = al_get_app_name();

         /* to avoid writing directly into the user's directory, require at least an app name */
         if (!app_name)
            return NULL;

         /* find the appropriate path from the xdg environment variables, if possible */
         if (id == ALLEGRO_USER_DATA_PATH) {
            const char *xdg_data_home = getenv("XDG_DATA_HOME");
            local_path = al_create_path_for_directory(xdg_data_home ? xdg_data_home : ".local/share");
         }
         else {
            const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
            local_path = al_create_path_for_directory(xdg_config_home ? xdg_config_home : ".config");
         }

         if (!local_path)
            return NULL;

         /* if the path is relative, prepend the user's home directory */
         if (al_path_cstr(local_path, '/')[0] != '/') {
            ALLEGRO_PATH *home_path = _unix_find_home();
            if (!home_path)
               return NULL;

            al_rebase_path(home_path, local_path);
            al_destroy_path(home_path);
         }

         /* only add org name if not blank */
         if (org_name && org_name[0]) {
            al_append_path_component(local_path, al_get_org_name());
         }

         al_append_path_component(local_path, al_get_app_name());

        return local_path;
      } break;

      case ALLEGRO_USER_HOME_PATH:
         return _unix_find_home();

      case ALLEGRO_USER_DOCUMENTS_PATH: {
         ALLEGRO_PATH *local_path = _get_xdg_path("DOCUMENTS");
         return local_path ? local_path : _unix_find_home();
      } break;

      case ALLEGRO_EXENAME_PATH:
         return get_executable_name();
         break;

      default:
         return NULL;
   }

   return NULL;
}

/* vim: set sts=3 sw=3 et: */
