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


extern struct ALLEGRO_FS_INTERFACE _al_fs_interface_stdio;

#define _al_fs_hook_destroy(handle)    (handle)->vtable->destroy(handle)

#define _al_fs_hook_entry_name(fp)                 (fp)->vtable->fname(fp)
#define _al_fs_hook_entry_read(fp, size, ptr)      (fp)->vtable->fread(fp, size, ptr)
#define _al_fs_hook_entry_write(fp, size, ptr)     (fp)->vtable->fwrite(fp, size, ptr)

#define _al_fs_hook_entry_stat(path) (fp)->vtable->fstat(path)
#define _al_fs_hook_entry_mode(fp)          (fp)->vtable->entry_mode(fp)
#define _al_fs_hook_entry_atime(fp)         (fp)->vtable->entry_atime(fp)
#define _al_fs_hook_entry_mtime(fp)         (fp)->vtable->entry_mtime(fp)
#define _al_fs_hook_entry_ctime(fp)         (fp)->vtable->entry_ctime(fp)
#define _al_fs_hook_entry_size(fp)          (fp)->vtable->entry_size(fp)

#define _al_fs_hook_entry_remove(fp) (fp)->vtable->remove(fp)
#define _al_fs_hook_entry_exists(fp) (fp)->vtable->exists(fp)

#define _al_fs_hook_opendir(dir) (dir)->vtable->opendir(dir)
#define _al_fs_hook_closedir(dir) (dir)->vtable->closedir(dir)
#define _al_fs_hook_readdir(dir)  (dir)->vtable->readdir(dir)


AL_END_EXTERN_C

#endif          /* ifndef __al_included_aintern_fshook_h */
