#include <stdarg.h>
#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog_cfg.h"

/* Text logs are only implemented for GTK+ so far. */
#ifdef ALLEGRO_CFG_NATIVE_DIALOG_GTK
   #define HAVE_TEXT_LOG
#endif


#ifdef HAVE_TEXT_LOG
/* This will only return when the text window is closed. */
static void *textlog_thread_proc(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_NATIVE_DIALOG *textlog = arg;
   _al_open_native_text_log(textlog);
   return thread;
}
#endif


/* Function: al_open_native_text_log
 */
ALLEGRO_NATIVE_DIALOG *al_open_native_text_log(char const *title, int flags)
{
   ALLEGRO_NATIVE_DIALOG *textlog = NULL;

   /* Avoid warnings when log windows are unimplemented. */
   (void)title;
   (void)flags;

#ifdef HAVE_TEXT_LOG
   /* XXX We should clean up and return NULL if window creation fails. */
   textlog = al_calloc(1, sizeof *textlog);
   textlog->title = al_ustr_new(title);
   textlog->mode = flags;
   textlog->thread = al_create_thread(textlog_thread_proc, textlog);
   textlog->text_cond = al_create_cond();
   textlog->text_mutex = al_create_mutex();
   al_init_user_event_source(&textlog->events);

   /* Unlike the other dialogs, this one never blocks as the intended
    * use case is a log window running in the background for debugging
    * purposes when no console can be used. Therefore we have it run
    * in a separate thread.
    */
   al_start_thread(textlog->thread);
   al_lock_mutex(textlog->text_mutex);
   textlog->done = false;
   while (!textlog->done) {
      al_wait_cond(textlog->text_cond, textlog->text_mutex);
   }
   al_unlock_mutex(textlog->text_mutex);
#endif

   return textlog;
}


/* Function: al_close_native_text_log
 */
void al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
   if (!textlog)
      return;

#ifdef HAVE_TEXT_LOG
   al_lock_mutex(textlog->text_mutex);
   textlog->done = false;

   _al_close_native_text_log(textlog);

   while (!textlog->done) {
      al_wait_cond(textlog->text_cond, textlog->text_mutex);
   }

   al_ustr_free(textlog->title);

   al_destroy_user_event_source(&textlog->events);

   al_unlock_mutex(textlog->text_mutex);

   al_destroy_thread(textlog->thread);
   al_destroy_cond(textlog->text_cond);
   al_destroy_mutex(textlog->text_mutex);
   al_free(textlog);
#endif
}


/* Function: al_append_native_text_log
 */
void al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog,
   char const *format, ...)
{
   va_list args;

   /* Fall back to printf if no window. */
   if (!textlog) {
      va_start(args, format);
      vprintf(format, args);
      va_end(args);
      return;
   }

#ifdef HAVE_TEXT_LOG
   /* We could optimise the case where format="%s". */
   va_start(args, format);
   textlog->text = al_ustr_new("");
   al_ustr_vappendf(textlog->text, format, args);
   va_end(args);

   al_lock_mutex(textlog->text_mutex);

   _al_append_native_text_log(textlog);

   /* We wait until it is appended - that way we are sure it will
    * work correctly even if 10 threads are calling us 60 times
    * a second.  (Of course calling us that often would be slow.)
    */
   textlog->done = false;
   while (!textlog->done) {
      al_wait_cond(textlog->text_cond, textlog->text_mutex);
   }

   al_unlock_mutex(textlog->text_mutex);

   al_ustr_free(textlog->text);
#endif
}


/* Function: al_get_native_dialog_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_native_dialog_event_source(
   ALLEGRO_NATIVE_DIALOG *dialog)
{
   ASSERT(dialog);

   return &dialog->events;
}


/* vim: set sts=3 sw=3 et: */
