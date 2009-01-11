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
   AL_METHOD(ALLEGRO_FS_ENTRY *, create_handle, (AL_CONST char *path) );
   AL_METHOD(ALLEGRO_FS_ENTRY *, opendir,       (AL_CONST char *path) );
   AL_METHOD(ALLEGRO_FS_ENTRY *, fopen,         (AL_CONST char *path, AL_CONST char *mode) );
   AL_METHOD(ALLEGRO_FS_ENTRY *, mktemp,        (AL_CONST char *t, uint32_t ulink) );

   AL_METHOD(int32_t, getcwd, (char *buf, size_t len) );
   AL_METHOD(int32_t, chdir,  (AL_CONST char *path) );

   AL_METHOD(int32_t, add_search_path,   (AL_CONST char *path) );
   AL_METHOD(uint32_t, search_path_count, (void) );
   AL_METHOD(int32_t, get_search_path,   (uint32_t idx, char *dest, uint32_t len) );

   AL_METHOD(int32_t, drive_sep, (size_t len, char *sep) );
   AL_METHOD(int32_t, path_sep,  (size_t len, char *sep) );

   AL_METHOD(int32_t, path_to_sys, (AL_CONST char *orig, uint32_t len, char *path) );
   AL_METHOD(int32_t, path_to_uni, (AL_CONST char *orig, uint32_t len, char *path) );

   AL_METHOD(int32_t, exists, (AL_CONST char *) );
   AL_METHOD(int32_t, unlink, (AL_CONST char *) );

   AL_METHOD(int32_t, mkdir, (AL_CONST char *) );

   AL_METHOD(off_t,    stat_size,  (AL_CONST char *) );
   AL_METHOD(uint32_t, stat_mode,  (AL_CONST char *) );
   AL_METHOD(time_t,   stat_atime, (AL_CONST char *) );
   AL_METHOD(time_t,   stat_mtime, (AL_CONST char *) );
   AL_METHOD(time_t,   stat_ctime, (AL_CONST char *) );
};

struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE {
   AL_METHOD(void,     destroy_handle, (ALLEGRO_FS_ENTRY *handle) );
   AL_METHOD(int32_t,  open_handle,    (ALLEGRO_FS_ENTRY *handle, AL_CONST char *mode) );
   AL_METHOD(void,  close_handle,   (ALLEGRO_FS_ENTRY *handle) );

   AL_METHOD(void,     fname,  (ALLEGRO_FS_ENTRY *fh, size_t s, char *name) );

   AL_METHOD(void,  fclose, (ALLEGRO_FS_ENTRY *fp) );
   AL_METHOD(size_t,   fread,  (ALLEGRO_FS_ENTRY *fp, size_t size, void *ptr) );
   AL_METHOD(size_t,   fwrite, (ALLEGRO_FS_ENTRY *fp, size_t size, AL_CONST void *ptr) );
   AL_METHOD(int32_t,  fflush, (ALLEGRO_FS_ENTRY *fp) );
   AL_METHOD(int32_t,  fseek,  (ALLEGRO_FS_ENTRY *fp, uint32_t offset, uint32_t whence) );
   AL_METHOD(int32_t,  ftell,  (ALLEGRO_FS_ENTRY *fp) );
   AL_METHOD(int32_t,  ferror, (ALLEGRO_FS_ENTRY *fp) );
   AL_METHOD(int32_t,  feof,   (ALLEGRO_FS_ENTRY *fp) );
   AL_METHOD(int32_t,  fstat,  (ALLEGRO_FS_ENTRY *handle) );
   AL_METHOD(int32_t,  ungetc, (int32_t c, ALLEGRO_FS_ENTRY *fp) );

   AL_METHOD(off_t,   entry_size,  (ALLEGRO_FS_ENTRY *) );
   AL_METHOD(uint32_t, entry_mode,  (ALLEGRO_FS_ENTRY *) );
   AL_METHOD(time_t,   entry_atime, (ALLEGRO_FS_ENTRY *) );
   AL_METHOD(time_t,   entry_mtime, (ALLEGRO_FS_ENTRY *) );
   AL_METHOD(time_t,   entry_ctime, (ALLEGRO_FS_ENTRY *) );

   AL_METHOD(int32_t,  exists,  (ALLEGRO_FS_ENTRY *) );
   AL_METHOD(int32_t,  unlink,  (ALLEGRO_FS_ENTRY *) );

   AL_METHOD(int32_t,  readdir,  (ALLEGRO_FS_ENTRY *dir, size_t size, char *name) );
   AL_METHOD(int32_t,  closedir, (ALLEGRO_FS_ENTRY *dir) );
};

extern struct ALLEGRO_FS_HOOK_SYS_INTERFACE  *_al_sys_fshooks;
extern struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks;

extern struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE _al_stdio_entry_fshooks;
extern struct ALLEGRO_FS_HOOK_SYS_INTERFACE _al_stdio_sys_fshooks;

#define _al_fs_hook_create_handle(path)       _al_sys_fshooks->create_handle(path)
#define _al_fs_hook_destroy_handle(handle)    (handle)->vtable->destroy_handle(handle)
#define _al_fs_hook_open_handle(handle, mode) (handle)->vtable->open_handle(handle, mode)
#define _al_fs_hook_close_handle(handle)      (handle)->vtable->close_handle(handle)

#define _al_fs_hook_entry_name(fp, s, b)           (fp)->vtable->fname(fp, s, b)
#define _al_fs_hook_entry_open(path, mode)         _al_sys_fshooks->fopen(path, mode)
#define _al_fs_hook_entry_close(fp)                (fp)->vtable->fclose(fp)
#define _al_fs_hook_entry_read(fp, size, ptr)      (fp)->vtable->fread(fp, size, ptr)
#define _al_fs_hook_entry_write(fp, size, ptr)     (fp)->vtable->fwrite(fp, size, ptr)
#define _al_fs_hook_entry_flush(fp)                (fp)->vtable->fflush(fp)
#define _al_fs_hook_entry_seek(fp, offset, whence) (fp)->vtable->fseek(fp,offset,whence)
#define _al_fs_hook_entry_tell(fp)                 (fp)->vtable->ftell(fp)
#define _al_fs_hook_entry_error(fp)                (fp)->vtable->ferror(fp)
#define _al_fs_hook_entry_eof(fp)                  (fp)->vtable->feof(fp)
#define _al_fs_hook_entry_ungetc(c, fp)            (fp)->vtable->ungetc(c, fp)

#define _al_fs_hook_entry_stat(path) (fp)->vtable->fstat(path)
#define _al_fs_hook_entry_mode(fp)          (fp)->vtable->entry_mode(fp)
#define _al_fs_hook_entry_atime(fp)         (fp)->vtable->entry_atime(fp)
#define _al_fs_hook_entry_mtime(fp)         (fp)->vtable->entry_mtime(fp)
#define _al_fs_hook_entry_ctime(fp)         (fp)->vtable->entry_ctime(fp)
#define _al_fs_hook_entry_size(fp)          (fp)->vtable->entry_size(fp)

#define _al_fs_hook_entry_unlink(fp) (fp)->vtable->unlink(fp)

#define _al_fs_hook_entry_exists(fp) (fp)->vtable->exists(fp)

#define _al_fs_hook_opendir(path) _al_sys_fshooks->opendir(path)
#define _al_fs_hook_closedir(dir) (dir)->vtable->closedir(dir)
#define _al_fs_hook_readdir(dir, size, name)  (dir)->vtable->readdir(dir, size, name)

#define _al_fs_hook_mktemp(template, ulink)     _al_sys_fshooks->mktemp(template, ulink)
#define _al_fs_hook_getcwd(buf, len)     _al_sys_fshooks->getcwd(buf, len)
#define _al_fs_hook_chdir(path)          _al_sys_fshooks->chdir(path)

#define _al_fs_hook_add_search_path(path)           _al_sys_fshooks->add_search_path(path)
#define _al_fs_hook_search_path_count()             _al_sys_fshooks->search_path_count()
#define _al_fs_hook_get_search_path(idx, dest, len) _al_sys_fshooks->get_search_path(idx, dest, len)

#define _al_fs_hook_stat_mode(st)  _al_sys_fshooks->stat_mode(st)
#define _al_fs_hook_stat_atime(st) _al_sys_fshooks->stat_atime(st)
#define _al_fs_hook_stat_mtime(st) _al_sys_fshooks->stat_mtime(st)
#define _al_fs_hook_stat_ctime(st) _al_sys_fshooks->stat_ctime(st)
#define _al_fs_hook_stat_size(st)  _al_sys_fshooks->stat_size(st)

#define _al_fs_hook_mkdir(path) _al_sys_fshooks->mkdir(path)

#define _al_fs_hook_exists(path) _al_sys_fshooks->exists(path)
#define _al_fs_hook_unlink(path) _al_sys_fshooks->unlink(path)


// Still doing these 4?
// Yup, the vfs may provide a different view of the FS, ie: leaving out the concept of drives
#define _al_fs_hook_drive_sep(len, sep) _al_sys_fshooks->drive_sep(len, sep)
#define _al_fs_hook_path_sep(len, sep)  _al_sys_fshooks->path_sep(len, sep)

#define _al_fs_hook_path_to_sys(orig, len, path) _al_sys_fshooks->path_to_sys(orig, len, path)
#define _al_fs_hook_path_to_uni(orig, len, path) _al_sys_fshooks->path_to_uni(orig, len, path)

#endif /* ALLEGRO_LIB_BUILD */


AL_END_EXTERN_C

#endif          /* ifndef __al_included_aintern_fshook_h */
