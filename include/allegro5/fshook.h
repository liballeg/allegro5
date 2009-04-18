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

/* Title: Filesystem routines
*/

#ifndef ALLEGRO_FSHOOK_H
#define ALLEGRO_FSHOOK_H

#include "allegro5/base.h"
#include "allegro5/path.h"

#ifdef ALLEGRO_HAVE_SYS_TYPES_H
   #include <sys/types.h>
#else
// 4 Gig max offsets if sys/types doesn't exist.
typedef unsigned int off_t;
#endif

#ifdef __cplusplus
   extern "C" {
#endif

/* Type: ALLEGRO_FS_ENTRY
 */
typedef struct ALLEGRO_FS_ENTRY ALLEGRO_FS_ENTRY;

#ifdef ALLEGRO_LIB_BUILD
struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE;
struct ALLEGRO_FS_ENTRY {
   struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *vtable;
};
#endif /* ALLEGRO_LIB_BUILD */

/* Enum: ALLEGRO_FS_FILTER
 */
typedef enum ALLEGRO_FS_FILTER {
   ALLEGRO_FS_FILTER_DRIVES         = 0x001,
   ALLEGRO_FS_FILTER_DIRS           = 0x002,
   ALLEGRO_FS_FILTER_FILES          = 0x004,
   ALLEGRO_FS_FILTER_NOSYMLINKS     = 0x008,
   ALLEGRO_FS_FILTER_TYPEMASK       = 0x00F,

   ALLEGRO_FS_FILTER_ALLENTRIES     = ALLEGRO_FS_FILTER_DIRS | ALLEGRO_FS_FILTER_FILES | ALLEGRO_FS_FILTER_DRIVES,

   ALLEGRO_FS_FILTER_READABLE       = 0x010,
   ALLEGRO_FS_FILTER_WRITABLE       = 0x020,
   ALLEGRO_FS_FILTER_EXECUTABLE     = 0x040,
   ALLEGRO_FS_FILTER_PERMISSIONMASK = 0x070,

   ALLEGRO_FS_FILTER_HIDDEN         = 0x100,
   ALLEGRO_FS_FILTER_SYSTEM         = 0x200,
   ALLEGRO_FS_FILTER_ACCESSMASK     = 0x300,

   ALLEGRO_FS_FILTER_ALLDIRS        = 0x400,
   ALLEGRO_FS_FILTER_CASESENSITIVE  = 0x800,
   ALLEGRO_FS_FILTER_NODOTDOT       = 0x1000,

   ALLEGRO_FS_FILTER_NOFILTER       = -1,
} ALLEGRO_FS_FILTER;

/* Enum: ALLEGRO_FS_SORT
 */
typedef enum ALLEGRO_FS_SORT {
   ALLEGRO_FS_SORT_NAME          = 0x00,
   ALLEGRO_FS_SORT_TIME          = 0x01,
   ALLEGRO_FS_SORT_SIZE          = 0x02,
   ALLEGRO_FS_SORT_UNSORTED      = 0x03,
   ALLEGRO_FS_SORT_SORTBYMASK    = 0x03,

   ALLEGRO_FS_SORT_DIRSFIRST     = 0x04,
   ALLEGRO_FS_SORT_REVERSED      = 0x08,
   ALLEGRO_FS_SORT_CASESENSITIVE = 0x10,
   ALLEGRO_FS_SORT_DIRSLAST      = 0x20,
   ALLEGRO_FS_SORT_TYPE          = 0x80,
} ALLEGRO_FS_SORT;

/* Enum: ALLEGRO_FS_MKTEMP_REMOVE
 */
enum {
   ALLEGRO_FS_MKTEMP_REMOVE_NEVER = 0,
   ALLEGRO_FS_MKTEMP_REMOVE_NOW = 1,
   ALLEGRO_FS_MKTEMP_REMOVE_ON_CLOSE = 2,
};

// TODO: rename to ALLEGRO_FS_MODE_*/ALLEGRO_FS_TYPE_*
/* Enum: ALLEGRO_FS_MODE
 */
enum {
   ALLEGRO_FM_READ    = 1,
   ALLEGRO_FM_WRITE   = 1 << 1,
   ALLEGRO_FM_EXECUTE = 1 << 2,
   ALLEGRO_FM_HIDDEN  = 1 << 3,
   ALLEGRO_FM_ISFILE  = 1 << 4,
   ALLEGRO_FM_ISDIR   = 1 << 5,
};

/* Enum: ALLEGRO_SEEK
 */
enum {
   ALLEGRO_SEEK_SET = 0,
   ALLEGRO_SEEK_CUR,
   ALLEGRO_SEEK_END
};

#ifndef EOF
   #define EOF    (-1)
#endif

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

   3. Have no AL_STAT handle at all, cache the stat info in the ALLEGRO_FS_ENTRY handle.
      Pros: no annoying AL_STAT handle or ABI breakages to worry about.
            annoying access methods can be shortened.
      Cons: Needs a ALLEGRO_FS_ENTRY handle to access stat info ***

   ***) Possible Solution: allow an ALLEGRO_FS_ENTRY handle to be created, but not open the file

   <Thomas> 3 is my preference. I'm going to implement it, and worry about arguments for or against later.
*/

/* TODO: Have stat info cached in ALLEGRO_FS_ENTRY handles. */
/*  add al_fs_get_handle()/al_fs_open_handle()/_close_handle()
 *  alows one to grab a handle to get file info without opening the file
 */

AL_FUNC(ALLEGRO_FS_ENTRY*, al_create_entry, (const char *path));
AL_FUNC(void, al_destroy_entry, (ALLEGRO_FS_ENTRY *handle));
AL_FUNC(bool, al_open_entry, (ALLEGRO_FS_ENTRY *handle, const char *mode));
AL_FUNC(void, al_close_entry, (ALLEGRO_FS_ENTRY *handle));

AL_FUNC(ALLEGRO_FS_ENTRY*, al_mktemp, (const char *tmpl, uint32_t ulink));

AL_FUNC(ALLEGRO_FS_ENTRY*, al_fopen, (const char *path, const char *mode));
AL_FUNC(void, al_fclose, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(ALLEGRO_PATH *, al_get_entry_name, (ALLEGRO_FS_ENTRY *fp));

AL_FUNC(size_t, al_fread, (ALLEGRO_FS_ENTRY *fp, void *ptr, size_t size));
AL_FUNC(size_t, al_fwrite, (ALLEGRO_FS_ENTRY *fp, const void *ptr, size_t size));
AL_FUNC(bool, al_fflush, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(bool, al_fseek, (ALLEGRO_FS_ENTRY *fp, int64_t offset, uint32_t whence));
AL_FUNC(int64_t, al_ftell, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(bool, al_ferror, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(bool, al_feof, (ALLEGRO_FS_ENTRY *fp));

AL_FUNC(int, al_fungetc, (ALLEGRO_FS_ENTRY *fp, int c));

AL_FUNC(bool, al_fstat, (ALLEGRO_FS_ENTRY *fp));

AL_FUNC(uint32_t, al_get_entry_mode, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(time_t , al_get_entry_atime, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(time_t , al_get_entry_mtime, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(time_t , al_get_entry_ctime, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(off_t  , al_get_entry_size, (ALLEGRO_FS_ENTRY *st));

AL_FUNC(bool, al_remove_entry, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(bool, al_is_present, (ALLEGRO_FS_ENTRY *));

AL_FUNC(bool, al_is_directory, (ALLEGRO_FS_ENTRY *));
AL_FUNC(bool, al_is_file, (ALLEGRO_FS_ENTRY *));

AL_FUNC(ALLEGRO_FS_ENTRY *, al_opendir, (const char *path));
AL_FUNC(bool, al_closedir, (ALLEGRO_FS_ENTRY *dir));
AL_FUNC(ALLEGRO_FS_ENTRY *, al_readdir, (ALLEGRO_FS_ENTRY *dir));


AL_FUNC(uint32_t, al_get_entry_mode_str,  (const char *));
AL_FUNC(time_t,   al_get_entry_atime_str, (const char *));
AL_FUNC(time_t,   al_get_entry_mtime_str, (const char *));
AL_FUNC(time_t,   al_get_entry_ctime_str, (const char *));
AL_FUNC(off_t,    al_get_entry_size_str,  (const char *));

AL_FUNC(bool, al_is_directory_str, (const char *));
AL_FUNC(bool, al_is_file_str, (const char *));

AL_FUNC(bool, al_remove_str, (const char *path));
AL_FUNC(bool, al_is_present_str, (const char *));

AL_FUNC(bool, al_mkdir, (const char *));

AL_FUNC(ALLEGRO_PATH *, al_getcwd, (void));
AL_FUNC(bool, al_chdir, (const char *path));

AL_FUNC(bool, al_add_search_path, (const char *path));
AL_FUNC(int32_t, al_search_path_count, (void));
AL_FUNC(bool, al_get_search_path, (uint32_t idx, char *dest, size_t len));

AL_FUNC(int32_t, al_drive_sep, (char *sep, size_t len));
AL_FUNC(int32_t, al_path_sep, (char *sep, size_t len));

AL_FUNC(int, al_fgetc, (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int, al_fputc, (ALLEGRO_FS_ENTRY *f, int c));

AL_FUNC(int16_t, al_fread16le,  (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int32_t, al_fread32le,  (ALLEGRO_FS_ENTRY *f, bool *ret_success));
AL_FUNC(size_t, al_fwrite16le, (ALLEGRO_FS_ENTRY *f, int16_t w));
AL_FUNC(size_t, al_fwrite32le, (ALLEGRO_FS_ENTRY *f, int32_t l));
AL_FUNC(int16_t, al_fread16be,  (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int32_t, al_fread32be,  (ALLEGRO_FS_ENTRY *f, bool *ret_success));
AL_FUNC(size_t, al_fwrite16be, (ALLEGRO_FS_ENTRY *f, int16_t w));
AL_FUNC(size_t, al_fwrite32be, (ALLEGRO_FS_ENTRY *f, int32_t l));

AL_FUNC(char*, al_fgets, (ALLEGRO_FS_ENTRY *f, char *p, size_t max));
AL_FUNC(int,   al_fputs, (ALLEGRO_FS_ENTRY *f, const char *p));

/* Find stuff */

/*
typedef int (*AL_FILTER_PROC)(const char *, void *);
typedef int (*AL_SORT_PROC)(const char *, const char *, void *);

AL_FUNC(char **, al_fs_find_ex, (char *path, AL_FILTER_PROC filter, void *, AL_SORT_PROC sort, void *));
AL_FUNC(char **, al_fs_find, (char *path, ALLEGRO_FS_FILTER filter, ALLEGRO_FS_SORT sort));
AL_FUNC(void,  al_fs_free_list, (char **));
*/

AL_FUNC(char *, al_find_resource, (const char *base, const char *resource, uint32_t fm, char *buffer, size_t len));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FSHOOK_H */
