#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_x.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xsystem.h"
#include "allegro5/internal/aintern_xwindow.h"

#ifdef A5O_RASPBERRYPI
#include "allegro5/internal/aintern_raspberrypi.h"
#define A5O_SYSTEM_XGLX A5O_SYSTEM_RASPBERRYPI
#define A5O_DISPLAY_XGLX A5O_DISPLAY_RASPBERRYPI
#endif

A5O_DEBUG_CHANNEL("xwindow")

#define X11_ATOM(x)  XInternAtom(x11, #x, False);

void _al_xwin_set_size_hints(A5O_DISPLAY *d, int x_off, int y_off)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (void *)d;
   XSizeHints *sizehints;
   XWMHints *wmhints;
   XClassHint *classhints;
   int w, h;

   sizehints = XAllocSizeHints();
   sizehints->flags = 0;

#ifdef A5O_RASPBERRYPI
   int x, y;
   _al_raspberrypi_get_screen_info(&x, &y, &w, &h);
#else
   w = d->w;
   h = d->h;
#endif

   /* Do not force the size of the window on resizeable or fullscreen windows */
   /* on fullscreen windows, it confuses most X Window Managers */
   if (!(d->flags & A5O_RESIZABLE) && !(d->flags & A5O_FULLSCREEN)) {
      sizehints->flags |= PMinSize | PMaxSize | PBaseSize;
      sizehints->min_width  = sizehints->max_width  = sizehints->base_width  = w;
      sizehints->min_height = sizehints->max_height = sizehints->base_height = h;
   }

   /* Constrain the window if needed. */
   if (d->use_constraints && (d->flags & A5O_RESIZABLE) &&
      (d->min_w > 0 || d->min_h > 0 || d->max_w > 0 || d->max_h > 0))
   {
      sizehints->flags |= PMinSize | PMaxSize | PBaseSize;
      sizehints->min_width = (d->min_w > 0) ? d->min_w : 0;
      sizehints->min_height = (d->min_h > 0) ? d->min_h : 0;
      sizehints->max_width = (d->max_w > 0) ? d->max_w : INT_MAX;
      sizehints->max_height = (d->max_h > 0) ? d->max_h : INT_MAX;
      sizehints->base_width  = w;
      sizehints->base_height = h;
   }

   // Tell WMs to respect our chosen position, otherwise the x_off/y_off
   // positions passed to XCreateWindow will be ignored by most WMs.
   if (x_off != INT_MAX && y_off != INT_MAX) {
      A5O_DEBUG("Force window position to %d, %d.\n", x_off, y_off);
      sizehints->flags |= PPosition;
      sizehints->x = x_off;
      sizehints->y = y_off;
   }

   if (d->flags & A5O_FULLSCREEN) {
      /* kwin will improperly layer a panel over our window on a second display without this.
       * some other Size flags may cause glitches with various WMs, but this seems to be ok
       * with metacity and kwin. As noted in xdpy_create_display, compiz is just broken.
       */
      sizehints->flags |= PBaseSize;
      sizehints->base_width  = w;
      sizehints->base_height = h;
   }

   /* Setup the input hints so we get keyboard input */
   wmhints = XAllocWMHints();
   wmhints->input = True;
   wmhints->flags = InputHint;
   
   A5O_PATH *exepath = al_get_standard_path(A5O_EXENAME_PATH);
   
   /* Setup the class hints so we can get an icon (AfterStep)
    * We must use the executable filename here.
    */
   classhints = XAllocClassHint();
   classhints->res_name = strdup(al_get_path_basename(exepath));
   classhints->res_class = strdup(al_get_path_basename(exepath));

   /* Set the size, input and class hints, and define WM_CLIENT_MACHINE and WM_LOCALE_NAME */
   XSetWMProperties(system->x11display, glx->window, NULL, NULL, NULL, 0,
                    sizehints, wmhints, classhints);

   free(classhints->res_name);
   free(classhints->res_class);
   XFree(sizehints);
   XFree(wmhints);
   XFree(classhints);

   al_destroy_path(exepath);
}


void _al_xwin_reset_size_hints(A5O_DISPLAY *d)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (void *)d;
   XSizeHints *hints;

   hints  = XAllocSizeHints();
   hints->flags = PMinSize | PMaxSize;
   hints->min_width  = 0;
   hints->min_height = 0;
   // FIXME: Is there a way to remove/reset max dimensions?
   hints->max_width  = 32768;
   hints->max_height = 32768;
   XSetWMNormalHints(system->x11display, glx->window, hints);

   XFree(hints);
}


/* Note: The system mutex must be locked (exactly once) before
 * calling this as we call _al_display_xglx_await_resize.
 */
void _al_xwin_set_fullscreen_window(A5O_DISPLAY *display, int value)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
#ifndef A5O_RASPBERRYPI
   int old_resize_count = glx->resize_count;
#endif

   A5O_DEBUG("Toggling _NET_WM_STATE_FULLSCREEN hint: %d\n", value);

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
   xev.xclient.data.l[3] = 1;  /* 1 = normal application source */
   xev.xclient.data.l[4] = 0;

   XSendEvent(
      x11,
#if !defined A5O_RASPBERRYPI
      RootWindowOfScreen(ScreenOfDisplay(x11, glx->xscreen)),
#else
      RootWindowOfScreen(ScreenOfDisplay(x11, DefaultScreen(x11))),
#endif
      False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);

#if !defined A5O_RASPBERRYPI
   if (value == 2) {
      /* Only wait for a resize if toggling. */
      _al_display_xglx_await_resize(display, old_resize_count, true);
   }
#endif
}


void _al_xwin_set_above(A5O_DISPLAY *display, int value)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;

   A5O_DEBUG("Toggling _NET_WM_STATE_ABOVE hint: %d\n", value);

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

   xev.xclient.data.l[1] = X11_ATOM(_NET_WM_STATE_ABOVE);
   xev.xclient.data.l[2] = 0;
   xev.xclient.data.l[3] = 0;
   xev.xclient.data.l[4] = 1;

   XSendEvent(x11, DefaultRootWindow(x11), False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);
}


void _al_xwin_set_frame(A5O_DISPLAY *display, bool frame_on)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
   Atom hints;

#if 1
   /* This code is taken from the GDK sources. So it works perfectly in Gnome,
    * no idea if it will work anywhere else. X11 documentation itself only
    * describes a way how to make the window completely unmanaged, but that
    * would also require special care in the event handler.
    */
   hints = XInternAtom(x11, "_MOTIF_WM_HINTS", True);
   if (hints) {
      struct {
         unsigned long flags;
         unsigned long functions;
         unsigned long decorations;
         long input_mode;
         unsigned long status;
      } motif = {2, 0, frame_on, 0, 0};
      XChangeProperty(x11, glx->window, hints, hints, 32, PropModeReplace,
         (void *)&motif, sizeof motif / 4);

      if (frame_on)
         display->flags &= ~A5O_FRAMELESS;
      else
         display->flags |= A5O_FRAMELESS;
   }
#endif
}


/* Helper to set a window icon.  We use the _NET_WM_ICON property which is
 * supported by modern window managers.
 *
 * The old method is XSetWMHints but the (antiquated) ICCCM talks about 1-bit
 * pixmaps.  For colour icons, perhaps you're supposed use the icon_window,
 * and draw the window yourself?
 */
static bool xdpy_set_icon_inner(Display *x11display, Window window,
   A5O_BITMAP *bitmap, int prop_mode)
{
   int w, h;
   int data_size;
   unsigned long *data; /* Yes, unsigned long, even on 64-bit platforms! */
   A5O_LOCKED_REGION *lr;
   bool ret;

   w = al_get_bitmap_width(bitmap);
   h = al_get_bitmap_height(bitmap);
   data_size = 2 + w * h;
   data = al_malloc(data_size * sizeof(data[0]));
   if (!data)
      return false;

   lr = al_lock_bitmap(bitmap, A5O_PIXEL_FORMAT_ANY_WITH_ALPHA,
      A5O_LOCK_READONLY);
   if (lr) {
      int x, y;
      A5O_COLOR c;
      unsigned char r, g, b, a;
      Atom _NET_WM_ICON;

      data[0] = w;
      data[1] = h;
      for (y = 0; y < h; y++) {
         for (x = 0; x < w; x++) {
            c = al_get_pixel(bitmap, x, y);
            al_unmap_rgba(c, &r, &g, &b, &a);
            data[2 + y*w + x] = ((unsigned long)a << 24) | ((unsigned long)r << 16) |
                                ((unsigned long)g << 8) | (unsigned long)b;
         }
      }

      _NET_WM_ICON = XInternAtom(x11display, "_NET_WM_ICON", False);
      XChangeProperty(x11display, window, _NET_WM_ICON, XA_CARDINAL, 32,
         prop_mode, (unsigned char *)data, data_size);

      al_unlock_bitmap(bitmap);
      ret = true;
   }
   else {
      ret = false;
   }

   al_free(data);

   return ret;
}


void _al_xwin_set_icons(A5O_DISPLAY *d,
   int num_icons, A5O_BITMAP *bitmaps[])
{
   A5O_SYSTEM_XGLX *system = (A5O_SYSTEM_XGLX *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)d;
   int prop_mode = PropModeReplace;
   int i;

   _al_mutex_lock(&system->lock);

   for (i = 0; i < num_icons; i++) {
      if (xdpy_set_icon_inner(system->x11display, glx->window, bitmaps[i],
            prop_mode)) {
         prop_mode = PropModeAppend;
      }
   }

   _al_mutex_unlock(&system->lock);
}


/* Note: The system mutex must be locked (exactly once) before
 * calling this as we call _al_display_xglx_await_resize.
 */
void _al_xwin_maximize(A5O_DISPLAY *display, bool maximized)
{
#ifndef A5O_RASPBERRYPI
   if (!!(display->flags & A5O_MAXIMIZED) == maximized)
      return;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
   int old_resize_count = glx->resize_count;

   XEvent xev;
   xev.xclient.type = ClientMessage;
   xev.xclient.serial = 0;
   xev.xclient.send_event = True;
   xev.xclient.message_type = X11_ATOM(_NET_WM_STATE);
   xev.xclient.window = glx->window;
   xev.xclient.format = 32;

   xev.xclient.data.l[0] = maximized ? 1 : 0;
   xev.xclient.data.l[1] = X11_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
   xev.xclient.data.l[2] = X11_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
   xev.xclient.data.l[3] = 0;

   XSendEvent(
      x11,
      RootWindowOfScreen(ScreenOfDisplay(x11, glx->xscreen)),
      False,
      SubstructureRedirectMask | SubstructureNotifyMask, &xev);

   _al_display_xglx_await_resize(display, old_resize_count, true);
#endif
}


void _al_xwin_check_maximized(A5O_DISPLAY *display)
{
#ifndef A5O_RASPBERRYPI
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
   Atom type;
   Atom horz = X11_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
   Atom vert = X11_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
   Atom property = X11_ATOM(_NET_WM_STATE);
   int format;
   int maximized = 0;
   unsigned long n, remaining, i, *p32;
   unsigned char *p8 = NULL;
   if (XGetWindowProperty(x11, glx->window, property, 0, INT_MAX,
      False, AnyPropertyType, &type, &format, &n, &remaining, &p8)
         != Success) {
      return;
   }
   p32 = (unsigned long *)p8;
   for (i = 0; i < n; i++) {
      if (p32[i] == horz)
         maximized |= 1;
      if (p32[i] == vert)
         maximized |= 2;
   }
   XFree(p8);
   display->flags &= ~A5O_MAXIMIZED;
   if (maximized == 3)
      display->flags |= A5O_MAXIMIZED;
#endif
}


/* Function: al_get_x_window_id
 */
XID al_get_x_window_id(A5O_DISPLAY *display)
{
   ASSERT(display != NULL);
   return ((A5O_DISPLAY_XGLX*)display)->window;
}


// Note: this only seems to work after the window has been mapped
void _al_xwin_get_borders(A5O_DISPLAY *display) {
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)display;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Display *x11 = system->x11display;

   Atom type;
   int format;
   unsigned long nitems, bytes_after;
   unsigned char *property;
   Atom frame_extents = X11_ATOM(_NET_FRAME_EXTENTS);
   if (XGetWindowProperty(x11, glx->window,
         frame_extents, 0, 16, 0, XA_CARDINAL,
         &type, &format, &nitems, &bytes_after, &property) == Success) {
      if (type != None && nitems == 4) {
         glx->border_left = (int)((long *)property)[0];
         glx->border_right = (int)((long *)property)[1];
         glx->border_top = (int)((long *)property)[2];
         glx->border_bottom = (int)((long *)property)[3];
         glx->borders_known = true;
         A5O_DEBUG("_NET_FRAME_EXTENTS: %d %d %d %d\n", glx->border_left, glx->border_top,
            glx->border_right, glx->border_bottom);
      }
      else {
         A5O_DEBUG("Unexpected _NET_FRAME_EXTENTS format: nitems=%lu\n", nitems);
      }
      XFree(property);
   }
   else {
      A5O_DEBUG("Could not read _NET_FRAME_EXTENTS\n");
   }
}

/* vim: set sts=3 sw=3 et: */
