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

struct AL_FS_HOOK_VTABLE _al_fshooks;

int al_fs_set_hook(uint32_t phid, void *fshook)
{
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return -1;

   switch(phid) {
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

      case AL_FS_HOOK_GETDIR:
         _al_fshooks.getdir = fshook;
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
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return NULL;

   switch(phid) {
      case AL_FS_HOOK_FOPEN:
         return _al_fshooks.fopen;

      case AL_FS_HOOK_FCLOSE:
         return _al_fshooks.fclose;

      case AL_FS_HOOK_FREAD:
         return _al_fshooks.fread;

      case AL_FS_HOOK_FWRITE:
         return _al_fshooks.fwrite;

      case AL_FS_HOOK_FFLUSH:
         return _al_fshooks.fflush;

      case AL_FS_HOOK_FSEEK:
         return _al_fshooks.fseek;

      case AL_FS_HOOK_FTELL:
         return _al_fshooks.ftell;

      case AL_FS_HOOK_FERROR:
         return _al_fshooks.ferror;

      case AL_FS_HOOK_FEOF:
         return _al_fshooks.feof;

      case AL_FS_HOOK_FSTAT:
         return _al_fshooks.fstat;

      case AL_FS_HOOK_OPENDIR:
         return _al_fshooks.opendir;

      case AL_FS_HOOK_CLOSEDIR:
         return _al_fshooks.closedir;

      case AL_FS_HOOK_READDIR:
         return _al_fshooks.readdir;

      case AL_FS_HOOK_MKTEMP:
         return _al_fshooks.mktemp;

      case AL_FS_HOOK_GETCWD:
         return _al_fshooks.getcwd;

      case AL_FS_HOOK_CHDIR:
         return _al_fshooks.chdir;

      case AL_FS_HOOK_GETDIR:
         return _al_fshooks.getdir;

      case AL_FS_HOOK_ADD_SEARCH_PATH:
         return _al_fshooks.add_search_path;

      case AL_FS_HOOK_SEARCH_PATH_COUNT:
         return _al_fshooks.search_path_count;

      case AL_FS_HOOK_GET_SEARCH_PATH:
         return _al_fshooks.get_search_path;

      case AL_FS_HOOK_GET_STAT_MODE:
         return _al_fshooks.get_stat_mode;

      case AL_FS_HOOK_GET_STAT_ATIME:
         return _al_fshooks.get_stat_atime;

      case AL_FS_HOOK_GET_STAT_MTIME:
         return _al_fshooks.get_stat_mtime;

      case AL_FS_HOOK_GET_STAT_CTIME:
         return _al_fshooks.get_stat_ctime;

      case AL_FS_HOOK_PATH_TO_SYS:
         return _al_fshooks.path_to_sys;

      case AL_FS_HOOK_PATH_TO_UNI:
         return _al_fshooks.path_to_uni;

      default:
         return NULL;
   }

   return NULL;
}

AL_FILE *al_fs_fopen(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_fopen(path, mode);
}

uint32_t al_fs_fclose(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_fclose(fp);
}

size_t   al_fs_fread(void *ptr, size_t size, AL_FILE *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_fread(ptr, size, fp);
}

size_t   al_fs_fwrite(const void *ptr, size_t size, AL_FILE *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_fwrite(ptr, size, fp);
}

uint32_t al_fs_fflush(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_fflush(fp);
}

uint32_t al_fs_fseek(AL_FILE *fp, uint32_t offset, uint32_t whence)
{
   ASSERT(fp != NULL);
   ASSERT(offset >= 0);
   ASSERT(whence != AL_SEEK_SET && whence != AL_SEEK_CUR && whence != AL_SEEK_END);

   return _al_fs_hook_fseek(fp, offset, whence);
}

uint32_t al_fs_ftell(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_ftell(fp);
}

uint32_t al_fs_ferror(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_ferror(fp);
}

uint32_t al_fs_feof(AL_FILE *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_feof(fp);
}


uint32_t al_fs_fstat(const char *path, AL_STAT *stbuf)
{
   ASSERT(path != NULL);
   ASSERT(stbuf != NULL);

   return _al_fs_hook_fstat(path, stbuf);
}


AL_DIR    *al_fs_opendir(const char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_opendir(path);
}

uint32_t   al_fs_closedir(AL_DIR *dir)
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

uint32_t al_fs_getcwd(char *buf, size_t *len)
{
   ASSERT(buf != NULL);
   ASSERT(len != NULL);

   return _al_fs_hook_getcwd(buf, len);
}

uint32_t al_fs_chdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_chdir(path);
}

uint32_t al_fs_getdir(uint32_t id, char *dir, uint32_t *len)
{
   ASSERT(id < 0 || id > AL_DIR_LAST);
   ASSERT(dir);
   ASSERT(len);

   return _al_fs_hook_getdir(id, dir, len);
}


uint32_t al_fs_add_search_path(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_add_search_path(path);
}

uint32_t al_fs_search_path_count()
{
   return _al_fs_hook_search_path_count();
}

uint32_t al_fs_get_search_path(char *dest, uint32_t *len)
{
   ASSERT(dest);
   ASSERT(len);

   return _al_fs_hook_get_search_path(dest, len);
}


uint32_t al_fs_get_stat_mode()
{
   return _al_fs_hook_get_stat_mode();
}

time_t   al_fs_get_stat_atime()
{
   return _al_fs_hook_get_stat_atime();
}

time_t   al_fs_get_stat_mtime()
{
   return _al_fs_hook_get_stat_mtime();
}

time_t   al_fs_get_stat_ctime()
{
   return _al_fs_hook_get_stat_ctime();
}

uint32_t al_fs_path_to_sys(const char *orig, uint32_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_sys(orig, len, path);
}

uint32_t al_fs_path_to_uni(const char *orig, uint32_t len, char *path)
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
