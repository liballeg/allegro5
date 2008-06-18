/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */

#ifdef DEBUG_X11
extern int _Xdebug; /* part of Xlib */
#endif

#include "xglx.h"

static ALLEGRO_SYSTEM_INTERFACE *xglx_vt;

static void xglx_background_thread(_AL_THREAD *thread, void *arg)
{
   ALLEGRO_SYSTEM_XGLX *s = arg;
   XEvent event;
   unsigned int i;

   while (1) {
      ALLEGRO_DISPLAY_XGLX *d = NULL;
      XNextEvent(s->xdisplay, &event);

      _al_mutex_lock(&s->lock);

      // FIXME: With many windows, it's bad to loop through them all,
      // maybe can come up with a better system here.
      for (i = 0; i < _al_vector_size(&s->system.displays); i++) {
         ALLEGRO_DISPLAY_XGLX **dptr = _al_vector_ref(&s->system.displays, i);
         d = *dptr;
         if (d->window == event.xany.window) {
            break;
         }
      }

      switch (event.type) {
         case KeyPress:
            _al_xwin_keyboard_handler(&event.xkey, false);
            break;
         case KeyRelease:
            _al_xwin_keyboard_handler(&event.xkey, false);
            break;
         case ButtonPress:
            _al_xwin_mouse_button_press_handler(event.xbutton.button);
            break;
         case ButtonRelease:
            _al_xwin_mouse_button_release_handler(event.xbutton.button);
            break;
         case MotionNotify:
            _al_xwin_mouse_motion_notify_handler(
               event.xmotion.x, event.xmotion.y);
            break;
         case ConfigureNotify:
            _al_display_xglx_configure(&d->ogl_display.display,  &event);
            _al_cond_signal(&s->resized);
            break;
         case MapNotify:
            _al_cond_signal(&s->mapped);
            break;
         case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == d->wm_delete_window_atom) {
               _al_display_xglx_closebutton(&d->ogl_display.display, &event);
               break;
            }
      }

      _al_mutex_unlock(&s->lock);
   }
}

/* Create a new system object for the dummy X11 driver. */
static ALLEGRO_SYSTEM *xglx_initialize(int flags)
{
   ALLEGRO_SYSTEM_XGLX *s = _AL_MALLOC(sizeof *s);
   memset(s, 0, sizeof *s);

   #ifdef DEBUG_X11
   _Xdebug = 1;
   #endif

   _al_mutex_init(&s->lock);
   _al_cond_init(&s->mapped);
   _al_cond_init(&s->resized);

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_SYSTEM_XGLX *));

   XInitThreads();

   s->system.vt = xglx_vt;

   /* Get an X11 display handle. */
   s->xdisplay = XOpenDisplay(0);

   TRACE("xsystem: XGLX driver connected to X11 (%sys %d).\n",
      ServerVendor(s->xdisplay), VendorRelease(s->xdisplay));
   TRACE("xsystem: X11 protocol version %d.%d.\n",
      ProtocolVersion(s->xdisplay), ProtocolRevision(s->xdisplay));

   _al_thread_create(&s->thread, xglx_background_thread, s);

   TRACE("xsystem: events thread spawned.\n");

   _al_xglx_store_video_mode(s);

   return &s->system;
}

static void xglx_shutdown_system(void)
{
   TRACE("shutting down.\n");
   /* Close all open displays. */
   ALLEGRO_SYSTEM *s = al_system_driver();
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }
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
   xglx_vt->get_num_display_modes = _al_xglx_get_num_display_modes;
   xglx_vt->get_display_mode = _al_xglx_get_display_mode;
   xglx_vt->shutdown_system = xglx_shutdown_system;
   xglx_vt->get_path = _unix_get_path;

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
