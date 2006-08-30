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
 *      File System Hook, stdio "driver".
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/debug.h"
#include "allegro/fshook.h"
#include "allegro/internal/fshook.h"

#include <stdio.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_DIRENT_H
   #include <sys/types.h>
   #include <dirent.h>
   #define NAMLEN(dirent) (strlen((dirent)->d_name))
#else
   #define dirent direct
   #define NAMLEN(dirent) ((dirent)->d_namlen)
   #ifdef HAVE_SYS_NDIR_H
      #include <sys/ndir.h>
   #endif
   #ifdef HAVE_SYS_DIR_H
      #include <sys/dir.h>
   #endif
   #ifdef HAVE_NDIR_H
      #include <ndir.h>
   #endif
#endif

#ifdef TIME_WITH_SYS_TIME
   #include <sys/time.h>
   #include <time.h>
#else
   #ifdef HAVE_SYS_TIME_H
      #include <sys/time.h>
   #else
      #include <time.h>
   #endif
#endif

#define PREFIX_I "al-fs-stdio INFO: "

uint32_t al_fs_init_stdio(void)
{

   al_fs_set_hook(AL_FS_HOOK_FOPEN,  al_fs_stdio_fopen);
   al_fs_set_hook(AL_FS_HOOK_FCLOSE, al_fs_stdio_fclose);
   al_fs_set_hook(AL_FS_HOOK_FREAD,  al_fs_stdio_fread);
   al_fs_set_hook(AL_FS_HOOK_FWRITE, al_fs_stdio_fwrite);
   al_fs_set_hook(AL_FS_HOOK_FFLUSH, al_fs_stdio_fflush);
   al_fs_set_hook(AL_FS_HOOK_FSEEK,  al_fs_stdio_fseek);
   al_fs_set_hook(AL_FS_HOOK_FTELL,  al_fs_stdio_ftell);
   al_fs_set_hook(AL_FS_HOOK_FERROR, al_fs_stdio_ferror);
   al_fs_set_hook(AL_FS_HOOK_FEOF,   al_fs_stdio_feof);

   al_fs_set_hook(AL_FS_HOOK_FSTAT, al_fs_stdio_fstat);

   al_fs_set_hook(AL_FS_HOOK_OPENDIR,  al_fs_stdio_opendir);
   al_fs_set_hook(AL_FS_HOOK_CLOSEDIR, al_fs_stdio_closedir);
   al_fs_set_hook(AL_FS_HOOK_READDIR,  al_fs_stdio_readdir);

   al_fs_set_hook(AL_FS_HOOK_MKTEMP, al_fs_stdio_mktemp);
   al_fs_set_hook(AL_FS_HOOK_GETCWD, al_fs_stdio_getcwd);
   al_fs_set_hook(AL_FS_HOOK_CHDIR,  al_fs_stdio_chdir);
   al_fs_set_hook(AL_FS_HOOK_GETDIR, al_fs_stdio_getdir);

   al_fs_set_hook(AL_FS_HOOK_ADD_SEARCH_PATH,   al_fs_stdio_add_search_path);
   al_fs_set_hook(AL_FS_HOOK_SEARCH_PATH_COUNT, al_fs_stdio_search_path_count);
   al_fs_set_hook(AL_FS_HOOK_GET_SEARCH_PATH,   al_fs_stdio_get_search_path);

   al_fs_set_hook(AL_FS_HOOK_GET_STAT_MODE,  al_fs_stdio_get_stat_mode);
   al_fs_set_hook(AL_FS_HOOK_GET_STAT_ATIME, al_fs_stdio_get_stat_atime);
   al_fs_set_hook(AL_FS_HOOK_GET_STAT_MTIME, al_fs_stdio_get_stat_mtime);
   al_fs_set_hook(AL_FS_HOOK_GET_STAT_CTIME, al_fs_stdio_get_stat_ctime);
   al_fs_set_hook(AL_FS_HOOK_GET_STAT_SIZE,  al_fs_stdio_get_stat_size);

   al_fs_set_hook(AL_FS_HOOK_PATH_TO_SYS, al_fs_stdio_path_to_sys);
   al_fs_set_hook(AL_FS_HOOK_PATH_TO_UNI, al_fs_stdio_path_to_uni);

   return 0;
}

AL_FILE *al_fs_stdio_fopen(const char *path, const char *mode)
{
   FILE *fh = fopen(path, mode);
   if(!fh) {
      *allegro_errno = errno;
      return NULL;
   }

   return (AL_FILE *)fh;
}

int32_t al_fs_stdio_fclose(AL_FILE *fp)
{
   int32_t ret = fclose(fp);
   if(ret == EOF) {
      *allegro_errno = errno;
   }

   return ret;
}

size_t   al_fs_stdio_fread(void *ptr, size_t size, AL_FILE *fp)
{
   return fread(ptr, 1, size, (FILE *)fp);
}

size_t   al_fs_stdio_fwrite(const void *ptr, size_t size, AL_FILE *fp)
{
   return fwrite(ptr, 1, size, (FILE *)fp);
}

int32_t al_fs_stdio_fflush(AL_FILE *fp)
{
   int32_t ret = fflush((FILE *)fp);
   if(ret == EOF) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_fseek(AL_FILE *fp, uint32_t offset, uint32_t whence)
{
   int32_t ret;

   switch(whence) {
      case AL_SEEK_SET: whence = SEEK_SET; break;
      case AL_SEEK_CUR: whence = SEEK_CUR; break;
      case AL_SEEK_END: whence = SEEK_END; break;
   }

   ret = fseek((FILE *)fp, offset, whence);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_ftell(AL_FILE *fp)
{
   int32_t ret = ftell((FILE *)fp);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

int32_t al_fs_stdio_ferror(AL_FILE *fp)
{
   return ferror((FILE *)fp);
}

int32_t al_fs_stdio_feof(AL_FILE *fp)
{
   return feof((FILE *)fp);
}


AL_STAT *al_fs_stdio_fstat(const char *path)
{
   struct stat *st = NULL;
   int32_t ret = 0;

   st = malloc(sizeof(struct stat));
   if(!st) {
      *allegro_errno = ENOMEM;
      return NULL;
   }

   ret = stat(path, st);
   if(ret == -1) {
      *allegro_errno = errno;
      free(st);
      return NULL;
   }

   return (AL_STAT *)st;
}


AL_DIR    *al_fs_stdio_opendir(const char *path)
{
   DIR *dir = opendir(path);
   if(!dir) {
      *allegro_errno = errno;
   }

   return (AL_DIR)dir;
}

int32_t   al_fs_stdio_closedir(AL_DIR *dir)
{
   int32_t ret = closedir((DIR *)dir);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

AL_DIRENT *al_fs_stdio_readdir(AL_DIR *dir)
{
   struct dirent *ent = readdir((struct dirent *)dir);
   if(!ent) {
      *allegro_errno = errno;
      return NULL;
   }

   return ent;
}


AL_FILE *al_fs_stdio_mktemp(const char *template)
{
   int32_t fd = mkstemp(template);
   FILE *fh = NULL;

   if(fd == -1) {
      *allegro_errno = errno;
      return NULL;
   }

   fh = fdopen(fd, "r+");
   if(!fh) {
      *allegro_errno = errno;
      close(fd);
      return NULL;
   }

   return fh;
}

int32_t al_fs_stdio_getcwd(char *buf, size_t len)
{
   char *cwd = getcwd(buf, len);
   if(!cwd) {
      *allegro_errno = errno;
      return -1;
   }

   return 0;
}

int32_t al_fs_stdio_chdir(const char *path)
{
   int32_t ret = chdir(path);
   if(ret == -1) {
      *allegro_errno = errno;
   }

   return ret;
}

#ifdef ALLEGRO_WINDOWS

/*
   Will at most copy MAX_PATH bytes including nul to "dir"
   May cause some truncation, in some cases.
*/
int32_t al_fs_stdio_getdir(uint32_t id, char *dir, uint32_t *len)
{
   char path[MAX_PATH], tmp[256];
   uint32_t csidl = 0, path_len = MIN(*len, MAX_PATH);
   HRESULT ret = 0;
   HANDLE process = GetCurrentProcess();

   switch(id) {
      case AL_SYSTEM_PROGRAM_DIR: /* CSIDL_PROGRAM_FILES + ApplicationName */
         csidl = CSIDL_PROGRAM_FILES;
         break;

      case AL_PROGRAM_DIR: { /* where the program is in */
         HMODULE module = GetModuleHandle(NULL); /* Get handle for this process */
         DWORD mret = GetModuleFileNameEx(process, handle, path, MAX_PATH);
         char *ptr = strrchr(path, '\\');
         if(!ptr) { /* shouldn't happen */
            return -1;
         }

         /* chop off everything including and after the last slash */
         /* should this not chop the slash? */
         *ptr = '\0';

         return 0;
      } break;

      case AL_SYSTEM_DATA_DIR: /* CSIDL_COMMON_APPDATA */
         csidl = CSIDL_COMMON_APPDATA;
         break;

      case AL_USER_DATA_DIR: /* CSIDL_APPDATA */
         csidl = CSIDL_APPDATA;
         break;

      case AL_USER_HOME_DIR: /* CSIDL_PROFILE */
         csidl = CSIDL_PROFILE;
         break;

      default:
         return -1;
   }

   ret = SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, path);
   if(ret != S_OK) {
      return -1;
   }

   _al_sane_strncpy(dir, path, path_len);

   *len = path_len;

   return 0;
}

#else /* Unix */

int32_t _al_find_home(char *dir, uint32_t len)
{
   char *home_env = getenv("home");
   if(!home_env) {
      /* since HOME isn't set, we have to ask libc for the info */

      /* get user id */
      uid_t uid = getuid();

      /* grab user information */
      struct passwd *pass = getpwuid(uid);
      if(!pass) {
         *allegro_errno = errno;
         return -1;
      }

      if(pass->pw_dir) {
         /* hey, we got our home directory */
         _al_sane_strncpy(dir, pass->pw_dir, path_max);
         return 0;
      }
      else if(pass->pw_name) {
         /* assume we live in /home/<username> */
         /* TODO: might want to make a "configure" option for this (/home) */
         /* should we check to see if the dir exists? */
/*
         struct stat st;
         char tmp[path_max] = "/home/";
         _al_sane_strncpy(tmp, pass->pw_name, path_max);

         if(stat(tmp, &st) != 0) {
            *allegro_errno = errno;
            return -1;
         }
*/
         _al_sane_strncpy(dir, "/home/", len);
         strncat(dir, pass->pw_name, len);
         return 0;
      }
      else {
         /* no home directory? */
         /* what to set allegro_errno to? */
         return -1;
      }
   }
   else {
      _al_sane_strncpy(dir, home_env, path_max);
      return 0;
   }

   /* should not reach here */
   return -1;
}

int32_t _al_find_progam_dir(char *dir, uint32_t len)
{
   char path[PATH_MAX];
   char *prog = system_driver->argv[0];
   char *ptr = NULL;
   uint32_t prog_len = strlen(prog), path_len = 0;

   /* program name is longer than PATH_MAX, trying to grab the proper dir may result in garbled paths */
   /* Wait, is this even possible? */
   if(prog_len > PATH_MAX) {
      *allegro_errno = errno = ERANGE;
      return -1;
   }

   /* absolute path */
   if(prog[0] == '/') {
      _al_sane_strncpy(path, prog, prog_len);

      ptr = strrchr(path, '/');
      if(!ptr) {
         *allegro_errno = errno = EINVAL;
         return -1;
      }

      /* chop off everything including and after the last slash */
      /* should this not chop the slash? */
      *ptr = '\0';

      path_len = (uint32_t)( ptr - path  );

      /* actual path is longer than the provided memory block, try agian with a larger block */
      if(path_len > len) {
         *allegro_errno = errno = ERANGE;
         return -1;
      }

      _al_sane_strncpy(dir, path, len);

      return 0;
   }
   else {
      uint32_t cwd_len = 0;
      char *cwd = getcwd(path, PATH_MAX);
      if(!cwd) {
         /* keep errno from getcwd */
         *allegro_errno = errno;
         return -1;
      }

      cwd_len = strlen(cwd);

      /* properly cleaning up the path is a little long winded, todo for later */
      /* eg: tilde/~ expansion, ./ removal, .. handling */
      if(cwd_len+prog_len+1 >= PATH_MAX) {
         *allegro_errno = errno = ERANGE;
         return -1;
      }

      strncat(path, prog, PATH_MAX);
      _al_sane_strncpy(dir, path, len);
      return 0;
   }

   /* shouldn't reach here */
   return -1;
}

/*

Return:
   0 on success
   -1 on failure and sets allegro_errno and errno to the error code

FIXME:
   Replace hard coded paths with "configure checks".
*/
int32_t al_fs_stdio_getdir(uint32_t id, char *dir, uint32_t *len)
{
   /* this code should probably malloc the path and tmp arrays,
      theres a damn good chance it'll consume the entire stack */

   switch(id) {
      case AL_SYSTEM_PROGRAM_DIR: /* /usr/bin */
         if(strlen("/usr/bin") > *len) {
            *allegro_errno = errno = ERANGE;
            return -1;
         }

         _al_sane_strncpy(dir, "/usr/bin", *len);
         break;

      case AL_PROGRAM_DIR:
         if(_al_find_progam_dir(dir, *len) != 0) {
            return -1;
         }
      break;

      case AL_SYSTEM_DATA_DIR:
         _al_sane_strncpy(dir, "/usr/share", *len);
         break;

      case AL_USER_DATA_DIR: {
         int32_t ret = 0;
         uint32_t path_len = 0, ptr_len = 0;
         char path[PATH_MAX], tmp[PATH_MAX], *ptr = NULL;

         if(_al_find_home(path, PATH_MAX) != 0) {
            return -1;
         }

         path_len = strlen(path);

         if(_al_find_progam_dir(tmp, PATH_MAX) != 0) {
            return -1;
         }

         ptr = strrchr(tmp, '/');
         if(!ptr) {
            return -1;
         }

         *ptr = '\0';
         ptr++;

         ptr_len = strlen(ptr);

         if(path_len + ptr_len > *len) {
            *allegro_errno = errno = ERANGE;
            return -1;
         }

         /* got exe name */

         strncat(path, ptr, PATH_MAX);
         _al_sane_strncpy(dir, path, *len);

         return 0;
      } break;

      case AL_USER_HOME_DIR:
         if(_al_find_home(dir, *len) != 0) {
            return -1;
         }

         break;

      default:
         return -1;
   }

   return 0;
}

#endif

static char **search_path = NULL;
static uint32_t search_path_count = 0;

int32_t al_fs_stdio_add_search_path(const char *path)
{
   char **new_search_path = NULL;
   char *new_path = NULL;

   /* dup path first to elimiate need to re-resize search_path if dup fails */
   new_path = ustrdup(path);
   if(!new_path) {
      return -1;
   }

   /* extend search_path, store temporarily so original var isn't overwritten with NULL on failure */
   new_search_path = (char **)_al_realloc(search_path, sizeof(char *) * (search_path_count + 1));
   if(!new_search_path) {
      free(new_path);
      *allegro_errno = errno;
      return -1;
   }

   search_path = new_search_path;
   search_path[search_path_count] = new_path;
   search_path_count++;

   return 0;
}

uint32_t al_fs_stdio_search_path_count()
{
   return search_path_count;
}

int32_t al_fs_stdio_get_search_path(uint32_t idx, char *dest, uint32_t *len)
{

}


uint32_t al_fs_stdio_get_stat_mode(AL_STAT *st)
{

}

time_t   al_fs_stdio_get_stat_atime(AL_STAT *st)
{

}

time_t   al_fs_stdio_get_stat_mtime(AL_STAT *st)
{

}

time_t   al_fs_stdio_get_stat_ctime(AL_STAT *st)
{

}

uint32_t al_fs_stdio_drive_sep(size_t len, char *sep)
{
#ifdef ALLEGRO_WINDOWS
   char *s = ":";
   _al_sane_strncpy(sep, s, MIN(ustrlen(sep), ustrlen(s)));
   return 0;
#else
   if(len >= 1)
      sep[0] = '\0';

   return 0;
#endif
}

uint32_t al_fs_stdio_path_sep(size_t len, char *sep)
{
#ifdef ALLEGRO_WINDOWS
   char c = '\\';
#else
   char c = '/';
#endif

   if(len >= 2) {
      sep[0] = c;
      sep[1] = '\0';
   }

   return 0;
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
uint32_t al_fs_stdio_path_to_sys(const char *orig, uint32_t len, char *path)
{
   return 0;
}

uint32_t al_fs_stdio_path_to_uni(const char *orig, uint32_t len, char *path)
{
   return 0;
}

#endif

