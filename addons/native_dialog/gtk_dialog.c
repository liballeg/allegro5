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

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"

#ifdef ALLEGRO_WITH_XWINDOWS
#include "allegro5/internal/aintern_xglx.h"
#endif

ALLEGRO_DEBUG_CHANNEL("gtk")

typedef struct {
   ALLEGRO_DISPLAY         *display;
   ALLEGRO_NATIVE_DIALOG   *dialog;
} Msg;

#define ACK_OK       ((void *)0x1111)
#define ACK_ERROR    ((void *)0x2222)
#define ACK_OPENED   ((void *)0x3333)
#define ACK_CLOSED   ((void *)0x4444)

/*---------------------------------------------------------------------------*/
/* GTK thread                                                                */
/*---------------------------------------------------------------------------*/

/* GTK is not thread safe.  We launch a single thread which runs the GTK main
 * loop, and it is the only thread which calls into GTK.  (g_timeout_add may be
 * called from other threads without locking.)
 *
 * We used to attempt to use gdk_threads_enter/gdk_threads_leave but hit
 * some problems with deadlocks so switched to this.
 */

static GStaticMutex gtk_lock = G_STATIC_MUTEX_INIT;
static GThread *gtk_thread = NULL;
static int window_counter = 0;

static void *gtk_thread_func(void *data)
{
   GAsyncQueue *queue = data;
   int argc = 0;
   char **argv = NULL;
   bool again;

   ALLEGRO_DEBUG("Calling gtk_init_check.\n");
   if (gtk_init_check(&argc, &argv)) {
      g_async_queue_push(queue, ACK_OK);
   }
   else {
      ALLEGRO_ERROR("GTK initialisation failed.\n");
      g_async_queue_push(queue, ACK_ERROR);
      return NULL;
   }

   do {
      ALLEGRO_INFO("Entering GTK main loop.\n");
      gtk_main();

      /* Re-enter the main loop if a new window was created soon after the last
       * one was destroyed, which caused us to drop out of the GTK main loop.
       */
      g_static_mutex_lock(&gtk_lock);
      if (window_counter == 0) {
         gtk_thread = NULL;
         again = false;
      } else {
         again = true;
      }
      g_static_mutex_unlock(&gtk_lock);
   } while (again);

   ALLEGRO_INFO("GTK stopped.\n");
   return NULL;
}

static bool ensure_gtk_thread(void)
{
   bool ok = true;

   #if !GLIB_CHECK_VERSION(2, 31, 0)
   if (!g_thread_supported())
      g_thread_init(NULL);
   #endif

   g_static_mutex_lock(&gtk_lock);

   if (!gtk_thread) {
      GAsyncQueue *queue = g_async_queue_new();
      #if GLIB_CHECK_VERSION(2, 31, 0)
      gtk_thread = g_thread_new("gtk thread", gtk_thread_func, queue);
      #else
      bool joinable = FALSE;
      gtk_thread = g_thread_create(gtk_thread_func, queue, joinable, NULL);
      g_thread_unref(gtk_thread);
      #endif
      if (!gtk_thread) {
         ok = false;
      }
      else {
         ok = (g_async_queue_pop(queue) == ACK_OK);
      }
      g_async_queue_unref(queue);
   }

   if (ok) {
      ++window_counter;
      ALLEGRO_DEBUG("++window_counter = %d\n", window_counter);
   }

   g_static_mutex_unlock(&gtk_lock);

   return ok;
}

/*---------------------------------------------------------------------------*/
/* Shared functions                                                          */
/*---------------------------------------------------------------------------*/

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

static void dialog_destroy(GtkWidget *w, gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *nd = data;
   (void)w;

   ASSERT(nd->async_queue);
   g_async_queue_push(nd->async_queue, ACK_CLOSED);

   g_static_mutex_lock(&gtk_lock);
   --window_counter;
   ALLEGRO_DEBUG("--window_counter = %d\n", window_counter);
   if (window_counter == 0) {
      gtk_main_quit();
      ALLEGRO_DEBUG("Called gtk_main_quit.\n");
   }
   g_static_mutex_unlock(&gtk_lock);
}

/*---------------------------------------------------------------------------*/
/* File selector                                                             */
/*---------------------------------------------------------------------------*/

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

/* [gtk thread] */
static gboolean create_native_file_dialog(gpointer data)
{
   Msg *msg = data;
   ALLEGRO_DISPLAY *display = msg->display;
   ALLEGRO_NATIVE_DIALOG *fd = msg->dialog;
   GtkWidget *window;

   /* Create a new file selection widget */
   window = gtk_file_selection_new(al_cstr(fd->title));

   make_transient(display, window);

   /* Connect the destroy signal */
   g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(dialog_destroy), fd);

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

   if (!ensure_gtk_thread())
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

/*---------------------------------------------------------------------------*/
/* Message box                                                               */
/*---------------------------------------------------------------------------*/

/* Note: the message box code cannot assume that Allegro is installed. */

static void msgbox_response(GtkDialog *dialog, gint response_id,
   gpointer user_data)
{
   ALLEGRO_NATIVE_DIALOG *nd = (void *)user_data;
   (void)dialog;
   switch (response_id) {
      case GTK_RESPONSE_DELETE_EVENT:
         nd->mb_pressed_button = 0;
         break;
      case GTK_RESPONSE_YES:
      case GTK_RESPONSE_OK:
         nd->mb_pressed_button = 1;
         break;
      case GTK_RESPONSE_NO:
      case GTK_RESPONSE_CANCEL:
         nd->mb_pressed_button = 2;
         break;
      default:
         nd->mb_pressed_button = response_id;
   }
}

/* [gtk thread] */
static gboolean create_native_message_box(gpointer data)
{
   Msg *msg = data;
   ALLEGRO_DISPLAY *display = msg->display;
   ALLEGRO_NATIVE_DIALOG *fd = msg->dialog;
   GtkWidget *window;

   /* Create a new file selection widget */
   GtkMessageType type = GTK_MESSAGE_INFO;
   GtkButtonsType buttons = GTK_BUTTONS_OK;
   if (fd->flags & ALLEGRO_MESSAGEBOX_YES_NO) type = GTK_MESSAGE_QUESTION;
   if (fd->flags & ALLEGRO_MESSAGEBOX_QUESTION) type = GTK_MESSAGE_QUESTION;
   if (fd->flags & ALLEGRO_MESSAGEBOX_WARN) type = GTK_MESSAGE_WARNING;
   if (fd->flags & ALLEGRO_MESSAGEBOX_ERROR) type = GTK_MESSAGE_ERROR;
   if (fd->flags & ALLEGRO_MESSAGEBOX_YES_NO) buttons = GTK_BUTTONS_YES_NO;
   if (fd->flags & ALLEGRO_MESSAGEBOX_OK_CANCEL) buttons = GTK_BUTTONS_OK_CANCEL;
   if (fd->mb_buttons) buttons = GTK_BUTTONS_NONE;

   window = gtk_message_dialog_new(NULL, 0, type, buttons, "%s",
      al_cstr(fd->mb_heading));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(window), "%s",
      al_cstr(fd->mb_text));

   make_transient(display, window);

   if (fd->mb_buttons) {
      int i = 1;
      int pos = 0;
      while (1) {
         int next = al_ustr_find_chr(fd->mb_buttons, pos, '|');
         int pos2 = next;
         if (next == -1)
	     pos2 = al_ustr_size(fd->mb_buttons);
         ALLEGRO_USTR_INFO info;
         const ALLEGRO_USTR *button_text;
         button_text = al_ref_ustr(&info, fd->mb_buttons, pos, pos2);
         pos = pos2 + 1;
         char buffer[256];
         al_ustr_to_buffer(button_text, buffer, sizeof buffer);
         gtk_dialog_add_button(GTK_DIALOG(window), buffer, i++);
         if (next == -1)
	     break;
      }
   }

   gtk_window_set_title(GTK_WINDOW(window), al_cstr(fd->title));

   g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(dialog_destroy), fd);

   g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(msgbox_response), fd);
   g_signal_connect_swapped(G_OBJECT(window), "response", G_CALLBACK(gtk_widget_destroy), window);

   gtk_widget_show(window);
   return FALSE;
}

/* [user thread] */
int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd)
{
   Msg msg;

   if (!ensure_gtk_thread())
      return 0; /* "cancelled" */

   fd->async_queue = g_async_queue_new();

   msg.display = display;
   msg.dialog = fd;
   g_timeout_add(0, create_native_message_box, &msg);

   /* Wait for a signal that the window is closed. */
   while (g_async_queue_pop(fd->async_queue) != ACK_CLOSED)
      ;
   g_async_queue_unref(fd->async_queue);
   fd->async_queue = NULL;
   return fd->mb_pressed_button;
}

/*---------------------------------------------------------------------------*/
/* Text log                                                                  */
/*---------------------------------------------------------------------------*/

static void emit_close_event(ALLEGRO_NATIVE_DIALOG *textlog, bool keypress)
{
   ALLEGRO_EVENT event;
   event.user.type = ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE;
   event.user.timestamp = al_get_time();
   event.user.data1 = (intptr_t)textlog;
   event.user.data2 = (intptr_t)keypress;
   al_emit_user_event(&textlog->tl_events, &event, NULL);
}

static gboolean textlog_delete(GtkWidget *w, GdkEvent *gevent,
   gpointer userdata)
{
   ALLEGRO_NATIVE_DIALOG *textlog = userdata;
   (void)w;
   (void)gevent;

   if (!(textlog->flags & ALLEGRO_TEXTLOG_NO_CLOSE)) {
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

/* [gtk thread] */
static gboolean create_native_text_log(gpointer data)
{
   Msg *msg = data;
   ALLEGRO_NATIVE_DIALOG *textlog = msg->dialog;

   /* Create a new text log window. */
   GtkWidget *top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(top), 640, 480);
   gtk_window_set_title(GTK_WINDOW(top), al_cstr(textlog->title));

   if (textlog->flags & ALLEGRO_TEXTLOG_NO_CLOSE) {
      gtk_window_set_deletable(GTK_WINDOW(top), false);
   }
   else {
      g_signal_connect(G_OBJECT(top), "key-press-event", G_CALLBACK(textlog_key_press), textlog);
   }
   g_signal_connect(G_OBJECT(top), "delete-event", G_CALLBACK(textlog_delete), textlog);
   g_signal_connect(G_OBJECT(top), "destroy", G_CALLBACK(dialog_destroy), textlog);
   GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(top), scroll);
   GtkWidget *view = gtk_text_view_new();
   gtk_text_view_set_editable(GTK_TEXT_VIEW(view), false);
   if (textlog->flags & ALLEGRO_TEXTLOG_MONOSPACE) {
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
   textlog->tl_textview = view;

   ASSERT(textlog->async_queue);
   g_async_queue_push(textlog->async_queue, ACK_OPENED);
   return FALSE;
}

/* [user thread] */
bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   Msg msg;

   if (!ensure_gtk_thread()) {
      textlog->tl_init_error = true;
      return false;
   }

   textlog->async_queue = g_async_queue_new();

   msg.display = NULL;
   msg.dialog = textlog;
   g_timeout_add(0, create_native_text_log, &msg);

   while (g_async_queue_pop(textlog->async_queue) != ACK_OPENED)
      ;

   return true;
}

/* [gtk thread] */
static gboolean do_append_native_text_log(gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *textlog = data;
   al_lock_mutex(textlog->tl_text_mutex);

   GtkTextView *tv = GTK_TEXT_VIEW(textlog->tl_textview);
   GtkTextBuffer *buffer = gtk_text_view_get_buffer(tv);
   GtkTextIter iter;
   GtkTextMark *mark;

   gtk_text_buffer_get_end_iter(buffer, &iter);
   gtk_text_buffer_insert(buffer, &iter, al_cstr(textlog->tl_pending_text), -1);

   mark = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
   gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(textlog->tl_textview), mark);
   gtk_text_buffer_delete_mark(buffer, mark);

   al_ustr_truncate(textlog->tl_pending_text, 0);

   textlog->tl_have_pending = false;

   al_unlock_mutex(textlog->tl_text_mutex);
   return FALSE;
}

/* [user thread] */
void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   if (textlog->tl_have_pending)
      return;
   textlog->tl_have_pending = true;

   gdk_threads_add_timeout(100, do_append_native_text_log, textlog);
}

/* [gtk thread] */
static gboolean do_close_native_text_log(gpointer data)
{
   ALLEGRO_NATIVE_DIALOG *textlog = data;

   /* Delay closing until appends are completed. */
   if (textlog->tl_have_pending) {
      return TRUE;
   }

   /* This causes the GTK window as well as all of its children to
    * be freed. Further it will call the destroy function which we
    * connected to the destroy signal which in turn causes our
    * gtk thread to quit.
    */
   gtk_widget_destroy(textlog->window);
   return FALSE;
}

/* [user thread] */
void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   gdk_threads_add_timeout(0, do_close_native_text_log, textlog);

   while (g_async_queue_pop(textlog->async_queue) != ACK_CLOSED)
      ;
   g_async_queue_unref(textlog->async_queue);
   textlog->async_queue = NULL;
}

/* vim: set sts=3 sw=3 et: */
