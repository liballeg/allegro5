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

static struct AL_FS_HOOK_VTABLE _al_fshooks;

int al_fs_set_hook(uint32_t phid, void *fshook)
{
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return -1;

   switch(phid) {
      case AL_FS_HOOK_CREATE_HANDLE:
         _al_fshooks.create_handle = fshook;
         break;

      case AL_FS_HOOK_DESTROY_HANDLE:
         _al_fshooks.destroy_handle = fshook;
         break;

      case AL_FS_HOOK_OPEN_HANDLE:
         _al_fshooks.open_handle = fshook;
         break;

      case AL_FS_HOOK_CLOSE_HANDLE:
         _al_fshooks.close_handle = fshook;
         break;

      case AL_FS_HOOK_FOPEN:
         _al_fshooks.fopen = fshook;
         break;

      case AL_FS_HOOK_FCLOSE:
         _al_fshooks.fclose = fshook;
         break;

      case AL_FS_HOOK_FREAD:
         _al_fshooks.fread = fshook;
         break;

      case AL_FS_HOOK_FWRITE:
         _al_fshooks.fwrite = fshook;
         break;

      case AL_FS_HOOK_FFLUSH:
         _al_fshooks.fflush = fshook;
         break;

      case AL_FS_HOOK_FSEEK:
         _al_fshooks.fseek = fshook;
         break;

      case AL_FS_HOOK_FTELL:
         _al_fshooks.ftell = fshook;
         break;

      case AL_FS_HOOK_FERROR:
         _al_fshooks.ferror = fshook;
         break;

      case AL_FS_HOOK_FEOF:
         _al_fshooks.feof = fshook;
         break;

      case AL_FS_HOOK_FSTAT:
         _al_fshooks.fstat = fshook;
         break;

      case AL_FS_HOOK_OPENDIR:
         _al_fshooks.opendir = fshook;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         _al_fshooks.closedir = fshook;
         break;

      case AL_FS_HOOK_READDIR:
         _al_fshooks.readdir = fshook;
         break;

      case AL_FS_HOOK_MKTEMP:
         _al_fshooks.mktemp = fshook;
         break;

      case AL_FS_HOOK_GETCWD:
         _al_fshooks.getcwd = fshook;
         break;

      case AL_FS_HOOK_CHDIR:
         _al_fshooks.chdir = fshook;
         break;

      case AL_FS_HOOK_ADD_SEARCH_PATH:
         _al_fshooks.add_search_path = fshook;
         break;

      case AL_FS_HOOK_SEARCH_PATH_COUNT:
         _al_fshooks.search_path_count = fshook;
         break;

      case AL_FS_HOOK_GET_SEARCH_PATH:
         _al_fshooks.get_search_path = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_MODE:
         _al_fshooks.get_stat_mode = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_ATIME:
         _al_fshooks.get_stat_atime = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_MTIME:
         _al_fshooks.get_stat_mtime = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_CTIME:
         _al_fshooks.get_stat_ctime = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_SIZE:
         _al_fshooks.get_stat_size = fshook;
         break;

      case AL_FS_HOOK_PATH_TO_SYS:
         _al_fshooks.path_to_sys = fshook;
         break;

      case AL_FS_HOOK_PATH_TO_UNI:
         _al_fshooks.path_to_uni = fshook;
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
         ptr = _al_fshooks.create_handle;
         break;

      case AL_FS_HOOK_DESTROY_HANDLE:
         ptr = _al_fshooks.destroy_handle;
         break;

      case AL_FS_HOOK_OPEN_HANDLE:
         ptr = _al_fshooks.open_handle;
         break;

      case AL_FS_HOOK_CLOSE_HANDLE:
         ptr = _al_fshooks.close_handle;
         break;

      case AL_FS_HOOK_FOPEN:
         ptr = _al_fshooks.fopen;
         break;

      case AL_FS_HOOK_FCLOSE:
         ptr = _al_fshooks.fclose;
         break;

      case AL_FS_HOOK_FREAD:
         ptr = _al_fshooks.fread;
         break;

      case AL_FS_HOOK_FWRITE:
         ptr = _al_fshooks.fwrite;
         break;

      case AL_FS_HOOK_FFLUSH:
         ptr = _al_fshooks.fflush;
         break;

      case AL_FS_HOOK_FSEEK:
         ptr = _al_fshooks.fseek;
         break;

      case AL_FS_HOOK_FTELL:
         ptr = _al_fshooks.ftell;
         break;

      case AL_FS_HOOK_FERROR:
         ptr = _al_fshooks.ferror;
         break;

      case AL_FS_HOOK_FEOF:
         ptr = _al_fshooks.feof;
         break;

      case AL_FS_HOOK_FSTAT:
         ptr = _al_fshooks.fstat;
         break;

      case AL_FS_HOOK_OPENDIR:
         ptr = _al_fshooks.opendir;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         ptr = _al_fshooks.closedir;
         break;

      case AL_FS_HOOK_READDIR:
         ptr = _al_fshooks.readdir;
         break;

      case AL_FS_HOOK_MKTEMP:
         ptr = _al_fshooks.mktemp;
         break;

      case AL_FS_HOOK_GETCWD:
         ptr = _al_fshooks.getcwd;
         break;

      case AL_FS_HOOK_CHDIR:
         ptr = _al_fshooks.chdir;
         break;

      case AL_FS_HOOK_GETDIR:
         ptr = _al_fshooks.getdir;
         break;

      case AL_FS_HOOK_ADD_SEARCH_PATH:
         ptr = _al_fshooks.add_search_path;
         break;

      case AL_FS_HOOK_SEARCH_PATH_COUNT:
         ptr = _al_fshooks.search_path_count;
         break;

      case AL_FS_HOOK_GET_SEARCH_PATH:
         ptr = _al_fshooks.get_search_path;
         break;

      case AL_FS_HOOK_GET_STAT_MODE:
         ptr = _al_fshooks.get_stat_mode;
         break;

      case AL_FS_HOOK_GET_STAT_ATIME:
         ptr = _al_fshooks.get_stat_atime;
         break;

      case AL_FS_HOOK_GET_STAT_MTIME:
         ptr = _al_fshooks.get_stat_mtime;
         break;

      case AL_FS_HOOK_GET_STAT_SIZE:
         ptr = _al_fshooks.get_stat_size;
         break;

      case AL_FS_HOOK_GET_STAT_CTIME:
         ptr = _al_fshooks.get_stat_ctime;
         break;

      case AL_FS_HOOK_PATH_TO_SYS:
         ptr = _al_fshooks.path_to_sys;
         break;

      case AL_FS_HOOK_PATH_TO_UNI:
         ptr = _al_fshooks.path_to_uni;
         break;

      default:
         ptr = NULL;
         break;
   }

   return ptr;
}

/* NULL is a valid value for path */
AL_FILE *al_fs_create_handle(AL_CONST char *path)
{
   return _al_fs_hook_create_handle(path);
}

void al_fs_destroy_handle(AL_FILE *handle)
{
   _al_fs_hook_destroy_handle(handle);
}

int32_t al_fs_open_handle(AL_FILE *handle, AL_CONST char *mode)
{
   ASSERT(handle);
   ASSERT(mode);

   return _al_fs_hook_open_handle(handle, mode);
}

int32_t al_fs_close_handle(AL_FILE *handle)
{
   ASSERT(handle);

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


AL_DIR    *al_fs_opendir(const char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_opendir(path);
}

int32_t   al_fs_closedir(AL_DIR *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_closedir(dir);
}

AL_DIRENT *al_fs_readdir(AL_DIR *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_readdir(dir);
}


AL_FILE *al_fs_mktemp(const char *template)
{
   ASSERT(template != NULL);

   return _al_fs_hook_mktemp(template);
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

uint32_t al_fs_get_stat_mode(AL_STAT *st)
{
   return _al_fs_hook_get_stat_mode(st);
}

time_t   al_fs_get_stat_atime(AL_STAT *st)
{
   return _al_fs_hook_get_stat_atime(st);
}

time_t   al_fs_get_stat_mtime(AL_STAT *st)
{
   return _al_fs_hook_get_stat_mtime(st);
}

time_t   al_fs_get_stat_ctime(AL_STAT *st)
{
   return _al_fs_hook_get_stat_ctime(st);
}

size_t   al_fs_get_stat_size(AL_STAT *st)
{
   ASSERT(st);

   return _al_fs_hook_get_stat_size(st);
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

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
