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
#include <gdk/gdkkeysyms.h>
#include <pango/pango.h>

#include "allegro5/allegro5.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

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

   if (!g_thread_supported())
       g_thread_init(NULL);
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
   fc->paths = al_malloc(n * sizeof(void *));
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

#ifdef ALLEGRO_WITH_XWINDOWS
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
#endif /* ALLEGRO_WITH_XWINDOWS */

static void make_transient(ALLEGRO_DISPLAY *display, GtkWidget *window)
{
   /* Set the current display window (if any) as the parent of the dialog. */
   #ifdef ALLEGRO_WITH_XWINDOWS
   ALLEGRO_DISPLAY_XGLX *glx = (void *)display;
   if (glx) {
      if (!GTK_WIDGET_REALIZED(window))
         g_signal_connect(window, "realize", G_CALLBACK(realized), (void *)glx);
      else
         really_make_transient(window, glx);
   }
   #endif
}

void al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   GtkWidget *window;

   gtk_start_and_lock();

   /* Create a new file selection widget */
   window = gtk_file_selection_new(al_cstr(fd->title));

   make_transient(display, window);

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

int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
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

   make_transient(display, window);

   if (fd->buttons) {
      int i = 1;
      int pos = 0;
      while (1) {
         int next = al_ustr_find_chr(fd->buttons, pos, '|');
         int pos2 = next;
         if (next == -1)
	     pos2 = al_ustr_length(fd->buttons);
         ALLEGRO_USTR_INFO info;
         ALLEGRO_USTR *button_text;
         button_text = al_ref_ustr(&info, fd->buttons, pos, pos2);
         pos = pos2 + 1;
         char buffer[256];
         al_ustr_to_buffer(button_text, buffer, sizeof buffer);
         gtk_dialog_add_button(GTK_DIALOG(window), buffer, i++);
         if (next == -1)
	     break;
      }
   }

   gtk_window_set_title(GTK_WINDOW(window), al_cstr(fd->title));

   g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(response), fd);
   g_signal_connect_swapped(G_OBJECT(window), "response", G_CALLBACK(gtk_widget_destroy), window);

   gtk_widget_show(window);

   gtk_unlock_and_wait(fd);

   return fd->pressed_button;
}

static void emit_close_event(ALLEGRO_NATIVE_DIALOG *textlog, bool keypress)
{
   ALLEGRO_EVENT event;
   event.user.type = ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE;
   event.user.timestamp = al_current_time();
   event.user.data1 = (intptr_t)textlog;
   event.user.data2 = (intptr_t)keypress;
   al_emit_user_event(&textlog->events, &event, NULL);
}

static gboolean textlog_delete(GtkWidget *w, GdkEvent *gevent,
   gpointer userdata)
{
   ALLEGRO_NATIVE_DIALOG *textlog = userdata;
   (void)w;
   (void)gevent;

   if (!(textlog->mode & ALLEGRO_TEXTLOG_NO_CLOSE)) {
      emit_close_event(textlog, false);
   }

   /* Don't close the window. */
   return TRUE;
}

static gboolean textlog_key_press(GtkWidget *w, GdkEventKey *gevent,
    gpointer userdata)
{
   ALLEGRO_NATIVE_DIALOG *textlog = userdata;
   (void)w;

   if (gevent->keyval == GDK_Escape) {
      emit_close_event(textlog, true);
   }

   return FALSE;
}

void _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   al_lock_mutex(textlog->text_mutex);

   gtk_start_and_lock();

   /* Create a new text log window. */
   GtkWidget *top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(top), 640, 480);
   gtk_window_set_title(GTK_WINDOW(top), al_cstr(textlog->title));

   if (textlog->mode & ALLEGRO_TEXTLOG_NO_CLOSE) {
      gtk_window_set_deletable(GTK_WINDOW(top), false);
   }
   else {
      g_signal_connect(G_OBJECT(top), "key-press-event", G_CALLBACK(textlog_key_press), textlog);
   }
   g_signal_connect(G_OBJECT(top), "delete-event", G_CALLBACK(textlog_delete), textlog);
   g_signal_connect(G_OBJECT(top), "destroy", G_CALLBACK(destroy), textlog);
   GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(top), scroll);
   GtkWidget *view = gtk_text_view_new();
   gtk_text_view_set_editable(GTK_TEXT_VIEW(view), false);
   if (textlog->mode & ALLEGRO_TEXTLOG_MONOSPACE) {
      PangoFontDescription *font_desc;
      font_desc = pango_font_description_from_string("Monospace");
      gtk_widget_modify_font(view, font_desc);
      pango_font_description_free(font_desc);
   }
   gtk_container_add(GTK_CONTAINER(scroll), view);
   gtk_widget_show(view);
   gtk_widget_show(scroll);
   gtk_widget_show(top);
   textlog->window = top;
   textlog->textview = view;

   /* Now notify al_show_native_textlog that the text log is ready. */
   textlog->done = true;
   al_signal_cond(textlog->text_cond);
   al_unlock_mutex(textlog->text_mutex);

   /* Keep running until the textlog is closed. */
   gtk_unlock_and_wait(textlog);

   /* Notify everyone that we're gone. */
   al_lock_mutex(textlog->text_mutex);
   textlog->done = true;
   al_signal_cond(textlog->text_cond);
   al_unlock_mutex(textlog->text_mutex);

}

static gboolean do_append_native_text_log(gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *textlog = data;
   al_lock_mutex(textlog->text_mutex);

   GtkTextView *tv = GTK_TEXT_VIEW(textlog->textview);
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(tv);
   GtkTextIter iter;

   gtk_text_buffer_get_end_iter(buffer, &iter);
   gtk_text_buffer_insert(buffer, &iter, al_cstr(textlog->text), -1);

   gtk_text_buffer_get_end_iter(buffer, &iter);
   gtk_text_view_scroll_to_iter(tv, &iter, 0, false, 0, 0);

   /* Notify the original caller that we are all done. */
   textlog->done = true;
   al_signal_cond(textlog->text_cond);
   al_unlock_mutex(textlog->text_mutex);
   return false;
}

void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   gdk_threads_add_timeout(0, do_append_native_text_log, textlog);
}

static gboolean do_close_native_text_log(gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *textlog = data;
   /* This causes the GTK window as well as all of its children to
    * be freed. Further it will call the destroy function which we
    * connected to the destroy signal which in turn causes our
    * gtk thread to quit.
    */
   gtk_widget_destroy(textlog->window);
   return false;
}

void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   gdk_threads_add_timeout(0, do_close_native_text_log, textlog);
}
