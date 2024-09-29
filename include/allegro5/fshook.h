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
 *      File System Hooks.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_allegro5_fshook_h
#define __al_included_allegro5_fshook_h

#include "allegro5/base.h"
#include "allegro5/file.h"
#include "allegro5/path.h"

#ifdef A5O_HAVE_SYS_TYPES_H
   #include <sys/types.h>
#else
/* 4 Gig max offsets if sys/types doesn't exist. */
typedef unsigned int off_t;
#endif

#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_FS_ENTRY
 */
typedef struct A5O_FS_ENTRY A5O_FS_ENTRY;

struct A5O_FS_ENTRY {
   struct A5O_FS_INTERFACE const *vtable;
};


/* Enum: A5O_FILE_MODE
 */
typedef enum A5O_FILE_MODE
{
   A5O_FILEMODE_READ    = 1,
   A5O_FILEMODE_WRITE   = 1 << 1,
   A5O_FILEMODE_EXECUTE = 1 << 2,
   A5O_FILEMODE_HIDDEN  = 1 << 3,
   A5O_FILEMODE_ISFILE  = 1 << 4,
   A5O_FILEMODE_ISDIR   = 1 << 5
} A5O_FILE_MODE;


#ifndef EOF
   #define EOF    (-1)
#endif


/* Type: A5O_FS_INTERFACE
 */
typedef struct A5O_FS_INTERFACE A5O_FS_INTERFACE;

struct A5O_FS_INTERFACE {
   AL_METHOD(A5O_FS_ENTRY *, fs_create_entry,  (const char *path));
   AL_METHOD(void,            fs_destroy_entry,    (A5O_FS_ENTRY *e));
   AL_METHOD(const char *,    fs_entry_name,       (A5O_FS_ENTRY *e));
   AL_METHOD(bool,            fs_update_entry,     (A5O_FS_ENTRY *e));
   AL_METHOD(uint32_t,        fs_entry_mode,       (A5O_FS_ENTRY *e));
   AL_METHOD(time_t,          fs_entry_atime,      (A5O_FS_ENTRY *e));
   AL_METHOD(time_t,          fs_entry_mtime,      (A5O_FS_ENTRY *e));
   AL_METHOD(time_t,          fs_entry_ctime,      (A5O_FS_ENTRY *e));
   AL_METHOD(off_t,           fs_entry_size,       (A5O_FS_ENTRY *e));
   AL_METHOD(bool,            fs_entry_exists,     (A5O_FS_ENTRY *e));
   AL_METHOD(bool,            fs_remove_entry,     (A5O_FS_ENTRY *e));

   AL_METHOD(bool,            fs_open_directory,   (A5O_FS_ENTRY *e));
   AL_METHOD(A5O_FS_ENTRY *, fs_read_directory,(A5O_FS_ENTRY *e));
   AL_METHOD(bool,            fs_close_directory,  (A5O_FS_ENTRY *e));

   AL_METHOD(bool,            fs_filename_exists,  (const char *path));
   AL_METHOD(bool,            fs_remove_filename,  (const char *path));
   AL_METHOD(char *,          fs_get_current_directory, (void));
   AL_METHOD(bool,            fs_change_directory, (const char *path));
   AL_METHOD(bool,            fs_make_directory,   (const char *path));

   AL_METHOD(A5O_FILE *,  fs_open_file,        (A5O_FS_ENTRY *e,
                                                    const char *mode));
};

AL_FUNC(A5O_FS_ENTRY *,   al_create_fs_entry,  (const char *path));
AL_FUNC(void,                 al_destroy_fs_entry, (A5O_FS_ENTRY *e));
AL_FUNC(const char *,         al_get_fs_entry_name,(A5O_FS_ENTRY *e));
AL_FUNC(bool,                 al_update_fs_entry,  (A5O_FS_ENTRY *e));
AL_FUNC(uint32_t,             al_get_fs_entry_mode,(A5O_FS_ENTRY *e));
AL_FUNC(time_t,               al_get_fs_entry_atime,(A5O_FS_ENTRY *e));
AL_FUNC(time_t,               al_get_fs_entry_mtime,(A5O_FS_ENTRY *e));
AL_FUNC(time_t,               al_get_fs_entry_ctime,(A5O_FS_ENTRY *e));
AL_FUNC(off_t,                al_get_fs_entry_size,(A5O_FS_ENTRY *e));
AL_FUNC(bool,                 al_fs_entry_exists,  (A5O_FS_ENTRY *e));
AL_FUNC(bool,                 al_remove_fs_entry,  (A5O_FS_ENTRY *e));

AL_FUNC(bool,                 al_open_directory,   (A5O_FS_ENTRY *e));
AL_FUNC(A5O_FS_ENTRY *,   al_read_directory,   (A5O_FS_ENTRY *e));
AL_FUNC(bool,                 al_close_directory,  (A5O_FS_ENTRY *e));

AL_FUNC(bool,                 al_filename_exists,  (const char *path));
AL_FUNC(bool,                 al_remove_filename,  (const char *path));
AL_FUNC(char *,               al_get_current_directory, (void));
AL_FUNC(bool,                 al_change_directory, (const char *path));
AL_FUNC(bool,                 al_make_directory,   (const char *path));

AL_FUNC(A5O_FILE *,       al_open_fs_entry,    (A5O_FS_ENTRY *e,
                                                    const char *mode));



/* Helper function for iterating over a directory using a callback. */

/* Type: A5O_FOR_EACH_FS_ENTRY_RESULT
 */
typedef enum A5O_FOR_EACH_FS_ENTRY_RESULT {
   A5O_FOR_EACH_FS_ENTRY_ERROR = -1,
   A5O_FOR_EACH_FS_ENTRY_OK    =  0,
   A5O_FOR_EACH_FS_ENTRY_SKIP  =  1,
   A5O_FOR_EACH_FS_ENTRY_STOP  =  2
} A5O_FOR_EACH_FS_ENTRY_RESULT;

AL_FUNC(int,  al_for_each_fs_entry, (A5O_FS_ENTRY *dir,
                                     int (*callback)(A5O_FS_ENTRY *entry, void *extra),
                                     void *extra));


/* Thread-local state. */
AL_FUNC(const A5O_FS_INTERFACE *, al_get_fs_interface, (void));
AL_FUNC(void, al_set_fs_interface, (const A5O_FS_INTERFACE *vtable));
AL_FUNC(void, al_set_standard_fs_interface, (void));


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
