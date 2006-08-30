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
 *      File System Hooks, draft.
 *
 *      See readme.txt for copyright information.
 */

#ifndef ALLEGRO_FSHOOK_H
#define ALLEGRO_FSHOOK_H

#include "base.h"

AL_BEGIN_EXTERN_C

typedef void *AL_STAT;
typedef void *AL_FILE;
typedef void *AL_DIR;
typedef void *AL_DIRENT;

enum {
   AL_PROGRAM_DIR = 0,
   AL_SYSTEM_PROGRAM_DIR, /* eg: C:\Program Files\ on windows */
   AL_SYSTEM_DATA_DIR,
   AL_USER_DATA_DIR,
   AL_USER_HOME_DIR,
   AL_DIR_LAST // must be last
};

enum {
   AL_STAT_READ    = 1,
   AL_STAT_WRITE   = 1 << 1,
   AL_STAT_EXECUTE = 1 << 2,
   AL_STAT_HIDDEN  = 1 << 3,
   AL_STAT_ISFILE  = 1 << 4,
   AL_STAT_ISDIR   = 1 << 5,
};

enum {
   AL_SEEK_SET = 0,
   AL_SEEK_CUR,
   AL_SEEK_END
};

enum {
   AL_FS_HOOK_FOPEN = 0,
   AL_FS_HOOK_FCLOSE,
   AL_FS_HOOK_FREAD,
   AL_FS_HOOK_FWRITE,
   AL_FS_HOOK_FFLUSH,
   AL_FS_HOOK_FSEEK,
   AL_FS_HOOK_FTELL,
   AL_FS_HOOK_FERROR,
   AL_FS_HOOK_FEOF,

   AL_FS_HOOK_FSTAT,

   AL_FS_HOOK_OPENDIR,
   AL_FS_HOOK_CLOSEDIR,
   AL_FS_HOOK_READDIR,

   AL_FS_HOOK_MKTEMP,
   AL_FS_HOOK_GETCWD,
   AL_FS_HOOK_CHDIR,
   AL_FS_HOOK_GETDIR,
   AL_FS_HOOK_ADD_SEARCH_PATH,
   AL_FS_HOOK_SEARCH_PATH_COUNT,
   AL_FS_HOOK_GET_SEARCH_PATH,

   AL_FS_HOOK_GET_STAT_MODE,
   AL_FS_HOOK_GET_STAT_ATIME,
   AL_FS_HOOK_GET_STAT_MTIME,
   AL_FS_HOOK_GET_STAT_CTIME,
   AL_FS_HOOK_GET_STAT_SIZE,

   AL_FS_HOOK_PATH_TO_SYS,
   AL_FS_HOOK_PATH_TO_UNI, /* universal */

   AL_FS_HOOK_DRIVE_SEP,
   AL_FS_HOOK_PATH_SEP,

   AL_FS_HOOK_LAST /* must be last */
};

/*
set the given <phid> hook to <fshook>
*/
int al_fs_set_hook(uint32_t phid, void *fshook);

void *al_fs_get_hook(uint32_t phid);

AL_FILE *al_fs_fopen(const char *path, const char *mode);
int32_t al_fs_fclose(AL_FILE *fp);
ssize_t  al_fs_fread(void *ptr, size_t size, AL_FILE *fp);
ssize_t  al_fs_fwrite(const void *ptr, size_t size, AL_FILE *fp);
int32_t al_fs_fflush(AL_FILE *fp);
int32_t al_fs_fseek(AL_FILE *fp, uint32_t offset, uint32_t whence);
int32_t al_fs_ftell(AL_FILE *fp);
int32_t al_fs_ferror(AL_FILE *fp);
int32_t al_fs_feof(AL_FILE *fp);

int32_t al_fs_fstat(const char *path);
#warning "Implement/Fix al_fs_stat_free"
int32_t al_fs_stat_free(AL_STAT *stbuf);

AL_DIR    *al_fs_opendir(const char *path);
int32_t   al_fs_closedir(AL_DIR *dir);
AL_DIRENT *al_fs_readdir(AL_DIR *dir);

AL_FILE *al_fs_mktemp(const char *tmpl);
int32_t al_fs_getcwd(char *buf, size_t len);
int32_t al_fs_chdir(const char *path);

int32_t al_fs_getdir(uint32_t id, char *dir, size_t *len);

int32_t al_fs_add_search_path(const char *path);
int32_t al_fs_search_path_count();
int32_t al_fs_get_search_path(uint32_t idx, char *dest, size_t len);

uint32_t al_fs_get_stat_mode(AL_STAT *st);
time_t   al_fs_get_stat_atime(AL_STAT *st);
time_t   al_fs_get_stat_mtime(AL_STAT *st);
time_t   al_fs_get_stat_ctime(AL_STAT *st);
size_t   al_fs_get_stat_size(AL_STAT *st);

uint32_t al_fs_get_drive_sep(size_t len, char *sep);
uint32_t al_fs_get_path_sep(size_t len, char *sep);

uint32_t al_fs_path_to_sys(const char *orig, size_t len, char *path);
uint32_t al_fs_path_to_uni(const char *orig, size_t len, char *path);


AL_END_EXTERN_C

#endif          /* ifndef ALLEGRO_FSHOOK_H */
