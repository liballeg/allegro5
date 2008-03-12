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

#ifndef ALLEGRO_FS_ENTRY_DEFINED

#ifdef ALLEGRO_LIB_BUILD
typedef struct AL_FS_ENTRY {
   AL_FS_HOOK_ENTRY_VTABLE *vtable;
} AL_FS_ENTRY;
#else
typedef void AL_FS_ENTRY;
#endif /* ALLEGRO_LIB_BUILD */

#endif /* ALLEGRO_FS_ENTRY_DEFINED */

typedef enum AL_FS_FILTER {
   AL_FS_FILTER_DIRS           = 0x001,
   AL_FS_FILTER_FILES          = 0x002,
   AL_FS_FILTER_NOSYMLINKS     = 0x004,
   AL_FS_FILTER_TYPEMASK       = 0x007,

   AL_FS_FILTER_ALLENTRIES     = AL_FS_FILTER_DIRS | AL_FS_FILTER_FILES | AL_FS_FILTER_DRIVES,

   AL_FS_FILTER_READABLE       = 0x010,
   AL_FS_FILTER_WRITABLE       = 0x020,
   AL_FS_FILTER_EXECUTABLE     = 0x040,
   AL_FS_FILTER_PERMISSIONMASK = 0x070,

   AL_FS_FILTER_HIDDEN         = 0x100,
   AL_FS_FILTER_SYSTEM         = 0x200,
   AL_FS_FILTER_ACCESSMASK     = 0x300

   AL_FS_FILTER_ALLDIRS        = 0x400,
   AL_FS_FILTER_CASESENSITIVE  = 0x800,
   AL_FS_FILTER_NODOTDOT       = 0x1000,

   AL_FS_FILTER_NOFILTER       = -1,
} AL_FS_FILTER;

typedef enum AL_FS_SORT {
   AL_FS_SORT_NAME          = 0x00,
   AL_FS_SORT_TIME          = 0x01,
   AL_FS_SORT_SIZE          = 0x02,
   AL_FS_SORT_UNSORTED      = 0x03,
   AL_FS_SORT_SORTBYMASK    = 0x03,

   AL_FS_SORT_DIRSFIRST     = 0x04,
   AL_FS_SORT_REVERSED      = 0x08,
   AL_FS_SORT_CASESENSITIVE = 0x10,
   AL_FS_SORT_DIRSLAST      = 0x20,
   AL_FS_SORT_TYPE          = 0x80,
} AL_FS_SORT;

enum {
   AL_MKTEMP_UNLINK_NEVER = 0,
   AL_MKTEMP_UNLINK_NOW = 1,
   AL_MKTEMP_UNLINK_ON_CLOSE = 2,
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

   AL_FS_HOOK_ENTRY_OPEN,
   AL_FS_HOOK_ENTRY_CLOSE,
   AL_FS_HOOK_ENTRY_READ,
   AL_FS_HOOK_ENTRY_WRITE,
   AL_FS_HOOK_ENTRY_FLUSH,
   AL_FS_HOOK_ENTRY_SEEK,
   AL_FS_HOOK_ENTRY_TELL,
   AL_FS_HOOK_ENTRY_ERROR,
   AL_FS_HOOK_ENTRY_EOF,
   AL_FS_HOOK_ENTRY_NAME,

   AL_FS_HOOK_ENTRY_STAT,

   AL_FS_HOOK_OPENDIR,
   AL_FS_HOOK_CLOSEDIR,
   AL_FS_HOOK_READDIR,

   AL_FS_HOOK_MKTEMP,
   AL_FS_HOOK_GETCWD,
   AL_FS_HOOK_CHDIR,

   AL_FS_HOOK_ADD_SEARCH_PATH,
   AL_FS_HOOK_SEARCH_PATH_COUNT,
   AL_FS_HOOK_GET_SEARCH_PATH,

   AL_FS_HOOK_ENTRY_MODE,
   AL_FS_HOOK_ENTRY_ATIME,
   AL_FS_HOOK_ENTRY_MTIME,
   AL_FS_HOOK_ENTRY_CTIME,
   AL_FS_HOOK_ENTRY_SIZE,

   AL_FS_HOOK_ENTRY_EXISTS,
   AL_FS_HOOK_ENTRY_UNLINK,

   AL_FS_HOOK_PATH_TO_SYS,
   AL_FS_HOOK_PATH_TO_UNI, /* universal */

   AL_FS_HOOK_DRIVE_SEP,
   AL_FS_HOOK_PATH_SEP,

   AL_FS_HOOK_STAT_MODE,
   AL_FS_HOOK_STAT_ATIME,
   AL_FS_HOOK_STAT_MTIME,
   AL_FS_HOOK_STAT_CTIME,
   AL_FS_HOOK_STAT_SIZE,

   AL_FS_HOOK_EXISTS,
   AL_FS_HOOK_UNLINK,

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

   3. Have no AL_STAT handle at all, cache the stat info in the AL_FS_ENTRY handle.
      Pros: no annoying AL_STAT handle or ABI breakages to worry about.
            annoying access methods can be shortened.
      Cons: Needs a AL_FS_ENTRY handle to access stat info ***

   ***) Possible Solution: allow an AL_FS_ENTRY handle to be created, but not open the file

   <Thomas> 3 is my preference. I'm going to implement it, and worry about arguments for or against later.
*/

/* TODO: Have stat info cached in AL_FS_ENTRY handles. */
/*  add al_fs_get_handle()/al_fs_open_handle()/_close_handle()
 *  alows one to grab a handle to get file info without opening the file
 */

/*
set the given <phid> hook to <fshook>
*/
int al_fs_set_hook(uint32_t phid, void *fshook);
void *al_fs_get_hook(uint32_t phid);

AL_FS_ENTRY *al_fs_create_handle(AL_CONST char *path);
void     al_fs_destroy_handle(AL_FS_ENTRY *handle);
int32_t  al_fs_open_handle(AL_FS_ENTRY *handle, AL_CONST char *mode);
int32_t  al_fs_close_handle(AL_FS_ENTRY *handle);

AL_FS_ENTRY *al_fs_mktemp(const char *tmpl, uint32_t ulink);

AL_FS_ENTRY *al_fs_entry_open(const char *path, const char *mode);
void    al_fs_entry_name(AL_FS_ENTRY *fp, size_t, char *fn);
int32_t al_fs_entry_close(AL_FS_ENTRY *fp);
ssize_t al_fs_entry_read(void *ptr, size_t size, AL_FS_ENTRY *fp);
ssize_t al_fs_entry_write(const void *ptr, size_t size, AL_FS_ENTRY *fp);
int32_t al_fs_entry_flush(AL_FS_ENTRY *fp);
int32_t al_fs_entry_seek(AL_FS_ENTRY *fp, uint32_t offset, uint32_t whence);
int32_t al_fs_entry_tell(AL_FS_ENTRY *fp);
int32_t al_fs_entry_error(AL_FS_ENTRY *fp);
int32_t al_fs_entry_eof(AL_FS_ENTRY *fp);

int32_t al_fs_entry_stat(AL_FS_ENTRY *fp);

uint32_t al_fs_entry_mode(AL_FS_ENTRY *st);
time_t   al_fs_entry_atime(AL_FS_ENTRY *st);
time_t   al_fs_entry_mtime(AL_FS_ENTRY *st);
time_t   al_fs_entry_ctime(AL_FS_ENTRY *st);
size_t   al_fs_entry_size(AL_FS_ENTRY *st);

int32_t al_fs_entry_unlink(AL_FS_ENTRY *fp);
int32_t al_fs_entry_exists(AL_FS_ENTRY *);

int32_t al_fs_entry_isdir(AL_FS_ENTRY *);
int32_t al_fs_entry_isfile(AL_FS_ENTRY *);

AL_FS_ENTRY  *al_fs_opendir(const char *path);
int32_t al_fs_closedir(AL_FS_ENTRY *dir);
int32_t al_fs_readdir(AL_FS_ENTRY *dir, size_t size, char *name);


uint32_t al_fs_stat_mode(AL_CONST char *);
time_t   al_fs_stat_atime(AL_CONST char *);
time_t   al_fs_stat_mtime(AL_CONST char *);
time_t   al_fs_stat_ctime(AL_CONST char *);
size_t   al_fs_stat_size(AL_CONST char *);

int32_t al_fs_isdir(AL_CONST char *);
int32_t al_fs_isfile(AL_CONST char *);

int32_t al_fs_unlink(AL_CONST char *path);
int32_t al_fs_exists(AL_CONST char *);

int32_t al_fs_getcwd(char *buf, size_t len);
int32_t al_fs_chdir(const char *path);

int32_t al_fs_add_search_path(const char *path);
int32_t al_fs_search_path_count();
int32_t al_fs_get_search_path(uint32_t idx, char *dest, size_t len);



int32_t al_fs_drive_sep(size_t len, char *sep);
int32_t al_fs_path_sep(size_t len, char *sep);

int32_t al_fs_path_to_sys(const char *orig, size_t len, char *path);
int32_t al_fs_path_to_uni(const char *orig, size_t len, char *path);

/* Find stuff */

typedef int (*AL_FILTER_PROC)(AL_CONST char *, void *);
typedef int (*AL_SORT_PROC)(AL_CONST char *, AL_CONST char *, void *);

char ** al_fs_find_ex(char *path, AL_FILTER_PROC filter, void *, AL_SORT_PROC sort, void *);
char ** al_fs_find(char *path, AL_FS_FILTER filter, AL_FS_SORT sort);
void al_fs_free_list(char **);


AL_END_EXTERN_C

#endif          /* ifndef ALLEGRO_FSHOOK_H */
