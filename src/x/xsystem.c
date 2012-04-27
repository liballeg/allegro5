/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */

#ifdef DEBUG_X11
extern int _Xdebug; /* part of Xlib */
#endif

#include <sys/time.h>

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_xembed.h"
#include "allegro5/internal/aintern_xglx.h"

ALLEGRO_DEBUG_CHANNEL("system")

static ALLEGRO_SYSTEM_INTERFACE *xglx_vt;

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
      case ButtonPress:
         _al_xwin_mouse_button_press_handler(event.xbutton.button,
            &d->display);
         break;
      case ButtonRelease:
         _al_xwin_mouse_button_release_handler(event.xbutton.button,
            &d->display);
         break;
      case MotionNotify:
         _al_xwin_mouse_motion_notify_handler(
            event.xmotion.x, event.xmotion.y, &d->display);
         break;
      case ConfigureNotify:
         _al_display_xglx_configure(&d->display,  &event);
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
      case ReparentNotify:
         if (event.xreparent.parent == RootWindow(s->x11display, d->xscreen)) {
            ALLEGRO_INFO("XEmbed protocol finished.\n");
            d->embedder_window = None;
         }
         break;
      case ClientMessage:
         if ((Atom)event.xclient.data.l[0] == d->wm_delete_window_atom) {
            _al_display_xglx_closebutton(&d->display, &event);
            break;
         }
         if (event.xclient.message_type == s->AllegroAtom) {
            d->mouse_warp = true;
            break;
         }
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
                  ALLEGRO_INFO("XEmbed begin: embedder window = %ld\n", data1);
                  break;
               case XEMBED_FOCUS_IN:
                  ALLEGRO_DEBUG("XEmbed focus in\n");
                  _al_xwin_display_switch_handler_inner(&d->display, true);
                  _al_xwin_keyboard_switch_handler(&d->display, true);
                  break;
               case XEMBED_FOCUS_OUT:
                  ALLEGRO_DEBUG("XEmbed focus out\n");
                  _al_xwin_display_switch_handler_inner(&d->display, false);
                  _al_xwin_keyboard_switch_handler(&d->display, false);
                  break;
            }
            break;
         }
         break;
         
      default:
         _al_xglx_handle_mmon_event(s, d, &event);
         break;
   }
}

static void xglx_background_thread(_AL_THREAD *self, void *arg)
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

/* Create a new system object for the dummy X11 driver. */
static ALLEGRO_SYSTEM *xglx_initialize(int flags)
{
   Display *x11display;
   Display *gfxdisplay;
   ALLEGRO_SYSTEM_XGLX *s;

   (void)flags;

#ifdef DEBUG_X11
   _Xdebug = 1;
#endif

   XInitThreads();

   /* Get an X11 display handle. */
   x11display = XOpenDisplay(0);
   if (x11display) {
      /* Never ask. */
      gfxdisplay = XOpenDisplay(0);
      if (!gfxdisplay) {
         ALLEGRO_ERROR("XOpenDisplay failed second time.\n");
         XCloseDisplay(x11display);
         return NULL;
      }
   }
   else {
      ALLEGRO_INFO("XOpenDisplay failed; assuming headless mode.\n");
      gfxdisplay = NULL;
   }

   _al_unix_init_time();

   s = al_calloc(1, sizeof *s);

   _al_mutex_init_recursive(&s->lock);
   _al_cond_init(&s->resized);
   s->inhibit_screensaver = false;

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_DISPLAY_XGLX *));

   s->gfxdisplay = gfxdisplay;
   s->x11display = x11display;

   s->system.vt = xglx_vt;

   if (s->x11display) {
      ALLEGRO_INFO("XGLX driver connected to X11 (%s %d).\n",
         ServerVendor(s->x11display), VendorRelease(s->x11display));
      ALLEGRO_INFO("X11 protocol version %d.%d.\n",
         ProtocolVersion(s->x11display), ProtocolRevision(s->x11display));

      /* We need to put *some* atom into the ClientMessage we send for
       * faking mouse movements with al_set_mouse_xy - so let's ask X11
       * for one here.
       */
      s->AllegroAtom = XInternAtom(x11display, "AllegroAtom", False);

      /* Message type for XEmbed protocol. */
      s->XEmbedAtom = XInternAtom(x11display, "_XEMBED", False);

      _al_thread_create(&s->thread, xglx_background_thread, s);
      ALLEGRO_INFO("events thread spawned.\n");
   }

   return &s->system;
}

static void xglx_shutdown_system(void)
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
   ALLEGRO_SYSTEM_XGLX *sx = (void *)s;

   ALLEGRO_INFO("shutting down.\n");

   if (sx->x11display) {
      /* Events thread only runs if we are connected to an X server. */
      _al_thread_join(&sx->thread);
   }

   /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);

   // Makes sure we wait for any commands sent to the X server when destroying the displays.
   // Should make sure we don't shutdown before modes are restored.
   if (sx->x11display) {
      XSync(sx->x11display, False);
   }
   
   _al_xsys_mmon_exit(sx);

   if (sx->x11display) {
      XCloseDisplay(sx->x11display);
      sx->x11display = None;
      ALLEGRO_DEBUG("xsys: close x11display.\n");
   }

   if (sx->gfxdisplay) {
      /* XXX for some reason, crashes if both XCloseDisplay calls are made */
      /* XCloseDisplay(sx->gfxdisplay); */
      sx->gfxdisplay = None;
   }

   al_free(sx);
}

static ALLEGRO_DISPLAY_INTERFACE *xglx_get_display_driver(void)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   /* Look up the toggle_mouse_grab_key binding.  This isn't such a great place
    * to do it, but the config file is not available until after the system driver
    * is initialised.
    */
   if (system->system.config && !system->toggle_mouse_grab_keycode) {
      const char *binding = al_get_config_value(system->system.config,
         "keyboard", "toggle_mouse_grab_key");
      if (binding) {
         system->toggle_mouse_grab_keycode = _al_parse_key_binding(binding,
            &system->toggle_mouse_grab_modifiers);
         if (system->toggle_mouse_grab_keycode) {
            ALLEGRO_DEBUG("Toggle mouse grab key: '%s'\n", binding);
         }
         else {
            ALLEGRO_WARN("Cannot parse key binding '%s'\n", binding);
         }
      }
   }

   return _al_display_xglx_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *xglx_get_keyboard_driver(void)
{
   return _al_xwin_keyboard_driver();
}

static ALLEGRO_MOUSE_DRIVER *xglx_get_mouse_driver(void)
{
   return _al_xwin_mouse_driver();
}

static ALLEGRO_JOYSTICK_DRIVER *xglx_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

static int xglx_get_num_video_adapters(void)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   return _al_xglx_get_num_video_adapters(system);
}

static bool xglx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   return _al_xglx_get_monitor_info(system, adapter, info);
}

static bool xglx_get_cursor_position(int *ret_x, int *ret_y)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Window root = RootWindow(system->x11display, 0);
   Window child;
   int wx, wy;
   unsigned int mask;

   _al_mutex_lock(&system->lock);
   XQueryPointer(system->x11display, root, &root, &child, ret_x, ret_y,
      &wx, &wy, &mask);
   _al_mutex_unlock(&system->lock);
   return true;
}

static bool xglx_grab_mouse(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   int grab;
   bool ret;

   _al_mutex_lock(&system->lock);
   grab = XGrabPointer(system->x11display, glx->window, False,
      PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
      GrabModeAsync, GrabModeAsync, glx->window, None, CurrentTime);
   if (grab == GrabSuccess) {
      system->mouse_grab_display = display;
      ret = true;
   }
   else {
      ret = false;
   }
   _al_mutex_unlock(&system->lock);
   return ret;
}

static bool xglx_ungrab_mouse(void)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   _al_mutex_lock(&system->lock);
   XUngrabPointer(system->x11display, CurrentTime);
   system->mouse_grab_display = NULL;
   _al_mutex_unlock(&system->lock);
   return true;
}

static bool xglx_inhibit_screensaver(bool inhibit)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();

   system->inhibit_screensaver = inhibit;
   return true;
}

static int xglx_get_num_display_modes(void)
{
   int adapter = al_get_new_display_adapter();
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   return _al_xglx_get_num_display_modes(s, adapter);
}

static ALLEGRO_DISPLAY_MODE *xglx_get_display_mode(int mode, ALLEGRO_DISPLAY_MODE *dm)
{
   int adapter = al_get_new_display_adapter();
   ALLEGRO_SYSTEM_XGLX *s = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();

   return _al_xglx_get_display_mode(s, adapter, mode, dm);
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_xglx_driver(void)
{
   if (xglx_vt)
      return xglx_vt;

   xglx_vt = al_calloc(1, sizeof *xglx_vt);

   xglx_vt->initialize = xglx_initialize;
   xglx_vt->get_display_driver = xglx_get_display_driver;
   xglx_vt->get_keyboard_driver = xglx_get_keyboard_driver;
   xglx_vt->get_mouse_driver = xglx_get_mouse_driver;
   xglx_vt->get_joystick_driver = xglx_get_joystick_driver;
   xglx_vt->get_num_display_modes = xglx_get_num_display_modes;
   xglx_vt->get_display_mode = xglx_get_display_mode;
   xglx_vt->shutdown_system = xglx_shutdown_system;
   xglx_vt->get_num_video_adapters = xglx_get_num_video_adapters;
   xglx_vt->get_monitor_info = xglx_get_monitor_info;
   xglx_vt->create_mouse_cursor = _al_xwin_create_mouse_cursor;
   xglx_vt->destroy_mouse_cursor = _al_xwin_destroy_mouse_cursor;
   xglx_vt->get_cursor_position = xglx_get_cursor_position;
   xglx_vt->grab_mouse = xglx_grab_mouse;
   xglx_vt->ungrab_mouse = xglx_ungrab_mouse;
   xglx_vt->get_path = _al_unix_get_path;
   xglx_vt->inhibit_screensaver = xglx_inhibit_screensaver;

   return xglx_vt;
}


/* vim: set sts=3 sw=3 et: */
