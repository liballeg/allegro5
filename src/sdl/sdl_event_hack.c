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
 *      SDL event hack (see below).
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_timer.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/platform/allegro_internal_sdl.h"

/* This is a thread which wakes up event queues from time to time (with fake
 * timer events) to prevent a deadlock in an unbound al_wait_for_event.
 */

static A5O_THREAD *thread;

static void wakeup_with_fake_timer_event(void)
{
   A5O_EVENT_SOURCE *es = al_get_keyboard_event_source();
   _al_event_source_lock(es);
   A5O_EVENT event;
   event.timer.type = A5O_EVENT_TIMER;
   event.timer.timestamp = al_get_time();
   event.timer.count = 0;
   event.timer.error = 0;
   _al_event_source_emit_event(es, &event);
   _al_event_source_unlock(es);
}

static void *wakeup_thread(A5O_THREAD *thread, void *user)
{
   al_rest(1);
   while (!al_get_thread_should_stop(thread)) {
      /* If the program uses timers, this hack is not required usually. */
      if (_al_get_active_timers_count())
         break;
      if (!al_is_keyboard_installed())
         break;
      wakeup_with_fake_timer_event();
      al_rest(0.01);
   }
   return user;
}

static void _uninstall_sdl_event_hack(void)
{
   if (thread) {
      al_set_thread_should_stop(thread);
      al_join_thread(thread, NULL);
      al_destroy_thread(thread);
   }
}

void _al_sdl_event_hack(void)
{
   if (thread)
      return;
   _al_add_exit_func(_uninstall_sdl_event_hack, "uninstall_sdl_event_hack");
   thread = al_create_thread(wakeup_thread, NULL);
   al_start_thread(thread);
}
