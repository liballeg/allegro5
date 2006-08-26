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

}

uint32_t al_fs_stdio_fclose(AL_FILE *fp)
{

}

size_t   al_fs_stdio_fread(void *ptr, size_t size, AL_FILE *fp)
{

}

size_t   al_fs_stdio_fwrite(const void *ptr, size_t size, AL_FILE *fp)
{

}

uint32_t al_fs_stdio_fflush(AL_FILE *fp)
{

}

uint32_t al_fs_stdio_fseek(AL_FILE *fp, uint32_t offset, uint32_t whence)
{

}

uint32_t al_fs_stdio_ftell(AL_FILE *fp)
{

}

uint32_t al_fs_stdio_ferror(AL_FILE *fp)
{

}

uint32_t al_fs_stdio_feof(AL_FILE *fp)
{

}


uint32_t al_fs_stdio_fstat(const char *path, AL_STAT *stbuf)
{

}


AL_DIR    *al_fs_stdio_opendir(const char *path)
{

}

uint32_t   al_fs_stdio_closedir(AL_DIR *dir)
{

}

AL_DIRENT *al_fs_stdio_readdir(AL_DIR *dir)
{

}


AL_FILE *al_fs_stdio_mktemp(const char *template)
{

}

uint32_t al_fs_stdio_getcwd(char *buf, size_t *len)
{

}

uint32_t al_fs_stdio_chdir(const char *path)
{

}

uint32_t al_fs_stdio_getdir(uint32_t id, char *dir, uint32_t *len)
{

}


uint32_t al_fs_stdio_add_search_path(const char *path)
{

}

uint32_t al_fs_stdio_search_path_count()
{

}

uint32_t al_fs_stdio_get_search_path(char *dest, uint32_t *len)
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


uint32_t al_fs_stdio_path_to_sys(const char *orig, uint32_t len, char *path)
{

}

uint32_t al_fs_stdio_path_to_uni(const char *orig, uint32_t len, char *path)
{

}

