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

/* Enum: ALLEGRO_FS_MKTEMP_REMOVE
 */
enum {
   ALLEGRO_FS_MKTEMP_REMOVE_NEVER = 0,
   ALLEGRO_FS_MKTEMP_REMOVE_NOW = 1,
   ALLEGRO_FS_MKTEMP_REMOVE_ON_CLOSE = 2,
};

// TODO: rename to ALLEGRO_FILE_MODE_*/ALLEGRO_FS_TYPE_*
/* Enum: ALLEGRO_FILE_MODE
 */
enum {
   ALLEGRO_FILEMODE_READ    = 1,
   ALLEGRO_FILEMODE_WRITE   = 1 << 1,
   ALLEGRO_FILEMODE_EXECUTE = 1 << 2,
   ALLEGRO_FILEMODE_HIDDEN  = 1 << 3,
   ALLEGRO_FILEMODE_ISFILE  = 1 << 4,
   ALLEGRO_FILEMODE_ISDIR   = 1 << 5,
};

#ifndef EOF
   #define EOF    (-1)
#endif

AL_FUNC(ALLEGRO_FS_ENTRY*, al_create_entry, (const char *path));
AL_FUNC(void, al_destroy_entry, (ALLEGRO_FS_ENTRY *handle));
AL_FUNC(bool, al_open_entry, (ALLEGRO_FS_ENTRY *handle, const char *mode));
AL_FUNC(void, al_close_entry, (ALLEGRO_FS_ENTRY *handle));

AL_FUNC(ALLEGRO_FS_ENTRY*, al_mktemp, (const char *tmpl, uint32_t ulink));

AL_FUNC(ALLEGRO_PATH *, al_get_entry_name, (ALLEGRO_FS_ENTRY *fp));

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

AL_FUNC(char *, al_find_resource, (const char *base, const char *resource, uint32_t fm, char *buffer, size_t len));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FSHOOK_H */
