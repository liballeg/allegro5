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
static struct AL_FS_HOOK_FILE_VTABLE _al_file_fshooks;
static struct AL_FS_HOOK_DIR_VTABLE  _al_dir_fshooks;

int al_fs_set_hook(uint32_t phid, void *fshook)
{
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return -1;

   switch(phid) {
      case AL_FS_HOOK_CREATE_HANDLE:
         _al_sys_fshooks.create_handle = fshook;
         break;

      case AL_FS_HOOK_DESTROY_HANDLE:
         _al_file_fshooks.destroy_handle = fshook;
         break;

      case AL_FS_HOOK_OPEN_HANDLE:
         _al_file_fshooks.open_handle = fshook;
         break;

      case AL_FS_HOOK_CLOSE_HANDLE:
         _al_file_fshooks.close_handle = fshook;
         break;

      case AL_FS_HOOK_FOPEN:
         _al_sys_fshooks.fopen = fshook;
         break;

      case AL_FS_HOOK_FCLOSE:
         _al_file_fshooks.fclose = fshook;
         break;

      case AL_FS_HOOK_FREAD:
         _al_file_fshooks.fread = fshook;
         break;

      case AL_FS_HOOK_FWRITE:
         _al_file_fshooks.fwrite = fshook;
         break;

      case AL_FS_HOOK_FFLUSH:
         _al_file_fshooks.fflush = fshook;
         break;

      case AL_FS_HOOK_FSEEK:
         _al_file_fshooks.fseek = fshook;
         break;

      case AL_FS_HOOK_FTELL:
         _al_file_fshooks.ftell = fshook;
         break;

      case AL_FS_HOOK_FERROR:
         _al_file_fshooks.ferror = fshook;
         break;

      case AL_FS_HOOK_FEOF:
         _al_file_fshooks.feof = fshook;
         break;

      case AL_FS_HOOK_FSTAT:
         _al_file_fshooks.fstat = fshook;
         break;

      case AL_FS_HOOK_OPENDIR:
         _al_sys_fshooks.opendir = fshook;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         _al_dir_fshooks.closedir = fshook;
         break;

      case AL_FS_HOOK_READDIR:
         _al_dir_fshooks.readdir = fshook;
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

      case AL_FS_HOOK_FILE_MODE:
         _al_file_fshooks.file_mode = fshook;
         break;

      case AL_FS_HOOK_FILE_ATIME:
         _al_file_fshooks.file_atime = fshook;
         break;

      case AL_FS_HOOK_FILE_MTIME:
         _al_file_fshooks.file_mtime = fshook;
         break;

      case AL_FS_HOOK_FILE_CTIME:
         _al_file_fshooks.file_ctime = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_SIZE:
         _al_file_fshooks.file_size = fshook;
         break;

      case AL_FS_HOOK_PATH_TO_SYS:
         _al_sys_fshooks.path_to_sys = fshook;
         break;

      case AL_FS_HOOK_PATH_TO_UNI:
         _al_sys_fshooks.path_to_uni = fshook;
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
         ptr = _al_file_fshooks.destroy_handle;
         break;

      case AL_FS_HOOK_OPEN_HANDLE:
         ptr = _al_sys_fshooks.open_handle;
         break;

      case AL_FS_HOOK_CLOSE_HANDLE:
         ptr = _al_file_fshooks.close_handle;
         break;

      case AL_FS_HOOK_FOPEN:
         ptr = _al_sys_fshooks.fopen;
         break;

      case AL_FS_HOOK_FCLOSE:
         ptr = _al_file_fshooks.fclose;
         break;

      case AL_FS_HOOK_FREAD:
         ptr = _al_file_fshooks.fread;
         break;

      case AL_FS_HOOK_FWRITE:
         ptr = _al_file_fshooks.fwrite;
         break;

      case AL_FS_HOOK_FFLUSH:
         ptr = _al_file_fshooks.fflush;
         break;

      case AL_FS_HOOK_FSEEK:
         ptr = _al_file_fshooks.fseek;
         break;

      case AL_FS_HOOK_FTELL:
         ptr = _al_file_fshooks.ftell;
         break;

      case AL_FS_HOOK_FERROR:
         ptr = _al_file_fshooks.ferror;
         break;

      case AL_FS_HOOK_FEOF:
         ptr = _al_file_fshooks.feof;
         break;

      case AL_FS_HOOK_FSTAT:
         ptr = _al_file_fshooks.fstat;
         break;

      case AL_FS_HOOK_OPENDIR:
         ptr = _al_sys_fshooks.opendir;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         ptr = _al_dir_fshooks.closedir;
         break;

      case AL_FS_HOOK_READDIR:
         ptr = _al_fir_fshooks.readdir;
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

      case AL_FS_HOOK_FILE_MODE:
         ptr = _al_file_fshooks.file_mode;
         break;

      case AL_FS_HOOK_FILE_ATIME:
         ptr = _al_file_fshooks.file_atime;
         break;

      case AL_FS_HOOK_FILE_MTIME:
         ptr = _al_file_fshooks.file_mtime;
         break;

      case AL_FS_HOOK_FILE_SIZE:
         ptr = _al_file_fshooks.file_size;
         break;

      case AL_FS_HOOK_FILE_CTIME:
         ptr = _al_file_fshooks.file_ctime;
         break;

      case AL_FS_HOOK_PATH_TO_SYS:
         ptr = _al_sys_fshooks.path_to_sys;
         break;

      case AL_FS_HOOK_PATH_TO_UNI:
         ptr = _al_sys_fshooks.path_to_uni;
         break;

      default:
         ptr = NULL;
         break;
   }

   return ptr;
}

AL_FILE *al_fs_create_handle(AL_CONST char *path)
{
   AL_FILE *handle = _al_fs_hook_create_handle(path);
   if(!handle)
      return NULL;

   handle->vtable = AL_MALLOC(sizeof(struct AL_FS_HOOK_FILE_VTABLE));
   if(!handle->vtable) {
      _al_fs_hook_destroy_handle(handle);
      return NULL;
   }

   *(handle->vtable) = _al_file_fshooks;

   return handle;
}

void al_fs_destroy_handle(AL_FILE *handle)
{
   struct _al_file_fshooks *vtable = NULL;

   ASSERT(handle != NULL);
   ASSERT(handle->vtable != NULL);

   vtable = handle->vtable;

   _al_fs_hook_destroy_handle(handle);

   AL_FREE(vtable);
}

int32_t al_fs_open_handle(AL_FILE *handle, AL_CONST char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open_handle(handle, mode);
}

int32_t al_fs_close_handle(AL_FILE *handle)
{
   ASSERT(handle != NULL);

   return _al_fs_hook_close_handle(handle);
}

AL_FILE *al_fs_fopen(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_fopen(path, mode);
}

int32_t al_fs_fclose(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_fclose(fp);
}

ssize_t   al_fs_fread(void *ptr, size_t size, AL_FILE *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_fread(ptr, size, fp);
}

ssize_t   al_fs_fwrite(const void *ptr, size_t size, AL_FILE *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_fwrite(ptr, size, fp);
}

int32_t al_fs_fflush(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_fflush(fp);
}

int32_t al_fs_fseek(AL_FILE *fp, uint32_t offset, uint32_t whence)
{
   ASSERT(fp != NULL);
   ASSERT(offset >= 0);
   ASSERT(whence == AL_SEEK_SET || whence == AL_SEEK_CUR || whence == AL_SEEK_END);

   return _al_fs_hook_fseek(fp, offset, whence);
}

int32_t al_fs_ftell(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_ftell(fp);
}

int32_t al_fs_ferror(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_ferror(fp);
}

int32_t al_fs_feof(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_feof(fp);
}


int32_t al_fs_fstat(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_fstat(fp);
}


AL_DIR *al_fs_opendir(const char *path)
{
   AL_DIR *dir = NULL;

   ASSERT(path != NULL);

   dir = _al_fs_hook_opendir(path);
   if(!dir)
      return NULL;

   dir->vtable = AL_MALLOC(sizeof(struct _al_dir_fshooks));
   if(!dir->vtable) {
      _al_fs_hook_closedir(dir);
      return NULL;
   }

   *(dir->vtable) = _al_dir_fshooks;

   return dir;
}

int32_t   al_fs_closedir(AL_DIR *dir)
{
   struct _al_dir_fshooks *vtable = NULL;
   int32_t ret = 0;

   ASSERT(dir != NULL);
   ASSERT(dir->vtable != NULL);

   vtable = dir->vtable;

   ret = _al_fs_hook_closedir(dir);

   AL_FREE(vtable);

   return ret;

}

int32_t al_fs_readdir(AL_DIR *dir, size_t size, char *name)
{
   ASSERT(dir != NULL);
   ASSERT(size > 0);
   ASSERT(name != NULL);

   return _al_fs_hook_readdir(dir, size, name);
}


AL_FILE *al_fs_mktemp(const char *template, uint32_t ulink)
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

int32_t al_fs_unlink( const char *path )
{
   AL_FILE *fp = NULL;
   int32_t ret = 0;

   ASSERT(path);
   fp = al_fs_create_handle( path );
   ret = al_fs_funlink( fp );
   al_fs_destroy_handle( fp );
   return fp;
}

int32_t al_fs_funlink( AL_FILE *fp )
{
   ASSERT(fp);

   return _al_fs_hook_funlink( fp );
}

uint32_t al_fs_file_mode(AL_STAT *st)
{
   return _al_fs_hook_file_mode(st);
}

time_t   al_fs_file_atime(AL_STAT *st)
{
   return _al_fs_hook_file_atime(st);
}

time_t   al_fs_file_mtime(AL_STAT *st)
{
   return _al_fs_hook_file_mtime(st);
}

time_t   al_fs_file_ctime(AL_STAT *st)
{
   return _al_fs_hook_file_ctime(st);
}

size_t   al_fs_file_size(AL_STAT *st)
{
   ASSERT(st);

   return _al_fs_hook_file_size(st);
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

uint32_t al_fs_fexists(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_fexists(fp);
}

uint32_t al_fs_exists(AL_CONST char *path)
{
   ASSERT(path != path);

   return _al_fs_hook_file_exists(path);
}

int32_t al_fs_rmdir(AL_CONST char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_rmdir(path);
}

void al_fs_fname(AL_FILE *fp, size_t s, char *buf)
{
   ASSERT(fp != NULL);
   ASSERT(s <= 0);
   ASSERT(buf != NULL);

   return _al_fs_hook_fname(fp, s, buf);
}

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
