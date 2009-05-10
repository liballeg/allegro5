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

#ifdef ALLEGRO_WITH_XWINDOWS
#include "allegro5/internal/aintern_xglx.h"
#endif

static int global_counter;
static bool gtk_is_running;
static ALLEGRO_MUTEX *gtk_lock;
static ALLEGRO_COND *gtk_cond;
static ALLEGRO_THREAD *gtk_thread;

/* All dialogs in the native dialogs addon are blocking. This makes the
 * implementation particularly simple in GTK:
 * - Start GTK (gtk_init) and create the GTK dialog.
 * - Run GTK (gtk_main).
 * - Exit GTK from dialog callback (gtk_main_quit) which makes the gtk_main()
 *   above return.
 *
 * However, this is only if we forget about threads. With threads, things look
 * different - as they make it possible to show more than one native dialog
 * at a time. But gtk_init/gtk_main can of course only be called once by an
 * application (without calling gtk_main_quit first). Here a simple solution
 * would be to have GTK be running as long as the addon is initialized - but we
 * did a somewhat more complex solution which has GTK run in its own thread as
 * long as at least one dialog is shown.
 */

static void *gtk_thread_func(ALLEGRO_THREAD *thread, void *data)
{
   int argc = 0;
   char **argv = NULL;
   (void)thread;
   (void)data;

   if (!g_thread_supported()) g_thread_init(NULL);
   gdk_threads_init();

   gtk_init_check(&argc, &argv);

   al_lock_mutex(gtk_lock);
   gtk_is_running = true;
   al_broadcast_cond(gtk_cond);
   al_unlock_mutex(gtk_lock);

   gdk_threads_enter();
   gtk_main();
   gdk_threads_leave();

   al_lock_mutex(gtk_lock);
   gtk_is_running = false;
   al_broadcast_cond(gtk_cond);
   al_unlock_mutex(gtk_lock);

   return NULL;
}

static void gtk_start_and_lock(void)
{
   /* Note: This is not 100% correct, best would be to create the lock from
    * the main thread *before* there is any chance for concurrency.
    */
   if (!gtk_lock)
      gtk_lock = al_create_mutex();

   al_lock_mutex(gtk_lock);

   global_counter++;

   if (!gtk_cond)
      gtk_cond = al_create_cond();

   if (global_counter == 1) {
      gtk_thread = al_create_thread(gtk_thread_func, NULL);
      al_start_thread(gtk_thread);
      while (!gtk_is_running)
         al_wait_cond(gtk_cond, gtk_lock);
   }

   gdk_threads_enter();
}

static void gtk_unlock_and_wait(ALLEGRO_NATIVE_DIALOG *nd)
{
   gdk_threads_leave();

   nd->cond = al_create_cond();

   nd->is_active = true;
   while (nd->is_active)
      al_wait_cond(nd->cond, gtk_lock);

   global_counter--;
   if (global_counter == 0) {
      gtk_main_quit();
      while (gtk_is_running)
         al_wait_cond(gtk_cond, gtk_lock);
   }

   al_unlock_mutex(gtk_lock);
}

static void gtk_end(ALLEGRO_NATIVE_DIALOG *nd)
{
   al_lock_mutex(gtk_lock);

   nd->is_active = false;
   al_broadcast_cond(nd->cond);

   al_unlock_mutex(gtk_lock);
}

static void destroy(GtkWidget *w, gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *nd = data;
   (void)data;
   (void)w;
   gtk_end(nd);
}

static void ok(GtkWidget *w, GtkFileSelection *fs)
{
   ALLEGRO_NATIVE_DIALOG *fc;
   fc = g_object_get_data(G_OBJECT(w), "ALLEGRO_NATIVE_DIALOG");
   gchar **paths = gtk_file_selection_get_selections(fs);
   int n = 0, i;
   while (paths[n]) {
      n++;
   }
   fc->count = n;
   fc->paths = _AL_MALLOC(n * sizeof(void *));
   for (i = 0; i < n; i++)
      fc->paths[i] = al_create_path(paths[i]);
   g_strfreev(paths);
}

static void response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
   ALLEGRO_NATIVE_DIALOG *nd = (void *)user_data;
   (void)dialog;
   switch (response_id) {
      case GTK_RESPONSE_DELETE_EVENT:
         nd->pressed_button = 0;
         break;
      case GTK_RESPONSE_YES:
      case GTK_RESPONSE_OK:
         nd->pressed_button = 1;
         break;
      case GTK_RESPONSE_NO:
      case GTK_RESPONSE_CANCEL:
         nd->pressed_button = 2;
         break;
      default:
         nd->pressed_button = response_id;
   }
   gtk_end(nd);
}

static void really_make_transient(GtkWidget *window, ALLEGRO_DISPLAY_XGLX *glx)
{
   GdkDisplay *gdk = gdk_drawable_get_display(GDK_DRAWABLE(window->window));
   GdkWindow *parent = gdk_window_lookup_for_display(gdk, glx->window);
   if (!parent)
      parent = gdk_window_foreign_new_for_display(gdk, glx->window);
   gdk_window_set_transient_for(window->window, parent);
}

static void realized(GtkWidget *window, gpointer data)
{
   really_make_transient(window, (void *)data);
}

static void make_transient(GtkWidget *window)
{
   /* Set the current display window (if any) as the parent of the dialog. */
   #ifdef ALLEGRO_WITH_XWINDOWS
   ALLEGRO_DISPLAY_XGLX *glx = (void *)al_get_current_display();
   if (glx) {
      if (!GTK_WIDGET_REALIZED(window))
         g_signal_connect(window, "realize", G_CALLBACK(realized), (void *)glx);
      else
         really_make_transient(window, glx);
   }
   #endif
}

void al_show_native_file_dialog(ALLEGRO_NATIVE_DIALOG *fd)
{
   GtkWidget *window;

   gtk_start_and_lock();

   /* Create a new file selection widget */
   window = gtk_file_selection_new(al_cstr(fd->title));

   make_transient(window);

   /* Connect the destroy signal */
   g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), fd);

   /* Connect the ok_button */
   g_signal_connect(G_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
                    "clicked", G_CALLBACK(ok), (gpointer) window);

   /* Connect both buttons to gtk_widget_destroy */
   g_signal_connect_swapped(GTK_FILE_SELECTION(window)->ok_button,
                            "clicked", G_CALLBACK(gtk_widget_destroy), window);
   g_signal_connect_swapped(GTK_FILE_SELECTION(window)->cancel_button,
                            "clicked", G_CALLBACK(gtk_widget_destroy), window);

   g_object_set_data(G_OBJECT(GTK_FILE_SELECTION(window)->ok_button),
      "ALLEGRO_NATIVE_DIALOG", fd);

   if (fd->initial_path) {
      gtk_file_selection_set_filename(GTK_FILE_SELECTION(window),
         al_path_cstr(fd->initial_path, '/'));
   }

   if (fd->mode & ALLEGRO_FILECHOOSER_MULTIPLE)
      gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(window), true);

   gtk_widget_show(window);

   gtk_unlock_and_wait(fd);
}

int _al_show_native_message_box(ALLEGRO_NATIVE_DIALOG *fd)
{
   GtkWidget *window;
   gtk_start_and_lock();

   /* Create a new file selection widget */
   GtkMessageType type = GTK_MESSAGE_INFO;
   GtkButtonsType buttons = GTK_BUTTONS_OK;
   if (fd->mode & ALLEGRO_MESSAGEBOX_YES_NO) type = GTK_MESSAGE_QUESTION;
   if (fd->mode & ALLEGRO_MESSAGEBOX_QUESTION) type = GTK_MESSAGE_QUESTION;
   if (fd->mode & ALLEGRO_MESSAGEBOX_WARN) type = GTK_MESSAGE_WARNING;
   if (fd->mode & ALLEGRO_MESSAGEBOX_ERROR) type = GTK_MESSAGE_ERROR;
   if (fd->mode & ALLEGRO_MESSAGEBOX_YES_NO) buttons = GTK_BUTTONS_YES_NO;
   if (fd->mode & ALLEGRO_MESSAGEBOX_OK_CANCEL) buttons = GTK_BUTTONS_OK_CANCEL;
   if (fd->buttons) buttons = GTK_BUTTONS_NONE;

   window = gtk_message_dialog_new(NULL, 0, type, buttons, "%s",
      al_cstr(fd->heading));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(window), "%s",
      al_cstr(fd->text));

   make_transient(window);

   if (fd->buttons) {
      int i = 1;
      int pos = 0;
      while (1) {
         int next = al_ustr_find_chr(fd->buttons, pos, '|');
         int pos2 = next;
         if (next == -1) pos2 = al_ustr_length(fd->buttons);
         ALLEGRO_USTR_INFO info;
         ALLEGRO_USTR *button_text;
         button_text = al_ref_ustr(&info, fd->buttons, pos, pos2);
         pos = pos2 + 1;
         char buffer[256];
         al_ustr_to_buffer(button_text, buffer, sizeof buffer);
         gtk_dialog_add_button(GTK_DIALOG(window), buffer, i++);
         if (next == -1) break;
      }
   }

   gtk_window_set_title(GTK_WINDOW(window), al_cstr(fd->title));

   g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(response), fd);
   g_signal_connect_swapped(G_OBJECT(window), "response", G_CALLBACK(gtk_widget_destroy), window);

   gtk_widget_show(window);

   gtk_unlock_and_wait(fd);

   return fd->pressed_button;
}
