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

#define AL_FS_HOOK_NONE AL_ID('N','O','N','E')

struct AL_FS_HOOK_VTABLE {
   uint32_t id;
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

/*
   AL_FS_HOOK_MKSTMP,
   AL_FS_HOOK_GETCWD,
   AL_FS_HOOK_CHDIR,
   AL_FS_HOOK_GETDIR,
   AL_FS_HOOK_ADD_SEARCH_PATH,
   AL_FS_HOOK_SEARCH_PATH_COUNT,
   AL_FS_HOOK_GET_SEARCH_PATH,

   AL_FS_HOOK_GET_STAT_MODE,
   AL_FS_HOOK_GET_STAT_ATIME,
   AL_FS_HOOK_GET_STAT_MTIME,
   AL_FS_HOOK_GET_STAT_CTIME,
*/

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

/* for you freaks running vim/emacs. */
/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
