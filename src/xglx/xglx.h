#include <X11/Xlib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/platform/aintxglx.h"
#include "allegro5/internal/aintern_opengl.h"

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif

typedef struct ALLEGRO_SYSTEM_XGLX ALLEGRO_SYSTEM_XGLX;
typedef struct ALLEGRO_DISPLAY_XGLX ALLEGRO_DISPLAY_XGLX;

/* This is our version of ALLEGRO_SYSTEM with driver specific extra data. */
struct ALLEGRO_SYSTEM_XGLX
{
   ALLEGRO_SYSTEM system; /* This must be the first member, we "derive" from it. */

   /* Driver specifics. */

   Display *xdisplay; /* The X11 display. */

   #ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   /* For VidMode extension. */
   int modes_count;
   XF86VidModeModeInfo **modes;
   XF86VidModeModeInfo *original_mode;
   #endif
   
   _AL_THREAD thread; /* background thread. */
   _AL_MUTEX lock; /* thread lock for whenever we access internals. */
   // FIXME: One condition variable really would be enough.
   _AL_COND mapped; /* Condition variable to wait for mapping a window. */
   _AL_COND resized; /* Condition variable to wait for resizing a window. */
   bool pointer_grabbed; /* Is an XGrabPointer in effect? */
};

/* This is our version of ALLEGRO_DISPLAY with driver specific extra data. */
struct ALLEGRO_DISPLAY_XGLX
{
   ALLEGRO_DISPLAY_OGL ogl_display; /* This must be the first member. */
   
   /* Driver specifics. */

   int opengl_initialized; /* Did we have a chance to set up OpenGL? */

   Window window;
   int xscreen; /* TODO: what is this? something with multi-monitor? */
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
   XVisualInfo *xvinfo; /* Used when selecting the X11 visual to use. */
   GLXFBConfig *fbc; /* Used when creating the OpenGL context. */
   float glx_version;
   bool got_extensions;

   /* Cursor for this window. */
   Cursor invisible_cursor;
   bool cursor_hidden;

   /* Icon for this window. */
   Pixmap icon, icon_mask;
};

/* Functions private to the X11 driver. */

/* display */
void _al_display_xglx_configure(ALLEGRO_DISPLAY *d, XEvent *event);
void _al_display_xglx_closebutton(ALLEGRO_DISPLAY *d, XEvent *xevent);

/* keyboard */
void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack);

/* mouse */
void _al_xwin_mouse_button_press_handler(int button);
void _al_xwin_mouse_button_release_handler(int button);
void _al_xwin_mouse_motion_notify_handler(int x, int y);

/* fullscreen */
int _al_xglx_get_num_display_modes(void);
ALLEGRO_DISPLAY_MODE *_al_xglx_get_display_mode(
   int index, ALLEGRO_DISPLAY_MODE *mode);
bool _al_xglx_fullscreen_set_mode(ALLEGRO_SYSTEM_XGLX *s, int w, int h,
   int format, int refresh_rate);
void _al_xglx_store_video_mode(ALLEGRO_SYSTEM_XGLX *s);
void _al_xglx_restore_video_mode(ALLEGRO_SYSTEM_XGLX *s);
void _al_xglx_fullscreen_to_display(ALLEGRO_SYSTEM_XGLX *s,
   ALLEGRO_DISPLAY_XGLX *d);

/* compat */
void _al_xglx_display_upload_compat_screen(BITMAP *bitmap,
   int x, int y, int w, int h);

/* glx_config */
void _xglx_config_select_visual(ALLEGRO_DISPLAY_XGLX *glx);
void _xglx_config_create_context(ALLEGRO_DISPLAY_XGLX *glx);
