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
#include <gdk/gdkkeysyms.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "gtk_dialog.h"
#include "gtk_xgtk.h"


typedef struct {
   A5O_NATIVE_DIALOG   *dialog;
} Msg;


static void emit_close_event(A5O_NATIVE_DIALOG *textlog, bool keypress)
{
   A5O_EVENT event;
   event.user.type = A5O_EVENT_NATIVE_DIALOG_CLOSE;
   event.user.timestamp = al_get_time();
   event.user.data1 = (intptr_t)textlog;
   event.user.data2 = (intptr_t)keypress;
   al_emit_user_event(&textlog->tl_events, &event, NULL);
}

static gboolean textlog_delete(GtkWidget *w, GdkEvent *gevent,
   gpointer userdata)
{
   A5O_NATIVE_DIALOG *textlog = userdata;
   (void)w;
   (void)gevent;

   if (!(textlog->flags & A5O_TEXTLOG_NO_CLOSE)) {
      emit_close_event(textlog, false);
   }

   /* Don't close the window. */
   return TRUE;
}

static gboolean textlog_key_press(GtkWidget *w, GdkEventKey *gevent,
    gpointer userdata)
{
   A5O_NATIVE_DIALOG *textlog = userdata;
   (void)w;

   if (gevent->keyval == GDK_KEY_Escape) {
      emit_close_event(textlog, true);
   }

   return FALSE;
}

static void textlog_destroy(GtkWidget *w, gpointer data)
{
   A5O_NATIVE_DIALOG *nd = data;
   (void)w;

   ASSERT(nd->async_queue);
   g_async_queue_push(nd->async_queue, ACK_CLOSED);
}

/* [gtk thread] */
static gboolean create_native_text_log(gpointer data)
{
   Msg *msg = data;
   A5O_NATIVE_DIALOG *textlog = msg->dialog;
   GtkCssProvider *css_provider;
   GtkStyleContext *context;

   /* Create a new text log window. */
   GtkWidget *top = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(top), 640, 480);
   gtk_window_set_title(GTK_WINDOW(top), al_cstr(textlog->title));

   if (textlog->flags & A5O_TEXTLOG_NO_CLOSE) {
      gtk_window_set_deletable(GTK_WINDOW(top), false);
   }
   else {
      g_signal_connect(G_OBJECT(top), "key-press-event", G_CALLBACK(textlog_key_press), textlog);
   }
   g_signal_connect(G_OBJECT(top), "delete-event", G_CALLBACK(textlog_delete), textlog);
   g_signal_connect(G_OBJECT(top), "destroy", G_CALLBACK(textlog_destroy), textlog);
   GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_container_add(GTK_CONTAINER(top), scroll);
   GtkWidget *view = gtk_text_view_new();
   gtk_text_view_set_editable(GTK_TEXT_VIEW(view), false);
   gtk_widget_set_name(GTK_WIDGET(view), "native_text_log");
   if (textlog->flags & A5O_TEXTLOG_MONOSPACE) {
      css_provider = gtk_css_provider_new();
      gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(css_provider),
                                      "#native_text_log {\n"
                                      "   font-family: monospace;\n"
                                      "}\n", -1, NULL);
      context = gtk_widget_get_style_context(GTK_WIDGET(view));
      gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      g_object_unref(css_provider);
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
bool _al_open_native_text_log(A5O_NATIVE_DIALOG *textlog)
{
   Msg msg;

   if (!_al_gtk_ensure_thread()) {
      textlog->tl_init_error = true;
      return false;
   }

   textlog->async_queue = g_async_queue_new();

   msg.dialog = textlog;
   g_timeout_add(0, create_native_text_log, &msg);

   while (g_async_queue_pop(textlog->async_queue) != ACK_OPENED)
      ;

   return true;
}

/* [gtk thread] */
static gboolean do_append_native_text_log(gpointer data)
{
   A5O_NATIVE_DIALOG *textlog = data;
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
void _al_append_native_text_log(A5O_NATIVE_DIALOG *textlog)
{
   if (textlog->tl_have_pending)
      return;
   textlog->tl_have_pending = true;

   gdk_threads_add_timeout(100, do_append_native_text_log, textlog);
}

/* [gtk thread] */
static gboolean do_close_native_text_log(gpointer data)
{
   A5O_NATIVE_DIALOG *textlog = data;

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
void _al_close_native_text_log(A5O_NATIVE_DIALOG *textlog)
{
   gdk_threads_add_timeout(0, do_close_native_text_log, textlog);

   while (g_async_queue_pop(textlog->async_queue) != ACK_CLOSED)
      ;
   g_async_queue_unref(textlog->async_queue);
   textlog->async_queue = NULL;
}

/* vim: set sts=3 sw=3 et: */
