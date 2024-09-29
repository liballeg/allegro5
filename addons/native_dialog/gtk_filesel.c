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
} GTK_FILE_DIALOG_MESSAGE;

/* [nd_gtk thread] */
static gboolean create_gtk_file_dialog(gpointer data)
{
   GTK_FILE_DIALOG_MESSAGE *msg = data;
   A5O_DISPLAY *display = msg->display;
   A5O_NATIVE_DIALOG *fd = msg->dialog;
   bool save = fd->flags & A5O_FILECHOOSER_SAVE;
   bool folder = fd->flags & A5O_FILECHOOSER_FOLDER;
   gint result;

   GtkWidget *window;

   window =
      gtk_file_chooser_dialog_new(al_cstr(fd->title),
                                  NULL,
                                  save ? GTK_FILE_CHOOSER_ACTION_SAVE : folder ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN,
                                  "_Cancel", GTK_RESPONSE_CANCEL,
                                  save ? "_Save" : "_Open", GTK_RESPONSE_ACCEPT, NULL);

   _al_gtk_make_transient(display, window);

   if (save) {
      gtk_file_chooser_set_do_overwrite_confirmation
         (GTK_FILE_CHOOSER(window), true);
   }

   if (fd->fc_initial_path) {
      bool is_dir;
      bool exists;
      const char *path = al_path_cstr(fd->fc_initial_path, A5O_NATIVE_PATH_SEP);

      if (al_filename_exists(path)) {
         exists = true;
         A5O_FS_ENTRY *fs = al_create_fs_entry(path);
         is_dir = al_get_fs_entry_mode(fs) & A5O_FILEMODE_ISDIR;
         al_destroy_fs_entry(fs);
      }
      else {
         exists = false;
         is_dir = false;
      }

      if (is_dir) {
         gtk_file_chooser_set_current_folder
            (GTK_FILE_CHOOSER(window),
             al_path_cstr(fd->fc_initial_path, A5O_NATIVE_PATH_SEP));
      }
      else if (exists) {
         gtk_file_chooser_set_filename
             (GTK_FILE_CHOOSER(window),
              al_path_cstr(fd->fc_initial_path, A5O_NATIVE_PATH_SEP));
      }
      else {
         A5O_PATH *dir_path = al_clone_path(fd->fc_initial_path);
         if (dir_path) {
            al_set_path_filename(dir_path, NULL);
            gtk_file_chooser_set_current_folder
               (GTK_FILE_CHOOSER(window),
                al_path_cstr(dir_path, A5O_NATIVE_PATH_SEP));
            if (save) {
               gtk_file_chooser_set_current_name
                  (GTK_FILE_CHOOSER(window),
                   al_get_path_filename(fd->fc_initial_path));
            }
            al_destroy_path(dir_path);
         }
      }
   }

   if (fd->flags & A5O_FILECHOOSER_MULTIPLE)
      gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(window), true);

   for (size_t i = 0; i < _al_vector_size(&fd->fc_patterns); i++)
   {
      _AL_PATTERNS_AND_DESC *patterns_and_desc = _al_vector_ref(&fd->fc_patterns, i);
      const A5O_USTR *desc = al_ref_info(&patterns_and_desc->desc);
      GtkFileFilter* filter = gtk_file_filter_new();
      if (al_ustr_size(desc) > 0) {
         char *cstr = al_cstr_dup(desc);
         gtk_file_filter_set_name(filter, cstr);
         al_free(cstr);
      }
      else {
         gtk_file_filter_set_name(filter, "All supported files");
      }
      for (size_t j = 0; j < _al_vector_size(&patterns_and_desc->patterns_vec); j++) {
         _AL_PATTERN *pattern = _al_vector_ref(&patterns_and_desc->patterns_vec, j);
         char *cstr = al_cstr_dup(al_ref_info(&pattern->info));
         if (pattern->is_mime) {
            gtk_file_filter_add_mime_type(filter, cstr);
         }
         else {
            gtk_file_filter_add_pattern(filter, cstr);
         }
         al_free(cstr);
      }
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(window), filter);
   }

   result = gtk_dialog_run(GTK_DIALOG(window));
   if (result == GTK_RESPONSE_ACCEPT) {
      GSList* filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(window));
      int i;
      GSList* iter;

      fd->fc_path_count = g_slist_length(filenames);
      fd->fc_paths = al_malloc(fd->fc_path_count * sizeof(void *));
      for (i = 0, iter = filenames; i < (int)fd->fc_path_count; i++, iter = g_slist_next(iter)) {
         fd->fc_paths[i] = al_create_path((const char*)iter->data);
         g_free(iter->data);
      }
      g_slist_free(filenames);
   }

   gtk_widget_destroy(window);

   ASSERT(fd->async_queue);
   g_async_queue_push(fd->async_queue, ACK_CLOSED);

   return FALSE;
}

/* [user thread] */
bool _al_show_native_file_dialog(A5O_DISPLAY *display,
   A5O_NATIVE_DIALOG *fd)
{
   GTK_FILE_DIALOG_MESSAGE msg;

   if (!_al_gtk_ensure_thread())
      return false;

   fd->async_queue = g_async_queue_new();

   msg.display = display;
   msg.dialog = fd;
   g_timeout_add(0, create_gtk_file_dialog, &msg);

   /* Wait for a signal that the window is closed. */
   while (g_async_queue_pop(fd->async_queue) != ACK_CLOSED)
      ;
   g_async_queue_unref(fd->async_queue);
   fd->async_queue = NULL;
   return true;
}

/* vim: set sts=3 sw=3 et: */
