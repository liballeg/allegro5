#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xwindow.h"

ALLEGRO_DEBUG_CHANNEL("xwindow")

void _al_xwin_set_size_hints(ALLEGRO_DISPLAY *d, int x_off, int y_off)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   XSizeHints *hints = XAllocSizeHints();
   hints->flags = 0;

   /* Do not force the size of the window on resizeable or fullscreen windows */
   /* on fullscreen windows, it confuses most X Window Managers */
   if (!(d->flags & ALLEGRO_RESIZABLE) && !(d->flags & ALLEGRO_FULLSCREEN)) {
      hints->flags |= PMinSize | PMaxSize | PBaseSize;
      hints->min_width  = hints->max_width  = hints->base_width  = d->w;
      hints->min_height = hints->max_height = hints->base_height = d->h;
   }

   // Tell WMs to respect our chosen position, otherwise the x_off/y_off
   // positions passed to XCreateWindow will be ignored by most WMs.
   if (x_off != INT_MAX && y_off != INT_MAX) {
      ALLEGRO_DEBUG("Force window position to %d, %d.\n", x_off, y_off);
      hints->flags |= PPosition;
      hints->x = x_off;
      hints->y = y_off;
   }

   if (d->flags & ALLEGRO_FULLSCREEN) {
      /* kwin will improperly layer a panel over our window on a second display without this.
       * some other Size flags may cause glitches with various WMs, but this seems to be ok
       * with metacity and kwin. As noted in xdpy_create_display, compiz is just broken.
       */
      hints->flags |= PBaseSize;
      hints->base_width  = d->w;
      hints->base_height = d->h;
   }

   XSetWMNormalHints(system->x11display, glx->window, hints);

   XFree(hints);
}

void _al_xwin_reset_size_hints(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   XSizeHints *hints = XAllocSizeHints();

   hints->flags = PMinSize | PMaxSize;
   hints->min_width  = 0;
   hints->min_height = 0;
   // FIXME: Is there a way to remove/reset max dimensions?
   hints->max_width  = 32768;
   hints->max_height = 32768;
   XSetWMNormalHints(system->x11display, glx->window, hints);

   XFree(hints);
}

#define X11_ATOM(x)  XInternAtom(x11, #x, False);

/* Note: The system mutex must be locked (exactly once) before
 * calling this as we call _al_display_xglx_await_resize.
 */
void _al_xwin_set_fullscreen_window(ALLEGRO_DISPLAY *display, int value)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
#ifndef ALLEGRO_RASPBERRYPI
   int old_resize_count = glx->resize_count;
#endif

   ALLEGRO_DEBUG("Toggling _NET_WM_STATE_FULLSCREEN hint: %d\n", value);

   XEvent xev;
   xev.xclient.type = ClientMessage;
   xev.xclient.serial = 0;
   xev.xclient.send_event = True;
   xev.xclient.message_type = X11_ATOM(_NET_WM_STATE);
   xev.xclient.window = glx->window;
   xev.xclient.format = 32;

   // Note: It seems 0 is not reliable except when mapping a window -
   // 2 is all we need though.
   xev.xclient.data.l[0] = value; /* 0 = off, 1 = on, 2 = toggle */

   xev.xclient.data.l[1] = X11_ATOM(_NET_WM_STATE_FULLSCREEN);
   xev.xclient.data.l[2] = 0;
   xev.xclient.data.l[3] = 0;
   xev.xclient.data.l[4] = 1;

   XSendEvent(
      x11,
#ifndef ALLEGRO_RASPBERRYPI
      RootWindowOfScreen(ScreenOfDisplay(x11, glx->xscreen)),
#else
      RootWindowOfScreen(ScreenOfDisplay(x11, DefaultScreen(x11))),
#endif
      False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);

#ifndef ALLEGRO_RASPBERRYPI
   if (value == 2) {
      /* Only wait for a resize if toggling. */
      _al_display_xglx_await_resize(display, old_resize_count, true);
   }
#endif
}

