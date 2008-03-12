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

#include "allegro.h"
#include "allegro/debug.h"
#include "allegro/fshook.h"
#include "allegro/internal/fshook.h"
#include ALLEGRO_INTERNAL_HEADER

static struct AL_FS_HOOK_SYS_VTABLE  _al_sys_fshooks;
static struct AL_FS_HOOK_ENTRY_VTABLE _al_entry_fshooks;

int al_fs_set_hook(uint32_t phid, void *fshook)
{
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return -1;

   switch(phid) {
      case AL_FS_HOOK_CREATE_HANDLE:
         _al_sys_fshooks.create_handle = fshook;
         break;

      case AL_FS_HOOK_DESTROY_HANDLE:
         _al_entry_fshooks.destroy_handle = fshook;
         break;

      case AL_FS_HOOK_OPEN_HANDLE:
         _al_entry_fshooks.open_handle = fshook;
         break;

      case AL_FS_HOOK_CLOSE_HANDLE:
         _al_entry_fshooks.close_handle = fshook;
         break;

      case AL_FS_HOOK_ENTRY_OPEN:
         _al_sys_fshooks.fopen = fshook;
         break;

      case AL_FS_HOOK_ENTRY_CLOSE:
         _al_entry_fshooks.fclose = fshook;
         break;

      case AL_FS_HOOK_ENTRY_READ:
         _al_entry_fshooks.fread = fshook;
         break;

      case AL_FS_HOOK_ENTRY_WRITE:
         _al_entry_fshooks.fwrite = fshook;
         break;

      case AL_FS_HOOK_ENTRY_FLUSH:
         _al_entry_fshooks.fflush = fshook;
         break;

      case AL_FS_HOOK_ENTRY_SEEK:
         _al_entry_fshooks.fseek = fshook;
         break;

      case AL_FS_HOOK_ENTRY_TELL:
         _al_entry_fshooks.ftell = fshook;
         break;

      case AL_FS_HOOK_ENTRY_ERROR:
         _al_entry_fshooks.ferror = fshook;
         break;

      case AL_FS_HOOK_ENTRY_EOF:
         _al_entry_fshooks.feof = fshook;
         break;

      case AL_FS_HOOK_ENTRY_STAT:
         _al_entry_fshooks.fstat = fshook;
         break;

      case AL_FS_HOOK_OPENDIR:
         _al_sys_fshooks.opendir = fshook;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         _al_entry_fshooks.closedir = fshook;
         break;

      case AL_FS_HOOK_READDIR:
         _al_entry_fshooks.readdir = fshook;
         break;

      case AL_FS_HOOK_MKTEMP:
         _al_sys_fshooks.mktemp = fshook;
         break;

      case AL_FS_HOOK_GETCWD:
         _al_sys_fshooks.getcwd = fshook;
         break;

      case AL_FS_HOOK_CHDIR:
         _al_sys_fshooks.chdir = fshook;
         break;

      case AL_FS_HOOK_ADD_SEARCH_PATH:
         _al_sys_fshooks.add_search_path = fshook;
         break;

      case AL_FS_HOOK_SEARCH_PATH_COUNT:
         _al_sys_fshooks.search_path_count = fshook;
         break;

      case AL_FS_HOOK_GET_SEARCH_PATH:
         _al_sys_fshooks.get_search_path = fshook;
         break;

      case AL_FS_HOOK_ENTRY_MODE:
         _al_entry_fshooks.entry_mode = fshook;
         break;

      case AL_FS_HOOK_ENTRY_ATIME:
         _al_entry_fshooks.entry_atime = fshook;
         break;

      case AL_FS_HOOK_ENTRY_MTIME:
         _al_entry_fshooks.entry_mtime = fshook;
         break;

      case AL_FS_HOOK_ENTRY_CTIME:
         _al_entry_fshooks.entry_ctime = fshook;
         break;

      case AL_FS_HOOK_ENTRY_SIZE:
         _al_entry_fshooks.entry_size = fshook;
         break;

      case AL_FS_HOOK_ENTRY_UNLINK:
         _al_entry_fshooks.entry_unlink = fshook;
         break;

      case AL_FS_HOOK_ENTRY_EXISTS:
         _al_entry_fshooks.entry_exists = fshook;
         break;

      case AL_FS_HOOK_STAT_MODE:
         _al_sys_fshooks.file_mode = fshook;
         break;

      case AL_FS_HOOK_STAT_ATIME:
         _al_sys_fshooks.file_atime = fshook;
         break;

      case AL_FS_HOOK_STAT_MTIME:
         _al_sys_fshooks.file_mtime = fshook;
         break;

      case AL_FS_HOOK_STAT_CTIME:
         _al_sys_fshooks.file_ctime = fshook;
         break;

      case AL_FS_HOOK_STAT_SIZE:
         _al_sys_fshooks.file_size = fshook;
         break;

      case AL_FS_HOOK_PATH_TO_SYS:
         _al_sys_fshooks.path_to_sys = fshook;
         break;

      case AL_FS_HOOK_PATH_TO_UNI:
         _al_sys_fshooks.path_to_uni = fshook;
         break;

      case AL_FS_HOOK_UNLINK:
         break;

      case AL_FS_HOOK_EXISTS:
         break;

      default:
         break;
   }

   return 0;
}

void *al_fs_get_hook(uint32_t phid)
{
   /* ptr exists in case we need to add a mutex. */
   void *ptr = NULL;
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return NULL;

   switch(phid) {
      case AL_FS_HOOK_CREATE_HANDLE:
         ptr = _al_sys_fshooks.create_handle;
         break;

      case AL_FS_HOOK_DESTROY_HANDLE:
         ptr = _al_entry_fshooks.destroy_handle;
         break;

      case AL_FS_HOOK_OPEN_HANDLE:
         ptr = _al_sys_fshooks.open_handle;
         break;

      case AL_FS_HOOK_CLOSE_HANDLE:
         ptr = _al_entry_fshooks.close_handle;
         break;

      case AL_FS_HOOK_ENTRY_OPEN:
         ptr = _al_sys_fshooks.fopen;
         break;

      case AL_FS_HOOK_ENTRY_CLOSE:
         ptr = _al_entry_fshooks.fclose;
         break;

      case AL_FS_HOOK_ENTRY_READ:
         ptr = _al_entry_fshooks.fread;
         break;

      case AL_FS_HOOK_ENTRY_WRITE:
         ptr = _al_entry_fshooks.fwrite;
         break;

      case AL_FS_HOOK_ENTRY_FLUSH:
         ptr = _al_entry_fshooks.fflush;
         break;

      case AL_FS_HOOK_ENTRY_SEEK:
         ptr = _al_entry_fshooks.fseek;
         break;

      case AL_FS_HOOK_ENTRY_TELL:
         ptr = _al_entry_fshooks.ftell;
         break;

      case AL_FS_HOOK_ENTRY_ERROR:
         ptr = _al_entry_fshooks.ferror;
         break;

      case AL_FS_HOOK_ENTRY_EOF:
         ptr = _al_entry_fshooks.feof;
         break;

      case AL_FS_HOOK_ENTRY_STAT:
         ptr = _al_entry_fshooks.fstat;
         break;

      case AL_FS_HOOK_OPENDIR:
         ptr = _al_sys_fshooks.opendir;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         ptr = _al_entry_fshooks.closedir;
         break;

      case AL_FS_HOOK_READDIR:
         ptr = _al_entry_fshooks.readdir;
         break;

      case AL_FS_HOOK_MKTEMP:
         ptr = _al_sys_fshooks.mktemp;
         break;

      case AL_FS_HOOK_GETCWD:
         ptr = _al_sys_fshooks.getcwd;
         break;

      case AL_FS_HOOK_CHDIR:
         ptr = _al_sys_fshooks.chdir;
         break;

      case AL_FS_HOOK_ADD_SEARCH_PATH:
         ptr = _al_sys_fshooks.add_search_path;
         break;

      case AL_FS_HOOK_SEARCH_PATH_COUNT:
         ptr = _al_sys_fshooks.search_path_count;
         break;

      case AL_FS_HOOK_GET_SEARCH_PATH:
         ptr = _al_sys_fshooks.get_search_path;
         break;

      case AL_FS_HOOK_ENTRY_MODE:
         ptr = _al_entry_fshooks.entry_mode;
         break;

      case AL_FS_HOOK_ENTRY_ATIME:
         ptr = _al_entry_fshooks.entry_atime;
         break;

      case AL_FS_HOOK_ENTRY_MTIME:
         ptr = _al_entry_fshooks.entry_mtime;
         break;

      case AL_FS_HOOK_ENTRY_SIZE:
         ptr = _al_entry_fshooks.entry_size;
         break;

      case AL_FS_HOOK_ENTRY_CTIME:
         ptr = _al_entry_fshooks.entry_ctime;
         break;

      case AL_FS_HOOK_ENTRY_UNLINK:
         ptr = _al_entry_fshooks.unlink;
         break

      case AL_FS_HOOK_ENTRY_EXISTS:
         ptr = _al_entry_fshooks.exists;
         break;

      case AL_FS_HOOK_PATH_TO_SYS:
         ptr = _al_sys_fshooks.path_to_sys;
         break;

      case AL_FS_HOOK_PATH_TO_UNI:
         ptr = _al_sys_fshooks.path_to_uni;
         break;

      case AL_FS_HOOK_STAT_MODE:
         ptr = _al_sys_fshooks.stat_mode;
         break;

      case AL_FS_HOOK_STAT_ATIME:
         ptr = _al_sys_fshooks.stat_atime;
         break;

      case AL_FS_HOOK_STAT_MTIME:
         ptr = _al_sys_fshooks.stat_mtime;
         break;

      case AL_FS_HOOK_STAT_SIZE:
         ptr = _al_sys_fshooks.stat_size;
         break;

      case AL_FS_HOOK_STAT_CTIME:
         ptr = _al_sys_fshooks.stat_ctime;
         break;

      case AL_FS_HOOK_UNLINK:
         ptr = _al_sys_fshooks.unlink;
         break

      case AL_FS_HOOK_EXISTS:
         ptr = _al_sys_fshooks.exists;
         break;

      default:
         ptr = NULL;
         break;
   }

   return ptr;
}

AL_FS_ENTRY *al_fs_create_handle(AL_CONST char *path)
{
   AL_FS_ENTRY *handle = _al_fs_hook_create_handle(path);
   if(!handle)
      return NULL;

   handle->vtable = AL_MALLOC(sizeof(struct AL_FS_HOOK_ENTRY_VTABLE));
   if(!handle->vtable) {
      _al_fs_hook_destroy_handle(handle);
      return NULL;
   }

   *(handle->vtable) = _al_entry_fshooks;

   return handle;
}

void al_fs_destroy_handle(AL_FS_ENTRY *handle)
{
   struct _al_entry_fshooks *vtable = NULL;

   ASSERT(handle != NULL);
   ASSERT(handle->vtable != NULL);

   vtable = handle->vtable;

   _al_fs_hook_destroy_handle(handle);

   AL_FREE(vtable);
}

int32_t al_fs_open_handle(AL_FS_ENTRY *handle, AL_CONST char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open_handle(handle, mode);
}

int32_t al_fs_close_handle(AL_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);

   return _al_fs_hook_close_handle(handle);
}

void al_fs_entry_name(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_name(fp);
}

AL_FS_ENTRY *al_fs_entry_open(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_entry_open(path, mode);
}

int32_t al_fs_entry_close(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_close(fp);
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

   dir->vtable = AL_MALLOC(sizeof(struct AL_FS_HOOK_ENTRY_VTABLE));
   if(!dir->vtable) {
      _al_fs_hook_closedir(dir);
      return NULL;
   }

   *(dir->vtable) = _al_entry_fshooks;

   return dir;
}

int32_t al_fs_closedir(AL_FS_ENTRY *dir)
{
   struct _al_entry_fshooks *vtable = NULL;
   int32_t ret = 0;

   ASSERT(dir != NULL);
   ASSERT(dir->vtable != NULL);

   vtable = dir->vtable;

   ret = _al_fs_hook_closedir(dir);

   AL_FREE(vtable);

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
   return _al_fs_hook_entry_mode(st);
}

time_t al_fs_entry_atime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_atime(st);
}

time_t al_fs_entry_mtime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mtime(st);
}

time_t al_fs_entry_ctime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_ctime(st);
}

size_t al_fs_entry_size(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_size(st);
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
   ASSERT(ulink != AL_MKTEMP_UNLINK_NOW && ulink != AL_MKTEMP_UNLINK_ON_CLOSE);

   return _al_fs_hook_mktemp(template, ulink);
}

int32_t al_fs_getcwd(char *buf, size_t len)
{
   ASSERT(buf != NULL);
   ASSERT(len != NULL);

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

uint32_t al_fs_drive_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(len, sep);
}

uint32_t al_fs_path_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(len, sep);
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup, driver may want to expose a "environment" that doesn't match the curren't platform's */
uint32_t al_fs_path_to_sys(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_sys(orig, len, path);
}

uint32_t al_fs_path_to_uni(const char *orig, size_t len, char *path)
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

size_t al_fs_stat_size( const char *path )
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
   return al_fs_stat_mode() & AL_FM_ISDIR;
}

int32_t al_fs_isfile(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return al_fs_stat_mode() & AL_FM_ISFILE;
}

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
