#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xkeyboard.h"
#include "allegro5/internal/aintern_xmouse.h"
#include "allegro5/internal/aintern_xsystem.h"

ALLEGRO_DEBUG_CHANNEL("xevents")

/* Handle an X11 close button event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
static void _al_display_xglx_closebutton(ALLEGRO_DISPLAY *d, XEvent *xevent)
{
   ALLEGRO_EVENT_SOURCE *es = &d->es;
   (void)xevent;

   _al_event_source_lock(es);

   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT event;
      event.display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
      event.display.timestamp = al_get_time();
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}

static void process_x11_event(ALLEGRO_SYSTEM_XGLX *s, XEvent event)
{
   unsigned int i;
   ALLEGRO_DISPLAY_XGLX *d = NULL;

   /* With many windows, it's bad to loop through them all, but typically
    * we have one or at most two or so.
    */
   for (i = 0; i < _al_vector_size(&s->system.displays); i++) {
      ALLEGRO_DISPLAY_XGLX **dptr = _al_vector_ref(&s->system.displays, i);
      d = *dptr;
      if (d->window == event.xany.window) {
         break;
      }
   }

   if (!d) {
      /* The display was probably destroyed already. */
      return;
   }

   switch (event.type) {
      case KeyPress:
         _al_xwin_keyboard_handler(&event.xkey, &d->display);
         break;
      case KeyRelease:
         _al_xwin_keyboard_handler(&event.xkey, &d->display);
         break;
      case MotionNotify:
         _al_xwin_mouse_motion_notify_handler(
            event.xmotion.x, event.xmotion.y, &d->display);
         break;
      case ButtonPress:
         _al_xwin_mouse_button_press_handler(event.xbutton.button,
            &d->display);
         break;
      case ButtonRelease:
         _al_xwin_mouse_button_release_handler(event.xbutton.button,
            &d->display);
         break;
      case ClientMessage:
         if (event.xclient.message_type == s->AllegroAtom) {
            d->mouse_warp = true;
            break;
         }
         if ((Atom)event.xclient.data.l[0] == d->wm_delete_window_atom) {
            _al_display_xglx_closebutton(&d->display, &event);
            break;
         }
         break;
      case EnterNotify:
         _al_xwin_mouse_switch_handler(&d->display, &event.xcrossing);
         break;
      case LeaveNotify:
         _al_xwin_mouse_switch_handler(&d->display, &event.xcrossing);
         break;
      case FocusIn:
         _al_xwin_display_switch_handler(&d->display, &event.xfocus);
         _al_xwin_keyboard_switch_handler(&d->display, true);
         break;
      case FocusOut:
         _al_xwin_display_switch_handler(&d->display, &event.xfocus);
         _al_xwin_keyboard_switch_handler(&d->display, false);
         break;
      case ConfigureNotify:
         _al_xglx_display_configure_event(&d->display,  &event);
         d->resize_count++;
         _al_cond_signal(&s->resized);
         break;
      case MapNotify:
         d->display.flags &= ~ALLEGRO_MINIMIZED;
         d->is_mapped = true;
         _al_cond_signal(&d->mapped);
         break;
      case UnmapNotify:
         d->display.flags |= ALLEGRO_MINIMIZED;
         break;
      case Expose:
         if (d->display.flags & ALLEGRO_GENERATE_EXPOSE_EVENTS) {
            _al_xwin_display_expose(&d->display, &event.xexpose);
         }
         break;
      default:
         _al_xglx_handle_mmon_event(s, d, &event);
         break;
   }
}

void _al_xwin_background_thread(_AL_THREAD *self, void *arg)
{
   ALLEGRO_SYSTEM_XGLX *s = arg;
   XEvent event;
   double last_reset_screensaver_time = 0.0;

   while (!_al_get_thread_should_stop(self)) {
      /* Note:
       * Most older X11 implementations are not thread-safe no matter what, so
       * we simply cannot sit inside a blocking XNextEvent from another thread
       * if another thread also uses X11 functions.
       * 
       * The usual use of XNextEvent is to only call it from the main thread. We
       * could of course do this for A5, just needs some slight adjustments to
       * the events system (polling for an Allegro event would call a function
       * of the system driver).
       * 
       * As an alternative, we can use locking. This however can never fully
       * work, as for example OpenGL implementations also will access X11, in a
       * way we cannot know and cannot control (and we can't require users to
       * only call graphics functions inside a lock).
       * 
       * However, most X11 implementations are somewhat thread safe, and do
       * use locking quite a bit themselves, so locking mostly does work.
       * 
       * (Yet another alternative might be to use a separate X11 display
       * connection for graphics output.)
       *
       */

      _al_mutex_lock(&s->lock);

      while (XEventsQueued(s->x11display, QueuedAfterFlush)) {
         XNextEvent(s->x11display, &event);
         process_x11_event(s, event);
      }

      /* The Xlib manual is particularly useless about the XResetScreenSaver()
       * function.  Nevertheless, this does seem to work to inhibit the native
       * screensaver facility.  Probably it won't do anything for other
       * systems, though.
       */
      if (s->inhibit_screensaver) {
         double now = al_get_time();
         if (now - last_reset_screensaver_time > 10.0) {
            XResetScreenSaver(s->x11display);
            last_reset_screensaver_time = now;
         }
      }

      _al_mutex_unlock(&s->lock);

      /* If no X11 events are there, unlock so other threads can run. We use
       * a select call to wake up when as soon as anything is available on
       * the X11 connection - and just for safety also wake up 10 times
       * a second regardless.
       */
      int x11_fd = ConnectionNumber(s->x11display);
      fd_set fdset;
      FD_ZERO(&fdset);
      FD_SET(x11_fd, &fdset);
      struct timeval small_time = {0, 100000}; /* 10 times a second */
      select(x11_fd + 1, &fdset, NULL, NULL, &small_time);
   }
}

