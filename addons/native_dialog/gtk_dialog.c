/* Each of these files implements the same, for different GUI toolkits:
 * 
 * dialog.c - code shared between all platforms
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
   gchar **paths = gtk_file_selection_get_selections(fs);
   int n = 0, i;
   while (paths[n]) {
      n++;
   }
   fc->count = n;
   fc->paths = _AL_MALLOC(n * sizeof(void *));
   for (i = 0; i < n; i++)
      fc->paths[i] = al_path_create(paths[i]);
   g_strfreev(paths);
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
