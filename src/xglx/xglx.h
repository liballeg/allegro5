#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/platform/aintxglx.h"

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif

typedef struct ALLEGRO_SYSTEM_XGLX ALLEGRO_SYSTEM_XGLX;
typedef struct ALLEGRO_DISPLAY_XGLX ALLEGRO_DISPLAY_XGLX;
typedef struct ALLEGRO_BITMAP_XGLX ALLEGRO_BITMAP_XGLX;

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

struct ALLEGRO_BITMAP_XGLX
{
   ALLEGRO_BITMAP bitmap; /* This must be the first member. */
   
   /* Driver specifics. */
   
   GLuint texture; /* 0 means, not uploaded yet. */
   float left, top, right, bottom; /* Texture coordinates. */
   bool is_backbuffer; /* This is not a real bitmap, but the backbuffer. */
};

/* This is our version of ALLEGRO_DISPLAY with driver specific extra data. */
struct ALLEGRO_DISPLAY_XGLX
{
   ALLEGRO_DISPLAY display; /* This must be the first member. */
   
   /* Driver specifics. */

   int opengl_initialized; /* Did we have a chance to set up OpenGL? */

   ALLEGRO_BITMAP *backbuffer;

   Window window;
   int xscreen; /* TODO: what is this? something with multi-monitor? */
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
};

/* Functions private to the X11 driver. */

/* display */
void _al_display_xglx_configure(ALLEGRO_DISPLAY *d, XEvent *event);
void _al_display_xglx_closebutton(ALLEGRO_DISPLAY *d, XEvent *xevent);

/* keyboard */
void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack);

/* bitmap */
ALLEGRO_BITMAP *_al_xglx_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h);

/* draw */
void _xglx_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt);

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
void _al_xglx_display_upload_compat_screen(BITMAP *bitmap, int x, int y, int w, int h);
