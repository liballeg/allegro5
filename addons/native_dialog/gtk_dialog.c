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

typedef struct
{
   ALLEGRO_EVENT_SOURCE *event_source;
   ALLEGRO_THREAD *thread;
   ALLEGRO_PATH *initial_path;
   ALLEGRO_PATH **pathes;
   int n;
   char *patterns;
   int mode;
   GtkWidget *window;
} ALLEGRO_NATIVE_FILE_CHOOSER;

static void destroy(GtkWidget *w, gpointer data)
{
   (void)data;
   (void)w;
   gtk_main_quit();
}

static void ok(GtkWidget *w, GtkFileSelection *fs)
{
   ALLEGRO_NATIVE_FILE_CHOOSER *fc;
   fc = g_object_get_data(G_OBJECT(w), "ALLEGRO_NATIVE_FILE_CHOOSER");
   gchar **pathes = gtk_file_selection_get_selections(fs);
   int n = 0, i;
   while (pathes[n]) {
      n++;
   }
   fc->n = n;
   fc->pathes = _AL_MALLOC(n * sizeof(void *));
   for (i = 0; i < n; i++)
      fc->pathes[i] = al_path_create(pathes[i]);
}

/* Because of the way Allegro5's event handling works, we cannot hook
 * our own event handler in there. Therefore we spawn an extra thread
 * and simply use the standard GTK main loop.
 * 
 * If A5 would allow hooking extra system event handlers, this could
 * be done without an extra thread - we simply would add a callback
 * which would be called by A5 whenever the user waits for new events,
 * and do GTK event handling in there.
 * 
 * Running GTK from an extra thread works just fine though.
 */
static void *gtk_thread(ALLEGRO_THREAD *thread, void *arg)
{
   (void)thread;
   int argc = 0;
   char **argv = NULL;
   GtkWidget *window;
   gtk_init_check(&argc, &argv);

   /* Create a new file selection widget */
   window = gtk_file_selection_new("File selection");
   
   ALLEGRO_NATIVE_FILE_CHOOSER *fc = arg;
   fc->window = window;

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
      "ALLEGRO_NATIVE_FILE_CHOOSER", fc);

   if (fc->initial_path) {
      char name[PATH_MAX];
      al_path_to_string(fc->initial_path, name,
                        sizeof name, '/');
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(window), name);
   }

   if (fc->mode & ALLEGRO_FILECHOOSER_MULTIPLE)
      gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(window), true);

   gtk_widget_show(window);
   gtk_main();

   ALLEGRO_EVENT event;
   event.user.type = ALLEGRO_EVENT_NATIVE_FILE_CHOOSER;
   event.user.data1 = (intptr_t)fc;
   al_emit_user_event(fc->event_source, &event, NULL);

   return NULL;
}

/* Function: al_spawn_native_file_dialog
 * 
 * Parameters:
 * - queue: Event queue which gets the cancel/ok event.
 * - initial_path: The initial search path and filename. Can be NULL.
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
 * False if no dialog could be shown.
 */
ALLEGRO_EVENT_SOURCE *al_spawn_native_file_dialog(
    ALLEGRO_PATH const *initial_path, char const *patterns, int mode)
{
   ALLEGRO_NATIVE_FILE_CHOOSER *fc;
   fc = _AL_MALLOC(sizeof *fc);
   memset(fc, 0, sizeof *fc);
   fc->event_source = al_create_user_event_source();
   fc->mode = mode;
   if (initial_path)
      fc->initial_path = al_path_clone(initial_path);
   if (patterns)
      fc->patterns = strdup(patterns);
   fc->mode = mode;

   fc->thread = al_create_thread(gtk_thread, fc);
   al_start_thread(fc->thread);
   return fc->event_source;
}

/* Function: al_finalize_native_file_dialog
 * 
 * Parameters:
 * - queue: Pass the same queue you passed when spawning the dialog.
 * - event: The notification event.
 * - pathes: An array of pathes which will receive the selection.
 * - n: Maximum number of pathes to retrieve. Usually you will use 1
 *   here, unless the ALLEGRO_FILECHOOSER_MULTIPLE flags was used.
 * 
 * Returns:
 * 0 if the dialog was cancelled by the user, else the number of
 * stored pathes (at most n).
 */
int al_finalize_native_file_dialog(ALLEGRO_EVENT *event,
    ALLEGRO_PATH * pathes[], int n)
{
   int count = 0;
   ASSERT(event->user.type == ALLEGRO_EVENT_NATIVE_FILE_CHOOSER);

   ALLEGRO_NATIVE_FILE_CHOOSER *fc = (void *)event->user.data1;

   if (fc->pathes) {
      int i;
      for (i = 0; i < fc->n; i++) {
         if (i < n) {
            count++;
            pathes[i] = fc->pathes[i];
         }
         else
            al_path_free(fc->pathes[i]);
      }
   }

   al_destroy_user_event_source(fc->event_source);
   al_destroy_thread(fc->thread);
   if (fc->initial_path)
      al_path_free(fc->initial_path);
   if (fc->patterns)
      free(fc->patterns);
   _AL_FREE(fc);
   return count;
}
