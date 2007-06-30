/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */

#include "xdummy.h"

static AL_SYSTEM_INTERFACE *vt;

static void background_thread(_AL_THREAD *thread, void *arg)
{
   AL_SYSTEM_XDUMMY *s = arg;
   XEvent event;
   unsigned int i;

   while (1) {
      AL_DISPLAY_XDUMMY *d = NULL;
      XNextEvent(s->xdisplay, &event);

      _al_mutex_lock(&s->lock);

      // FIXME: With many windows, it's bad to loop through them all,
      // maybe can come up with a better system here.
      // TODO: am I supposed to access ._size?
      for (i = 0; i < s->system.displays._size; i++) {
         AL_DISPLAY_XDUMMY **dptr = _al_vector_ref(&s->system.displays, i);
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
         case ConfigureNotify:
            _al_display_xdummy_configure(&d->display,  &event);
            _al_cond_signal(&s->resized);
            break;
         case MapNotify:
            _al_cond_signal(&s->mapped);
            break;
         case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == d->wm_delete_window_atom) {
               _al_display_xdummy_closebutton(&d->display, &event);
               break;
            }
      }

      _al_mutex_unlock(&s->lock);
   }
}

/* Create a new system object for the dummy X11 driver. */
static AL_SYSTEM *initialize(int flags)
{
   AL_SYSTEM_XDUMMY *s = _AL_MALLOC(sizeof *s);
   memset(s, 0, sizeof *s);

   _al_mutex_init(&s->lock);
   _al_cond_init(&s->mapped);
   _al_cond_init(&s->resized);

   _al_vector_init(&s->system.displays, sizeof (AL_SYSTEM_XDUMMY *));

   XInitThreads();

   s->system.vt = vt;

   /* Get an X11 display handle. */
   s->xdisplay = XOpenDisplay(0);

   TRACE("xsystem: XDummy driver connected to X11.\n");

   _al_thread_create(&s->thread, background_thread, s);

   TRACE("events thread spawned.\n");

   _al_xdummy_store_video_mode(s);

   return &s->system;
}

static void shutdown_system(void)
{
   TRACE("shutting down.\n");
   /* Close all open displays. */
   AL_SYSTEM *s = al_system_driver();
   while (s->displays._size)
   {
      AL_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      AL_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
}

// FIXME: This is just for now, the real way is of course a list of
// available display drivers. Possibly such drivers can be attached at runtime
// to the system driver, so addons could provide additional drivers.
AL_DISPLAY_INTERFACE *get_display_driver(void)
{
    return _al_display_xdummy_driver();
}

// FIXME: Use the list.
AL_KEYBOARD_DRIVER *get_keyboard_driver(void)
{
   // FIXME: i would prefer a dynamic way to list drivers, not a static list
   return _al_xwin_keyboard_driver_list[0].driver;
}

/* Internal function to get a reference to this driver. */
AL_SYSTEM_INTERFACE *_al_system_xdummy_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = initialize;
   vt->get_display_driver = get_display_driver;
   vt->get_keyboard_driver = get_keyboard_driver;
   vt->get_num_display_modes = _al_xdummy_get_num_display_modes;
   vt->get_display_mode = _al_xdummy_get_display_mode;
   vt->shutdown_system = shutdown_system;
   
   return vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces()
{
   AL_SYSTEM_INTERFACE **add;

#if defined ALLEGRO_UNIX
   /* This is the only system driver right now */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_xdummy_driver();
#endif
}
