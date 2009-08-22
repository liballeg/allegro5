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

struct ALLEGRO_FS_ENTRY {
   struct ALLEGRO_FS_INTERFACE const *vtable;
};

/* Enum: ALLEGRO_FILE_MODE
 */
typedef enum ALLEGRO_FILE_MODE
{ // prototype above appears in docs
   ALLEGRO_FILEMODE_READ    = 1,
   ALLEGRO_FILEMODE_WRITE   = 1 << 1,
   ALLEGRO_FILEMODE_EXECUTE = 1 << 2,
   ALLEGRO_FILEMODE_HIDDEN  = 1 << 3,
   ALLEGRO_FILEMODE_ISFILE  = 1 << 4,
   ALLEGRO_FILEMODE_ISDIR   = 1 << 5,
} ALLEGRO_FILE_MODE;

#ifndef EOF
   #define EOF    (-1)
#endif

/* Type: ALLEGRO_FS_INTERFACE
 */
typedef struct ALLEGRO_FS_INTERFACE ALLEGRO_FS_INTERFACE;

struct ALLEGRO_FS_INTERFACE {
   AL_METHOD(ALLEGRO_FS_ENTRY *, create,  (const char *path) );

   AL_METHOD(ALLEGRO_PATH *, getcwd, (void));
   AL_METHOD(bool, chdir, (const char *path));

   AL_METHOD(bool, exists_str, (const char *path));
   AL_METHOD(bool, remove_str, (const char *path));

   AL_METHOD(bool, mkdir, (const char *path));

   AL_METHOD(void, destroy, (ALLEGRO_FS_ENTRY *handle));

   AL_METHOD(ALLEGRO_PATH *, fname, (ALLEGRO_FS_ENTRY *handle));

   AL_METHOD(bool,    fstat,  (ALLEGRO_FS_ENTRY *handle));

   AL_METHOD(off_t,    entry_size,  (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(uint32_t, entry_mode,  (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(time_t,   entry_atime, (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(time_t,   entry_mtime, (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(time_t,   entry_ctime, (ALLEGRO_FS_ENTRY *fh));

   AL_METHOD(bool,  exists, (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(bool,  remove, (ALLEGRO_FS_ENTRY *fh));

   AL_METHOD(bool, opendir, (ALLEGRO_FS_ENTRY *dir));
   AL_METHOD(ALLEGRO_FS_ENTRY *, readdir, (ALLEGRO_FS_ENTRY *dir));
   AL_METHOD(bool, closedir, (ALLEGRO_FS_ENTRY *dir));
};

AL_FUNC(ALLEGRO_FS_ENTRY*, al_create_entry, (const char *path));
AL_FUNC(void, al_destroy_entry, (ALLEGRO_FS_ENTRY *handle));

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

AL_FUNC(bool, al_opendir, (ALLEGRO_FS_ENTRY *dir));
AL_FUNC(bool, al_closedir, (ALLEGRO_FS_ENTRY *dir));
AL_FUNC(ALLEGRO_FS_ENTRY *, al_readdir, (ALLEGRO_FS_ENTRY *dir));

AL_FUNC(bool, al_remove_str, (const char *path));
AL_FUNC(bool, al_is_present_str, (const char *));

AL_FUNC(bool, al_mkdir, (const char *));

AL_FUNC(ALLEGRO_PATH *, al_getcwd, (void));
AL_FUNC(bool, al_chdir, (const char *path));


/* Thread-local state. */
AL_FUNC(const ALLEGRO_FS_INTERFACE *, al_get_fs_interface, (void));
AL_FUNC(void, al_set_fs_interface, (const ALLEGRO_FS_INTERFACE *vtable));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FSHOOK_H */
