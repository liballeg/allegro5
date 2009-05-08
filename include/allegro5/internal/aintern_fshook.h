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

extern struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks;

extern struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE _al_stdio_entry_fshooks;
extern struct ALLEGRO_FS_INTERFACE _al_fs_interface_stdio;

#define _al_fs_hook_destroy(handle)    (handle)->vtable->destroy(handle)
#define _al_fs_hook_open(handle)       (handle)->vtable->open(handle)
#define _al_fs_hook_close(handle)      (handle)->vtable->close(handle)

#define _al_fs_hook_entry_name(fp)                 (fp)->vtable->fname(fp)
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

#define _al_fs_hook_closedir(dir) (dir)->vtable->closedir(dir)
#define _al_fs_hook_readdir(dir)  (dir)->vtable->readdir(dir)


AL_END_EXTERN_C

#endif          /* ifndef __al_included_aintern_fshook_h */
