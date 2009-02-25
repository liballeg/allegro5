#include "allegro5/allegro5.h"
#include "allegro5/a5_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_memory.h"

/* Function: al_create_native_file_dialog
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

