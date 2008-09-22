/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */

#ifdef DEBUG_X11
extern int _Xdebug; /* part of Xlib */
#endif

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"
#include "xglx.h"

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
         _al_xwin_keyboard_handler(&event.xkey, false,
            &d->display);
         break;
      case KeyRelease:
         _al_xwin_keyboard_handler(&event.xkey, false,
            &d->display);
         break;
      case FocusIn:
         //_al_xwin_keyboard_focus_handler(&event.xfocus);
         _al_xwin_display_switch_handler(&d->display, &event.xfocus);
         break;
      case FocusOut:
         //_al_xwin_keyboard_focus_handler(&event.xfocus);
         _al_xwin_display_switch_handler(&d->display, &event.xfocus);
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
         _al_cond_signal(&s->resized);
         break;
      case MapNotify:
         _al_cond_signal(&s->mapped);
         break;
      case Expose:
         if (d->display.flags & ALLEGRO_GENERATE_EXPOSE_EVENTS) {
            _al_xwin_display_expose(&d->display, &event.xexpose);
         }
         break;
      case ClientMessage:
         if ((Atom)event.xclient.data.l[0] == d->wm_delete_window_atom) {
            _al_display_xglx_closebutton(&d->display, &event);
            break;
         }
   }
}

static void xglx_background_thread(_AL_THREAD *self, void *arg)
{
   ALLEGRO_SYSTEM_XGLX *s = arg;
   XEvent event;

   while (!_al_thread_should_stop(self)) {
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

   #ifdef DEBUG_X11
   _Xdebug = 1;
   #endif

   XInitThreads();

   /* Get an X11 display handle. */
   x11display = XOpenDisplay(0);
   if (!x11display) {
      TRACE("xsystem: XOpenDisplay failed.\n");
      return NULL;
   }

   /* Never ask. */
   gfxdisplay = XOpenDisplay(0);
   if (!gfxdisplay) {
      TRACE("xsystem: XOpenDisplay failed.\n");
      XCloseDisplay(x11display);
      return NULL;
   }

   _al_unix_init_time();

   s = _AL_MALLOC(sizeof *s);
   memset(s, 0, sizeof *s);

   _al_mutex_init_recursive(&s->lock);
   _al_cond_init(&s->mapped);
   _al_cond_init(&s->resized);

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_SYSTEM_XGLX *));

   s->gfxdisplay = gfxdisplay;
   s->x11display = x11display;

   s->system.vt = xglx_vt;

   TRACE("xsystem: XGLX driver connected to X11 (%sys %d).\n",
      ServerVendor(s->x11display), VendorRelease(s->x11display));
   TRACE("xsystem: X11 protocol version %d.%d.\n",
      ProtocolVersion(s->x11display), ProtocolRevision(s->x11display));

   _al_xglx_store_video_mode(s);
   
   _al_thread_create(&s->thread, xglx_background_thread, s);

   TRACE("xsystem: events thread spawned.\n");

   return &s->system;
}

static void xglx_shutdown_system(void)
{
   /* Close all open displays. */
   ALLEGRO_SYSTEM *s = al_system_driver();
   ALLEGRO_SYSTEM_XGLX *sx = (void *)s;

   TRACE("shutting down.\n");

   _al_thread_join(&sx->thread);

   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);

   _al_xglx_free_mode_infos(sx);

   if (sx->x11display) {
      XCloseDisplay(sx->x11display);
      sx->x11display = None;
   }

   if (sx->gfxdisplay) {
      /* XXX for some reason, crashes if both XCloseDisplay calls are made */
      /* XCloseDisplay(sx->gfxdisplay); */
      sx->gfxdisplay = None;
   }

   _AL_FREE(sx);
}

// FIXME: This is just for now, the real way is of course a list of
// available display drivers. Possibly such drivers can be attached at runtime
// to the system driver, so addons could provide additional drivers.
static ALLEGRO_DISPLAY_INTERFACE *xglx_get_display_driver(void)
{
   return _al_display_xglx_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *xglx_get_keyboard_driver(void)
{
   // FIXME: Select the best driver somehow
   return _al_xwin_keyboard_driver_list[0].driver;
}

static ALLEGRO_MOUSE_DRIVER *xglx_get_mouse_driver(void)
{
   // FIXME: Select the best driver somehow
   return _al_xwin_mouse_driver_list[0].driver;
}

static ALLEGRO_JOYSTICK_DRIVER *xglx_get_joystick_driver(void)
{
   return _al_joystick_driver_list[0].driver;
}

// FIXME: Implement.
static int xglx_get_num_video_adapters(void)
{
   return 1;
}

// FIXME: Implement. Right now it just reads the root window size and ignores
// the adapter number.
static void xglx_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   XWindowAttributes xwa;
   Window root = RootWindow(system->x11display, 0);
   _al_mutex_lock(&system->lock);
   XGetWindowAttributes(system->x11display, root, &xwa);
   _al_mutex_unlock(&system->lock);
   
   info->x1 = 0;
   info->y1 = 0;
   info->x2 = xwa.width;
   info->y2 = xwa.height;
}

static bool xglx_get_cursor_position(int *ret_x, int *ret_y)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
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

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_xglx_driver(void)
{
   if (xglx_vt) return xglx_vt;

   xglx_vt = _AL_MALLOC(sizeof *xglx_vt);
   memset(xglx_vt, 0, sizeof *xglx_vt);

   xglx_vt->initialize = xglx_initialize;
   xglx_vt->get_display_driver = xglx_get_display_driver;
   xglx_vt->get_keyboard_driver = xglx_get_keyboard_driver;
   xglx_vt->get_mouse_driver = xglx_get_mouse_driver;
   xglx_vt->get_joystick_driver = xglx_get_joystick_driver;
   xglx_vt->get_num_display_modes = _al_xglx_get_num_display_modes;
   xglx_vt->get_display_mode = _al_xglx_get_display_mode;
   xglx_vt->shutdown_system = xglx_shutdown_system;
   xglx_vt->get_num_video_adapters = xglx_get_num_video_adapters;
   xglx_vt->get_monitor_info = xglx_get_monitor_info;
   xglx_vt->get_cursor_position = xglx_get_cursor_position;
   
   return xglx_vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces(void)
{
   ALLEGRO_SYSTEM_INTERFACE **add;

#if defined ALLEGRO_UNIX
   /* This is the only system driver right now */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_xglx_driver();
#endif
}

/* vim: set sts=3 sw=3 et: */
