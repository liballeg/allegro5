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
   AL_SYSTEM_DATA_DIR,
   AL_APP_DATA_DIR,
   AL_USER_DATA_DIR,
   AL_USER_HOME_DIR,
   AL__LAST_DIR // must be last
};

int al_fs_add_search_path();
int al_fs_search_path_count();
int al_fs_get_search_path(int id, char *data, int &len);

enum {
   AL_STAT_READ    = 1,
   AL_STAT_WRITE   = 1 << 1,
   AL_STAT_EXECUTE = 1 << 2,
   AL_STAT_HIDDEN  = 1 << 3,
   AL_STAT_ISFILE  = 1 << 4,
   AL_STAT_ISDIR   = 1 << 5,
};

enum {
   AL_SEEK_SET = 1,
   AL_SEEK_END,
   AL_SEEK_CUR
}

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

   AL_FS_HOOK_MKSTMP,
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

   AL_FS_HOOK_LAST // must be last
};

/*
set the given <phid> hook to <fshook>
*/
int al_fs_set_hook(uint32_t phid, void *fshook);

void *al_fs_get_hook(uint32_t phid);

AL_END_EXTERN_C

#endif          /* ifndef ALLEGRO_FSHOOK_H */
