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

/* Title: Filesystem routines
*/

#include "allegro5/allegro5.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/path.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_memory.h"


struct ALLEGRO_FS_HOOK_SYS_INTERFACE *_al_sys_fshooks = &_al_stdio_sys_fshooks;
struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks = &_al_stdio_entry_fshooks;


/* Function: al_create_entry
 */
ALLEGRO_FS_ENTRY *al_create_entry(const char *path)
{
   return _al_fs_hook_create(path);
}


/* Function: al_destroy_entry
 */
void al_destroy_entry(ALLEGRO_FS_ENTRY *handle)
{
   if (handle) {
      _al_fs_hook_destroy(handle);
   }
}


/* Function: al_open_entry
 */
bool al_open_entry(ALLEGRO_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);

   return _al_fs_hook_open(handle);
}


/* Function: al_close_entry
 */
void al_close_entry(ALLEGRO_FS_ENTRY *handle)
{
   if (handle) {
      _al_fs_hook_close(handle);
   }
}


/* Function: al_get_entry_name
 */
ALLEGRO_PATH *al_get_entry_name(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_name(fp);
}


/* Function: al_fstat
 */
bool al_fstat(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_stat(fp);
}


/* Function: al_opendir
 */
ALLEGRO_FS_ENTRY *al_opendir(const char *path)
{
   ALLEGRO_FS_ENTRY *dir = NULL;

   ASSERT(path != NULL);

   dir = _al_fs_hook_opendir(path);
   if (!dir)
      return NULL;

   return dir;
}


/* Function: al_closedir
 */
bool al_closedir(ALLEGRO_FS_ENTRY *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_closedir(dir);
}


/* Function: al_readdir
 */
ALLEGRO_FS_ENTRY *al_readdir(ALLEGRO_FS_ENTRY *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_readdir(dir);
}


/* Function: al_get_entry_mode
 */
uint32_t al_get_entry_mode(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mode(e);
}


/* Function: al_get_entry_atime
 */
time_t al_get_entry_atime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_atime(e);
}


/* Function: al_get_entry_mtime
 */
time_t al_get_entry_mtime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mtime(e);
}


/* Function: al_get_entry_ctime
 */
time_t al_get_entry_ctime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_ctime(e);
}


/* Function: al_get_entry_size
 */
off_t al_get_entry_size(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_size(e);
}


/* Function: al_remove_entry
 */
bool al_remove_entry(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_remove(e);
}


/* Function: al_is_present
 */
bool al_is_present(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_exists(e);
}


/* Function: al_is_directory
 */
bool al_is_directory(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_get_entry_mode(e) & ALLEGRO_FILEMODE_ISDIR;
}


/* Function: al_is_file
 */
bool al_is_file(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_get_entry_mode(e) & ALLEGRO_FILEMODE_ISFILE;
}


/* Function: al_getcwd
 */
ALLEGRO_PATH *al_getcwd(void)
{
   return _al_fs_hook_getcwd();
}


/* Function: al_chdir
 */
bool al_chdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_chdir(path);
}


/* Function: al_mkdir
 */
bool al_mkdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_mkdir(path);
}


/* Function: al_drive_sep
 */
int32_t al_drive_sep(char *sep, size_t len)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(sep, len);
}


/* Function: al_path_sep
 */
int32_t al_path_sep(char *sep, size_t len)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(sep, len);
}


/* Function: al_remove_str
 */
bool al_remove_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_remove(path);
}


/* Function: al_is_present_str
 */
bool al_is_present_str(const char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_exists(path);
}




/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
