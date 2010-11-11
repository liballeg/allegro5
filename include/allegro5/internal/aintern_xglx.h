#ifndef __al_included_allegro5_aintern_xglx_h
#define __al_included_allegro5_aintern_xglx_h

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/platform/aintxglx.h"
#include "allegro5/internal/aintern_opengl.h"

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
#include <X11/extensions/Xrandr.h>
#endif

typedef struct ALLEGRO_SYSTEM_XGLX ALLEGRO_SYSTEM_XGLX;
typedef struct ALLEGRO_DISPLAY_XGLX ALLEGRO_DISPLAY_XGLX;
typedef struct ALLEGRO_MOUSE_CURSOR_XGLX ALLEGRO_MOUSE_CURSOR_XGLX;

/* This is our version of ALLEGRO_SYSTEM with driver specific extra data. */
struct ALLEGRO_SYSTEM_XGLX
{
   ALLEGRO_SYSTEM system; /* This must be the first member, we "derive" from it. */

   /* Driver specifics. */

   /* X11 is not thread-safe. But we use a separate thread to handle X11 events.
    * Plus, users may call OpenGL commands in the main thread, and we have no
    * way to enforce locking for them.
    * The only solution seems to be two X11 display connections. One to do our
    * input handling, and one for OpenGL graphics.
    *
    * Note: these may be NULL if we are not connected to an X server, for
    * headless command-line tools. We don't have a separate "null" system
    * driver.
    */
   Display *x11display; /* The X11 display. You *MUST* only access this from one
    * thread at a time, use the mutex lock below to ensure it.
    */
   Display *gfxdisplay; /* Another X11 display we use for graphics. You *MUST*
    * only use this in the main thread.
    */

   Atom AllegroAtom;

   #ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   /* For VidMode extension. */
   int xfvm_available;
   int xfvm_screen_count;
   struct {
      int mode_count;
      XF86VidModeModeInfo **modes;
      XF86VidModeModeInfo *original_mode;
   } *xfvm_screen;
   #endif

   _AL_THREAD thread; /* background thread. */
   _AL_MUTEX lock; /* thread lock for whenever we access internals. */
   // FIXME: One condition variable really would be enough.
   _AL_COND resized; /* Condition variable to wait for resizing a window. */
   bool pointer_grabbed; /* Is an XGrabPointer in effect? */
   bool inhibit_screensaver; /* Should we inhibit the screensaver? */

   bool mmon_interface_inited;
#ifdef ALLEGRO_XWINDOWS_WITH_XINERAMA
   int xinerama_available;
   int xinerama_screen_count;
   XineramaScreenInfo *xinerama_screen_info;
#endif
#ifdef ALLEGRO_XWINDOWS_WITH_XRANDR
   int xrandr_available;
   XRRScreenResources *xrandr_res;
   // these are technically changeable at runtime if we handle XRandR events.
   // or more accurately, they can become stale at runtime if we don't handle XRandR events.
   int xrandr_output_count;
   XRROutputInfo **xrandr_outputs;
   XRRModeInfo **xrandr_stored_modes;
#endif
};

/* This is our version of ALLEGRO_DISPLAY with driver specific extra data. */
struct ALLEGRO_DISPLAY_XGLX
{
   ALLEGRO_DISPLAY display; /* This must be the first member. */

   /* Driver specifics. */

   Window window;
   int xscreen; /* TODO: what is this? something with multi-monitor? */
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
   XVisualInfo *xvinfo; /* Used when selecting the X11 visual to use. */
   GLXFBConfig *fbc; /* Used when creating the OpenGL context. */
   int glx_version; /* 130 means 1 major and 3 minor, aka 1.3 */

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

struct ALLEGRO_MOUSE_CURSOR_XGLX
{
   Cursor cursor;
};

/* Functions private to the X11 driver. */

/* display */
void _al_display_xglx_configure(ALLEGRO_DISPLAY *d, XEvent *event);
void _al_display_xglx_closebutton(ALLEGRO_DISPLAY *d, XEvent *xevent);
void _al_xwin_display_switch_handler(ALLEGRO_DISPLAY *d,
   XFocusChangeEvent *event);
void _al_xwin_display_expose(ALLEGRO_DISPLAY *display, XExposeEvent *xevent);

/* keyboard */
void _al_xwin_keyboard_handler(XKeyEvent *event, ALLEGRO_DISPLAY *display);
void _al_xwin_keyboard_switch_handler(ALLEGRO_DISPLAY *display,
   const XFocusChangeEvent *event);

/* mouse */
void _al_xwin_mouse_button_press_handler(int button, ALLEGRO_DISPLAY *display);
void _al_xwin_mouse_button_release_handler(int button, ALLEGRO_DISPLAY *d);
void _al_xwin_mouse_motion_notify_handler(int x, int y, ALLEGRO_DISPLAY *d);
void _al_xwin_mouse_switch_handler(ALLEGRO_DISPLAY *display,
   const XCrossingEvent *event);

/* cursor */
ALLEGRO_MOUSE_CURSOR *_al_xwin_create_mouse_cursor(ALLEGRO_BITMAP *bmp,
   int x_focus, int y_focus);
void _al_xwin_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor);
void _al_xglx_add_cursor_functions(ALLEGRO_DISPLAY_INTERFACE *vt);

/* fullscreen and multi monitor stuff */

void _al_xsys_mmon_exit(ALLEGRO_SYSTEM_XGLX *s);

int _al_xglx_get_num_display_modes(ALLEGRO_SYSTEM_XGLX *s, int adapter);
ALLEGRO_DISPLAY_MODE *_al_xglx_get_display_mode(
   ALLEGRO_SYSTEM_XGLX *s, int adapter, int index, ALLEGRO_DISPLAY_MODE *mode);
bool _al_xglx_fullscreen_set_mode(ALLEGRO_SYSTEM_XGLX *s, ALLEGRO_DISPLAY_XGLX *d, int w, int h,
   int format, int refresh_rate);
void _al_xglx_store_video_mode(ALLEGRO_SYSTEM_XGLX *s);
void _al_xglx_restore_video_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter);
void _al_xglx_fullscreen_to_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d);
void _al_xglx_toggle_fullscreen_window(ALLEGRO_DISPLAY *display, int value);
void _al_display_xglx_await_resize(ALLEGRO_DISPLAY *d, int old_resize_count, bool delay_hack);
void _al_xglx_get_display_offset(ALLEGRO_SYSTEM_XGLX *s, int adapter, int *x, int *y);

int _al_xglx_fullscreen_select_mode(ALLEGRO_SYSTEM_XGLX *s, int adapter, int w, int h, int format, int refresh_rate);

void _al_xglx_get_monitor_info(ALLEGRO_SYSTEM_XGLX *s, int adapter, ALLEGRO_MONITOR_INFO *info);
int _al_xglx_get_num_video_adapters(ALLEGRO_SYSTEM_XGLX *s);


/* glx_config */
void _al_xglx_config_select_visual(ALLEGRO_DISPLAY_XGLX *glx);
bool _al_xglx_config_create_context(ALLEGRO_DISPLAY_XGLX *glx);

#endif
