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
#include "gtk_xgtk.h"


typedef struct {
   A5O_DISPLAY         *display;
   A5O_NATIVE_DIALOG   *dialog;
} Msg;


/* Note: the message box code cannot assume that Allegro is installed. */

static void msgbox_destroy(GtkWidget *w, gpointer data)
{
   A5O_NATIVE_DIALOG *nd = data;
   (void)w;

   ASSERT(nd->async_queue);
   g_async_queue_push(nd->async_queue, ACK_CLOSED);
}

static void msgbox_response(GtkDialog *dialog, gint response_id,
   gpointer user_data)
{
   A5O_NATIVE_DIALOG *nd = (void *)user_data;
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
   A5O_DISPLAY *display = msg->display;
   A5O_NATIVE_DIALOG *fd = msg->dialog;
   GtkWidget *window;

   /* Create a new file selection widget */
   GtkMessageType type = GTK_MESSAGE_INFO;
   GtkButtonsType buttons = GTK_BUTTONS_OK;
   if (fd->flags & A5O_MESSAGEBOX_YES_NO) type = GTK_MESSAGE_QUESTION;
   if (fd->flags & A5O_MESSAGEBOX_QUESTION) type = GTK_MESSAGE_QUESTION;
   if (fd->flags & A5O_MESSAGEBOX_WARN) type = GTK_MESSAGE_WARNING;
   if (fd->flags & A5O_MESSAGEBOX_ERROR) type = GTK_MESSAGE_ERROR;
   if (fd->flags & A5O_MESSAGEBOX_YES_NO) buttons = GTK_BUTTONS_YES_NO;
   if (fd->flags & A5O_MESSAGEBOX_OK_CANCEL) buttons = GTK_BUTTONS_OK_CANCEL;
   if (fd->mb_buttons) buttons = GTK_BUTTONS_NONE;

   window = gtk_message_dialog_new(NULL, 0, type, buttons, "%s",
      al_cstr(fd->mb_heading));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(window), "%s",
      al_cstr(fd->mb_text));

   _al_gtk_make_transient(display, window);

   if (fd->mb_buttons) {
      int i = 1;
      int pos = 0;
      while (1) {
         int next = al_ustr_find_chr(fd->mb_buttons, pos, '|');
         int pos2 = next;
         if (next == -1)
	     pos2 = al_ustr_size(fd->mb_buttons);
         A5O_USTR_INFO info;
         const A5O_USTR *button_text;
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

   g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(msgbox_destroy), fd);

   g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(msgbox_response), fd);
   g_signal_connect_swapped(G_OBJECT(window), "response", G_CALLBACK(gtk_widget_destroy), window);

   gtk_widget_show(window);
   return FALSE;
}

/* [user thread] */
int _al_show_native_message_box(A5O_DISPLAY *display,
   A5O_NATIVE_DIALOG *fd)
{
   Msg msg;

   if (!_al_gtk_ensure_thread())
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

/* vim: set sts=3 sw=3 et: */
