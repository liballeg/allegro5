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

#include "base.h"

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
 * Opaque filesystem entry object.
 * Represents a file or a directory (check with <al_fs_entry_isdir>
 * or <al_fs_entry_isfile>).
 * There are no user accessible member variables.
 */
typedef struct ALLEGRO_FS_ENTRY ALLEGRO_FS_ENTRY;

#ifdef ALLEGRO_LIB_BUILD
struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE;
struct ALLEGRO_FS_ENTRY {
   struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *vtable;
};
#endif /* ALLEGRO_LIB_BUILD */

/* Enum: ALLEGRO_FS_FILTER
 * File system search filters.
 *
 * Type Filters:
 * ALLEGRO_FS_FILTER_DRIVES - Include Drive letters
 * ALLEGRO_FS_FILTER_DIRS - Include Directories
 * ALLEGRO_FS_FILTER_FILES - Include Files
 * ALLEGRO_FS_FILTER_NOSYMLINKS - Exclude symlinks
 * ALLEGRO_FS_FILTER_ALLENTRIES - Include all entries (including symlinks)
 *
 * Permission Filters:
 * ALLEGRO_FS_FILTER_READABLE - Entry is Readable
 * ALLEGRO_FS_FILTER_WRITABLE - Entry is Writeable
 * ALLEGRO_FS_FILTER_EXECUTABLE - Entry is Executable
 *
 * Access Filters:
 * ALLEGRO_FS_FILTER_HIDDEN - Entry is hidden
 * ALLEGRO_FS_FILTER_SYSTEM - Entry is marked as a system file
 *
 * Other Filters:
 * ALLEGRO_FS_FILTER_ALLDIRS - Include all directories, including '.' and '..'
 * ALLEGRO_FS_FILTER_CASESENSITIVE - Search case sentitively
 * ALLEGRO_FS_FILTER_NODOTDOT - Exclude '..'
 * 
 * ALLEGRO_FS_FILTER_NOFILTER - No filtering
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
 * Filesystem search sort flags
 *
 * ALLEGRO_FS_SORT_NAME - By name
 * ALLEGRO_FS_SORT_TIME - By time
 * ALLEGRO_FS_SORT_SIZE - By size
 * ALLEGRO_FS_SORT_UNSORTED - Don't sort
 * 
 * ALLEGRO_FS_SORT_DIRSFIRST - Directories first
 * ALLEGRO_FS_SORT_REVERSED - Reverse sort
 * ALLEGRO_FS_SORT_CASESENSITIVE - Case Sensitive
 * ALLEGRO_FS_SORT_DIRSLAST - Directories Last
 * ALLEGRO_FS_SORT_TYPE - By Type (extension?)
 *
 * XXX do we need ALLEGRO_FS_SORT_TYPE?  It sounds like we don't even
 * know what it should do.
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

/* Enum: ALLEGRO_FS_MKTEMP_UNLINK
 * al_fs_mktemp unlink modes
 * 
 * ALLEGRO_FS_MKTEMP_UNLINK_NEVER - Don't auto unlink
 * ALLEGRO_FS_MKTEMP_UNLINK_NOW - Unlink on open, makes file "hidden"
 * ALLEGRO_FS_MKTEMP_UNLINK_ON_CLOSE - Unlink on close
 */
enum {
   ALLEGRO_FS_MKTEMP_UNLINK_NEVER = 0,
   ALLEGRO_FS_MKTEMP_UNLINK_NOW = 1,
   ALLEGRO_FS_MKTEMP_UNLINK_ON_CLOSE = 2,
};

// TODO: rename to ALLEGRO_FS_MODE_*/ALLEGRO_FS_TYPE_*
/* Enum: ALLEGRO_FS_MODE
 * Filesystem modes/types
 *
 * AL_FM_READ - Readable
 * AL_FM_WRITE - Writable
 * AL_FM_EXECUTE - Executable
 * AL_FM_HIDDEN - Hidden
 * AL_FM_ISFILE - Regular file
 * AL_FM_ISDIR - Directory
 */
enum {
   AL_FM_READ    = 1,
   AL_FM_WRITE   = 1 << 1,
   AL_FM_EXECUTE = 1 << 2,
   AL_FM_HIDDEN  = 1 << 3,
   AL_FM_ISFILE  = 1 << 4,
   AL_FM_ISDIR   = 1 << 5,
};

/* Enum: ALLEGRO_SEEK
 * Seek modes
 *
 * ALLEGRO_SEEK_SET - Seek to pos from beginning of file
 * ALLEGRO_SEEK_CUR - Seek to pos from curent position
 * ALLEGRO_SEEK_END - Seek to pos from end of file
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

AL_FUNC(ALLEGRO_FS_ENTRY*, al_fs_create_handle, (AL_CONST char *path));
AL_FUNC(void, al_fs_destroy_handle, (ALLEGRO_FS_ENTRY *handle));
AL_FUNC(int32_t, al_fs_open_handle, (ALLEGRO_FS_ENTRY *handle, AL_CONST char *mode));
AL_FUNC(void, al_fs_close_handle, (ALLEGRO_FS_ENTRY *handle));

AL_FUNC(ALLEGRO_FS_ENTRY*, al_fs_mktemp, (const char *tmpl, uint32_t ulink));

AL_FUNC(ALLEGRO_FS_ENTRY*, al_fs_entry_open, (const char *path, const char *mode));
AL_FUNC(void, al_fs_entry_name, (ALLEGRO_FS_ENTRY *fp, size_t, char *fn));
AL_FUNC(void, al_fs_entry_close, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(size_t, al_fs_entry_read, (ALLEGRO_FS_ENTRY *fp, size_t size, void *ptr));
AL_FUNC(size_t, al_fs_entry_write, (ALLEGRO_FS_ENTRY *fp, size_t size, const void *ptr));
AL_FUNC(int32_t, al_fs_entry_flush, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(int32_t, al_fs_entry_seek, (ALLEGRO_FS_ENTRY *fp, uint32_t offset, uint32_t whence));
AL_FUNC(int32_t, al_fs_entry_tell, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(int32_t, al_fs_entry_error, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(int32_t, al_fs_entry_eof, (ALLEGRO_FS_ENTRY *fp));

AL_FUNC(int32_t, al_fs_entry_ungetc, (int32_t c, ALLEGRO_FS_ENTRY *fp));

AL_FUNC(int32_t, al_fs_entry_stat, (ALLEGRO_FS_ENTRY *fp));

AL_FUNC(uint32_t, al_fs_entry_mode, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(time_t , al_fs_entry_atime, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(time_t , al_fs_entry_mtime, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(time_t , al_fs_entry_ctime, (ALLEGRO_FS_ENTRY *st));
AL_FUNC(off_t  , al_fs_entry_size, (ALLEGRO_FS_ENTRY *st));

AL_FUNC(int32_t, al_fs_entry_unlink, (ALLEGRO_FS_ENTRY *fp));
AL_FUNC(int32_t, al_fs_entry_exists, (ALLEGRO_FS_ENTRY *));

AL_FUNC(int32_t, al_fs_entry_isdir, (ALLEGRO_FS_ENTRY *));
AL_FUNC(int32_t, al_fs_entry_isfile, (ALLEGRO_FS_ENTRY *));

AL_FUNC(ALLEGRO_FS_ENTRY *, al_fs_opendir, (const char *path));
AL_FUNC(int32_t, al_fs_closedir, (ALLEGRO_FS_ENTRY *dir));
AL_FUNC(int32_t, al_fs_readdir, (ALLEGRO_FS_ENTRY *dir, size_t size, char *name));


AL_FUNC(uint32_t, al_fs_stat_mode,  (AL_CONST char *));
AL_FUNC(time_t,   al_fs_stat_atime, (AL_CONST char *));
AL_FUNC(time_t,   al_fs_stat_mtime, (AL_CONST char *));
AL_FUNC(time_t,   al_fs_stat_ctime, (AL_CONST char *));
AL_FUNC(off_t,    al_fs_stat_size,  (AL_CONST char *));

AL_FUNC(int32_t, al_fs_isdir, (AL_CONST char *));
AL_FUNC(int32_t, al_fs_isfile, (AL_CONST char *));

AL_FUNC(int32_t, al_fs_unlink, (AL_CONST char *path));
AL_FUNC(int32_t, al_fs_exists, (AL_CONST char *));

AL_FUNC(int32_t, al_fs_mkdir, (AL_CONST char *));

AL_FUNC(int32_t, al_fs_getcwd, (char *buf, size_t len));
AL_FUNC(int32_t, al_fs_chdir, (const char *path));

AL_FUNC(int32_t, al_fs_add_search_path, (const char *path));
AL_FUNC(int32_t, al_fs_search_path_count, (void));
AL_FUNC(int32_t, al_fs_get_search_path, (uint32_t idx, char *dest, size_t len));

AL_FUNC(int32_t, al_fs_drive_sep, (size_t len, char *sep));
AL_FUNC(int32_t, al_fs_path_sep, (size_t len, char *sep));

AL_FUNC(int32_t, al_fs_path_to_sys, (AL_CONST char *orig, size_t len, char *path));
AL_FUNC(int32_t, al_fs_path_to_uni, (AL_CONST char *orig, size_t len, char *path));

AL_FUNC(int, al_fs_entry_getc, (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int, al_fs_entry_putc, (int c, ALLEGRO_FS_ENTRY *f));

AL_FUNC(int16_t, al_fs_entry_igetw, (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int32_t, al_fs_entry_igetl, (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int16_t, al_fs_entry_iputw, (int16_t w, ALLEGRO_FS_ENTRY *f));
AL_FUNC(int32_t, al_fs_entry_iputl, (int32_t l, ALLEGRO_FS_ENTRY *f));
AL_FUNC(int16_t, al_fs_entry_mgetw, (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int32_t, al_fs_entry_mgetl, (ALLEGRO_FS_ENTRY *f));
AL_FUNC(int16_t, al_fs_entry_mputw, (int16_t w, ALLEGRO_FS_ENTRY *f));
AL_FUNC(int32_t, al_fs_entry_mputl, (int32_t l, ALLEGRO_FS_ENTRY *f));

AL_FUNC(char*, al_fs_entry_fgets, (char *p, size_t max, ALLEGRO_FS_ENTRY *f));
AL_FUNC(int, al_fs_entry_fputs, (AL_CONST char *p, ALLEGRO_FS_ENTRY *f));

/* Find stuff */

/*
typedef int (*AL_FILTER_PROC)(AL_CONST char *, void *);
typedef int (*AL_SORT_PROC)(AL_CONST char *, AL_CONST char *, void *);

AL_FUNC(char **, al_fs_find_ex, (char *path, AL_FILTER_PROC filter, void *, AL_SORT_PROC sort, void *));
AL_FUNC(char **, al_fs_find, (char *path, ALLEGRO_FS_FILTER filter, ALLEGRO_FS_SORT sort));
AL_FUNC(void,  al_fs_free_list, (char **));
*/

AL_FUNC(char *, al_find_resource, (const char *base, const char *resource, uint32_t fm, char *buffer, size_t len));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FSHOOK_H */
