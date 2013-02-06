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
 *      GTK native dialog implementation.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <gtk/gtk.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "gtk_dialog.h"


typedef struct {
   ALLEGRO_DISPLAY         *display;
   ALLEGRO_NATIVE_DIALOG   *dialog;
} Msg;


static void filesel_destroy(GtkWidget *w, gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *nd = data;
   (void)w;

   ASSERT(nd->async_queue);
   g_async_queue_push(nd->async_queue, ACK_CLOSED);
}

static void filesel_ok(GtkWidget *w, GtkFileSelection *fs)
{
   ALLEGRO_NATIVE_DIALOG *fc;
   fc = g_object_get_data(G_OBJECT(w), "ALLEGRO_NATIVE_DIALOG");
   gchar **paths = gtk_file_selection_get_selections(fs);
   int n = 0, i;
   while (paths[n]) {
      n++;
   }
   fc->fc_path_count = n;
   fc->fc_paths = al_malloc(n * sizeof(void *));
   for (i = 0; i < n; i++)
      fc->fc_paths[i] = al_create_path(paths[i]);
   g_strfreev(paths);
}

/* [nd_gtk thread] */
static gboolean create_native_file_dialog(gpointer data)
{
   Msg *msg = data;
   ALLEGRO_DISPLAY *display = msg->display;
   ALLEGRO_NATIVE_DIALOG *fd = msg->dialog;
   GtkWidget *window;

   /* Create a new file selection widget */
   window = gtk_file_selection_new(al_cstr(fd->title));

   _al_gtk_make_transient(display, window);

   /* Connect the destroy signal */
   g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(filesel_destroy), fd);

   /* Connect the ok_button */
   g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
                    "clicked", G_CALLBACK(filesel_ok), (gpointer) window);

   /* Connect both buttons to gtk_widget_destroy */
   g_signal_connect_swapped(GTK_FILE_SELECTION(window)->ok_button,
                            "clicked", G_CALLBACK(gtk_widget_destroy), window);
   g_signal_connect_swapped(GTK_FILE_SELECTION(window)->cancel_button,
                            "clicked", G_CALLBACK(gtk_widget_destroy), window);

   g_object_set_data(G_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
      "ALLEGRO_NATIVE_DIALOG", fd);

   if (fd->fc_initial_path) {
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(window),
         al_path_cstr(fd->fc_initial_path, '/'));
   }

   if (fd->flags & ALLEGRO_FILECHOOSER_MULTIPLE)
      gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(window), true);

   gtk_widget_show(window);
   return FALSE;
}

/* [user thread] */
bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   Msg msg;

   if (!_al_gtk_ensure_thread())
      return false;

   fd->async_queue = g_async_queue_new();

   msg.display = display;
   msg.dialog = fd;
   g_timeout_add(0, create_native_file_dialog, &msg);

   /* Wait for a signal that the window is closed. */
   while (g_async_queue_pop(fd->async_queue) != ACK_CLOSED)
      ;
   g_async_queue_unref(fd->async_queue);
   fd->async_queue = NULL;
   return true;
}

/* vim: set sts=3 sw=3 et: */
