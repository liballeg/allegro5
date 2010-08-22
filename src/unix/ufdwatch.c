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
 *      Unix fd watcher thread.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 *
 *      This module implements a background thread that waits for data
 *      to arrive in file descriptors, at which point it dispatches to
 *      functions which will process that data.
 */


#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/platform/aintunix.h"



typedef struct WATCH_ITEM
{
   int fd;
   void (*callback)(void *);
   void *cb_data;
} WATCH_ITEM;


static _AL_THREAD fd_watch_thread;
static _AL_MUTEX fd_watch_mutex = _AL_MUTEX_UNINITED;
static _AL_VECTOR fd_watch_list = _AL_VECTOR_INITIALIZER(WATCH_ITEM);



/* fd_watch_thread_func: [fdwatch thread]
 *  The thread loop function.
 */
static void fd_watch_thread_func(_AL_THREAD *self, void *unused)
{
   (void)unused;

   while (!_al_get_thread_should_stop(self)) {

      fd_set rfds;
      int max_fd;

      /* set up max_fd and rfds */
      _al_mutex_lock(&fd_watch_mutex);
      {
         WATCH_ITEM *wi;
         unsigned int i;

         FD_ZERO(&rfds);
         max_fd = -1;

         for (i = 0; i < _al_vector_size(&fd_watch_list); i++) {
            wi = _al_vector_ref(&fd_watch_list, i);
            FD_SET(wi->fd, &rfds);
            if (wi->fd > max_fd)
               max_fd = wi->fd;
         }
      }
      _al_mutex_unlock(&fd_watch_mutex);

      /* wait for something to happen on one of the fds */
      {
         struct timeval tv;
         int retval;

         tv.tv_sec = 0;
         tv.tv_usec = 250000;

         retval = select(max_fd+1, &rfds, NULL, NULL, &tv);
         if (retval < 1)
            continue;
      }

      /* one or more of the fds has activity */
      _al_mutex_lock(&fd_watch_mutex);
      {
         WATCH_ITEM *wi;
         unsigned int i;

         for (i = 0; i < _al_vector_size(&fd_watch_list); i++) {
            wi = _al_vector_ref(&fd_watch_list, i);
            if (FD_ISSET(wi->fd, &rfds)) {
               /* The callback is allowed to modify the watch list so the mutex
                * must be recursive.
                */
               wi->callback(wi->cb_data);
            }
         }
      }
      _al_mutex_unlock(&fd_watch_mutex);
   }
}



/* _al_unix_start_watching_fd: [primary thread]
 * 
 *  Start watching for data on file descriptor `fd'.  This is done in
 *  a background thread, which is started if necessary.  When there is
 *  data waiting to be read on fd, `callback' is applied to `cb_data'.
 *  The callback function should read as much data off fd as possible.
 *
 *  Note: the callback is run from the background thread.  You can
 *  assume there is only one callback being called from the fdwatch
 *  module at a time.
 */
void _al_unix_start_watching_fd(int fd, void (*callback)(void *), void *cb_data)
{
   ASSERT(fd >= 0);
   ASSERT(callback);

   /* start the background thread if necessary */
   if (_al_vector_size(&fd_watch_list) == 0) {
      /* We need a recursive mutex to allow callbacks to modify the fd watch
       * list.
       */
      _al_mutex_init_recursive(&fd_watch_mutex);
      _al_thread_create(&fd_watch_thread, fd_watch_thread_func, NULL);
   }

   /* now add the watch item to the list */
   _al_mutex_lock(&fd_watch_mutex);
   {
      WATCH_ITEM *wi = _al_vector_alloc_back(&fd_watch_list);

      wi->fd = fd;
      wi->callback = callback;
      wi->cb_data = cb_data;
   }
   _al_mutex_unlock(&fd_watch_mutex);
}



/* _al_unix_stop_watching_fd: [primary thread]
 *
 *  Stop watching for data on `fd'.  Once there are no more file
 *  descriptors to watch, the background thread will be stopped.  This
 *  function is synchronised with the background thread, so you don't
 *  have to do any locking before calling it.
 */
void _al_unix_stop_watching_fd(int fd)
{
   bool list_empty = false;

   /* find the fd in the watch list and remove it */
   _al_mutex_lock(&fd_watch_mutex);
   {
      WATCH_ITEM *wi;
      unsigned int i;

      for (i = 0; i < _al_vector_size(&fd_watch_list); i++) {
         wi = _al_vector_ref(&fd_watch_list, i);
         if (wi->fd == fd) {
            _al_vector_delete_at(&fd_watch_list, i);
            list_empty = _al_vector_is_empty(&fd_watch_list);
            break;
         }
      }
   }
   _al_mutex_unlock(&fd_watch_mutex);

   /* if no more fd's are being watched, stop the background thread */
   if (list_empty) {
      _al_thread_join(&fd_watch_thread);
      _al_mutex_destroy(&fd_watch_mutex);
      _al_vector_free(&fd_watch_list);
   }
}



/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
