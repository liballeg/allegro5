#ifndef __al_included_allegro5_aintern_xdisplay_h
#define __al_included_allegro5_aintern_xdisplay_h

/* XXX The Raspberry Pi port does not use GLX. It currently substitutes its own
 * ALLEGRO_DISPLAY_RASPBERRYPI for ALLEGRO_DISPLAY_XGLX by a macro renaming
 * hack.
 */
#ifndef ALLEGRO_RASPBERRYPI

#include <GL/glx.h>

#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_x.h"

typedef struct ALLEGRO_DISPLAY_XGLX_GTKGLEXT ALLEGRO_DISPLAY_XGLX_GTKGLEXT;

/* This is our version of ALLEGRO_DISPLAY with driver specific extra data. */
struct ALLEGRO_DISPLAY_XGLX
{
   /* This must be the first member. */
   ALLEGRO_DISPLAY display;

   /* Driver specifics. */

   /* If GtkGLExt is used then this points to a structure containing fields
    * specific to that backend, otherwise it is NULL.  Some following fields
    * are inapplicable in GtkGLExt mode; precisely which should be made
    * obvious, but isn't yet.
    */
   ALLEGRO_DISPLAY_XGLX_GTKGLEXT *gtk;

   Window window;
   int xscreen; /* X Screen ID */
   int adapter; /* allegro virtual adapter id/index */
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
   XVisualInfo *xvinfo; /* Used when selecting the X11 visual to use. */
   GLXFBConfig *fbc; /* Used when creating the OpenGL context. */
   int glx_version; /* 130 means 1 major and 3 minor, aka 1.3 */

   /* If our window is embedded by the XEmbed protocol, this gives
    * the window ID of the embedder; Otherwise None.
    */
   Window embedder_window;

   _AL_COND mapped; /* Condition variable to wait for mapping a window. */
   bool is_mapped;  /* Set to true when mapped. */

   int resize_count; /* Increments when resized. */
   bool programmatic_resize; /* Set while programmatic resize in progress. */

   /* Cursor for this window. */
   Cursor invisible_cursor;
   Cursor current_cursor;
   bool cursor_hidden;

   /* Icon for this window. */
   Pixmap icon, icon_mask;

   /* Desktop position. */
   int x, y;

   /* al_set_mouse_xy implementation */
   bool mouse_warp;
};

void _al_display_xglx_await_resize(ALLEGRO_DISPLAY *d, int old_resize_count, bool delay_hack);
void _al_xglx_display_configure(ALLEGRO_DISPLAY *d, int x, int y, int width, int height, bool setglxy);
void _al_xglx_display_configure_event(ALLEGRO_DISPLAY *d, XEvent *event);
void _al_xwin_display_switch_handler(ALLEGRO_DISPLAY *d,
   XFocusChangeEvent *event);
void _al_xwin_display_switch_handler_inner(ALLEGRO_DISPLAY *d, bool focus_in);
void _al_xwin_display_expose(ALLEGRO_DISPLAY *display, XExposeEvent *xevent);

#endif /* !ALLEGRO_RASPBERRYPI */

#endif
