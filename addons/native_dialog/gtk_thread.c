#include <gtk/gtk.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog.h"
#include "allegro5/internal/aintern_native_dialog_cfg.h"
#include "gtk_dialog.h"

#include "allegro5/internal/aintern_vector.h"

A5O_DEBUG_CHANNEL("gtk")


/* GTK is not thread safe.  We launch a single thread which runs the GTK main
 * loop, and it is the only thread which calls into GTK.  (g_timeout_add may be
 * called from other threads without locking.)
 *
 * We used to attempt to use gdk_threads_enter/gdk_threads_leave but hit
 * some problems with deadlocks so switched to this.
 */

// G_STATIC_MUTEX_INIT causes a warning about a missing initializer, so if we
// have version 2.32 or newer don't use it to avoid the warning.
#if GLIB_CHECK_VERSION(2, 32, 0)
   #define NEWER_GLIB   1
#else
   #define NEWER_GLIB   0
#endif
#if NEWER_GLIB
   static GMutex nd_gtk_mutex;
   static void nd_gtk_lock(void)    { g_mutex_lock(&nd_gtk_mutex); }
   static void nd_gtk_unlock(void)  { g_mutex_unlock(&nd_gtk_mutex); }
#else
   static GStaticMutex nd_gtk_mutex = G_STATIC_MUTEX_INIT;
   static void nd_gtk_lock(void)    { g_static_mutex_lock(&nd_gtk_mutex); }
   static void nd_gtk_unlock(void)  { g_static_mutex_unlock(&nd_gtk_mutex); }
#endif
static GThread *nd_gtk_thread = NULL;


static void *nd_gtk_thread_func(void *data)
{
   GAsyncQueue *queue = data;

   A5O_DEBUG("GLIB %d.%d.%d\n",
      GLIB_MAJOR_VERSION,
      GLIB_MINOR_VERSION,
      GLIB_MICRO_VERSION);

   g_async_queue_push(queue, ACK_OK);

   gtk_main();

   A5O_INFO("GTK stopped.\n");
   return NULL;
}


bool _al_gtk_ensure_thread(void)
{
   bool ok = true;

#if !NEWER_GLIB
   if (!g_thread_supported())
      g_thread_init(NULL);
#endif

   /* al_init_native_dialog_addon() didn't always exist so GTK might not have
    * been initialised.  gtk_init_check knows if it's been initialised already
    * so we can just call it again.
    */
   {
      int argc = 0;
      char **argv = NULL;
      if (!gtk_init_check(&argc, &argv)) {
         A5O_ERROR("gtk_init_check failed\n");
         return false;
      }
   }

   nd_gtk_lock();

   if (!nd_gtk_thread) {
      GAsyncQueue *queue = g_async_queue_new();
#if NEWER_GLIB
      nd_gtk_thread = g_thread_new("gtk thread", nd_gtk_thread_func, queue);
#else
      bool joinable = FALSE;
      nd_gtk_thread = g_thread_create(nd_gtk_thread_func, queue, joinable, NULL);
#endif
      if (!nd_gtk_thread) {
         ok = false;
      }
      else {
         ok = (g_async_queue_pop(queue) == ACK_OK);
      }
      g_async_queue_unref(queue);
   }

   nd_gtk_unlock();

   return ok;
}


/* [user thread] */
bool _al_gtk_init_args(void *ptr, size_t size)
{
   ARGS_BASE *args = (ARGS_BASE *)ptr;
   memset(args, 0, size);
   args->mutex = al_create_mutex();
   if (!args->mutex) {
      return false;
   }
   args->cond = al_create_cond();
   if (!args->cond) {
      al_destroy_mutex(args->mutex);
      return false;
   }
   args->done = false;
   args->response = true;
   return args;
}


/* [user thread] */
bool _al_gtk_wait_for_args(GSourceFunc func, void *data)
{
   ARGS_BASE *args = (ARGS_BASE *) data;
   bool response;

   al_lock_mutex(args->mutex);
   g_timeout_add(0, func, data);
   while (args->done == false) {
      al_wait_cond(args->cond, args->mutex);
   }
   al_unlock_mutex(args->mutex);

   response = args->response;

   al_destroy_mutex(args->mutex);
   al_destroy_cond(args->cond);

   return response;
}


/* [gtk thread] */
void *_al_gtk_lock_args(gpointer data)
{
   ARGS_BASE *args = (ARGS_BASE *) data;
   al_lock_mutex(args->mutex);
   return args;
}


/* [gtk thread] */
gboolean _al_gtk_release_args(gpointer data)
{
   ARGS_BASE *args = (ARGS_BASE *) data;

   args->done = true;
   al_signal_cond(args->cond);
   al_unlock_mutex(args->mutex);

   return FALSE;
}


/* vim: set sts=3 sw=3 et: */
