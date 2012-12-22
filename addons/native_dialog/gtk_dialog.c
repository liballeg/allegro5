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

#if defined ALLEGRO_WITH_XWINDOWS && !defined ALLEGRO_RASPBERRYPI
#define WITH_XGLX
#include "allegro5/internal/aintern_xglx.h"
#endif

ALLEGRO_DEBUG_CHANNEL("gtk")

#define ACK_OK       ((void *)0x1111)
#define ACK_ERROR    ((void *)0x2222)
#define ACK_OPENED   ((void *)0x3333)
#define ACK_CLOSED   ((void *)0x4444)

typedef struct {
   ALLEGRO_DISPLAY         *display;
   ALLEGRO_NATIVE_DIALOG   *dialog;
} Msg;

/*---------------------------------------------------------------------------*/
/* Shared functions                                                          */
/*---------------------------------------------------------------------------*/


#ifdef WITH_XGLX
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
#endif /* WITH_XGLX */

static void make_transient(ALLEGRO_DISPLAY *display, GtkWidget *window)
{
   /* Set the current display window (if any) as the parent of the dialog. */
   #ifdef WITH_XGLX
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

/* [nd_gtk thread] */
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
