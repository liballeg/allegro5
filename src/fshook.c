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



/* Function: al_create_entry
 */
ALLEGRO_FS_ENTRY *al_create_entry(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->create);
   return vt->create(path);
}


/* Function: al_destroy_entry
 */
void al_destroy_entry(ALLEGRO_FS_ENTRY *handle)
{
   if (handle) {
      _al_fs_hook_destroy(handle);
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
bool al_opendir(ALLEGRO_FS_ENTRY *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_opendir(dir);
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
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->getcwd);
   return vt->getcwd();
}


/* Function: al_chdir
 */
bool al_chdir(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->chdir);
   ASSERT(path);

   return vt->chdir(path);
}


/* Function: al_mkdir
 */
bool al_mkdir(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(path);
   ASSERT(vt->mkdir);

   return vt->mkdir(path);
}


/* Function: al_remove_str
 */
bool al_remove_str(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->remove);
   ASSERT(path != NULL);
   
   return vt->remove_str(path);
}


/* Function: al_is_present_str
 */
bool al_is_present_str(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(path != NULL);
   ASSERT(vt->exists);

   return vt->exists_str(path);
}




/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
