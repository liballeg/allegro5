/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_//_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      X11 clipboard handling.
 *
 *      By Beoran.
 *
 *      See readme.txt for copyright information.
 */

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xclipboard.h"
#include "allegro5/internal/aintern_xcursor.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xsystem.h"

#ifdef A5O_RASPBERRYPI
#include "allegro5/internal/aintern_raspberrypi.h"
#define A5O_SYSTEM_XGLX A5O_SYSTEM_RASPBERRYPI
#define A5O_DISPLAY_XGLX A5O_DISPLAY_RASPBERRYPI
#endif

A5O_DEBUG_CHANNEL("clipboard")


void _al_xwin_display_selection_notify(A5O_DISPLAY *display, XSelectionEvent *xselection)
{
   (void) display; (void) xselection;
}


void _al_xwin_display_selection_request(A5O_DISPLAY *display, XSelectionRequestEvent *xselectionrequest)
{
   (void) display;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Display *xdisplay = system->x11display;


   XSelectionRequestEvent *req;
   XEvent sevent = { 0 };
   int seln_format;
   unsigned long nbytes;
   unsigned long overflow;
   unsigned char *seln_data;

   req = xselectionrequest;

   A5O_DEBUG("window %p: SelectionRequest (requestor = %ld, target = %ld)\n", xdisplay,
                 req->requestor, req->target);

   memset(&sevent, 0, sizeof(sevent));
   sevent.xany.type = SelectionNotify;
   sevent.xselection.selection = req->selection;
   sevent.xselection.target = None;
   sevent.xselection.property = None;
   sevent.xselection.requestor = req->requestor;
   sevent.xselection.time = req->time;

   if (XGetWindowProperty(xdisplay, DefaultRootWindow(xdisplay),
                          XA_CUT_BUFFER0, 0, INT_MAX/4, False, req->target,
                          &sevent.xselection.target, &seln_format, &nbytes,
                          &overflow, &seln_data) == Success) {
      Atom XA_TARGETS = XInternAtom(xdisplay, "TARGETS", 0);
      if (sevent.xselection.target == req->target) {
         XChangeProperty(xdisplay, req->requestor, req->property,
                         sevent.xselection.target, seln_format, PropModeReplace,
                         seln_data, nbytes);
         sevent.xselection.property = req->property;
      } else if (XA_TARGETS == req->target) {
         Atom SupportedFormats[] = { sevent.xselection.target, XA_TARGETS };
         XChangeProperty(xdisplay, req->requestor, req->property,
                         XA_ATOM, 32, PropModeReplace,
                         (unsigned char *)SupportedFormats,
                         sizeof(SupportedFormats)/sizeof(*SupportedFormats));
         sevent.xselection.property = req->property;
      }
      XFree(seln_data);
   }
   XSendEvent(xdisplay, req->requestor, False, 0, &sevent);
   XSync(xdisplay, False);
}


/* Waits for a selection (copy/paste or DND event) */
static bool _al_display_xglx_await_selection_event(A5O_DISPLAY *d)
{
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx = (A5O_DISPLAY_XGLX *)d;
   A5O_TIMEOUT timeout;

   A5O_DEBUG("Awaiting selection event\n");

   XSync(system->x11display, False);

   /* Wait until the selection event is notified.
    * Don't wait forever if an event never comes.
    */
   al_init_timeout(&timeout, 1.0);
   if (_al_cond_timedwait(&glx->selectioned, &system->lock, &timeout) == -1) {
      A5O_ERROR("Timeout while waiting for selection event.\n");
      return false;
   }

   return true;
}


static bool xdpy_set_clipboard_text(A5O_DISPLAY *display, const char *text)
{

   A5O_DISPLAY_XGLX *glx = (void *)display;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   Atom format;
   Atom XA_CLIPBOARD = XInternAtom(xdisplay, "CLIPBOARD", 0);

   /* Get the window that will own the selection */
   if (xwindow == None) {
      A5O_DEBUG("Couldn't find a window to own the selection");
      return false;
   }

   /* Save the selection on the root window */
   /* If you don't support UTF-8, you might use XA_STRING here */
   format = XInternAtom(xdisplay, "UTF8_STRING", False);
   XChangeProperty(xdisplay, DefaultRootWindow(xdisplay),
                   XA_CUT_BUFFER0, format, 8, PropModeReplace,
                   (const unsigned char *)text, strlen(text));

   if (XA_CLIPBOARD != None &&
         XGetSelectionOwner(xdisplay, XA_CLIPBOARD) != xwindow) {
      XSetSelectionOwner(xdisplay, XA_CLIPBOARD, xwindow, CurrentTime);
   }

   if (XGetSelectionOwner(xdisplay, XA_PRIMARY) != xwindow) {
      XSetSelectionOwner(xdisplay, XA_PRIMARY, xwindow, CurrentTime);
   }

   return true;
}


static char *xdpy_get_clipboard_text(A5O_DISPLAY *display)
{
   A5O_DISPLAY_XGLX *glx = (void *)display;
   A5O_SYSTEM_XGLX *system = (void *)al_get_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   Atom format;
   Window owner;
   Atom selection;
   Atom seln_type;
   int seln_format;
   unsigned long nbytes;
   unsigned long overflow;
   unsigned char *src;
   char *text = NULL;

   Atom XA_CLIPBOARD = XInternAtom(xdisplay, "CLIPBOARD", 0);
   if (XA_CLIPBOARD == None) {
      A5O_DEBUG("Couldn't access X clipboard");
      return NULL;
   }

   /* Get the window that holds the selection */
   format = XInternAtom(xdisplay, "UTF8_STRING", 0);
   owner = XGetSelectionOwner(xdisplay, XA_CLIPBOARD);
   if ((owner == None) || (owner == xwindow)) {
      owner = DefaultRootWindow(xdisplay);
      selection = XA_CUT_BUFFER0;
   } else {
      /* Request that the selection owner copy the data to our window. */
      owner = xwindow;
      selection = XInternAtom(xdisplay, "A5O_SELECTION", False);
      XConvertSelection(xdisplay, XA_CLIPBOARD, format, selection, owner,
                        CurrentTime);

      glx->is_selectioned = false;
      if (!_al_display_xglx_await_selection_event(display))
         return NULL;
   }

   if (XGetWindowProperty(xdisplay, owner, selection, 0, INT_MAX/4, False,
                          format, &seln_type, &seln_format, &nbytes, &overflow, &src)
         == Success) {
      if (seln_type == format) {
         text = (char *)al_malloc(nbytes+1);
         if (text) {
            memcpy(text, src, nbytes);
            text[nbytes] = '\0';
         }
      }
   }
   XFree(src);
   return text;
}

static bool xdpy_has_clipboard_text(A5O_DISPLAY *display)
{
   char *text = xdpy_get_clipboard_text(display);

   if (!text) {
      return false;
   }

   al_free(text);
   return true;
}



void _al_xwin_add_clipboard_functions(A5O_DISPLAY_INTERFACE *vt)
{
   vt->set_clipboard_text = xdpy_set_clipboard_text;
   vt->get_clipboard_text = xdpy_get_clipboard_text;
   vt->has_clipboard_text = xdpy_has_clipboard_text;
}


/* vim: set sts=3 sw=3 et: */
