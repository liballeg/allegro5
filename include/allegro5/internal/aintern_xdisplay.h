#ifndef __al_included_allegro5_aintern_xdisplay_h
#define __al_included_allegro5_aintern_xdisplay_h

/* XXX The Raspberry Pi port does not use GLX. It currently substitutes its own
 * A5O_DISPLAY_RASPBERRYPI for A5O_DISPLAY_XGLX by a macro renaming
 * hack.
 */
#ifndef A5O_RASPBERRYPI

#include <GL/glx.h>

#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_x.h"

typedef struct A5O_DISPLAY_XGLX_GTK A5O_DISPLAY_XGLX_GTK;
typedef struct A5O_XWIN_DISPLAY_OVERRIDABLE_INTERFACE A5O_XWIN_DISPLAY_OVERRIDABLE_INTERFACE;

/* This is our version of A5O_DISPLAY with driver specific extra data. */
struct A5O_DISPLAY_XGLX
{
   /* This must be the first member. */
   A5O_DISPLAY display;

   /* Driver specifics. */

   const struct A5O_XWIN_DISPLAY_OVERRIDABLE_INTERFACE *overridable_vt;

   Window window;
   int xscreen; /* X Screen ID */
   int adapter; /* allegro virtual adapter id/index */
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
   XVisualInfo *xvinfo; /* Used when selecting the X11 visual to use. */
   GLXFBConfig *fbc; /* Used when creating the OpenGL context. */
   int glx_version; /* 130 means 1 major and 3 minor, aka 1.3 */

   /* Points to a structure if this display is contained by a GTK top-level
    * window, otherwise it is NULL.
    */
   A5O_DISPLAY_XGLX_GTK *gtk;

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
   bool need_initial_position_adjust;
   int border_left, border_right, border_top, border_bottom;
   bool borders_known;

   /* al_set_mouse_xy implementation */
   bool mouse_warp;
   
   _AL_COND selectioned; /* Condition variable to wait for a selection event a window. */
   bool is_selectioned;  /* Set to true when selection event received. */

};

void _al_display_xglx_await_resize(A5O_DISPLAY *d, int old_resize_count, bool delay_hack);
void _al_xglx_display_configure(A5O_DISPLAY *d, int x, int y, int width, int height, bool setglxy);
void _al_xglx_display_configure_event(A5O_DISPLAY *d, XEvent *event);
void _al_xwin_display_switch_handler(A5O_DISPLAY *d,
   XFocusChangeEvent *event);
void _al_xwin_display_switch_handler_inner(A5O_DISPLAY *d, bool focus_in);
void _al_xwin_display_expose(A5O_DISPLAY *display, XExposeEvent *xevent);


/* An ad-hoc interface to allow the GTK backend to override some of the
 * normal X display interface implementation.
 */
struct A5O_XWIN_DISPLAY_OVERRIDABLE_INTERFACE
{
   bool (*create_display_hook)(A5O_DISPLAY *d, int w, int h);
   void (*destroy_display_hook)(A5O_DISPLAY *d, bool is_last);
   bool (*resize_display)(A5O_DISPLAY *d, int w, int h);
   void (*set_window_title)(A5O_DISPLAY *display, const char *title);
   void (*set_fullscreen_window)(A5O_DISPLAY *display, bool onoff);
   void (*set_window_position)(A5O_DISPLAY *display, int x, int y);
   bool (*set_window_constraints)(A5O_DISPLAY *display, int min_w, int min_h, int max_w, int max_h);
   bool (*set_display_flag)(A5O_DISPLAY *display, int flag, bool onoff);
   void (*check_maximized)(A5O_DISPLAY *display);
};

bool _al_xwin_set_gtk_display_overridable_interface(uint32_t check_version,
   const A5O_XWIN_DISPLAY_OVERRIDABLE_INTERFACE *vt);

#endif /* !A5O_RASPBERRYPI */

#endif

/* vim: set sts=3 sw=3 et: */
