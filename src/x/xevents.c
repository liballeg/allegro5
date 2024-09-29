#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xclipboard.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xembed.h"
#include "allegro5/internal/aintern_xevents.h"
#include "allegro5/internal/aintern_xfullscreen.h"
#include "allegro5/internal/aintern_xkeyboard.h"
#include "allegro5/internal/aintern_xmouse.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xtouch.h"
#include "allegro5/internal/aintern_xdnd.h"

#ifdef A5O_RASPBERRYPI
#include "allegro5/internal/aintern_raspberrypi.h"
#define A5O_SYSTEM_XGLX A5O_SYSTEM_RASPBERRYPI
#define A5O_DISPLAY_XGLX A5O_DISPLAY_RASPBERRYPI
#endif

A5O_DEBUG_CHANNEL("xevents")

/* Handle an X11 close button event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
void _al_display_xglx_closebutton(A5O_DISPLAY *d, XEvent *xevent)
{
   A5O_EVENT_SOURCE *es = &d->es;
   (void)xevent;

   _al_event_source_lock(es);
   if (_al_event_source_needs_to_generate_event(es)) {
      A5O_EVENT event;
      event.display.type = A5O_EVENT_DISPLAY_CLOSE;
      event.display.timestamp = al_get_time();
      _al_event_source_emit_event(es, &event);
   }
   _al_event_source_unlock(es);
}

static void process_x11_event(A5O_SYSTEM_XGLX *s, XEvent event)
{
   unsigned int i;
   A5O_DISPLAY_XGLX *d = NULL;

   /* With many windows, it's bad to loop through them all, but typically
    * we have one or at most two or so.
    */
   for (i = 0; i < _al_vector_size(&s->system.displays); i++) {
      A5O_DISPLAY_XGLX **dptr = _al_vector_ref(&s->system.displays, i);
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
         if (d->wm_delete_window_atom != None &&
            (Atom)event.xclient.data.l[0] == d->wm_delete_window_atom)
         {
            _al_display_xglx_closebutton(&d->display, &event);
            break;
         }
         if (_al_display_xglx_handle_drag_and_drop(s, d, &event)) {
            break;
         }
#ifndef A5O_RASPBERRYPI
         if (event.xclient.message_type == s->XEmbedAtom) {
            const long xtime = event.xclient.data.l[0];
            const long major = event.xclient.data.l[1];
            const long detail = event.xclient.data.l[2];
            const long data1 = event.xclient.data.l[3];
            const long data2 = event.xclient.data.l[4];

            (void)xtime;
            (void)detail;
            (void)data2;

            switch (major) {
               case XEMBED_EMBEDDED_NOTIFY:
                  d->embedder_window = data1;
                  A5O_INFO("XEmbed begin: embedder window = %ld\n", data1);
                  break;
               case XEMBED_FOCUS_IN:
                  A5O_DEBUG("XEmbed focus in\n");
                  _al_xwin_display_switch_handler_inner(&d->display, true);
                  _al_xwin_keyboard_switch_handler(&d->display, true);
                  break;
               case XEMBED_FOCUS_OUT:
                  A5O_DEBUG("XEmbed focus out\n");
                  _al_xwin_display_switch_handler_inner(&d->display, false);
                  _al_xwin_keyboard_switch_handler(&d->display, false);
                  break;
            }
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
         d->display.flags &= ~A5O_MINIMIZED;
         d->is_mapped = true;
         _al_cond_signal(&d->mapped);
         break;
      case UnmapNotify:
         d->display.flags |= A5O_MINIMIZED;
         break;
      case Expose:
         if (d->display.flags & A5O_GENERATE_EXPOSE_EVENTS) {
            _al_xwin_display_expose(&d->display, &event.xexpose);
         }
         break;
      case ReparentNotify:
         if (event.xreparent.parent == RootWindow(s->x11display, d->xscreen)) {
            A5O_INFO("XEmbed protocol finished.\n");
            d->embedder_window = None;
         }
         break;

      case SelectionNotify:
         if (_al_display_xglx_handle_drag_and_drop_selection(s, d, &event))
            break;
        _al_xwin_display_selection_notify(&d->display, &event.xselection);
        d->is_selectioned = true;
        _al_cond_signal(&d->selectioned);
        break;

      case SelectionRequest:
        _al_xwin_display_selection_request(&d->display, &event.xselectionrequest);
        break;

      default:
         _al_x_handle_touch_event(s, d, &event);
         _al_xglx_handle_mmon_event(s, d, &event);
         break;
#endif
   }
}

void _al_xwin_background_thread(_AL_THREAD *self, void *arg)
{
   A5O_SYSTEM_XGLX *s = arg;
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
      if (!s->screen_saver_query_available && s->inhibit_screensaver) {
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
