#include "allegro5/allegro5.h"
#include "allegro5/a5_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_memory.h"

/* Function: al_create_native_file_dialog
 * 
 * Creates a new native file dialog.
 * 
 * Parameters:
 * - initial_path: The initial search path and filename. Can be NULL.
 * - title: Title of the dialog.
 * - patterns: A list of semi-colon separated patterns to match. You
 *   should always include the pattern "*.*" as usually the MIME type
 *   and not the file pattern is relevant. If no file patterns are
 *   supported by the native dialog, this parameter is ignored.
 * - mode: 0, or a combination of the flags below.
 * 
 * Possible flags for the 'mode' parameter are:
 * 
 * - ALLEGRO_FILECHOOSER_FILE_MUST_EXIST: If supported by the native dialog,
 *   it will not allow entering new names, but just allow existing files
 *   to be selected. Else it is ignored.
 * - ALLEGRO_FILECHOOSER_SAVE: If the native dialog system has a
 *   different dialog for saving (for example one which allows creating
 *   new directories), it is used. Else ignored.
 * - ALLEGRO_FILECHOOSER_FOLDER: If there is support for a separate
 *   dialog to select a folder instead of a file, it will be used.
 * - ALLEGRO_FILECHOOSER_PICTURES: If a different dialog is available
 *   for selecting pictures, it is used. Else ignored.
 * - ALLEGRO_FILECHOOSER_SHOW_HIDDEN: If the platform supports it, also
 *   hidden files will be shown.
 * - ALLEGRO_FILECHOOSER_MULTIPLE: If supported, allow selecting
 *   multiple files.
 *
 * Returns:
 * A handle to the dialog from which you can query the results. When you
 * are done, call [al_destroy_native_file_dialog] on it.
 */
ALLEGRO_NATIVE_FILE_DIALOG *al_create_native_file_dialog(
    ALLEGRO_PATH const *initial_path,
    char const *title,
    char const *patterns,
    int mode)
{
   ALLEGRO_NATIVE_FILE_DIALOG *fc;
   fc = _AL_MALLOC(sizeof *fc);
   memset(fc, 0, sizeof *fc);

   if (initial_path)
      fc->initial_path = al_path_clone(initial_path);
   fc->title = al_ustr_new(title);
   fc->patterns = al_ustr_new(patterns);
   fc->mode = mode;

   return fc;
}

/* al_get_native_file_dialog_count
 */
int al_get_native_file_dialog_count(const ALLEGRO_NATIVE_FILE_DIALOG *fc)
{
   return fc->count;
}

/* al_get_native_file_dialog_path
 */
const ALLEGRO_PATH *al_get_native_file_dialog_path(
   const ALLEGRO_NATIVE_FILE_DIALOG *fc, size_t i)
{
   if (i < fc->count)
      return fc->paths[i];
   return NULL;
}

/* al_destroy_native_file_dialog
 */
void al_destroy_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fd)
{
   if (!fd)
      return;

   if (fd->paths) {
      size_t i;
      for (i = 0; i < fd->count; i++) {
         al_path_free(fd->paths[i]);
      }
   }
   _AL_FREE(fd->paths);
   if (fd->initial_path)
      al_path_free(fd->initial_path);
   al_ustr_free(fd->title);
   al_ustr_free(fd->patterns);
   _AL_FREE(fd);
}

