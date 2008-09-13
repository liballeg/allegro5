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
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/internal/fshook.h"

struct AL_FS_HOOK_SYS_INTERFACE  *_al_sys_fshooks = &_al_stdio_sys_fshooks;
struct AL_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks = &_al_stdio_entry_fshooks;

AL_FS_ENTRY *al_fs_create_handle(AL_CONST char *path)
{
   AL_FS_ENTRY *handle = _al_fs_hook_create_handle(path);
   if(!handle)
      return NULL;

   return handle;
}

void al_fs_destroy_handle(AL_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);

   _al_fs_hook_destroy_handle(handle);

}

int32_t al_fs_open_handle(AL_FS_ENTRY *handle, AL_CONST char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open_handle(handle, mode);
}

void al_fs_close_handle(AL_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);
   _al_fs_hook_close_handle(handle);
}

void al_fs_entry_name(AL_FS_ENTRY *fp, size_t size, char *buf)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_name(fp, size, buf);
}

AL_FS_ENTRY *al_fs_entry_open(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_entry_open(path, mode);
}

void al_fs_entry_close(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   _al_fs_hook_entry_close(fp);
}

ssize_t   al_fs_entry_read(void *ptr, size_t size, AL_FS_ENTRY *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_read(ptr, size, fp);
}

ssize_t   al_fs_entry_write(const void *ptr, size_t size, AL_FS_ENTRY *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_write(ptr, size, fp);
}

int32_t al_fs_entry_flush(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_flush(fp);
}

int32_t al_fs_entry_seek(AL_FS_ENTRY *fp, uint32_t offset, uint32_t whence)
{
   ASSERT(fp != NULL);
   ASSERT(offset >= 0);
   ASSERT(whence == AL_SEEK_SET || whence == AL_SEEK_CUR || whence == AL_SEEK_END);

   return _al_fs_hook_entry_seek(fp, offset, whence);
}

int32_t al_fs_entry_tell(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_tell(fp);
}

int32_t al_fs_entry_error(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_error(fp);
}

int32_t al_fs_entry_eof(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_eof(fp);
}


int32_t al_fs_entry_stat(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_stat(fp);
}


AL_FS_ENTRY *al_fs_opendir(const char *path)
{
   AL_FS_ENTRY *dir = NULL;

   ASSERT(path != NULL);

   dir = _al_fs_hook_opendir(path);
   if(!dir)
      return NULL;

   return dir;
}

int32_t al_fs_closedir(AL_FS_ENTRY *dir)
{
   struct _al_entry_fshooks *vtable = NULL;
   int32_t ret = 0;

   ASSERT(dir != NULL);

   ret = _al_fs_hook_closedir(dir);

   return ret;
}

int32_t al_fs_readdir(AL_FS_ENTRY *dir, size_t size, char *name)
{
   ASSERT(dir != NULL);
   ASSERT(size > 0);
   ASSERT(name != NULL);

   return _al_fs_hook_readdir(dir, size, name);
}

uint32_t al_fs_entry_mode(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mode(e);
}

time_t al_fs_entry_atime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_atime(e);
}

time_t al_fs_entry_mtime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mtime(e);
}

time_t al_fs_entry_ctime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_ctime(e);
}

off_t al_fs_entry_size(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_size(e);
}

int32_t al_fs_entry_unlink( AL_FS_ENTRY *e )
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_unlink( e );
}

int32_t al_fs_entry_exists( AL_FS_ENTRY *e )
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_exists( e );
}

int32_t al_fs_entry_isdir(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_fs_entry_mode(e) & AL_FM_ISDIR;
}

int32_t al_fs_entry_isfile(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_fs_entry_mode(e) & AL_FM_ISFILE;
}

AL_FS_ENTRY *al_fs_mktemp(const char *template, uint32_t ulink)
{
   ASSERT(template != NULL);
   ASSERT(ulink != AL_MKTEMP_UNLINK_NOW && ulink != AL_MKTEMP_UNLINK_ON_CLOSE && ulink != AL_MKTEMP_UNLINK_NEVER);

   return _al_fs_hook_mktemp(template, ulink);
}

int32_t al_fs_getcwd(char *buf, size_t len)
{
   ASSERT(buf != NULL);
   ASSERT(len != 0);

   return _al_fs_hook_getcwd(buf, len);
}

int32_t al_fs_chdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_chdir(path);
}

int32_t al_fs_add_search_path(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_add_search_path(path);
}

int32_t al_fs_search_path_count()
{
   return _al_fs_hook_search_path_count();
}

int32_t al_fs_get_search_path(uint32_t idx, char *dest, size_t len)
{
   ASSERT(dest);
   ASSERT(len);

   return _al_fs_hook_get_search_path(idx, dest, len);
}

int32_t al_fs_drive_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(len, sep);
}

int32_t al_fs_path_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(len, sep);
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup, driver may want to expose a "environment" that doesn't match the curren't platform's */
int32_t al_fs_path_to_sys(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_sys(orig, len, path);
}

int32_t al_fs_path_to_uni(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_uni(orig, len, path);
}

uint32_t al_fs_stat_mode( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path);
}

time_t al_fs_stat_atime( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_atime(path);
}

time_t al_fs_stat_mtime( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mtime(path);
}

time_t al_fs_stat_ctime( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_ctime(path);
}

off_t al_fs_stat_size( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_size(path);
}

int32_t al_fs_unlink( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_unlink(path);
}

int32_t al_fs_exists( const char *path )
{
   ASSERT(path != NULL);

   return _al_fs_hook_exists( path );
}

int32_t al_fs_isdir(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISDIR;
}

int32_t al_fs_isfile(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISFILE;
}

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
