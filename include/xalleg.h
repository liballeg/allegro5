/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      X header file for the Allegro library.
 *
 *      This prototypes some things which might be useful to 
 *      the calling application, but you don't need it.
 */


#ifndef X_ALLEGRO_H
#define X_ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

#ifndef ALLEGRO_H
#error Please include allegro.h before xalleg.h!
#endif


#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
#include <pwd.h>
#endif
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
#include <X11/extensions/xf86dga.h>
#endif


/* X-Windows resources used by the library.  */
extern struct _xwin_type
{
   Display *display;
   volatile int lock_count;
   int screen;
   Window window;
   GC gc;
   Visual *visual;
   Colormap colormap;
   XImage *ximage;
   Cursor cursor;
   int cursor_shape;

   void (*bank_switch)(int line);
   void (*screen_to_buffer)(int sx, int sy, int sw, int sh);
   void (*set_colors)(AL_CONST PALETTE p, int from, int to);

   unsigned char *screen_data;
   unsigned char **screen_line;
   unsigned char **buffer_line;

   int scroll_x;
   int scroll_y;

   int window_width;
   int window_height;
   int window_depth;

   int screen_width;
   int screen_height;
   int screen_depth;

   int virtual_width;
   int virtual_height;

   int mouse_warped;
   int keycode_to_scancode[256];

   int matching_formats;
   int fast_visual_depth;
   int visual_is_truecolor;

   int rsize;
   int gsize;
   int bsize;
   int rshift;
   int gshift;
   int bshift;

   unsigned long cmap[0x1000];
   unsigned long rmap[0x100];
   unsigned long gmap[0x100];
   unsigned long bmap[0x100];

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   XShmSegmentInfo shminfo;
#endif
   int use_shm;

   int in_dga_mode;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   int disable_dga_mouse;
#endif

   int keyboard_grabbed;
   int mouse_grabbed;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   XF86VidModeModeInfo **modesinfo;
   int num_modes;
   int mode_switched;
   int override_redirected;
#endif

   char window_title[1024];
   char application_name[1024];
   char application_class[1024];

   void (*window_close_hook)(void);
} _xwin;



AL_FUNCPTR (int, _xwin_window_creator, (void));
AL_FUNCPTR (void, _xwin_window_defaultor, (void));
AL_FUNCPTR (void, _xwin_window_redrawer, (int, int, int, int));
AL_FUNCPTR (void, _xwin_input_handler, (void));

AL_FUNCPTR (void, _xwin_keyboard_callback, (int, int));



#ifdef __cplusplus
   }
#endif

#endif

