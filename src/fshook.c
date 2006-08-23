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
#include "allegro/internal/fshook.h"
#include ALLEGRO_INTERNAL_HEADER

/* option one */
typedef void (*AL_FS_HOOK)();
typedef void AL_FS_HOOKS[AL_FS_HOOK_LAST];

/* option two */
#if 0
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
   AL_METHOD(uint32_t, get_stat_mode,  (void) );
   AL_METHOD(time_t,   get_stat_atime, (void) );
   AL_METHOD(time_t,   get_stat_mtime, (void) );
   AL_METHOD(time_t,   get_stat_ctime, (void) );
};
#endif

int al_fs_set_hook(uint32_t phid, void *fshook)
{
   if(phid < 0 || phid >= AL_FS_HOOK_LAST)
      return -1;

   /* option one */
   /* I (Thomas Fjellstrom), prefer this option. */
   system->fshooks[phid] = fshook;

#if 0
   /* option two */
   switch(phid) {
      case AL_FS_HOOK_FOPEN:
         system->fshooks->fopen = fshook;
         break;

      case AL_FS_HOOK_FCLOSE:
         system->fshooks->fclose = fshook;
         break;

      case AL_FS_HOOK_FREAD:
         system->fshooks->fread = fshook;
         break;

      case AL_FS_HOOK_FWRITE:
         system->fshooks->fwrite = fshook;
         break;

      case AL_FS_HOOK_FFLUSH:
         system->fshooks->fflush = fshook;
         break;

      case AL_FS_HOOK_FSEEK:
         system->fshooks->fseek = fshook;
         break;

      case AL_FS_HOOK_FTELL:
         system->fshooks->ftell = fshook;
         break;

      case AL_FS_HOOK_FERROR:
         system->fshooks->ferror = fshook;
         break;

      case AL_FS_HOOK_FEOF:
         system->fshooks->feof = fshook;
         break;


      case AL_FS_HOOK_FSTAT:
         system->fshooks->fstat = fshook;
         break;


      case AL_FS_HOOK_OPENDIR:
         system->fshooks->opendir = fshook;
         break;

      case AL_FS_HOOK_CLOSEDIR:
         system->fshooks->closedir = fshook;
         break;

      case AL_FS_HOOK_READDIR:
         system->fshooks->readdir = fshook;
         break;


      case AL_FS_HOOK_MKTEMP:
         system->fshooks->mktemp = fshook;
         break;

      case AL_FS_HOOK_GETCWD:
         system->fshooks->getcwd = fshook;
         break;

      case AL_FS_HOOK_CHDIR:
         system->fshooks->chdir = fshook;
         break;

      case AL_FS_HOOK_GETDIR:
         system->fshooks->getdir = fshook;
         break;

      case AL_FS_HOOK_ADD_SEARCH_PATH:
         system->fshooks->add_search_path = fshook;
         break;

      case AL_FS_HOOK_SEARCH_PATH_COUNT:
         system->fshooks->search_path_count = fshook;
         break;

      case AL_FS_HOOK_GET_SEARCH_PATH:
         system->fshooks->get_search_path = fshook;
         break;


      case AL_FS_HOOK_GET_STAT_MODE:
         system->fshooks->get_stat_mode = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_ATIME:
         system->fshooks->get_stat_atime = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_MTIME:
         system->fshooks->get_stat_mtime = fshook;
         break;

      case AL_FS_HOOK_GET_STAT_CTIME:
         system->fshooks->get_stat_ctime = fshook;
         break;
   }
#endif

}

void *al_fs_get_hook(uint32_t phid);

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
