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

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_fshook.h"



/* Function: al_create_fs_entry
 */
ALLEGRO_FS_ENTRY *al_create_fs_entry(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->fs_create_entry);
   return vt->fs_create_entry(path);
}


/* Function: al_destroy_fs_entry
 */
void al_destroy_fs_entry(ALLEGRO_FS_ENTRY *fh)
{
   if (fh) {
      fh->vtable->fs_destroy_entry(fh);
   }
}


/* Function: al_get_fs_entry_name
 */
const char *al_get_fs_entry_name(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_name(e);
}


/* Function: al_update_fs_entry
 */
bool al_update_fs_entry(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_update_entry(e);
}


/* Function: al_get_fs_entry_mode
 */
uint32_t al_get_fs_entry_mode(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_mode(e);
}


/* Function: al_get_fs_entry_atime
 */
time_t al_get_fs_entry_atime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_atime(e);
}


/* Function: al_get_fs_entry_mtime
 */
time_t al_get_fs_entry_mtime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_mtime(e);
}


/* Function: al_get_fs_entry_ctime
 */
time_t al_get_fs_entry_ctime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_ctime(e);
}


/* Function: al_get_fs_entry_size
 */
off_t al_get_fs_entry_size(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_size(e);
}


/* Function: al_remove_fs_entry
 */
bool al_remove_fs_entry(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_remove_entry(e);
}


/* Function: al_fs_entry_exists
 */
bool al_fs_entry_exists(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_entry_exists(e);
}


/* Function: al_open_directory
 */
bool al_open_directory(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_open_directory(e);
}


/* Function: al_close_directory
 */
bool al_close_directory(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_close_directory(e);
}


/* Function: al_read_directory
 */
ALLEGRO_FS_ENTRY *al_read_directory(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return e->vtable->fs_read_directory(e);
}


/* Function: al_get_current_directory
 */
char *al_get_current_directory(void)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->fs_get_current_directory);
   return vt->fs_get_current_directory();
}


/* Function: al_change_directory
 */
bool al_change_directory(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->fs_change_directory);
   ASSERT(path);

   return vt->fs_change_directory(path);
}


/* Function: al_make_directory
 */
bool al_make_directory(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(path);
   ASSERT(vt->fs_make_directory);

   return vt->fs_make_directory(path);
}


/* Function: al_filename_exists
 */
bool al_filename_exists(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(path != NULL);
   ASSERT(vt->fs_filename_exists);

   return vt->fs_filename_exists(path);
}


/* Function: al_remove_filename
 */
bool al_remove_filename(const char *path)
{
   const ALLEGRO_FS_INTERFACE *vt = al_get_fs_interface();
   ASSERT(vt->fs_remove_filename);
   ASSERT(path != NULL);

   return vt->fs_remove_filename(path);
}


/* Function: al_open_fs_entry
 */
ALLEGRO_FILE *al_open_fs_entry(ALLEGRO_FS_ENTRY *e, const char *mode)
{
   ASSERT(e != NULL);

   if (e->vtable->fs_open_file)
      return e->vtable->fs_open_file(e, mode);

   al_set_errno(EINVAL);
   return NULL;
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
