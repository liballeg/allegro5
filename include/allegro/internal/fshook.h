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

#ifndef AINTERN_FHOOK_H
#define AINTERN_FHOOK_H

#include "base.h"

AL_BEGIN_EXTERN_C

/* INTERNAL STUFF */

/* This is totally internal. the access functions SHOULD be used instead. This vtable can change at any time. */
struct AL_FS_HOOK_VTABLE {
   AL_METHOD(AL_FILE *, fopen,  (AL_CONST char *path, AL_CONST char *mode) );
   AL_METHOD(uint32_t,  fclose, (AL_FILE *fp) );
   AL_METHOD(size_t,    fread,  (void *ptr, size_t size, AL_FILE *fp) );
   AL_METHOD(size_t,    fwrite, (AL_CONST void *ptr, size_t size, AL_FILE *fp) );
   AL_METHOD(uint32_t,  fflush, (AL_FILE *fp) );
   AL_METHOD(uint32_t,  fseek,  (AL_FILE *fp, uint32_t offset, uint32_t whence) );
   AL_METHOD(uint32_t,  ftell,  (AL_FILE *fp) );
   AL_METHOD(uint32_t,  ferror, (AL_FILE *fp) );
   AL_METHOD(uint32_t,  feof,   (AL_FILE *fp) );

   AL_METHOD(uint32_t,  fstat, (AL_CONST char *path, AL_STAT *stbuf) );

   AL_METHOD(AL_DIR *,    opendir,  (AL_CONST char *path) );
   AL_METHOD(AL_DIRENT *, readdir,  (AL_DIR *dir) );
   AL_METHOD(uint32_t,    closedir, (AL_DIR *dir) );

   AL_METHOD(AL_FILE *, mktemp, (AL_CONST char *template) );
   AL_METHOD(uint32_t,  getcwd, (char *buf, size_t *len) );
   AL_METHOD(uint32_t,  chdir,  (AL_CONST char *path) );
   AL_METHOD(uint32_t,  getdir, (uint32_t id, char *dir, uint32_t *len) );

   AL_METHOD(uint32_t, add_search_path, (AL_CONST char *path) );
   AL_METHOD(uint32_t, search_path_count, (void) );
   AL_METHOD(uint32_t, get_search_path, (char *dest, uint32_t *len) );

   /* I had wanted to make these a single function with a few enum entries, but can I assume
      all the types are the same size? */
   AL_METHOD(uint32_t, get_stat_mode,  (AL_STAT *) );
   AL_METHOD(time_t,   get_stat_atime, (AL_STAT *) );
   AL_METHOD(time_t,   get_stat_mtime, (AL_STAT *) );
   AL_METHOD(time_t,   get_stat_ctime, (AL_STAT *) );
   AL_METHOD(size_t,   get_stat_size,  (AL_STAT *) );

   AL_METHOD(uint32_t, path_to_sys, (const char *orig, uint32_t len, char *path) );
   AL_METHOD(uint32_t, path_to_uni, (const char *orig, uint32_t len, char *path) );
};

/* Can't figure out a better way. Global it is */
AL_VAR(struct AL_FS_HOOK_VTABLE, _al_fshooks);

#define _al_fs_hook_fopen(path, mode)         _al_fshooks.fopen(path, mode)
#define _al_fs_hook_fclose(fp)                _al_fshooks.fclose(fp)
#define _al_fs_hook_fread(ptr, size, fp)      _al_fshooks.fread(ptr, size, fp)
#define _al_fs_hook_fwrite(ptr, size, fp)     _al_fshooks.fwrite(ptr, size, fp)
#define _al_fs_hook_fflush(fp)                _al_fshooks.fflush(fp)
#define _al_fs_hook_fseek(fp, offset, whence) _al_fshooks.fseek(fp,offset,whence)
#define _al_fs_hook_ftell(fp)                 _al_fshooks.ftell(fp)
#define _al_fs_hook_ferror(fp)                _al_fshooks.ferror(fp)
#define _al_fs_hook_feof(fp)                  _al_fshooks.feof(fp)

#define _al_fs_hook_fstat(path, stbuf) _al_fshooks.fstat(path, stbuf)

#define _al_fs_hook_opendir(path) _al_fshooks.opendir(path)
#define _al_fs_hook_closedir(dir) _al_fshooks.closedir(dir)
#define _al_fs_hook_readdir(dir)  _al_fshooks.readdir(dir)

#define _al_fs_hook_mktemp(template)     _al_fshooks.mktemp(template)
#define _al_fs_hook_getcwd(buf, len)     _al_fshooks.getcwd(buf, len)
#define _al_fs_hook_chdir(path)          _al_fshooks.chdir(path)
#define _al_fs_hook_getdir(id, dir, len) _al_fshooks.getdir(id, dir, len)

#define _al_fs_hook_add_search_path(path)      _al_fshooks.add_search_path(path)
#define _al_fs_hook_search_path_count()        _al_fshooks.search_path_count()
#define _al_fs_hook_get_search_path(dest, len) _al_fshooks.get_search_path(dest, len)

#define _al_fs_hook_get_stat_mode(st)  _al_fshooks.get_stat_mode(st)
#define _al_fs_hook_get_stat_atime(st) _al_fshooks.get_stat_atime(st)
#define _al_fs_hook_get_stat_mtime(st) _al_fshooks.get_stat_mtime(st)
#define _al_fs_hook_get_stat_ctime(st) _al_fshooks.get_stat_ctime(st)
#define _al_fs_hook_get_stat_size(st)  _al_fshooks.get_stat_size(st)

#define _al_fs_hook_path_to_sys(orig, len, path) _al_fshooks.path_to_sys(orig, len, path)
#define _al_fs_hook_path_to_uni(orig, len, path) _al_fshooks.path_to_uni(orig, len, path)

AL_END_EXTERN_C

#endif          /* ifndef AINTERN_FHOOK_H */
