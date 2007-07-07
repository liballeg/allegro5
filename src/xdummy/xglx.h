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

typedef struct AL_SYSTEM_XGLX AL_SYSTEM_XGLX;
typedef struct AL_DISPLAY_XGLX AL_DISPLAY_XGLX;
typedef struct AL_BITMAP_XGLX AL_BITMAP_XGLX;

/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_XGLX
{
   AL_SYSTEM system; /* This must be the first member, we "derive" from it. */

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

struct AL_BITMAP_XGLX
{
   AL_BITMAP bitmap; /* This must be the first member. */
   
   /* Driver specifics. */
   
   GLuint texture; /* 0 means, not uploaded yet. */
   float left, top, right, bottom; /* Texture coordinates. */
   bool is_backbuffer; /* This is not a real bitmap, but the backbuffer. */
};

/* This is our version of AL_DISPLAY with driver specific extra data. */
struct AL_DISPLAY_XGLX
{
   AL_DISPLAY display; /* This must be the first member. */
   
   /* Driver specifics. */

   int opengl_initialized; /* Did we have a chance to set up OpenGL? */

   AL_BITMAP *backbuffer;

   Window window;
   int xscreen; /* TODO: what is this? something with multi-monitor? */
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
};

/* Functions private to the X11 driver. */

/* display */
void _al_display_xglx_configure(AL_DISPLAY *d, XEvent *event);
void _al_display_xglx_closebutton(AL_DISPLAY *d, XEvent *xevent);

/* keyboard */
void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack);

/* bitmap */
AL_BITMAP *_al_xglx_create_bitmap(AL_DISPLAY *d, int w, int h);

/* draw */
void _xglx_add_drawing_functions(AL_DISPLAY_INTERFACE *vt);

/* fullscreen */
int _al_xglx_get_num_display_modes(void);
AL_DISPLAY_MODE *_al_xglx_get_display_mode(
   int index, AL_DISPLAY_MODE *mode);
bool _al_xglx_fullscreen_set_mode(AL_SYSTEM_XGLX *s, int w, int h,
   int format, int refresh_rate);
void _al_xglx_store_video_mode(AL_SYSTEM_XGLX *s);
void _al_xglx_restore_video_mode(AL_SYSTEM_XGLX *s);
void _al_xglx_fullscreen_to_display(AL_SYSTEM_XGLX *s,
   AL_DISPLAY_XGLX *d);

/* compat */
void _al_xglx_display_upload_compat_screen(BITMAP *bitmap, int x, int y, int w, int h);
