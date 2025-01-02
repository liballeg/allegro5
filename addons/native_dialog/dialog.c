#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_vector.h"

ALLEGRO_DEBUG_CHANNEL("native_dialog")

static bool inited_addon = false;

static _AL_VECTOR make_patterns_and_desc_vec(const ALLEGRO_USTR* patterns_ustr);
static void free_patterns_and_desc_vec(_AL_VECTOR *patterns_and_desc_vec);

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
      al_init_user_event_source(al_get_default_menu_event_source());
      _al_add_exit_func(al_shutdown_native_dialog_addon,
         "al_shutdown_native_dialog_addon");
   }
   return true;
}

/* Function: al_is_native_dialog_addon_initialized
 */
bool al_is_native_dialog_addon_initialized(void)
{
   return inited_addon;
}

/* Function: al_shutdown_native_dialog_addon
 */
void al_shutdown_native_dialog_addon(void)
{
   if (inited_addon) {
      al_destroy_user_event_source(al_get_default_menu_event_source());
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
   fc->fc_patterns_ustr = al_ustr_new(patterns);
   fc->fc_patterns = make_patterns_and_desc_vec(fc->fc_patterns_ustr);
   fc->flags = mode;

   fc->dtor_item = _al_register_destructor(_al_dtor_list, "native_dialog", fc,
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

   _al_unregister_destructor(_al_dtor_list, fd->dtor_item);

   al_ustr_free(fd->title);
   al_destroy_path(fd->fc_initial_path);
   for (i = 0; i < fd->fc_path_count; i++) {
      al_destroy_path(fd->fc_paths[i]);
   }
   al_free(fd->fc_paths);
   free_patterns_and_desc_vec(&fd->fc_patterns);
   al_ustr_free(fd->fc_patterns_ustr);
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

   ASSERT(title);
   ASSERT(heading);
   ASSERT(text);

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


static _AL_VECTOR split_patterns(const ALLEGRO_USTR* ustr)
{
   int pattern_start = 0;
   int cur_pos = 0;
   bool is_mime = false;
   bool is_catchall = true;
   _AL_VECTOR patterns_vec;
   _al_vector_init(&patterns_vec, sizeof(_AL_PATTERN));
   /* This does a straightforward split on semicolons. We check for MIME types
    * and catchalls, as some backends care about this.
    */
   while (true) {
      int32_t c = al_ustr_get(ustr, cur_pos);
      if (c == -1 || c == ';') {
         ALLEGRO_USTR_INFO info;
         const ALLEGRO_USTR *pattern_ustr = al_ref_buffer(&info,
               al_cstr(ustr) + pattern_start, cur_pos - pattern_start);
         if (al_ustr_length(pattern_ustr) > 0) {
            _AL_PATTERN pattern = {
               .info = info, .is_mime = is_mime, .is_catchall = is_catchall
            };
           *((_AL_PATTERN*)_al_vector_alloc_back(&patterns_vec)) = pattern;
         }

         is_mime = false;
         is_catchall = true;
         pattern_start = cur_pos + 1;
      }
      else if (c == '/') {
         is_mime = true;
         is_catchall = false;
      }
      else if (c != '*' && c != '.') {
         is_catchall = false;
      }
      if (c == -1) {
         break;
      }
      al_ustr_next(ustr, &cur_pos);
   }
   return patterns_vec;
}


static _AL_VECTOR make_patterns_and_desc_vec(const ALLEGRO_USTR* patterns_ustr)
{
   _AL_VECTOR patterns_and_desc_vec;
   _al_vector_init(&patterns_and_desc_vec, sizeof(_AL_PATTERNS_AND_DESC));

   if (al_ustr_length(patterns_ustr) == 0)
      return patterns_and_desc_vec;

   int line_start = 0;
   int chunk_start = 0;
   int cur_pos = 0;
   /* Split by newlines + spaces simultaneously. Chunks are separated by
    * spaces, and the final chunk is interpreted as the actual patterns, while
    * the rest of the line is the "description".
    */
   while (true) {
      int32_t c = al_ustr_get(patterns_ustr, cur_pos);
      if (c == ' ') {
         chunk_start = cur_pos + 1;
      }
      else if (c == '\n' || c == -1) {
         ALLEGRO_USTR_INFO desc_info, real_patterns_info;
         const ALLEGRO_USTR *ustr;
         /* Strip trailing whitespace. */
         int desc_end = chunk_start - 1;
         for (; desc_end >= line_start; desc_end--) {
            if (al_ustr_get(patterns_ustr, desc_end) != ' ')
               break;
         }
         al_ref_ustr(&desc_info, patterns_ustr, line_start, desc_end + 1);
         ustr = al_ref_ustr(&real_patterns_info, patterns_ustr, chunk_start, cur_pos);

         _AL_VECTOR patterns_vec = split_patterns(ustr);
         if (_al_vector_size(&patterns_vec) > 0) {
            _AL_PATTERNS_AND_DESC patterns_and_desc = {
               .desc = desc_info,
               .patterns_vec = patterns_vec
            };
            *((_AL_PATTERNS_AND_DESC*)_al_vector_alloc_back(&patterns_and_desc_vec)) = patterns_and_desc;
         }

         chunk_start = cur_pos + 1;
         line_start = cur_pos + 1;
      }
      if (c == -1) {
         break;
      }
      al_ustr_next(patterns_ustr, &cur_pos);
   }

   return patterns_and_desc_vec;
}


static void free_patterns_and_desc_vec(_AL_VECTOR *patterns_and_desc_vec)
{
   for (size_t i = 0; i < _al_vector_size(patterns_and_desc_vec); i++) {
      _AL_PATTERNS_AND_DESC *patterns_and_desc =
         (_AL_PATTERNS_AND_DESC*)_al_vector_ref(patterns_and_desc_vec, i);
      _al_vector_free(&patterns_and_desc->patterns_vec);
   }
   _al_vector_free(patterns_and_desc_vec);
}

/* vim: set sts=3 sw=3 et: */
