#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_system.h"

ALLEGRO_DEBUG_CHANNEL("native_dialog")

static bool inited_addon = false;

/* Function: al_init_native_dialog_addon
 */
bool al_init_native_dialog_addon(void)
{
   if (!inited_addon) {
      if (!_al_init_native_dialog_addon()) {
         ALLEGRO_ERROR("_al_init_native_dialog_addon failed.\n");
         return false;
      }
      inited_addon = true;
      _al_add_exit_func(al_shutdown_native_dialog_addon,
         "al_shutdown_native_dialog_addon");
   }
   return true;
}

/* Function: al_shutdown_native_dialog_addon
 */
void al_shutdown_native_dialog_addon(void)
{
   if (inited_addon) {
      _al_shutdown_native_dialog_addon();
      inited_addon = false;
   }
}

/* Function: al_create_native_file_dialog
 */
ALLEGRO_FILECHOOSER *al_create_native_file_dialog(
   char const *initial_path,
   char const *title,
   char const *patterns,
   int mode)
{
   ALLEGRO_NATIVE_DIALOG *fc;
   fc = al_calloc(1, sizeof *fc);

   if (initial_path) {
      fc->fc_initial_path = al_create_path(initial_path);
   }
   fc->title = al_ustr_new(title);
   fc->fc_patterns = al_ustr_new(patterns);
   fc->flags = mode;

   _al_register_destructor(_al_dtor_list, fc,
      (void (*)(void *))al_destroy_native_file_dialog);

   return (ALLEGRO_FILECHOOSER *)fc;
}

/* Function: al_show_native_file_dialog
 */
bool al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_FILECHOOSER *dialog)
{
   ALLEGRO_NATIVE_DIALOG *fd = (ALLEGRO_NATIVE_DIALOG *)dialog;
   return _al_show_native_file_dialog(display, fd);
}

/* Function: al_get_native_file_dialog_count
 */
int al_get_native_file_dialog_count(const ALLEGRO_FILECHOOSER *dialog)
{
   const ALLEGRO_NATIVE_DIALOG *fc = (const ALLEGRO_NATIVE_DIALOG *)dialog;
   return fc->fc_path_count;
}

/* Function: al_get_native_file_dialog_path
 */
const char *al_get_native_file_dialog_path(
   const ALLEGRO_FILECHOOSER *dialog, size_t i)
{
   const ALLEGRO_NATIVE_DIALOG *fc = (const ALLEGRO_NATIVE_DIALOG *)dialog;
   if (i < fc->fc_path_count)
      return al_path_cstr(fc->fc_paths[i], ALLEGRO_NATIVE_PATH_SEP);
   return NULL;
}

/* Function: al_destroy_native_file_dialog
 */
void al_destroy_native_file_dialog(ALLEGRO_FILECHOOSER *dialog)
{
   ALLEGRO_NATIVE_DIALOG *fd = (ALLEGRO_NATIVE_DIALOG *)dialog;
   size_t i;

   if (!fd)
      return;

   _al_unregister_destructor(_al_dtor_list, fd);

   al_ustr_free(fd->title);
   al_destroy_path(fd->fc_initial_path);
   for (i = 0; i < fd->fc_path_count; i++) {
      al_destroy_path(fd->fc_paths[i]);
   }
   al_free(fd->fc_paths);
   al_ustr_free(fd->fc_patterns);
   al_free(fd);
}

/* Function: al_show_native_message_box
 */
int al_show_native_message_box(ALLEGRO_DISPLAY *display,
   char const *title, char const *heading, char const *text,
   char const *buttons, int flags)
{
   ALLEGRO_NATIVE_DIALOG *fc;
   int r;

   /* Note: the message box code cannot assume that Allegro is installed.
    * al_malloc and ustr functions are okay (with the assumption that the
    * user doesn't change the memory management functions in another thread
    * while this message box is open).
    */

   fc = al_calloc(1, sizeof *fc);

   fc->title = al_ustr_new(title);
   fc->mb_heading = al_ustr_new(heading);
   fc->mb_text = al_ustr_new(text);
   fc->mb_buttons = al_ustr_new(buttons);
   fc->flags = flags;

   r = _al_show_native_message_box(display, fc);

   al_ustr_free(fc->title);
   al_ustr_free(fc->mb_heading);
   al_ustr_free(fc->mb_text);
   al_ustr_free(fc->mb_buttons);
   al_free(fc);

   return r;
}


/* Function: al_get_allegro_native_dialog_version
 */
uint32_t al_get_allegro_native_dialog_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* vim: set sts=3 sw=3 et: */
