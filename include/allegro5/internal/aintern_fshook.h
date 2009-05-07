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
 *      Internal File System Hook support, draft.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __al_included_aintern_fshook_h
#define __al_included_aintern_fshook_h

#include "allegro5/base.h"

AL_BEGIN_EXTERN_C

/* INTERNAL STUFF */

/* This is totally internal. the access functions SHOULD be used instead. These vtables can change at any time. */
/* They _can_ change, but should only ever be extended from the end in stable releases.
   a devel release can feel free to insert or rearange items, but it will break compatibility with extensions */

#ifdef ALLEGRO_LIB_BUILD

struct ALLEGRO_FS_HOOK_SYS_INTERFACE {
   AL_METHOD(ALLEGRO_FS_ENTRY *, create,  (const char *path) );
   AL_METHOD(ALLEGRO_FS_ENTRY *, opendir, (const char *path) );

   AL_METHOD(ALLEGRO_PATH *, getcwd, (void));
   AL_METHOD(bool, chdir, (const char *path));

   AL_METHOD(int32_t, drive_sep, (char *sep, size_t len));
   AL_METHOD(int32_t, path_sep, (char *sep, size_t len));

   AL_METHOD(bool, exists, (const char *path));
   AL_METHOD(bool, remove, (const char *path));

   AL_METHOD(bool, mkdir, (const char *path));
};

struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE {
   AL_METHOD(void, destroy, (ALLEGRO_FS_ENTRY *handle));
   AL_METHOD(bool, open,    (ALLEGRO_FS_ENTRY *handle));
   AL_METHOD(void, close,   (ALLEGRO_FS_ENTRY *handle));

   AL_METHOD(ALLEGRO_PATH *, fname, (ALLEGRO_FS_ENTRY *handle));

   AL_METHOD(bool,    fstat,  (ALLEGRO_FS_ENTRY *handle));

   AL_METHOD(off_t,    entry_size,  (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(uint32_t, entry_mode,  (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(time_t,   entry_atime, (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(time_t,   entry_mtime, (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(time_t,   entry_ctime, (ALLEGRO_FS_ENTRY *fh));

   AL_METHOD(bool,  exists, (ALLEGRO_FS_ENTRY *fh));
   AL_METHOD(bool,  remove, (ALLEGRO_FS_ENTRY *fh));

   AL_METHOD(ALLEGRO_FS_ENTRY *, readdir, (ALLEGRO_FS_ENTRY *dir));
   AL_METHOD(bool, closedir, (ALLEGRO_FS_ENTRY *dir));
};

extern struct ALLEGRO_FS_HOOK_SYS_INTERFACE  *_al_sys_fshooks;
extern struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks;

extern struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE _al_stdio_entry_fshooks;
extern struct ALLEGRO_FS_HOOK_SYS_INTERFACE _al_stdio_sys_fshooks;

#define _al_fs_hook_create(path)       _al_sys_fshooks->create(path)
#define _al_fs_hook_destroy(handle)    (handle)->vtable->destroy(handle)
#define _al_fs_hook_open(handle)       (handle)->vtable->open(handle)
#define _al_fs_hook_close(handle)      (handle)->vtable->close(handle)

#define _al_fs_hook_entry_name(fp)                 (fp)->vtable->fname(fp)
#define _al_fs_hook_entry_open(path, mode)         _al_sys_fshooks->fopen(path, mode)
#define _al_fs_hook_entry_close(fp)                (fp)->vtable->fclose(fp)
#define _al_fs_hook_entry_read(fp, size, ptr)      (fp)->vtable->fread(fp, size, ptr)
#define _al_fs_hook_entry_write(fp, size, ptr)     (fp)->vtable->fwrite(fp, size, ptr)
#define _al_fs_hook_entry_flush(fp)                (fp)->vtable->fflush(fp)
#define _al_fs_hook_entry_seek(fp, offset, whence) (fp)->vtable->fseek(fp,offset,whence)
#define _al_fs_hook_entry_tell(fp)                 (fp)->vtable->ftell(fp)
#define _al_fs_hook_entry_error(fp)                (fp)->vtable->ferror(fp)
#define _al_fs_hook_entry_eof(fp)                  (fp)->vtable->feof(fp)
#define _al_fs_hook_entry_ungetc(fp, c)            (fp)->vtable->fungetc(fp, c)

#define _al_fs_hook_entry_stat(path) (fp)->vtable->fstat(path)
#define _al_fs_hook_entry_mode(fp)          (fp)->vtable->entry_mode(fp)
#define _al_fs_hook_entry_atime(fp)         (fp)->vtable->entry_atime(fp)
#define _al_fs_hook_entry_mtime(fp)         (fp)->vtable->entry_mtime(fp)
#define _al_fs_hook_entry_ctime(fp)         (fp)->vtable->entry_ctime(fp)
#define _al_fs_hook_entry_size(fp)          (fp)->vtable->entry_size(fp)

#define _al_fs_hook_entry_remove(fp) (fp)->vtable->remove(fp)

#define _al_fs_hook_entry_exists(fp) (fp)->vtable->exists(fp)

#define _al_fs_hook_opendir(path) _al_sys_fshooks->opendir(path)
#define _al_fs_hook_closedir(dir) (dir)->vtable->closedir(dir)
#define _al_fs_hook_readdir(dir)  (dir)->vtable->readdir(dir)

#define _al_fs_hook_getcwd()             _al_sys_fshooks->getcwd()
#define _al_fs_hook_chdir(path)          _al_sys_fshooks->chdir(path)

#define _al_fs_hook_mkdir(path) _al_sys_fshooks->mkdir(path)

#define _al_fs_hook_exists(path) _al_sys_fshooks->exists(path)
#define _al_fs_hook_remove(path) _al_sys_fshooks->remove(path)


// Still doing these 4?
// Yup, the vfs may provide a different view of the FS, ie: leaving out the concept of drives
#define _al_fs_hook_drive_sep(sep, len) _al_sys_fshooks->drive_sep(sep, len)
#define _al_fs_hook_path_sep(sep, len)  _al_sys_fshooks->path_sep(sep, len)

#endif /* ALLEGRO_LIB_BUILD */


AL_END_EXTERN_C

#endif          /* ifndef __al_included_aintern_fshook_h */
