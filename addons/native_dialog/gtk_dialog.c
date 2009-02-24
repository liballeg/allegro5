/* Each of these files implements the same, for different GUI toolkits:
 * 
 * gtk_dialog.c - GTK file open dialog
 * osx_dialog.m - OSX file open dialog
 * qt_dialog.cpp  - Qt file open dialog
 * win_dialog.c - Windows file open dialog
 * 
 */
#include <gtk/gtk.h>

#include "allegro5/allegro5.h"
#include "allegro5/a5_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_memory.h"

struct ALLEGRO_NATIVE_FILE_DIALOG
{
   ALLEGRO_PATH *initial_path;
   ALLEGRO_USTR *title;
   ALLEGRO_USTR *patterns;
   int mode;
   size_t count;
   ALLEGRO_PATH **pathes;
};

static void destroy(GtkWidget *w, gpointer data)
{
   (void)data;
   (void)w;
   gtk_main_quit();
}

static void ok(GtkWidget *w, GtkFileSelection *fs)
{
   ALLEGRO_NATIVE_FILE_DIALOG *fc;
   fc = g_object_get_data(G_OBJECT(w), "ALLEGRO_NATIVE_FILE_CHOOSER");
   gchar **pathes = gtk_file_selection_get_selections(fs);
   int n = 0, i;
   while (pathes[n]) {
      n++;
   }
   fc->count = n;
   fc->pathes = _AL_MALLOC(n * sizeof(void *));
   for (i = 0; i < n; i++)
      fc->pathes[i] = al_path_create(pathes[i]);
   g_strfreev(pathes);
}

/* Function: al_show_native_file_dialog
 * 
 * This functions completely blocks the calling thread until it returns,
 * so usually you may want to spawn a thread with [al_create_thread] and
 * call it from inside that thread.
 */
void al_show_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fd)
{
   int argc = 0;
   char **argv = NULL;
   GtkWidget *window;
   gtk_init_check(&argc, &argv);

   /* Create a new file selection widget */
   window = gtk_file_selection_new(al_cstr(fd->title));

   /* Connect the destroy signal */
   g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

   /* Connect the ok_button */
   g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
                    "clicked", G_CALLBACK(ok), (gpointer) window);

   /* Connect both buttons to gtk_widget_destroy */
   g_signal_connect_swapped(GTK_FILE_SELECTION(window)->ok_button,
                            "clicked", G_CALLBACK(gtk_widget_destroy), window);
   g_signal_connect_swapped(GTK_FILE_SELECTION(window)->cancel_button,
                            "clicked", G_CALLBACK(gtk_widget_destroy), window);

   g_object_set_data(G_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
      "ALLEGRO_NATIVE_FILE_CHOOSER", fd);

   if (fd->initial_path) {
      char name[PATH_MAX];
      al_path_to_string(fd->initial_path, name,
                        sizeof name, '/');
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(window), name);
   }

   if (fd->mode & ALLEGRO_FILECHOOSER_MULTIPLE)
      gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(window), true);

   gtk_widget_show(window);
   gtk_main();
}

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
int al_get_native_file_dialog_count(ALLEGRO_NATIVE_FILE_DIALOG *fc)
{
   return fc->count;
}

/* al_get_native_file_dialog_path
 */
ALLEGRO_PATH *al_get_native_file_dialog_path(
   ALLEGRO_NATIVE_FILE_DIALOG *fc, size_t i)
{
   if (i < fc->count)
      return fc->pathes[i];
   return NULL;
}

/* al_destroy_native_file_dialog
 */
void al_destroy_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fd)
{
   if (fd->pathes) {
      int i;
      for (i = 0; i < fd->count; i++) {
         al_path_free(fd->pathes[i]);
      }
   }
   _AL_FREE(fd->pathes);
   if (fd->initial_path)
      al_path_free(fd->initial_path);
   al_ustr_free(fd->title);
   al_ustr_free(fd->patterns);
   _AL_FREE(fd);
}
