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

/* The following defines are reserved for fs drivers.
 * Define to keep from defining types multiple times.
 */

#ifndef ALLEGRO_FS_FILE_DEFINED

#ifdef ALLEGRO_LIB_BUILD
typedef struct AL_FILE {
   AL_FS_HOOK_FILE_VTABLE *vtable;
} AL_FILE;
#else
typedef void *AL_FILE;
#endif /* ALLEGRO_LIB_BUILD */

#endif /* ALLEGRO_FS_FILE_DEFINED */

#ifndef ALLEGRO_FS_DIR_DEFINED

#ifdef ALLEGRO_LIB_BUILD
typedef struct AL_DIR {
   AL_FS_HOOK_DIR_VTABLE *vtable;
} AL_DIR;
#else
typedef void *AL_DIR;
#endif /* ALLEGRO_LIB_BUILD */

#endif /* ALLEGRO_FS_DIR_DEFINED */

enum {
   AL_MKTEMP_UNLINK_NOW = 1,
   AL_MKTEMP_UNLINK_ON_CLOSE
};

enum {
   AL_FM_READ    = 1,
   AL_FM_WRITE   = 1 << 1,
   AL_FM_EXECUTE = 1 << 2,
   AL_FM_HIDDEN  = 1 << 3,
   AL_FM_ISFILE  = 1 << 4,
   AL_FM_ISDIR   = 1 << 5,
};

enum {
   AL_SEEK_SET = 0,
   AL_SEEK_CUR,
   AL_SEEK_END
};

enum {
   AL_FS_HOOK_CREATE_HANDLE = 0,
   AL_FS_HOOK_DESTROY_HANDLE,
   AL_FS_HOOK_OPEN_HANDLE,
   AL_FS_HOOK_CLOSE_HANDLE,

   AL_FS_HOOK_FOPEN,
   AL_FS_HOOK_FCLOSE,
   AL_FS_HOOK_FREAD,
   AL_FS_HOOK_FWRITE,
   AL_FS_HOOK_FFLUSH,
   AL_FS_HOOK_FSEEK,
   AL_FS_HOOK_FTELL,
   AL_FS_HOOK_FERROR,
   AL_FS_HOOK_FEOF,
   AL_FS_HOOK_FNAME,

   AL_FS_HOOK_FSTAT,

   AL_FS_HOOK_OPENDIR,
   AL_FS_HOOK_CLOSEDIR,
   AL_FS_HOOK_READDIR,

   AL_FS_HOOK_MKTEMP,
   AL_FS_HOOK_GETCWD,
   AL_FS_HOOK_CHDIR,

   AL_FS_HOOK_ADD_SEARCH_PATH,
   AL_FS_HOOK_SEARCH_PATH_COUNT,
   AL_FS_HOOK_GET_SEARCH_PATH,

   AL_FS_HOOK_FILE_MODE,
   AL_FS_HOOK_FILE_ATIME,
   AL_FS_HOOK_FILE_MTIME,
   AL_FS_HOOK_FILE_CTIME,
   AL_FS_HOOK_FILE_SIZE,

   AL_FS_HOOK_PATH_TO_SYS,
   AL_FS_HOOK_PATH_TO_UNI, /* universal */

   AL_FS_HOOK_DRIVE_SEP,
   AL_FS_HOOK_PATH_SEP,

   AL_FS_HOOK_FEXITS,
   AL_FS_HOOK_FILE_EXISTS,
   AL_FS_HOOK_DIR_EXISTS,
   AL_FS_HOOK_RMDIR,

   AL_FS_HOOK_LAST /* must be last */
};


/*
   Different Methods of handling file stat info (ie: AL_STAT):

   1. AL_STAT *al_fs_fstat(char *file) && void al_fs_stat_free(stathandle);
      Pros: Can handle AL_STAT as a opaque type, hiding the struct members, and struct size away
            to avoid possible ABI breakages.
      Cons: The user has to free the handle, and use some annoyingly long named functions to access it.

   2. int32_t al_fs_fstat(char *file, AL_STAT *st);
      Pros: no handle to free, just pass in a AL_STAT on the stack.
      Cons: struct needs to be fully uservisible,
               may cause ABI breakages if items are added or removed from the struct
            again needs some annoyingly long and ugly (named) functions to access the info.

   3. Have no AL_STAT handle at all, cache the stat info in the AL_FILE handle.
      Pros: no annoying AL_STAT handle or ABI breakages to worry about.
            annoying access methods can be shortened.
      Cons: Needs a AL_FILE handle to access stat info ***

   ***) Possible Solution: allow an AL_FILE handle to be created, but not open the file

   <Thomas> 3 is my preference. I'm going to implement it, and worry about arguments for or against later.
*/

/* TODO: Have stat info cached in AL_FILE handles. */
/*  add al_fs_get_handle()/al_fs_open_handle()/_close_handle()
 *  alows one to grab a handle to get file info without opening the file
 */

/*
set the given <phid> hook to <fshook>
*/
int al_fs_set_hook(uint32_t phid, void *fshook);
void *al_fs_get_hook(uint32_t phid);

AL_FILE *al_fs_create_handle(AL_CONST char *path);
void     al_fs_destroy_handle(AL_FILE *handle);
int32_t  al_fs_open_handle(AL_FILE *handle, AL_CONST char *mode);
int32_t  al_fs_close_handle(AL_FILE *handle);

AL_FILE *al_fs_fopen(const char *path, const char *mode);
void    al_fs_fname(AL_FILE *fp, size_t, char *fn);
int32_t al_fs_fclose(AL_FILE *fp);
ssize_t al_fs_fread(void *ptr, size_t size, AL_FILE *fp);
ssize_t al_fs_fwrite(const void *ptr, size_t size, AL_FILE *fp);
int32_t al_fs_fflush(AL_FILE *fp);
int32_t al_fs_fseek(AL_FILE *fp, uint32_t offset, uint32_t whence);
int32_t al_fs_ftell(AL_FILE *fp);
int32_t al_fs_ferror(AL_FILE *fp);
int32_t al_fs_feof(AL_FILE *fp);

int32_t al_fs_funlink(AL_FILE *fp);
/* unlink is just a convienience function, it calls funlink */
int32_t al_fs_unlink(const char *path);

int32_t al_fs_fstat(AL_FILE *fp);

AL_DIR  *al_fs_opendir(const char *path);
int32_t al_fs_closedir(AL_DIR *dir);
int32_t al_fs_readdir(AL_DIR *dir, size_t size, char *name);

AL_FILE *al_fs_mktemp(const char *tmpl, uint32_t ulink);
int32_t al_fs_getcwd(char *buf, size_t len);
int32_t al_fs_chdir(const char *path);

int32_t al_fs_add_search_path(const char *path);
int32_t al_fs_search_path_count();
int32_t al_fs_get_search_path(uint32_t idx, char *dest, size_t len);

uint32_t al_fs_file_mode(AL_FILE *st);
time_t   al_fs_file_atime(AL_FILE *st);
time_t   al_fs_file_mtime(AL_FILE *st);
time_t   al_fs_file_ctime(AL_FILE *st);
size_t   al_fs_file_size(AL_FILE *st);

int32_t al_fs_drive_sep(size_t len, char *sep);
int32_t al_fs_path_sep(size_t len, char *sep);

int32_t al_fs_path_to_sys(const char *orig, size_t len, char *path);
int32_t al_fs_path_to_uni(const char *orig, size_t len, char *path);

int32_t al_fs_fexists(AL_FILE *);
int32_t al_fs_exists(AL_CONST char *);

int32_t al_fs_rmdir(AL_CONST char *);

AL_END_EXTERN_C

#endif          /* ifndef ALLEGRO_FSHOOK_H */
