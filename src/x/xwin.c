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
 *      Wrappers for Xlib functions.
 *
 *      By Michael Bukin.
 *
 *      Video mode switching by Peter Wang and Benjamin Stover.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"

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


#define XWIN_DEFAULT_WINDOW_TITLE "Allegro application"
#define XWIN_DEFAULT_APPLICATION_NAME "allegro"
#define XWIN_DEFAULT_APPLICATION_CLASS "Allegro"


struct _xwin_type _xwin =
{
   0,           /* display */
   0,           /* lock count */
   0,           /* screen */
   None,        /* window */
   None,        /* gc */
   0,           /* visual */
   None,        /* colormap */
   0,           /* ximage */
   None,        /* cursor */
   XC_heart,    /* cursor_shape */

   0,           /* bank_switch */
   0,           /* screen_to_buffer */
   0,           /* set_colors */

   0,           /* screen_data */
   0,           /* screen_line */
   0,           /* buffer_line */

   0,           /* scroll_x */
   0,           /* scroll_y */

   320,         /* window_width */
   200,         /* window_height */
   8,           /* window_depth */

   320,         /* screen_width */
   200,         /* screen_height */
   8,           /* screen_depth */

   320,         /* virtual width */
   200,         /* virtual_height */

   0,           /* mouse_warped */
   { 0 },       /* keycode_to_scancode */

   0,           /* matching formats */
   0,           /* fast_visual_depth */
   0,           /* visual_is_truecolor */

   1,           /* rsize */
   1,           /* gsize */
   1,           /* bsize */
   0,           /* rshift */
   0,           /* gshift */
   0,           /* bshift */

   { 0 },       /* cmap */
   { 0 },       /* rmap */
   { 0 },       /* gmap */
   { 0 },       /* bmap */

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   { 0 },       /* shminfo */
#endif
   0,           /* use_shm */

   0,           /* in_dga_mode */

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   0, 		/* disable_dga_mouse */
#endif

   0,           /* keyboard_grabbed */
   0,           /* mouse_grabbed */

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   0,           /* modesinfo */
   0,           /* num_modes */
   0,           /* mode_switched */
   0,           /* override_redirected */
#endif

   XWIN_DEFAULT_WINDOW_TITLE,           /* window_title */
   XWIN_DEFAULT_APPLICATION_NAME,       /* application_name */
   XWIN_DEFAULT_APPLICATION_CLASS,      /* application_class */

   NULL         /* window close hook */
};

int _xwin_last_line = -1;
int _xwin_in_gfx_call = 0;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
int _xdga_last_line = -1;
static int _xdga_installed_colormap; 
static Colormap _xdga_colormap[2];
#endif

#define MOUSE_WARP_DELAY   200

static char _xwin_driver_desc[256] = EMPTY_STRING;

/* Array of keycodes which are pressed now (used for auto-repeat).  */
int _xwin_keycode_pressed[256];

/* This is used to intercept window closing requests.  */
static Atom wm_delete_window;



/* Forward declarations for private functions.  */
static char *_xwin_safe_copy(char *dst, const char *src, int len);

static int _xwin_private_open_display(char *name);
static int _xwin_private_create_window(void);
static void _xwin_private_destroy_window(void);
static void _xwin_private_select_screen_to_buffer_function(void);
static void _xwin_private_select_set_colors_function(void);
static void _xwin_private_setup_driver_desc(GFX_DRIVER *drv, int dga);
static BITMAP *_xwin_private_create_screen(GFX_DRIVER *drv, int w, int h,
					   int vw, int vh, int depth, int fullscreen);
static void _xwin_private_destroy_screen(void);
static BITMAP *_xwin_private_create_screen_bitmap(GFX_DRIVER *drv, int dga,
						  unsigned char *frame_buffer,
						  int bytes_per_buffer_line);
static void _xwin_private_create_mapping_tables(void);
static void _xwin_private_create_mapping(unsigned long *map, int ssize, int dsize, int dshift);
static int _xwin_private_display_is_local(void);
static int _xwin_private_create_ximage(int w, int h);
static void _xwin_private_destroy_ximage(void);
static void _xwin_private_prepare_visual(void);
static int _xwin_private_matching_formats(void);
static int _xwin_private_fast_visual_depth(void);
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
static int _xdga_private_fast_visual_depth(void);
#endif
static void _xwin_private_set_matching_colors(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_truecolor_colors(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_palette_colors(AL_CONST PALETTE p, int from, int to);
static void _xwin_private_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync);
static void _xwin_private_set_window_defaults(void);
static void _xwin_private_flush_buffers(void);
static void _xwin_private_vsync(void);
static void _xwin_private_resize_window(int w, int h);
static void _xwin_private_process_event(XEvent *event);
static void _xwin_private_handle_input(void);
static void _xwin_private_set_warped_mouse_mode(int permanent);
static void _xwin_private_redraw_window(int x, int y, int w, int h);
static int _xwin_private_scroll_screen(int x, int y);
static void _xwin_private_update_screen(int x, int y, int w, int h);
static void _xwin_private_set_window_title(AL_CONST char *name);
static void _xwin_private_change_keyboard_control(int led, int on);
static int _xwin_private_get_pointer_mapping(unsigned char map[], int nmap);
static void _xwin_private_init_keyboard_tables(void);

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
static BITMAP *_xdga_private_create_screen(GFX_DRIVER *drv, int w, int h,
					   int vw, int vh, int depth, int fullscreen);
static void _xdga_private_destroy_screen(void);
static void _xdga_private_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync);
static int _xdga_private_scroll_screen(int x, int y);
#endif

static void _xwin_private_fast_truecolor_8_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_8_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_15_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_16_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_24_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_24(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_truecolor_32_to_32(int sx, int sy, int sw, int sh);

static void _xwin_private_slow_truecolor_8(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_15(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_16(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_24(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_truecolor_32(int sx, int sy, int sw, int sh);

static void _xwin_private_fast_palette_8_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_8_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_8_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_15_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_16_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_24_to_32(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_8(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_16(int sx, int sy, int sw, int sh);
static void _xwin_private_fast_palette_32_to_32(int sx, int sy, int sw, int sh);

static void _xwin_private_slow_palette_8(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_15(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_16(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_24(int sx, int sy, int sw, int sh);
static void _xwin_private_slow_palette_32(int sx, int sy, int sw, int sh);

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
static int _xvidmode_private_set_fullscreen(int w, int h, int vw, int vh);
static void _xvidmode_private_unset_fullscreen(void);
#endif

#ifdef ALLEGRO_NO_ASM
unsigned long _xwin_write_line (BITMAP *bmp, int line);
void _xwin_unwrite_line (BITMAP *bmp);
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
unsigned long _xdga_switch_bank (BITMAP *bmp, int line);
#endif
#else
unsigned long _xwin_write_line_asm (BITMAP *bmp, int line);
void _xwin_unwrite_line_asm (BITMAP *bmp);
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
unsigned long _xdga_switch_bank_asm (BITMAP *bmp, int line);
#endif
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
static void _xdga_switch_screen_bank(int line);
#endif



/* _xwin_safe_copy:
 *  Copy string, testing for buffer overrun (why isn't it in ANSI C?).
 */
static char* _xwin_safe_copy(char *dst, const char *src, int len)
{
   if (len <= 0)
      return dst;
   dst[0] = 0;
   strncat(dst, src, len - 1);
   return dst;
}



/* _xwin_open_display:
 *  Wrapper for XOpenDisplay.
 */
static int _xwin_private_open_display(char *name)
{
   if (_xwin.display != 0)
      return -1;

   _xwin.display = XOpenDisplay(name);
   _xwin.screen = ((_xwin.display == 0) ? 0 : XDefaultScreen(_xwin.display));

   return ((_xwin.display != 0) ? 0 : -1);
}

int _xwin_open_display(char *name)
{
   int result;
   XLOCK();
   result = _xwin_private_open_display(name);
   XUNLOCK();
   return result;
}



/* _xwin_close_display:
 *  Wrapper for XCloseDisplay.
 */
void _xwin_close_display(void)
{
   if (!_unix_bg_man->multi_threaded) {
      XLOCK();
   }

   if (_xwin.display != 0) {
      _xwin_destroy_window();
      XCloseDisplay(_xwin.display);
      _xwin.display = 0;
   }

   if (!_unix_bg_man->multi_threaded) {
      XUNLOCK();
   }
}



/* _xwin_create_window:
 *  Wrapper for XCreateWindow.
 */
static int _xwin_private_create_window(void)
{
   unsigned long gcmask;
   XGCValues gcvalues;
   XSetWindowAttributes setattr;
   XWindowAttributes getattr;
   Pixmap pixmap;

   if (_xwin.display == 0)
      return -1;

   _mouse_on = FALSE;

   /* Create window.  */
   setattr.border_pixel = XBlackPixel(_xwin.display, _xwin.screen);
   setattr.event_mask = (KeyPressMask | KeyReleaseMask 
			 | EnterWindowMask | LeaveWindowMask
			 | FocusChangeMask | ExposureMask | PropertyChangeMask
			 | ButtonPressMask | ButtonReleaseMask | PointerMotionMask
			 /*| MappingNotifyMask (SubstructureRedirectMask?)*/);
   _xwin.window = XCreateWindow(_xwin.display, XDefaultRootWindow(_xwin.display),
				0, 0, 320, 200, 0,
				CopyFromParent, InputOutput, CopyFromParent,
				CWBorderPixel | CWEventMask, &setattr);

   /* Get associated visual and window depth (bits per pixel).  */
   XGetWindowAttributes(_xwin.display, _xwin.window, &getattr);
   _xwin.visual = getattr.visual;
   _xwin.window_depth = getattr.depth;

   /* Create and install colormap.  */
   if ((_xwin.visual->class == PseudoColor)
       || (_xwin.visual->class == GrayScale)
       || (_xwin.visual->class == DirectColor))
      _xwin.colormap = XCreateColormap(_xwin.display, _xwin.window, _xwin.visual, AllocAll);
   else
      _xwin.colormap = XCreateColormap(_xwin.display, _xwin.window, _xwin.visual, AllocNone);
   XSetWindowColormap(_xwin.display, _xwin.window, _xwin.colormap);
   XInstallColormap(_xwin.display, _xwin.colormap);

   /* Set WM_DELETE_WINDOW atom in WM_PROTOCOLS property (to get window_delete requests).  */
   wm_delete_window = XInternAtom (_xwin.display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols (_xwin.display, _xwin.window, &wm_delete_window, 1);

   /* Set default window parameters.  */
   (*_xwin_window_defaultor)();

   /* Create graphics context.  */
   gcmask = GCFunction | GCForeground | GCBackground | GCFillStyle | GCPlaneMask;
   gcvalues.function = GXcopy;
   gcvalues.foreground = setattr.border_pixel;
   gcvalues.background = setattr.border_pixel;
   gcvalues.fill_style = FillSolid;
   gcvalues.plane_mask = AllPlanes;
   _xwin.gc = XCreateGC(_xwin.display, _xwin.window, gcmask, &gcvalues);

   /* Create invisible X cursor.  */
   pixmap = XCreatePixmap(_xwin.display, _xwin.window, 1, 1, 1);
   if (pixmap != None) {
      GC temp_gc;
      XColor color;

      gcmask = GCFunction | GCForeground | GCBackground;
      gcvalues.function = GXcopy;
      gcvalues.foreground = 0;
      gcvalues.background = 0;
      temp_gc = XCreateGC(_xwin.display, pixmap, gcmask, &gcvalues);
      XDrawPoint(_xwin.display, pixmap, temp_gc, 0, 0);
      XFreeGC(_xwin.display, temp_gc);
      color.pixel = 0;
      color.red = color.green = color.blue = 0;
      color.flags = DoRed | DoGreen | DoBlue;
      _xwin.cursor = XCreatePixmapCursor(_xwin.display, pixmap, pixmap, &color, &color, 0, 0);
      XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
      XFreePixmap(_xwin.display, pixmap);
   }
   else {
      _xwin.cursor = XCreateFontCursor(_xwin.display, _xwin.cursor_shape);
      XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
   }

   return 0;
}

int _xwin_create_window(void)
{
   int result;
   XLOCK();
   result = (*_xwin_window_creator)();
   XUNLOCK();
   return result;
}



/* _xwin_destroy_window:
 *  Wrapper for XDestroyWindow.
 */
static void _xwin_private_destroy_window(void)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   _xdga_private_destroy_screen();
#endif
   _xwin_private_destroy_screen();

   if (_xwin.cursor != None) {
      XUndefineCursor(_xwin.display, _xwin.window);
      XFreeCursor(_xwin.display, _xwin.cursor);
      _xwin.cursor = None;
   }

   _xwin.visual = 0;

   if (_xwin.gc != None) {
      XFreeGC(_xwin.display, _xwin.gc);
      _xwin.gc = None;
   }

   if (_xwin.colormap != None) {
      XUninstallColormap(_xwin.display, _xwin.colormap);
      XFreeColormap(_xwin.display, _xwin.colormap);
      _xwin.colormap = None;
   }

   if (_xwin.window != None) {
      XUnmapWindow(_xwin.display, _xwin.window);
      XDestroyWindow(_xwin.display, _xwin.window);
      _xwin.window = None;
   }
}

void _xwin_destroy_window(void)
{
   XLOCK();
   _xwin_private_destroy_window();
   XUNLOCK();
}



typedef void (*_XWIN_SCREEN_TO_BUFFER)(int x, int y, int w, int h);
static _XWIN_SCREEN_TO_BUFFER _xwin_screen_to_buffer_function[5][10] =
{
   {
      _xwin_private_slow_truecolor_8,
      _xwin_private_fast_truecolor_8_to_8,
      _xwin_private_fast_truecolor_8_to_16,
      _xwin_private_fast_truecolor_8_to_24,
      _xwin_private_fast_truecolor_8_to_32,
      _xwin_private_slow_palette_8,
      _xwin_private_fast_palette_8_to_8,
      _xwin_private_fast_palette_8_to_16,
      0,
      _xwin_private_fast_palette_8_to_32
   },
   {
      _xwin_private_slow_truecolor_15,
      _xwin_private_fast_truecolor_15_to_8,
      _xwin_private_fast_truecolor_15_to_16,
      _xwin_private_fast_truecolor_15_to_24,
      _xwin_private_fast_truecolor_15_to_32,
      _xwin_private_slow_palette_15,
      _xwin_private_fast_palette_15_to_8,
      _xwin_private_fast_palette_15_to_16,
      0,
      _xwin_private_fast_palette_15_to_32
   },
   {
      _xwin_private_slow_truecolor_16,
      _xwin_private_fast_truecolor_16_to_8,
      _xwin_private_fast_truecolor_16_to_16,
      _xwin_private_fast_truecolor_16_to_24,
      _xwin_private_fast_truecolor_16_to_32,
      _xwin_private_slow_palette_16,
      _xwin_private_fast_palette_16_to_8,
      _xwin_private_fast_palette_16_to_16,
      0,
      _xwin_private_fast_palette_16_to_32
   },
   {
      _xwin_private_slow_truecolor_24,
      _xwin_private_fast_truecolor_24_to_8,
      _xwin_private_fast_truecolor_24_to_16,
      _xwin_private_fast_truecolor_24_to_24,
      _xwin_private_fast_truecolor_24_to_32,
      _xwin_private_slow_palette_24,
      _xwin_private_fast_palette_24_to_8,
      _xwin_private_fast_palette_24_to_16,
      0,
      _xwin_private_fast_palette_24_to_32
   },
   {
      _xwin_private_slow_truecolor_32,
      _xwin_private_fast_truecolor_32_to_8,
      _xwin_private_fast_truecolor_32_to_16,
      _xwin_private_fast_truecolor_32_to_24,
      _xwin_private_fast_truecolor_32_to_32,
      _xwin_private_slow_palette_32,
      _xwin_private_fast_palette_32_to_8,
      _xwin_private_fast_palette_32_to_16,
      0,
      _xwin_private_fast_palette_32_to_32
   },
};



/* _xwin_select_screen_to_buffer_function:
 *  Select which function should be used for updating frame buffer with screen data.
 */
static void _xwin_private_select_screen_to_buffer_function(void)
{
   int i, j;

   if (_xwin.matching_formats) {
      _xwin.screen_to_buffer = 0;
   }
   else {
      switch (_xwin.screen_depth) {
	 case 8: i = 0; break;
	 case 15: i = 1; break;
	 case 16: i = 2; break;
	 case 24: i = 3; break;
	 case 32: i = 4; break;
	 default: i = 0; break;
      }
      switch (_xwin.fast_visual_depth) {
	 case 0: j = 0; break;
	 case 8: j = 1; break;
	 case 16: j = 2; break;
	 case 24: j = 3; break;
	 case 32: j = 4; break;
	 default: j = 0; break;
      }
      if (!_xwin.visual_is_truecolor)
	 j += 5;
      _xwin.screen_to_buffer = _xwin_screen_to_buffer_function[i][j];
   }
}



/* _xwin_select_set_colors_function:
 *  Select which function should be used for setting hardware colors.
 */
static void _xwin_private_select_set_colors_function(void)
{
   if (_xwin.screen_depth != 8) {
      _xwin.set_colors = 0;
   }
   else {
      if (_xwin.matching_formats) {
	 _xwin.set_colors = _xwin_private_set_matching_colors;
      }
      else if (_xwin.visual_is_truecolor) {
	 _xwin.set_colors = _xwin_private_set_truecolor_colors;
      }
      else {
	 _xwin.set_colors = _xwin_private_set_palette_colors;
      }
   }
}



/* _xwin_setup_driver_desc:
 *  Sets up the X-Windows driver description string.
 */
static void _xwin_private_setup_driver_desc(GFX_DRIVER *drv, int dga)
{
   char tmp1[256], tmp2[128], tmp3[128], tmp4[128];

   /* Prepare driver description.  */
   if (_xwin.matching_formats) {
      uszprintf(_xwin_driver_desc, sizeof(_xwin_driver_desc), 
	        uconvert_ascii("X-Windows graphics, in matching, %d bpp %s", tmp1),
	        _xwin.window_depth,
	        uconvert_ascii((dga ? "DGA 1.0 mode" : "real depth"), tmp2));
   }
   else {
      uszprintf(_xwin_driver_desc, sizeof(_xwin_driver_desc),
	        uconvert_ascii("X-Windows graphics, in %s %s, %d bpp %s", tmp1),
	        uconvert_ascii((_xwin.fast_visual_depth ? "fast" : "slow"), tmp2),
	        uconvert_ascii((_xwin.visual_is_truecolor ? "truecolor" : "paletted"), tmp3),
	        _xwin.window_depth,
	        uconvert_ascii((dga ? "DGA 1.0 mode" : "real depth"), tmp4));
   }
   drv->desc = _xwin_driver_desc;
}



/* _xwin_create_screen:
 *  Creates screen data and other resources.
 */
static BITMAP *_xwin_private_create_screen(GFX_DRIVER *drv, int w, int h,
					   int vw, int vh, int depth, int fullscreen)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   XSetWindowAttributes setattr;
#endif

   if (_xwin.window == None) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("No window"));
      return 0;
   }

   /* Choose convenient size.  */
   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }

   if ((w < 80) || (h < 80) || (w > 4096) || (h > 4096)
       || (vw > 4096) || (vh > 4096)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported screen size"));
      return 0;
   }

   if (vw < w)
      vw = w;
   if (vh < h)
      vh = h;

   if (1
#ifdef ALLEGRO_COLOR8
       && (depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (depth != 15)
       && (depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return 0;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   /* If we are going fullscreen, disable window decorations.  */
   if (fullscreen) {
      setattr.override_redirect = True;
      XChangeWindowAttributes(_xwin.display, _xwin.window,
			      CWOverrideRedirect, &setattr);
      _xwin.override_redirected = 1;
   }
#endif

   /* Set window size and save dimensions.  */
   _xwin_private_resize_window(w, h);
   _xwin.screen_width = w;
   _xwin.screen_height = h;
   _xwin.screen_depth = depth;
   _xwin.virtual_width = vw;
   _xwin.virtual_height = vh;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   if (fullscreen) {
      /* Switch video mode.  */
      if (!_xvidmode_private_set_fullscreen(w, h, 0, 0)) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not set video mode"));
	 return 0;
      }

      /* Hack: make the window fully visible and center cursor.  */
      XMoveWindow(_xwin.display, _xwin.window, 0, 0);
      XF86VidModeSetViewPort(_xwin.display, _xwin.screen, 0, 0);
      XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, 0, 0);
      XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, w - 1, 0);
      XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, 0, h - 1);
      XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, w - 1, h - 1);
      XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, w / 2, h / 2);
      XSync(_xwin.display, False);
	  
      /* Grab the keyboard and mouse.  */
      if (XGrabKeyboard(_xwin.display, XDefaultRootWindow(_xwin.display), False,
			GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not grab keyboard"));
	 return 0;
      }
      _xwin.keyboard_grabbed = 1;
      if (XGrabPointer(_xwin.display, _xwin.window, False,
		       PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		       GrabModeAsync, GrabModeAsync, _xwin.window, None, CurrentTime) != GrabSuccess) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not grab mouse"));
	 return 0;
      }
      _xwin.mouse_grabbed = 1;
   }
#endif

   /* Create XImage with the size of virtual screen.  */
   if (_xwin_private_create_ximage(vw, vh) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not create XImage"));
      return 0;
   }

   /* Prepare visual for further use.  */
   _xwin_private_prepare_visual();

   /* Test that frame buffer is fast (can be accessed directly).  */
   _xwin.fast_visual_depth = _xwin_private_fast_visual_depth();

   /* Create screen bitmap from frame buffer.  */
   return _xwin_private_create_screen_bitmap(drv, 0, _xwin.ximage->data + _xwin.ximage->xoffset,
					     _xwin.ximage->bytes_per_line);
}

BITMAP *_xwin_create_screen(GFX_DRIVER *drv, int w, int h,
			    int vw, int vh, int depth, int fullscreen)
{
   BITMAP *bmp;
   XLOCK();
   bmp = _xwin_private_create_screen(drv, w, h, vw, vh, depth, fullscreen);
   if (bmp == 0)
      _xwin_private_destroy_screen();
   XUNLOCK();
   return bmp;
}



/* _xwin_destroy_screen:
 *  Destroys screen resources.
 */
static void _xwin_private_destroy_screen(void)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   XSetWindowAttributes setattr;
#endif

   if (_xwin.buffer_line != 0) {
      free(_xwin.buffer_line);
      _xwin.buffer_line = 0;
   }

   if (_xwin.screen_line != 0) {
      free(_xwin.screen_line);
      _xwin.screen_line = 0;
   }

   if (_xwin.screen_data != 0) {
      free(_xwin.screen_data);
      _xwin.screen_data = 0;
   }

   _xwin_private_destroy_ximage();

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   if (_xwin.mouse_grabbed) {
      XUngrabPointer(_xwin.display, CurrentTime);
      _xwin.mouse_grabbed = 0;
   }

   if (_xwin.keyboard_grabbed) {
      XUngrabKeyboard(_xwin.display, CurrentTime);
      _xwin.keyboard_grabbed = 0;
   }

   _xvidmode_private_unset_fullscreen();

   if (_xwin.override_redirected) {
      setattr.override_redirect = False;
      XChangeWindowAttributes(_xwin.display, _xwin.window,
			      CWOverrideRedirect, &setattr);
      _xwin.override_redirected = 0;
   }
#endif

   (*_xwin_window_defaultor)();
}

void _xwin_destroy_screen(void)
{
   XLOCK();
   _xwin_private_destroy_screen();
   XUNLOCK();
}



/* _xwin_create_screen_bitmap:
 *  Create screen bitmap from frame buffer.
 */
static BITMAP *_xwin_private_create_screen_bitmap(GFX_DRIVER *drv, int dga,
						  unsigned char *frame_buffer,
						  int bytes_per_buffer_line)
{
   int line;
   int bytes_per_screen_line;
   BITMAP *bmp;

   /* Test that Allegro and X-Windows pixel formats are the same.  */
   _xwin.matching_formats = _xwin_private_matching_formats();

   /* Create mapping tables for color components.  */
   _xwin_private_create_mapping_tables();

   /* Determine how to update frame buffer with screen data.  */
   _xwin_private_select_screen_to_buffer_function();

   /* Determine how to set colors in "hardware".  */
   _xwin_private_select_set_colors_function();

   /* Create line accelerators for screen data.  */
   _xwin.screen_line = malloc(_xwin.virtual_height * sizeof(unsigned char*));
   if (_xwin.screen_line == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return 0;
   }

   /* If formats match, then use frame buffer as screen data, otherwise malloc.  */
   if (_xwin.matching_formats) {
      bytes_per_screen_line = bytes_per_buffer_line;
      _xwin.screen_data = 0;
      _xwin.screen_line[0] = frame_buffer;
   }
   else {
      bytes_per_screen_line = _xwin.virtual_width * BYTES_PER_PIXEL(_xwin.screen_depth);
      _xwin.screen_data = malloc(_xwin.virtual_height * bytes_per_screen_line);
      if (_xwin.screen_data == 0) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
	 return 0;
      }
      _xwin.screen_line[0] = _xwin.screen_data;
   }

   /* Initialize line starts.  */
   for (line = 1; line < _xwin.virtual_height; line++)
      _xwin.screen_line[line] = _xwin.screen_line[line - 1] + bytes_per_screen_line;

   /* Create line accelerators for frame buffer.  */
   if (!_xwin.matching_formats && _xwin.fast_visual_depth) {
      _xwin.buffer_line = malloc(_xwin.virtual_height * sizeof(unsigned char*));
      if (_xwin.buffer_line == 0) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
	 return 0;
      }

      _xwin.buffer_line[0] = frame_buffer;
      for (line = 1; line < _xwin.virtual_height; line++)
	 _xwin.buffer_line[line] = _xwin.buffer_line[line - 1] + bytes_per_buffer_line;
   }

   /* Create bitmap.  */
   bmp = _make_bitmap(_xwin.virtual_width, _xwin.virtual_height,
		      (unsigned long) (_xwin.screen_line[0]), drv,
		      _xwin.screen_depth, bytes_per_screen_line);
   if (bmp == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return 0;
   }

   /* Fixup bitmap fields.  */
   drv->w = bmp->cr = _xwin.screen_width;
   drv->h = bmp->cb = _xwin.screen_height;
   drv->vid_mem = _xwin.virtual_width * _xwin.virtual_height * BYTES_PER_PIXEL(_xwin.screen_depth);

   /* Set bank switch routines.  */
   if (dga) {
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
      if (drv->linear) {
	 /* No need to switch banks in hardware.  */
	 _xwin.bank_switch = 0;
      }
      else {
	 /* Need hardware bank switching.  */
	 _xwin.bank_switch = _xdga_switch_screen_bank;
      }

      if (!_xwin.matching_formats) {
	 /* Need some magic for updating frame buffer.  */
#ifndef ALLEGRO_NO_ASM
	 bmp->write_bank = _xwin_write_line_asm;
	 bmp->vtable->unwrite_bank = _xwin_unwrite_line_asm;
#else
	 bmp->write_bank = _xwin_write_line;
	 bmp->vtable->unwrite_bank = _xwin_unwrite_line;
#endif

	 /* Replace entries in vtable with magical wrappers.  */
	 _xwin_replace_vtable(bmp->vtable);
      }
      else if (!drv->linear) {
	 /* Need hardware bank switching, but no magic.  */
#ifndef ALLEGRO_NO_ASM
	 bmp->read_bank = _xdga_switch_bank_asm;
	 bmp->write_bank = _xdga_switch_bank_asm;
#else
	 bmp->read_bank = _xdga_switch_bank;
	 bmp->write_bank = _xdga_switch_bank;
#endif
      }
#else
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Internal error"));
      return 0;
#endif
   }
   else {
      /* Need some magic for updating frame buffer.  */
#ifndef ALLEGRO_NO_ASM
      bmp->write_bank = _xwin_write_line_asm;
      bmp->vtable->unwrite_bank = _xwin_unwrite_line_asm;
#else
      bmp->write_bank = _xwin_write_line;
      bmp->vtable->unwrite_bank = _xwin_unwrite_line;
#endif

      /* No need to switch banks in hardware.  */
      _xwin.bank_switch = 0;

      /* Replace entries in vtable with magical wrappers.  */
      _xwin_replace_vtable(bmp->vtable);
   }

   /* Initialize other fields in _xwin structure.  */
   _xwin_last_line = -1;
   _xwin_in_gfx_call = 0;
   _xwin.scroll_x = 0;
   _xwin.scroll_y = 0;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   _xdga_last_line = -1;
#endif

   /* Setup driver description string.  */
   _xwin_private_setup_driver_desc(drv, dga);

   return bmp;
}



/* _xwin_create_mapping_tables:
 *  Create mapping between Allegro color component and X-Windows color component.
 */
static void _xwin_private_create_mapping_tables(void)
{
   if (!_xwin.matching_formats) {
      if (_xwin.visual_is_truecolor) {
	 switch (_xwin.screen_depth) {
	    case 8:
	       /* Will be modified later in set_palette.  */
	       _xwin_private_create_mapping(_xwin.rmap, 256, 0, 0);
	       _xwin_private_create_mapping(_xwin.gmap, 256, 0, 0);
	       _xwin_private_create_mapping(_xwin.bmap, 256, 0, 0);
	       break;
	    case 15:
	       _xwin_private_create_mapping(_xwin.rmap, 32, _xwin.rsize, _xwin.rshift);
	       _xwin_private_create_mapping(_xwin.gmap, 32, _xwin.gsize, _xwin.gshift);
	       _xwin_private_create_mapping(_xwin.bmap, 32, _xwin.bsize, _xwin.bshift);
	       break;
	    case 16:
	       _xwin_private_create_mapping(_xwin.rmap, 32, _xwin.rsize, _xwin.rshift);
	       _xwin_private_create_mapping(_xwin.gmap, 64, _xwin.gsize, _xwin.gshift);
	       _xwin_private_create_mapping(_xwin.bmap, 32, _xwin.bsize, _xwin.bshift);
	       break;
	    case 24:
	    case 32:
	       _xwin_private_create_mapping(_xwin.rmap, 256, _xwin.rsize, _xwin.rshift);
	       _xwin_private_create_mapping(_xwin.gmap, 256, _xwin.gsize, _xwin.gshift);
	       _xwin_private_create_mapping(_xwin.bmap, 256, _xwin.bsize, _xwin.bshift);
	       break;
	 }
      }
      else {
	 int i;

	 /* Might be modified later in set_palette.  */
	 for (i = 0; i < 256; i++)
	    _xwin.rmap[i] = _xwin.gmap[i] = _xwin.bmap[i] = 0;
      }
   }
}



/* _xwin_create_mapping:
 *  Create mapping between Allegro color component and X-Windows color component.
 */
static void _xwin_private_create_mapping(unsigned long *map, int ssize, int dsize, int dshift)
{
   int i, smax, dmax;

   smax = ssize - 1;
   dmax = dsize - 1;
   for (i = 0; i < ssize; i++)
      map[i] = ((dmax * i) / smax) << dshift;
   for (; i < 256; i++)
      map[i] = map[i % ssize];
}



/* _xwin_display_is_local:
 *  Tests that display connection is local.
 *  (Note: this is duplicated in xdga2.c).
 */
static int _xwin_private_display_is_local(void)
{
   char *name;

   if (_xwin.display == 0)
      return 0;

   /* Get display name and test for local display.  */
   name = XDisplayName(0);

   return (((name == 0) || (name[0] == ':') || (strncmp(name, "unix:", 5) == 0)) ? 1 : 0);
}



/* _xwin_create_ximage:
 *  Create XImage for accessing window.
 */
static int _xwin_private_create_ximage(int w, int h)
{
   XImage *image = 0; /* I'm mage or I'm old?  */

   if (_xwin.display == 0)
      return -1;

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   if (_xwin_private_display_is_local() && XShmQueryExtension(_xwin.display))
      _xwin.use_shm = 1;
   else
      _xwin.use_shm = 0;
#else
   _xwin.use_shm = 0;
#endif

#ifdef ALLEGRO_XWINDOWS_WITH_SHM
   if (_xwin.use_shm) {
      /* Try to create shared memory XImage.  */
      image = XShmCreateImage(_xwin.display, _xwin.visual, _xwin.window_depth,
			      ZPixmap, 0, &_xwin.shminfo, w, h);
      do {
	 if (image != 0) {
	    /* Create shared memory segment.  */
	    _xwin.shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,
					 IPC_CREAT | 0777);
	    if (_xwin.shminfo.shmid != -1) {
	       /* Attach shared memory to our address space.  */
	       _xwin.shminfo.shmaddr = image->data = shmat(_xwin.shminfo.shmid, 0, 0);
	       if (_xwin.shminfo.shmaddr != (char*) -1) {
		  _xwin.shminfo.readOnly = True;

		  /* Attach shared memory to the X-server address space.  */
		  if (XShmAttach(_xwin.display, &_xwin.shminfo))
		     break;

		  shmdt(_xwin.shminfo.shmaddr);
	       }
	       shmctl(_xwin.shminfo.shmid, IPC_RMID, 0);
	    }
	    XDestroyImage(image);
	    image = 0;
	 }
	 _xwin.use_shm = 0;
      } while (0);
   }
#endif

   if (image == 0) {
      /* Try to create ordinary XImage.  */
#if 0
      Pixmap pixmap;

      pixmap = XCreatePixmap(_xwin.display, _xwin.window, w, h, _xwin.window_depth);
      if (pixmap != None) {
	 image = XGetImage(_xwin.display, pixmap, 0, 0, w, h, AllPlanes, ZPixmap);
	 XFreePixmap(_xwin.display, pixmap);
      }
#else
      image = XCreateImage(_xwin.display, _xwin.visual, _xwin.window_depth,
			   ZPixmap, 0, 0, w, h, 32, 0);
      if (image != 0) {
	 image->data = malloc(image->bytes_per_line * image->height);
	 if (image->data == 0) {
	    XDestroyImage(image);
	    image = 0;
	 }
      }
#endif
   }

   _xwin.ximage = image;

   return ((image != 0) ? 0 : -1);
}



/* _xwin_destroy_ximage:
 *  Destroy XImage.
 */
static void _xwin_private_destroy_ximage(void)
{
   if (_xwin.ximage != 0) {
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
      if (_xwin.use_shm) {
	 XShmDetach(_xwin.display, &_xwin.shminfo);
	 shmdt(_xwin.shminfo.shmaddr);
	 shmctl(_xwin.shminfo.shmid, IPC_RMID, 0);
      }
#endif
      XDestroyImage(_xwin.ximage);
      _xwin.ximage = 0;
   }
}



/* _xwin_prepare_visual:
 *  Prepare visual for further use.
 */
static void _xwin_private_prepare_visual(void)
{
   int i, r, g, b;
   int rmax, gmax, bmax;
   unsigned long mask;
   XColor color;

   if ((_xwin.visual->class == TrueColor)
       || (_xwin.visual->class == DirectColor)) {
      /* Use TrueColor and DirectColor visuals as truecolor.  */

      /* Red shift and size.  */
      for (mask = _xwin.visual->red_mask, i = 0; (mask & 1) != 1; mask >>= 1)
	 i++;
      _xwin.rshift = i;
      for (i = 0; mask != 0; mask >>= 1)
	 i++;
      _xwin.rsize = 1 << i;

      /* Green shift and size.  */
      for (mask = _xwin.visual->green_mask, i = 0; (mask & 1) != 1; mask >>= 1)
	 i++;
      _xwin.gshift = i;
      for (i = 0; mask != 0; mask >>= 1)
	 i++;
      _xwin.gsize = 1 << i;

      /* Blue shift and size.  */
      for (mask = _xwin.visual->blue_mask, i = 0; (mask & 1) != 1; mask >>= 1)
	 i++;
      _xwin.bshift = i;
      for (i = 0; mask != 0; mask >>= 1)
	 i++;
      _xwin.bsize = 1 << i;

      /* Convert DirectColor visual into true color visual.  */
      if (_xwin.visual->class == DirectColor) {
	 color.flags = DoRed;
	 rmax = _xwin.rsize - 1;
	 gmax = _xwin.gsize - 1;
	 bmax = _xwin.bsize - 1;
	 for (i = 0; i < _xwin.rsize; i++) {
	    color.pixel = i << _xwin.rshift;
	    color.red = ((rmax <= 0) ? 0 : ((i * 65535L) / rmax));
	    XStoreColor(_xwin.display, _xwin.colormap, &color);
	 }
	 color.flags = DoGreen;
	 for (i = 0; i < _xwin.gsize; i++) {
	    color.pixel = i << _xwin.gshift;
	    color.green = ((gmax <= 0) ? 0 : ((i * 65535L) / gmax));
	    XStoreColor(_xwin.display, _xwin.colormap, &color);
	 }
	 color.flags = DoBlue;
	 for (i = 0; i < _xwin.bsize; i++) {
	    color.pixel = i << _xwin.bshift;
	    color.blue = ((bmax <= 0) ? 0 : ((i * 65535L) / bmax));
	    XStoreColor(_xwin.display, _xwin.colormap, &color);
	 }
      }

      _xwin.visual_is_truecolor = 1;
   }
   else if ((_xwin.visual->class == PseudoColor)
	    || (_xwin.visual->class == GrayScale)) {
      /* Convert read-write palette visual into true color visual.  */
      b = _xwin.window_depth / 3;
      r = (_xwin.window_depth - b) / 2;
      g = _xwin.window_depth - r - b;

      _xwin.rsize = 1 << r;
      _xwin.gsize = 1 << g;
      _xwin.bsize = 1 << b;
      _xwin.rshift = g + b;
      _xwin.gshift = b;
      _xwin.bshift = 0;

      _xwin.visual_is_truecolor = 1;

      rmax = _xwin.rsize - 1;
      gmax = _xwin.gsize - 1;
      bmax = _xwin.bsize - 1;

      color.flags = DoRed | DoGreen | DoBlue;
      for (r = 0; r < _xwin.rsize; r++) {
	 for (g = 0; g < _xwin.gsize; g++) {
	    for (b = 0; b < _xwin.bsize; b++) {
	       color.pixel = (r << _xwin.rshift) | (g << _xwin.gshift) | (b << _xwin.bshift);
	       color.red = ((rmax <= 0) ? 0 : ((r * 65535L) / rmax));
	       color.green = ((gmax <= 0) ? 0 : ((g * 65535L) / gmax));
	       color.blue = ((bmax <= 0) ? 0 : ((b * 65535L) / bmax));
	       XStoreColor(_xwin.display, _xwin.colormap, &color);
	    }
	 }
      }
   }
   else {
      /* All other visual types are paletted.  */
      _xwin.rsize = 1;
      _xwin.bsize = 1;
      _xwin.gsize = 1;
      _xwin.rshift = 0;
      _xwin.gshift = 0;
      _xwin.bshift = 0;

      _xwin.visual_is_truecolor = 0;

      /* Make fixed palette and create mapping RRRRGGGGBBBB -> palette index.  */
      for (r = 0; r < 16; r++) {
	 for (g = 0; g < 16; g++) {
	    for (b = 0; b < 16; b++) {
	       color.red = (r * 65535L) / 15;
	       color.green = (g * 65535L) / 15;
	       color.blue = (b * 65535L) / 15;
	       XAllocColor(_xwin.display, _xwin.colormap, &color);
	       _xwin.cmap[(r << 8) | (g << 4) | b] = color.pixel;
	    }
	 }
      }
   }
}



/* _xwin_matching_formats:
 *  Find if color formats match.
 */
static int _xwin_private_matching_formats(void)
{
   if (_xwin.fast_visual_depth == 0)
      return 0;

   if (_xwin.screen_depth == 8) {
      /* For matching 8 bpp modes visual must be PseudoColor or GrayScale.  */
      if (((_xwin.visual->class != PseudoColor)
	   && (_xwin.visual->class != GrayScale))
	  || (_xwin.fast_visual_depth != 8)
	  || (_xwin.window_depth != 8))
	 return 0;
   }
   else if ((_xwin.visual->class != TrueColor)
	    && (_xwin.visual->class != DirectColor)) {
      /* For matching true color modes visual must be TrueColor or DirectColor.  */
      return 0;
   }
   else if (_xwin.screen_depth == 15) {
      if ((_xwin.fast_visual_depth != 16)
	  || (_xwin.rsize != 32) || (_xwin.gsize != 32) || (_xwin.bsize != 32)
	  || ((_xwin.rshift != 0) && (_xwin.rshift != 10))
	  || ((_xwin.bshift != 0) && (_xwin.bshift != 10))
	  || (_xwin.gshift != 5))
	 return 0;
      _rgb_r_shift_15 = _xwin.rshift;
      _rgb_g_shift_15 = _xwin.gshift;
      _rgb_b_shift_15 = _xwin.bshift;
   }
   else if (_xwin.screen_depth == 16) {
      if ((_xwin.fast_visual_depth != 16)
	  || (_xwin.rsize != 32) || (_xwin.gsize != 64) || (_xwin.bsize != 32)
	  || ((_xwin.rshift != 0) && (_xwin.rshift != 11))
	  || ((_xwin.bshift != 0) && (_xwin.bshift != 11))
	  || (_xwin.gshift != 5))
	 return 0;
      _rgb_r_shift_16 = _xwin.rshift;
      _rgb_g_shift_16 = _xwin.gshift;
      _rgb_b_shift_16 = _xwin.bshift;
   }
   else if (_xwin.screen_depth == 24){
      if ((_xwin.fast_visual_depth != 24)
	  || (_xwin.rsize != 256) || (_xwin.gsize != 256) || (_xwin.bsize != 256)
	  || ((_xwin.rshift != 0) && (_xwin.rshift != 16))
	  || ((_xwin.bshift != 0) && (_xwin.bshift != 16))
	  || (_xwin.gshift != 8))
	 return 0;
      _rgb_r_shift_24 = _xwin.rshift;
      _rgb_g_shift_24 = _xwin.gshift;
      _rgb_b_shift_24 = _xwin.bshift;
   }
   else if (_xwin.screen_depth == 32) {
      if ((_xwin.fast_visual_depth != 32)
	  || (_xwin.rsize != 256) || (_xwin.gsize != 256) || (_xwin.bsize != 256)
	  || ((_xwin.rshift != 0) && (_xwin.rshift != 16))
	  || ((_xwin.bshift != 0) && (_xwin.bshift != 16))
	  || (_xwin.gshift != 8))
	 return 0;
      _rgb_r_shift_32 = _xwin.rshift;
      _rgb_g_shift_32 = _xwin.gshift;
      _rgb_b_shift_32 = _xwin.bshift;
   }
   else {
      /* How did you get in here?  */
      return 0;
   }

   return 1;
}



/* _xwin_fast_visual_depth:
 *  Find which depth is fast (when XImage can be accessed directly).
 */
static int _xwin_private_fast_visual_depth(void)
{
   int ok, x, sizex;
   int test_depth;
   unsigned char *p8;
   unsigned short *p16;
   unsigned long *p32;

   if (_xwin.ximage == 0)
      return 0;

   /* Use first line of XImage for test.  */
   p8 = _xwin.ximage->data + _xwin.ximage->xoffset;
   p16 = (unsigned short*) p8;
   p32 = (unsigned long*) p8;

   sizex = _xwin.ximage->bytes_per_line - _xwin.ximage->xoffset;

   if ((_xwin.window_depth < 1) || (_xwin.window_depth > 32)) {
      return 0;
   }
   else if (_xwin.window_depth > 16) {
      test_depth = 32;
      sizex /= sizeof (unsigned long);
   }
   else if (_xwin.window_depth > 8) {
      test_depth = 16;
      sizex /= sizeof (unsigned short);
   }
   else {
      test_depth = 8;
   }
   if (sizex > _xwin.ximage->width)
      sizex = _xwin.ximage->width;

   /* Need at least two pixels wide line for test.  */
   if (sizex < 2)
      return 0;

   ok = 1;
   for (x = 0; x < sizex; x++) {
      int bit;

      for (bit = -1; bit < _xwin.window_depth; bit++) {
	 unsigned long color = ((bit < 0) ? 0 : ((unsigned long) 1 << bit));

	 /* Write color through XImage API.  */
	 XPutPixel(_xwin.ximage, x, 0, color);

	 /* Read color with direct access.  */
	 switch (test_depth) {
	    case 8:
	       if (p8[x] != color)
		  ok = 0;
	       break;
	    case 16:
	       if (p16[x] != color)
		  ok = 0;
	       break;
	    case 32:
	       if (p32[x] != color)
		  ok = 0;
	       break;
	    default:
	       ok = 0;
	       break;
	 }
	 XPutPixel(_xwin.ximage, x, 0, 0);

	 if (!ok)
	    return 0;
      }
   }

   return test_depth;
}



#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA

/* _xdga_fast_visual_depth:
 *  Find which depth is fast (when video buffer can be accessed directly).
 */
static int _xdga_private_fast_visual_depth(void)
{
/* Quoted from the xmms sources:
   "The things we must go through to get a proper depth...
   If I find the idiot that thought making 32bit report 24bit was a good idea,
   there may be one less `programmer' in this world..." */
   XImage *img = XGetImage(_xwin.display, XDefaultRootWindow(_xwin.display),
			   0, 0, 1, 1, AllPlanes, ZPixmap);
   int dga_depth = img->bits_per_pixel;

   if (dga_depth == 15)
      dga_depth = 16;

   XDestroyImage(img);

   return dga_depth;
}

#endif



/* Macro for switching banks (mainly for DGA mode).  */
#define XWIN_BANK_SWITCH(line)          \
if (_xwin.bank_switch)                  \
   (*_xwin.bank_switch)(line)



/*
 * Functions for copying screen data to frame buffer.
 */
#define MAKE_FAST_TRUECOLOR(name,stype,dtype,rshift,gshift,bshift,rmask,gmask,bmask)    \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int y, x;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; x--) {                                                   \
	 unsigned long color = *s++;                                                    \
	 *d++ = (_xwin.rmap[(color >> (rshift)) & (rmask)]                              \
		 | _xwin.gmap[(color >> (gshift)) & (gmask)]                            \
		 | _xwin.bmap[(color >> (bshift)) & (bmask)]);                          \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_TRUECOLOR24(name,dtype)                                               \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; s += 3, x--) {                                           \
	 *d++ = (_xwin.rmap[s[0]] | _xwin.gmap[s[1]] | _xwin.bmap[s[2]]);               \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_TRUECOLOR_TO24(name,stype,rshift,gshift,bshift,rmask,gmask,bmask)     \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      unsigned char *d = _xwin.buffer_line[y] + 3 * sx;                                 \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; d += 3, x--) {                                           \
	 unsigned long color = *s++;                                                    \
	 color = (_xwin.rmap[(color >> (rshift)) & (rmask)]                             \
		  | _xwin.gmap[(color >> (gshift)) & (gmask)]                           \
		  | _xwin.bmap[(color >> (bshift)) & (bmask)]);                         \
	 d[0] = (color & 0xFF);                                                         \
	 d[1] = (color >> 8) & 0xFF;                                                    \
	 d[2] = (color >> 16) & 0xFF;                                                   \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_TRUECOLOR24_TO24(name)                                                \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      unsigned char *d = _xwin.buffer_line[y] + 3 * sx;                                 \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; s += 3, d += 3, x--) {                                   \
	 unsigned long color = _xwin.rmap[s[0]] | _xwin.gmap[s[1]] | _xwin.bmap[s[2]];  \
	 d[0] = (color & 0xFF);                                                         \
	 d[1] = (color >> 8) & 0xFF;                                                    \
	 d[2] = (color >> 16) & 0xFF;                                                   \
      }                                                                                 \
   }                                                                                    \
}

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_8,
		    unsigned char, unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_16,
		    unsigned char, unsigned short, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_8_to_32,
		    unsigned char, unsigned long, 0, 0, 0, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_8,
		    unsigned short, unsigned char, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_16,
		    unsigned short, unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_15_to_32,
		    unsigned short, unsigned long, 0, 5, 10, 0x1F, 0x1F, 0x1F);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_8,
		    unsigned short, unsigned char, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_16,
		    unsigned short, unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_16_to_32,
		    unsigned short, unsigned long, 0, 5, 11, 0x1F, 0x3F, 0x1F);

MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_8,
		    unsigned long, unsigned char, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_16,
		    unsigned long, unsigned short, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR(_xwin_private_fast_truecolor_32_to_32,
		    unsigned long, unsigned long, 0, 8, 16, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_8,
		      unsigned char);
MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_16,
		      unsigned short);
MAKE_FAST_TRUECOLOR24(_xwin_private_fast_truecolor_24_to_32,
		      unsigned long);

MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_8_to_24,
			 unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_15_to_24,
			 unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_16_to_24,
			 unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_FAST_TRUECOLOR_TO24(_xwin_private_fast_truecolor_32_to_24,
			 unsigned long, 0, 8, 16, 0xFF, 0xFF, 0xFF);

MAKE_FAST_TRUECOLOR24_TO24(_xwin_private_fast_truecolor_24_to_24);

#define MAKE_FAST_PALETTE8(name,dtype)                                                  \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + sx;                                     \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; x--) {                                                   \
	 unsigned long color = *s++;                                                    \
	 *d++ = _xwin.cmap[(_xwin.rmap[color]                                           \
			    | _xwin.gmap[color]                                         \
			    | _xwin.bmap[color])];                                      \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_PALETTE(name,stype,dtype,rshift,gshift,bshift)                        \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; x--) {                                                   \
	 unsigned long color = *s++;                                                    \
	 *d++ = _xwin.cmap[((((color >> (rshift)) & 0x0F) << 8)                         \
			    | (((color >> (gshift)) & 0x0F) << 4)                       \
			    | ((color >> (bshift)) & 0x0F))];                           \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_FAST_PALETTE24(name,dtype)                                                 \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      dtype *d = (dtype*) (_xwin.buffer_line[y]) + sx;                                  \
      XWIN_BANK_SWITCH(y);                                                              \
      for (x = sw - 1; x >= 0; s += 3, x--) {                                           \
	 *d++ = _xwin.cmap[((((unsigned long) s[0] << 4) & 0xF00)                       \
			    | ((unsigned long) s[1] & 0xF0)                             \
			    | (((unsigned long) s[2] >> 4) & 0x0F))];                   \
      }                                                                                 \
   }                                                                                    \
}

MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_8,
		   unsigned char);
MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_16,
		   unsigned short);
MAKE_FAST_PALETTE8(_xwin_private_fast_palette_8_to_32,
		   unsigned long);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_8,
		  unsigned short, unsigned char, 1, 6, 11);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_16,
		  unsigned short, unsigned short, 1, 6, 11);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_15_to_32,
		  unsigned short, unsigned long, 1, 6, 11);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_8,
		  unsigned short, unsigned char, 1, 7, 12);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_16,
		  unsigned short, unsigned short, 1, 7, 12);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_16_to_32,
		  unsigned short, unsigned long, 1, 7, 12);

MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_8,
		  unsigned long, unsigned char, 4, 12, 20);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_16,
		  unsigned long, unsigned short, 4, 12, 20);
MAKE_FAST_PALETTE(_xwin_private_fast_palette_32_to_32,
		  unsigned long, unsigned long, 4, 12, 20);

MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_8,
		    unsigned char);
MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_16,
		    unsigned short);
MAKE_FAST_PALETTE24(_xwin_private_fast_palette_24_to_32,
		    unsigned long);

#define MAKE_SLOW_TRUECOLOR(name,stype,rshift,gshift,bshift,rmask,gmask,bmask)          \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      for (x = sx; x < (sx + sw); x++) {                                                \
	 unsigned long color = *s++;                                                    \
	 XPutPixel (_xwin.ximage, x, y,                                                 \
		    (_xwin.rmap[(color >> (rshift)) & (rmask)]                          \
		     | _xwin.gmap[(color >> (gshift)) & (gmask)]                        \
		     | _xwin.bmap[(color >> (bshift)) & (bmask)]));                     \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_SLOW_TRUECOLOR24(name)                                                     \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      for (x = sx; x < (sx + sw); s += 3, x++) {                                        \
	 XPutPixel(_xwin.ximage, x, y,                                                  \
		   (_xwin.rmap[s[0]] | _xwin.gmap[s[1]] | _xwin.bmap[s[2]]));           \
      }                                                                                 \
   }                                                                                    \
}

MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_8, unsigned char, 0, 0, 0, 0xFF, 0xFF, 0xFF);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_15, unsigned short, 0, 5, 10, 0x1F, 0x1F, 0x1F);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_16, unsigned short, 0, 5, 11, 0x1F, 0x3F, 0x1F);
MAKE_SLOW_TRUECOLOR(_xwin_private_slow_truecolor_32, unsigned long, 0, 8, 16, 0xFF, 0xFF, 0xFF);
MAKE_SLOW_TRUECOLOR24(_xwin_private_slow_truecolor_24);

#define MAKE_SLOW_PALETTE8(name)                                                        \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + sx;                                     \
      for (x = sx; x < (sx + sw); x++) {                                                \
	 unsigned long color = *s++;                                                    \
	 XPutPixel(_xwin.ximage, x, y,                                                  \
		   _xwin.cmap[(_xwin.rmap[color]                                        \
			       | _xwin.gmap[color]                                      \
			       | _xwin.bmap[color])]);                                  \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_SLOW_PALETTE(name,stype,rshift,gshift,bshift)                              \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      stype *s = (stype*) (_xwin.screen_line[y]) + sx;                                  \
      for (x = sx; x < (sx + sw); x++) {                                                \
	 unsigned long color = *s++;                                                    \
	 XPutPixel(_xwin.ximage, x, y,                                                  \
		   _xwin.cmap[((((color >> (rshift)) & 0x0F) << 8)                      \
			       | (((color >> (gshift)) & 0x0F) << 4)                    \
			       | ((color >> (bshift)) & 0x0F))]);                       \
      }                                                                                 \
   }                                                                                    \
}

#define MAKE_SLOW_PALETTE24(name)                                                       \
static void name(int sx, int sy, int sw, int sh)                                        \
{                                                                                       \
   int x, y;                                                                            \
   for (y = sy; y < (sy + sh); y++) {                                                   \
      unsigned char *s = _xwin.screen_line[y] + 3 * sx;                                 \
      for (x = sx; x < (sx + sw); s += 3, x++) {                                        \
	 XPutPixel(_xwin.ximage, x, y,                                                  \
		   _xwin.cmap[((((unsigned long) s[0] << 4) & 0xF00)                    \
			       | ((unsigned long) s[1] & 0xF0)                          \
			       | (((unsigned long) s[2] >> 4) & 0x0F))]);               \
      }                                                                                 \
   }                                                                                    \
}

MAKE_SLOW_PALETTE8(_xwin_private_slow_palette_8);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_15, unsigned short, 1, 6, 11);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_16, unsigned short, 1, 7, 12);
MAKE_SLOW_PALETTE(_xwin_private_slow_palette_32, unsigned long, 4, 12, 20);
MAKE_SLOW_PALETTE24(_xwin_private_slow_palette_24);

/*
 * Functions for setting "hardware" colors in 8bpp modes.
 */
static void _xwin_private_set_matching_colors(AL_CONST PALETTE p, int from, int to)
{
   int i;
   static XColor color[256];

   for (i = from; i <= to; i++) {
      color[i].flags = DoRed | DoGreen | DoBlue;
      color[i].pixel = i;
      color[i].red = ((p[i].r & 0x3F) * 65535L) / 0x3F;
      color[i].green = ((p[i].g & 0x3F) * 65535L) / 0x3F;
      color[i].blue = ((p[i].b & 0x3F) * 65535L) / 0x3F;
   }
   XStoreColors(_xwin.display, _xwin.colormap, color + from, to - from + 1);
}

static void _xwin_private_set_truecolor_colors(AL_CONST PALETTE p, int from, int to)
{
   int i, rmax, gmax, bmax;

   rmax = _xwin.rsize - 1;
   gmax = _xwin.gsize - 1;
   bmax = _xwin.bsize - 1;
   for (i = from; i <= to; i++) {
      _xwin.rmap[i] = (((p[i].r & 0x3F) * rmax) / 0x3F) << _xwin.rshift;
      _xwin.gmap[i] = (((p[i].g & 0x3F) * gmax) / 0x3F) << _xwin.gshift;
      _xwin.bmap[i] = (((p[i].b & 0x3F) * bmax) / 0x3F) << _xwin.bshift;
   }
}

static void _xwin_private_set_palette_colors(AL_CONST PALETTE p, int from, int to)
{
   int i;

   for (i = from; i <= to; i++) {
      _xwin.rmap[i] = (((p[i].r & 0x3F) * 15) / 0x3F) << 8;
      _xwin.gmap[i] = (((p[i].g & 0x3F) * 15) / 0x3F) << 4;
      _xwin.bmap[i] = (((p[i].b & 0x3F) * 15) / 0x3F);
   }
}

static void _xwin_private_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   /* Wait for VBI.  */
   if (vsync)
      _xwin_private_vsync();

   if (_xwin.set_colors != 0) {
      /* Set "hardware" colors.  */
      (*(_xwin.set_colors))(p, from, to);

      /* Update XImage and window.  */
      if (!_xwin.matching_formats)
	 _xwin_private_update_screen(0, 0, _xwin.virtual_width, _xwin.virtual_height);
   }
}

void _xwin_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   XLOCK();
   _xwin_private_set_palette_range(p, from, to, vsync);
   XUNLOCK();
}



/* _xwin_set_window_defaults:
 *  Set default window parameters.
 */
static void _xwin_private_set_window_defaults(void)
{
   XClassHint hint;
   XWMHints wm_hints;

   if (_xwin.window == None)
      return;

   /* Set window title.  */
   XStoreName(_xwin.display, _xwin.window, _xwin.window_title);

   /* Set hints.  */
   hint.res_name = _xwin.application_name;
   hint.res_class = _xwin.application_class;
   XSetClassHint(_xwin.display, _xwin.window, &hint);

   wm_hints.flags = InputHint | StateHint;
   wm_hints.input = True;
   wm_hints.initial_state = NormalState;
   XSetWMHints(_xwin.display, _xwin.window, &wm_hints);
}



/* _xwin_flush_buffers:
 *  Flush input and output X-buffers.
 */
static void _xwin_private_flush_buffers(void)
{
   if (_xwin.display != 0)
      XSync(_xwin.display, False);
}

void _xwin_flush_buffers(void)
{
   XLOCK();
   _xwin_private_flush_buffers();
   XUNLOCK();
}



/* _xwin_vsync:
 *  Emulation of vsync.
 */
static void _xwin_private_vsync(void)
{
#if 0
   (*_xwin_window_redrawer)(0, 0, _xwin.screen_width, _xwin.screen_height);
#endif
   _xwin_private_flush_buffers();
}

void _xwin_vsync(void)
{
   XLOCK();
   _xwin_private_vsync();
   XUNLOCK();

   if (_timer_installed) {
      int prev = retrace_count;

      do {
      } while (retrace_count == prev);
   }
}



/* _xwin_resize_window:
 *  Wrapper for XResizeWindow.
 */
static void _xwin_private_resize_window(int w, int h)
{
   XSizeHints *hints;

   if (_xwin.window == None)
      return;

   /* New window size.  */
   _xwin.window_width = w;
   _xwin.window_height = h;

   /* Resize window.  */
   XUnmapWindow(_xwin.display, _xwin.window);
   XResizeWindow(_xwin.display, _xwin.window, w, h);
   XMapWindow(_xwin.display, _xwin.window);

   hints = XAllocSizeHints();
   if (hints == 0)
      return;

   /* Set size and position hints for Window Manager.  */
   hints->flags = PMinSize | PMaxSize | PBaseSize;
   hints->min_width  = hints->max_width  = hints->base_width  = w;
   hints->min_height = hints->max_height = hints->base_height = h;
   XSetWMNormalHints(_xwin.display, _xwin.window, hints);

   XFree(hints);
}



/* _xwin_process_event:
 *  Process one event.
 */
static void _xwin_private_process_event(XEvent *event)
{
   int kcode, scode, dx, dy, dz = 0;
   static int mouse_buttons = 0;
   static int mouse_savedx = 0;
   static int mouse_savedy = 0;
   static int mouse_warp_now = 0;
   static int mouse_was_warped = 0;
   static int keyboard_got_focus = FALSE;

   switch (event->type) {
      case KeyPress:
         if (keyboard_got_focus && _xwin_keyboard_focused) {
            int state = 0;

            if (event->xkey.state & Mod5Mask)
               state |= KB_SCROLOCK_FLAG;

            if (event->xkey.state & Mod2Mask)
               state |= KB_NUMLOCK_FLAG;

            if (event->xkey.state & LockMask)
               state |= KB_CAPSLOCK_FLAG;

            (*_xwin_keyboard_focused)(TRUE, state);
            keyboard_got_focus = FALSE;
         } 

	 /* Key pressed.  */
	 kcode = event->xkey.keycode;
	 if ((kcode >= 0) && (kcode < 256) && (!_xwin_keycode_pressed[kcode])) {
	    if (_xwin_keyboard_callback)
	       (*_xwin_keyboard_callback)(1, kcode);
	    scode = _xwin.keycode_to_scancode[kcode];
	    if ((scode > 0) && (_xwin_keyboard_interrupt != 0)) {
	       _xwin_keycode_pressed[kcode] = TRUE;
	       (*_xwin_keyboard_interrupt)(1, scode);
	    }
	 }
	 break;
      case KeyRelease:
	 /* Key release.  */
	 kcode = event->xkey.keycode;
	 if ((kcode >= 0) && (kcode < 256) && _xwin_keycode_pressed[kcode]) {
	    if (_xwin_keyboard_callback)
	       (*_xwin_keyboard_callback)(0, kcode);
	    scode = _xwin.keycode_to_scancode[kcode];
	    if ((scode > 0) && (_xwin_keyboard_interrupt != 0)) {
	       (*_xwin_keyboard_interrupt)(0, scode);
	       _xwin_keycode_pressed[kcode] = FALSE;
	    }
	 }
	 break;
      case FocusIn:
	 /* Gaining input focus.  */
         keyboard_got_focus = TRUE;
	 break;
      case FocusOut:
	 /* Losing input focus.  */
	 if (_xwin_keyboard_focused)
	    (*_xwin_keyboard_focused)(FALSE, 0);
	 for (kcode = 0; kcode < 256; kcode++)
	    _xwin_keycode_pressed[kcode] = FALSE;
	 break;
      case ButtonPress:
	 /* Mouse button pressed.  */
	 if (event->xbutton.button == Button1)
	    mouse_buttons |= 1;
	 else if (event->xbutton.button == Button3)
	    mouse_buttons |= 2;
	 else if (event->xbutton.button == Button2)
	    mouse_buttons |= 4;
	 else if (event->xbutton.button == Button4)
	    dz = 1;
	 else if (event->xbutton.button == Button5)
	    dz = -1;
	 if (_xwin_mouse_interrupt)
	    (*_xwin_mouse_interrupt)(0, 0, dz, mouse_buttons);
	 break;
      case ButtonRelease:
	 /* Mouse button released.  */
	 if (event->xbutton.button == Button1)
	    mouse_buttons &= ~1;
	 else if (event->xbutton.button == Button3)
	    mouse_buttons &= ~2;
	 else if (event->xbutton.button == Button2)
	    mouse_buttons &= ~4;
	 if (_xwin_mouse_interrupt)
	    (*_xwin_mouse_interrupt)(0, 0, 0, mouse_buttons);
	 break;
      case MotionNotify:
	 /* Mouse moved.  */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
	 if (_xwin.in_dga_mode) {
	    /* DGA mode.  */
	    if (!_xwin.disable_dga_mouse) {
	       dx = event->xmotion.x;
	       dy = event->xmotion.y;
	    }
	    else {
	       /* Buggy X servers send absolute instead of relative offsets.  */
	       dx = event->xmotion.x - mouse_savedx;
	       dy = event->xmotion.y - mouse_savedy;
	       mouse_savedx = event->xmotion.x;
	       mouse_savedy = event->xmotion.y;

	       /* Ignore the event we just generated with XWarpPointer.  */
	       if (mouse_was_warped) {
		  mouse_was_warped = 0;
		  break;
	       }
	     	       
	       /* Warp X-cursor to the center of the screen.  */
	       XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, 
			    _xwin.screen_width / 2, _xwin.screen_height / 2);
	       mouse_was_warped = 1;
	    }
	    
	    if (((dx != 0) || (dy != 0)) && _xwin_mouse_interrupt) {
	       /* Move Allegro cursor.  */
	       (*_xwin_mouse_interrupt)(dx, dy, 0, mouse_buttons);
	    }
	    break;
	 }
#endif
	 /* Windowed mode.  */
	 dx = event->xmotion.x - mouse_savedx;
	 dy = event->xmotion.y - mouse_savedy;
	 /* Discard some events after warp.  */
	 if (mouse_was_warped && ((dx != 0) || (dy != 0)) && (mouse_was_warped++ < 16))
	    break;
	 mouse_savedx = event->xmotion.x;
	 mouse_savedy = event->xmotion.y;
	 mouse_was_warped = 0;
	 if (!_xwin.mouse_warped) {
	    /* Move Allegro cursor to X-cursor.  */
	    dx = event->xmotion.x - _mouse_x;
	    dy = event->xmotion.y - _mouse_y;
	 }
	 if (((dx != 0) || (dy != 0)) && _xwin_mouse_interrupt) {
	    if (_xwin.mouse_warped && (mouse_warp_now++ & 4)) {
	       /* Warp X-cursor to the center of the window.  */
	       mouse_savedx = _xwin.window_width / 2;
	       mouse_savedy = _xwin.window_height / 2;
	       mouse_was_warped = 1;
	       XWarpPointer(_xwin.display, _xwin.window, _xwin.window,
			    0, 0, _xwin.window_width, _xwin.window_height,
			    mouse_savedx, mouse_savedy);
	    }
	    /* Move Allegro cursor.  */
	    (*_xwin_mouse_interrupt)(dx, dy, 0, mouse_buttons);
	 }
	 break;
      case EnterNotify:
	 /* Mouse entered window.  */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
	 if (_xwin.in_dga_mode) {
	    /* DGA mode.  */
	    if (_xwin_mouse_interrupt)
	       (*_xwin_mouse_interrupt)(0, 0, 0, mouse_buttons);
	    break;
	 }
	 else
#endif
	 /* Windowed mode.  */
	 _mouse_on = TRUE;
	 mouse_savedx = event->xcrossing.x;
	 mouse_savedy = event->xcrossing.y;
	 mouse_was_warped = 0;
	 if (!_xwin.mouse_warped) {
	    /* Move Allegro cursor to X-cursor.  */
	    dx = event->xcrossing.x - _mouse_x;
	    dy = event->xcrossing.y - _mouse_y;
	    if (((dx != 0) || (dy != 0)) && _xwin_mouse_interrupt)
	       (*_xwin_mouse_interrupt)(dx, dy, 0, mouse_buttons);
	 }
	 else if (_xwin_mouse_interrupt)
	    (*_xwin_mouse_interrupt)(0, 0, 0, mouse_buttons);
	 break;
      case LeaveNotify:
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
	 if (_xwin.in_dga_mode)
	    break;
#endif
	 _mouse_on = FALSE;
	 if (_xwin_mouse_interrupt)
	    (*_xwin_mouse_interrupt)(0, 0, 0, mouse_buttons);
	 break;
      case Expose:
	 /* Request to redraw part of the window.  */
	 (*_xwin_window_redrawer)(event->xexpose.x, event->xexpose.y,
				     event->xexpose.width, event->xexpose.height);
	 break;
      case MappingNotify:
	 /* Keyboard mapping changed.  */
	 if (event->xmapping.request == MappingKeyboard)
	    _xwin_private_init_keyboard_tables();
	 break;
      case ClientMessage:
         /* Window close request */
         if (event->xclient.data.l[0] == wm_delete_window) {
            if (_xwin.window_close_hook)
               _xwin.window_close_hook();
            else
               exit(-1);
         }
         break;
   }
}



/* _xwin_handle_input:
 *  Handle events from the queue.
 */
static void _xwin_private_handle_input(void)
{
   int i, events;
   static XEvent event[5];

   if (_xwin.display == 0)
      return;

   /* Switch mouse to non-warped mode if mickeys were not used recently (~2 seconds).  */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   if (!_xwin.in_dga_mode) {
#endif
      if (_xwin.mouse_warped && (_xwin.mouse_warped++ > MOUSE_WARP_DELAY)) {
	 _xwin.mouse_warped = 0;
	 /* Move X-cursor to Allegro cursor.  */
	 XWarpPointer(_xwin.display, _xwin.window, _xwin.window,
		      0, 0, _xwin.window_width, _xwin.window_height,
		      _mouse_x, _mouse_y);
      }
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   }
#endif

   /* Flush X-buffers.  */
   _xwin_private_flush_buffers();

   /* How much events are available in the queue.  */
   events = XEventsQueued(_xwin.display, QueuedAlready);
   if (events <= 0)
      return;

   /* Limit amount of events we read at once.  */
   if (events > 5)
      events = 5;

   /* Read pending events.  */
   for (i = 0; i < events; i++)
      XNextEvent(_xwin.display, &event[i]);

   /* Process all events.  */
   for (i = 0; i < events; i++)
      _xwin_private_process_event(&event[i]);
}

void _xwin_handle_input(void)
{
   if (_xwin.lock_count) return;

   XLOCK();

   if (_xwin_input_handler)
      _xwin_input_handler();
   else
      _xwin_private_handle_input();

   XUNLOCK();
}



/* _xwin_set_warped_mouse_mode:
 *  Switch mouse handling into warped mode.
 */
static void _xwin_private_set_warped_mouse_mode(int permanent)
{
   _xwin.mouse_warped = ((permanent) ? 1 : (MOUSE_WARP_DELAY*7/8));
}

void _xwin_set_warped_mouse_mode(int permanent)
{
   XLOCK();
   _xwin_private_set_warped_mouse_mode(permanent);
   XUNLOCK();
}



/* _xwin_redraw_window:
 *  Redraws part of the window.
 */
static void _xwin_private_redraw_window(int x, int y, int w, int h)
{
   if (_xwin.window == None)
      return;

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
   if (_xwin.in_dga_mode)
      return;
#endif

   /* Clip updated region.  */
   if (x >= _xwin.screen_width)
      return;
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (w >= (_xwin.screen_width - x))
      w = _xwin.screen_width - x;
   if (w <= 0)
      return;

   if (y >= _xwin.screen_height)
      return;
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (h >= (_xwin.screen_height - y))
      h = _xwin.screen_height - y;
   if (h <= 0)
      return;

   if (!_xwin.ximage)
      XFillRectangle(_xwin.display, _xwin.window, _xwin.gc, x, y, w, h);
   else {
#ifdef ALLEGRO_XWINDOWS_WITH_SHM
      if (_xwin.use_shm)
	 XShmPutImage(_xwin.display, _xwin.window, _xwin.gc, _xwin.ximage,
		      x + _xwin.scroll_x, y + _xwin.scroll_y, x, y, w, h, False);
      else
#endif
	 XPutImage(_xwin.display, _xwin.window, _xwin.gc, _xwin.ximage,
		   x + _xwin.scroll_x, y + _xwin.scroll_y, x, y, w, h);
   }
}

void _xwin_redraw_window(int x, int y, int w, int h)
{
   XLOCK();
   (*_xwin_window_redrawer)(x, y, w, h);
   XUNLOCK();
}



/* _xwin_scroll_screen:
 *  Scroll visible screen in window.
 */
static int _xwin_private_scroll_screen(int x, int y)
{
   _xwin.scroll_x = x;
   _xwin.scroll_y = y;
   (*_xwin_window_redrawer)(0, 0, _xwin.screen_width, _xwin.screen_height);
   _xwin_private_flush_buffers();

   return 0;
}

int _xwin_scroll_screen(int x, int y)
{
   int result;

   if (x < 0)
      x = 0;
   else if (x >= (_xwin.virtual_width - _xwin.screen_width))
      x = _xwin.virtual_width - _xwin.screen_width;
   if (y < 0)
      y = 0;
   else if (y >= (_xwin.virtual_height - _xwin.screen_height))
      y = _xwin.virtual_height - _xwin.screen_height;
   if ((_xwin.scroll_x == x) && (_xwin.scroll_y == y))
      return 0;

   XLOCK();
   result = _xwin_private_scroll_screen(x, y);
   XUNLOCK();
   return result;
}



/* _xwin_update_screen:
 *  Update part of the screen.
 */
static void _xwin_private_update_screen(int x, int y, int w, int h)
{
   /* Clip updated region.  */
   if (x >= _xwin.virtual_width)
      return;
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (w >= (_xwin.virtual_width - x))
      w = _xwin.virtual_width - x;
   if (w <= 0)
      return;

   if (y >= _xwin.virtual_height)
      return;
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (h >= (_xwin.virtual_height - y))
      h = _xwin.virtual_height - y;
   if (h <= 0)
      return;

   /* Update frame buffer with screen contents.  */
   if (_xwin.screen_to_buffer != 0)
      (*(_xwin.screen_to_buffer))(x, y, w, h);

   /* Update window.  */
   (*_xwin_window_redrawer)(x - _xwin.scroll_x, y - _xwin.scroll_y, w, h);
}

void _xwin_update_screen(int x, int y, int w, int h)
{
   XLOCK();
   _xwin_private_update_screen(x, y, w, h);
   XUNLOCK();
}



/* _xwin_set_window_title:
 *  Wrapper for XStoreName.
 */
static void _xwin_private_set_window_title(AL_CONST char *name)
{
   if (!name)
      _xwin_safe_copy(_xwin.window_title, XWIN_DEFAULT_WINDOW_TITLE, sizeof(_xwin.window_title));
   else
      _xwin_safe_copy(_xwin.window_title, name, sizeof(_xwin.window_title));

   if (_xwin.window != None)
      XStoreName(_xwin.display, _xwin.window, _xwin.window_title);
}

void _xwin_set_window_title(AL_CONST char *name)
{
   XLOCK();
   _xwin_private_set_window_title(name);
   XUNLOCK();
}



/* _xwin_change_keyboard_control:
 *  Wrapper for XChangeKeyboardControl.
 */
static void _xwin_private_change_keyboard_control(int led, int on)
{
   XKeyboardControl values;

   if (_xwin.display == 0)
      return;

   values.led = led;
   values.led_mode = (on ? LedModeOn : LedModeOff);

   XChangeKeyboardControl(_xwin.display, KBLed | KBLedMode, &values);
}

void _xwin_change_keyboard_control(int led, int on)
{
   XLOCK();
   _xwin_private_change_keyboard_control(led, on);
   XUNLOCK();
}



/* _xwin_get_pointer_mapping:
 *  Wrapper for XGetPointerMapping.
 */
static int _xwin_private_get_pointer_mapping(unsigned char map[], int nmap)
{
   return ((_xwin.display == 0) ? -1 : XGetPointerMapping(_xwin.display, map, nmap));
}

int _xwin_get_pointer_mapping(unsigned char map[], int nmap)
{
   int num;
   XLOCK();
   num = _xwin_private_get_pointer_mapping(map, nmap);
   XUNLOCK();
   return num;
}



/* Mappings between KeySym and Allegro scancodes.  */
static struct
{
   KeySym keysym;
   int scancode;
} _xwin_keysym_to_scancode[] =
{
   { XK_Escape, 0x01 },

   { XK_F1, 0x3B },
   { XK_F2, 0x3C },
   { XK_F3, 0x3D },
   { XK_F4, 0x3E },
   { XK_F5, 0x3F },
   { XK_F6, 0x40 },
   { XK_F7, 0x41 },
   { XK_F8, 0x42 },
   { XK_F9, 0x43 },
   { XK_F10, 0x44 },
   { XK_F11, 0x57 },
   { XK_F12, 0x58 },

   { XK_Print, 0x54 | 0x80 },
   { XK_Scroll_Lock, 0x46 },
   { XK_Pause, 0x00 | 0x100 },

   { XK_grave, 0x29 },
   { XK_quoteleft, 0x29 },
   { XK_asciitilde, 0x29 },
   { XK_1, 0x02 },
   { XK_2, 0x03 },
   { XK_3, 0x04 },
   { XK_4, 0x05 },
   { XK_5, 0x06 },
   { XK_6, 0x07 },
   { XK_7, 0x08 },
   { XK_8, 0x09 },
   { XK_9, 0x0A },
   { XK_0, 0x0B },
   { XK_minus, 0x0C },
   { XK_equal, 0x0D },
   { XK_backslash, 0x2B },
   { XK_BackSpace, 0x0E },

   { XK_Tab, 0x0F },
   { XK_q, 0x10 },
   { XK_w, 0x11 },
   { XK_e, 0x12 },
   { XK_r, 0x13 },
   { XK_t, 0x14 },
   { XK_y, 0x15 },
   { XK_u, 0x16 },
   { XK_i, 0x17 },
   { XK_o, 0x18 },
   { XK_p, 0x19 },
   { XK_bracketleft, 0x1A },
   { XK_bracketright, 0x1B },
   { XK_Return, 0x1C },

   { XK_Caps_Lock, 0x3A },
   { XK_a, 0x1E },
   { XK_s, 0x1F },
   { XK_d, 0x20 },
   { XK_f, 0x21 },
   { XK_g, 0x22 },
   { XK_h, 0x23 },
   { XK_j, 0x24 },
   { XK_k, 0x25 },
   { XK_l, 0x26 },
   { XK_semicolon, 0x27 },
   { XK_apostrophe, 0x28 },

   { XK_Shift_L, 0x2A },
   { XK_z, 0x2C },
   { XK_x, 0x2D },
   { XK_c, 0x2E },
   { XK_v, 0x2F },
   { XK_b, 0x30 },
   { XK_n, 0x31 },
   { XK_m, 0x32 },
   { XK_comma, 0x33 },
   { XK_period, 0x34 },
   { XK_slash, 0x35 },
   { XK_Shift_R, 0x36 },

   { XK_Control_L, 0x1D },
   { XK_Meta_L, 0x5B | 0x80 },
   { XK_Alt_L, 0x38 },
   { XK_space, 0x39 },
   { XK_Alt_R, 0x38 | 0x80 },
   { XK_Meta_R, 0x5C | 0x80 },
   { XK_Menu, 0x5D | 0x80 },
   { XK_Control_R, 0x1D | 0x80 },

   { XK_Insert, 0x52 | 0x80 },
   { XK_Home, 0x47 | 0x80 },
   { XK_Prior, 0x49 | 0x80 },
   { XK_Delete, 0x53 | 0x80 },
   { XK_End, 0x4F | 0x80 },
   { XK_Next, 0x51 | 0x80 },

   { XK_Up, 0x48 | 0x80 },
   { XK_Left, 0x4B | 0x80 },
   { XK_Down, 0x50 | 0x80 },
   { XK_Right, 0x4D | 0x80 },

   { XK_Num_Lock, 0x45 },
   { XK_KP_Divide, 0x35 | 0x80 },
   { XK_KP_Multiply, 0x37 },
   { XK_KP_Subtract, 0x4A | 0x80 },
   { XK_KP_Home, 0x47 },
   { XK_KP_Up, 0x48 },
   { XK_KP_Prior, 0x49 },
   { XK_KP_Add, 0x4E },
   { XK_KP_Left, 0x4B },
   { XK_KP_Begin, 0x4C },
   { XK_KP_Right, 0x4D },
   { XK_KP_End, 0x4F },
   { XK_KP_Down, 0x50 },
   { XK_KP_Next, 0x51 },
   { XK_KP_Enter, 0x1C | 0x80 },
   { XK_KP_Insert, 0x52 },
   { XK_KP_Delete, 0x53 },

   { NoSymbol, 0 },
};



/* _xwin_init_keyboard_tables:
 *  Initialize mapping between X-Windows keycodes and Allegro scancodes.
 */
static void _xwin_private_init_keyboard_tables(void)
{
   int i, j;
   int min_keycode;
   int max_keycode;
   KeySym keysym;
   char *section, *option_format;
   char option[128], tmp1[128], tmp2[128];

   if (_xwin.display == 0)
      return;

   for (i = 0; i < 256; i++) {
      /* Clear mappings.  */
      _xwin.keycode_to_scancode[i] = -1;
      /* Clear pressed key flags.  */
      _xwin_keycode_pressed[i] = FALSE;
   }

   /* Get the number of keycodes.  */
   XDisplayKeycodes(_xwin.display, &min_keycode, &max_keycode);
   if (min_keycode < 0)
      min_keycode = 0;
   if (max_keycode > 255)
      max_keycode = 255;

   /* Setup initial X keycode to Allegro scancode mappings.  */
   for (i = min_keycode; i <= max_keycode; i++) {
      keysym = XKeycodeToKeysym(_xwin.display, i, 0);
      if (keysym != NoSymbol) {
	 for (j = 0; _xwin_keysym_to_scancode[j].keysym != NoSymbol; j++) {
	    if (_xwin_keysym_to_scancode[j].keysym == keysym) {
	       _xwin.keycode_to_scancode[i] = _xwin_keysym_to_scancode[j].scancode;
	       break;
	    }
	 }
      }
   }

   /* Override with user's own mappings.  */
   section = uconvert_ascii("xkeymap", tmp1);
   option_format = uconvert_ascii("keycode%d", tmp2);

   for (i = min_keycode; i <= max_keycode; i++) {
      int scancode;

      uszprintf(option, sizeof(option), option_format, i);
      scancode = get_config_int(section, option, -1);
      if (scancode > 0)
	 _xwin.keycode_to_scancode[i] = scancode;
   }
}

void _xwin_init_keyboard_tables(void)
{
   XLOCK();
   _xwin_private_init_keyboard_tables();
   XUNLOCK();
}



/* _xwin_write_line:
 *  Update last selected line and select new line.
 */
unsigned long _xwin_write_line(BITMAP *bmp, int line)
{
   int new_line = line + bmp->y_ofs;
   if ((new_line != _xwin_last_line) && (!_xwin_in_gfx_call) && (_xwin_last_line >= 0))
      _xwin_update_screen(0, _xwin_last_line, _xwin.virtual_width, 1);
   _xwin_last_line = new_line;
   return (unsigned long) (bmp->line[line]);
}



/* _xwin_unwrite_line:
 *  Update last selected line.
 */
void _xwin_unwrite_line(BITMAP *bmp)
{
   if ((!_xwin_in_gfx_call) && (_xwin_last_line >= 0))
      _xwin_update_screen(0, _xwin_last_line, _xwin.virtual_width, 1);
   _xwin_last_line = -1;
}



/*
 * Support for XF86DGA extension (direct access to frame buffer).
 */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA
/* _xdga_create_screen:
 *  Create screen bitmap.
 */
static BITMAP *_xdga_private_create_screen(GFX_DRIVER *drv, int w, int h,
					   int vw, int vh, int depth, int fullscreen)
{
   int dga_flags, dotclock;
   int dga_event_base, dga_error_base;
   int dga_major_version, dga_minor_version;
   int fb_width, banksize, memsize;
   int s_w, s_h, v_w, v_h;
   int offset_x, offset_y;
   char *fb_addr;
   struct passwd *pass;
   char tmp1[128], tmp2[128];
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   int vid_event_base, vid_error_base;
   int vid_major_version, vid_minor_version;
   XF86VidModeModeLine modeline;
#endif
   
   if (_xwin.window == None) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("No window"));
      return 0;
   }

   /* Test that user has enough privileges.  */
   pass = getpwuid(geteuid());
   if (pass == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not obtain user name"));
      return 0;
   }
   if (strcmp("root", pass->pw_name) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("This driver needs root privileges"));
      return 0;
   }

   /* Test that display is local.  */
   if (!_xwin_private_display_is_local()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("This driver needs local display"));
      return 0;
   }

   /* Choose convenient size.  */
   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }

   if ((w < 80) || (h < 80) || (w > 4096) || (h > 4096)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported screen size"));
      return 0;
   }

   if (vw < w)
      vw = w;
   if (vh < h)
      vh = h;

   if (1
#ifdef ALLEGRO_COLOR8
       && (depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (depth != 15)
       && (depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return 0;
   }

   /* Test for presence of DGA extension.  */
   if (!XF86DGAQueryExtension(_xwin.display, &dga_event_base, &dga_error_base)
       || !XF86DGAQueryVersion(_xwin.display, &dga_major_version, &dga_minor_version)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("DGA extension is not supported"));
      return 0;
   }

   /* Test DGA version */
   if (dga_major_version != 1) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("DGA 1.0 is required"));
      return 0;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   /* Test for presence of VidMode extension.  */
   if (!XF86VidModeQueryExtension(_xwin.display, &vid_event_base, &vid_error_base)
       || !XF86VidModeQueryVersion(_xwin.display, &vid_major_version, &vid_minor_version)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VidMode extension is not supported"));
      return 0;
   }
#endif

   /* Query DirectVideo capabilities.  */
   if (!XF86DGAQueryDirectVideo(_xwin.display, _xwin.screen, &dga_flags)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not query DirectVideo"));
      return 0;
   }
   if (!(dga_flags & XF86DGADirectPresent)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("DirectVideo is not supported"));
      return 0;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   /* Switch video mode if appropriate.  */
   if ((fullscreen) && (!_xvidmode_private_set_fullscreen(w, h, vw, vh))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not set video mode"));
      return 0;
   }
#endif

   /* If you want to use debugger, comment grabbing (but there will be no input in Allegro).  */
#if 1
   /* Grab keyboard and mouse.  */
   if (XGrabKeyboard(_xwin.display, XDefaultRootWindow(_xwin.display), False,
		     GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not grab keyboard"));
      return 0;
   }
   _xwin.keyboard_grabbed = 1;
   if (XGrabPointer(_xwin.display, XDefaultRootWindow(_xwin.display), False,
		    PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		    GrabModeAsync, GrabModeAsync, None, None, CurrentTime) != GrabSuccess) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not grab mouse"));
      return 0;
   }
   _xwin.mouse_grabbed = 1;
#endif

   /* Test that frame buffer is fast (can be accessed directly) - what a mess.  */
   _xwin.fast_visual_depth = _xdga_private_fast_visual_depth ();
   if (_xwin.fast_visual_depth == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return 0;
   }

   /* Get current video mode settings.  */
   if (!XF86DGAGetVideo(_xwin.display, _xwin.screen, &fb_addr, &v_w, &banksize, &memsize)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not get video mode settings"));
      return 0;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   /* Get current screen size.  */
   if (!XF86VidModeGetModeLine(_xwin.display, _xwin.screen, &dotclock, &modeline)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not get video mode settings"));
      return 0;
   }

   if ((modeline.privsize > 0) && (modeline.private != 0))
      XFree(modeline.private);
#endif
   
   /* I'm not sure that it is correct, but what else?  */
   memsize *= 1024;
   fb_width = v_w * (_xwin.fast_visual_depth / 8);

   /* Test that each line is included in one bank only.  */
   if ((banksize < memsize) && ((banksize % fb_width) != 0)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported memory layout"));
      return 0;
   }

   v_h = memsize / fb_width;
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   s_w = modeline.hdisplay;
   s_h = modeline.vdisplay;
#else
   /* This is not completely correct, but it allows us to decouple DGA from
    * VidMode extensions (even if both are always available together).  */
   s_w = w;
   s_h = h;
#endif

   if ((s_w < w) || (s_h < h) || ((vw - w) > (v_w - s_w)) || ((vh - h) > (v_h - s_h))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported screen size"));
      return 0;
   }

   /* Centre Allegro screen inside display */
   offset_x = 0;
   offset_y = 0;
   if (banksize >= memsize) {
      if (get_config_int(uconvert_ascii("system", tmp1), uconvert_ascii("dga_centre", tmp2), 1)) {
         offset_x = (s_w - w) / 2;
         offset_y = (s_h - h) / 2;
      }
   }

   vw = v_w - s_w + w;
   vh = v_h - s_h + h;

   _xwin.screen_width = w;
   _xwin.screen_height = h;
   _xwin.screen_depth = depth;
   _xwin.virtual_width = vw;
   _xwin.virtual_height = vh;

   if (banksize < memsize) {
      /* Banked frame buffer.  */
      drv->linear = FALSE;
      drv->bank_size = banksize;
      drv->bank_gran = banksize;
   }
   else {
      /* Linear frame buffer.  */
      drv->linear = TRUE;
      drv->bank_size = memsize;
      drv->bank_gran = memsize;
   }

   /* Prepare visual for further use.  */
   _xwin_private_prepare_visual();

   /* Set DGA mode.  */
   if (!XF86DGADirectVideo(_xwin.display, _xwin.screen,
			   XF86DGADirectGraphics | XF86DGADirectMouse | XF86DGADirectKeyb)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not switch to DGA mode"));
      return 0;
   }
   _xwin.in_dga_mode = 1;
   
   /* Allow workaround for buggy servers (e.g. 3dfx Voodoo 3/Banshee).  */
   if (get_config_int(uconvert_ascii("system", tmp1), uconvert_ascii("dga_mouse", tmp2), 1) == 0)
      _xwin.disable_dga_mouse = 1;

   set_display_switch_mode(SWITCH_NONE);

   /* Uninstall colormap from X-server (must).  */
   XUninstallColormap(_xwin.display, _xwin.colormap);

   /* Install DGA colormap.  */
   if (!XF86DGAInstallColormap(_xwin.display, _xwin.screen, _xwin.colormap)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not install colormap"));
      return 0;
   }

   /* Create the colormaps for swapping in 8-bit mode */
   if (_xwin.fast_visual_depth == 8) {
      const int cmap_size = 256;
      XColor color[cmap_size];
      int i;

      _xdga_colormap[0] = _xwin.colormap;  
      _xdga_colormap[1] = XCreateColormap(_xwin.display, _xwin.window, _xwin.visual, AllocAll);

      for (i = 0; i < cmap_size; i++)
         color[i].pixel = i;

      XQueryColors(_xwin.display, _xdga_colormap[0], color, cmap_size);
      XStoreColors(_xwin.display, _xdga_colormap[1], color, cmap_size);

      _xdga_installed_colormap = 0;
   }

   /* Clear video memory */
   if (get_config_int(uconvert_ascii("system", tmp1), uconvert_ascii("dga_clear", tmp2), 1)) {
      int line, offset;
      int width = s_w * (_xwin.fast_visual_depth / 8);

      /* Clear all visible lines.  */
      for (offset = 0, line = s_h - 1; line >= 0; offset += fb_width, line--) {
	 if ((offset % banksize) == 0)
	    XF86DGASetVidPage(_xwin.display, _xwin.screen, offset / banksize);
	 memset(fb_addr + offset % banksize, 0, width);
      }
   }

   /* Switch to first bank.  */
   XF86DGASetVidPage(_xwin.display, _xwin.screen, 0);

   /* Set view port to (0, 0).  */
   XF86DGASetViewPort(_xwin.display, _xwin.screen, 0, 0);

   /* Create screen bitmap from frame buffer.  */
   return _xwin_private_create_screen_bitmap(drv, 1, fb_addr + offset_y * fb_width + offset_x * (_xwin.fast_visual_depth / 8), fb_width);
}

BITMAP *_xdga_create_screen(GFX_DRIVER *drv, int w, int h, int vw, int vh, int depth, int fullscreen)
{
   BITMAP *bmp;
   XLOCK();
   bmp = _xdga_private_create_screen(drv, w, h, vw, vh, depth, fullscreen);
   if (bmp == 0)
      _xdga_private_destroy_screen();
   else
      _mouse_on = TRUE;
   XUNLOCK();
   return bmp;
}



/* _xdga_destroy_screen:
 *  Destroys screen resources.
 */
static void _xdga_private_destroy_screen(void)
{
   if (_xwin.buffer_line != 0) {
      free(_xwin.buffer_line);
      _xwin.buffer_line = 0;
   }

   if (_xwin.screen_line != 0) {
      free(_xwin.screen_line);
      _xwin.screen_line = 0;
   }

   if (_xwin.screen_data != 0) {
      free(_xwin.screen_data);
      _xwin.screen_data = 0;
   }

   if (_xwin.in_dga_mode) {
      XF86DGADirectVideo(_xwin.display, _xwin.screen, 0);

      if (_xwin.fast_visual_depth == 8) {
         _xwin.colormap = _xdga_colormap[0];
         XFreeColormap(_xwin.display, _xdga_colormap[1]);
      }

      XInstallColormap(_xwin.display, _xwin.colormap);

      _xwin.in_dga_mode = 0;

      set_display_switch_mode(SWITCH_BACKGROUND);
   }

   if (_xwin.mouse_grabbed) {
      XUngrabPointer(_xwin.display, CurrentTime);
      _xwin.mouse_grabbed = 0;
   }

   if (_xwin.keyboard_grabbed) {
      XUngrabKeyboard(_xwin.display, CurrentTime);
      _xwin.keyboard_grabbed = 0;
   }

#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   _xvidmode_private_unset_fullscreen();
#endif
}

void _xdga_destroy_screen(void)
{
   XLOCK();
   _xdga_private_destroy_screen();
   XUNLOCK();
}



/* _xdga_switch_screen_bank:
 *  Switch screen bank.
 */
static void _xdga_switch_screen_bank(int line)
{
   if (line != _xdga_last_line) {
      XF86DGASetVidPage(_xwin.display, _xwin.screen, _gfx_bank[line]);
      _xdga_last_line = line;
   }
}



/* _xdga_switch_bank:
 *  Switch bank and return linear offset for line.
 */
unsigned long _xdga_switch_bank(BITMAP *bmp, int line)
{
   _xdga_switch_screen_bank(line + bmp->y_ofs);
   return (unsigned long) (bmp->line[line]);
}



/* _xdga_set_palette_range:
 *  Set hardware colors in DGA mode.
 */
static void _xdga_private_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   /* Wait for VBI.  */
   if (vsync)
      _xwin_private_vsync();

   if (_xwin.set_colors != 0) {
      /* Set "hardware" colors.  */
      (*(_xwin.set_colors))(p, from, to);

      if (_xwin.matching_formats) {
	 /* Have to install colormap again, but with another ID. */
         _xdga_installed_colormap = 1 - _xdga_installed_colormap;
         _xwin.colormap = _xdga_colormap[_xdga_installed_colormap];
         (*(_xwin.set_colors))(p, from, to);
	 XF86DGAInstallColormap(_xwin.display, _xwin.screen, _xwin.colormap);
      }
      else /* Update frame buffer.  */
	 _xwin_private_update_screen(0, 0, _xwin.virtual_width, _xwin.virtual_height);
   }
}

void _xdga_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   XLOCK();
   _xdga_private_set_palette_range(p, from, to, vsync);
   XUNLOCK();
}



/* _xdga_scroll_screen:
 *  Scroll visible screen in window.
 */
static int _xdga_private_scroll_screen(int x, int y)
{
   _xwin.scroll_x = x;
   _xwin.scroll_y = y;
   XF86DGASetViewPort (_xwin.display, _xwin.screen, x, y);
   _xwin_private_flush_buffers();

   return 0;
}

int _xdga_scroll_screen(int x, int y)
{
   int result;

   if (x < 0)
      x = 0;
   else if (x >= (_xwin.virtual_width - _xwin.screen_width))
      x = _xwin.virtual_width - _xwin.screen_width;
   if (y < 0)
      y = 0;
   else if (y >= (_xwin.virtual_height - _xwin.screen_height))
      y = _xwin.virtual_height - _xwin.screen_height;
   if ((_xwin.scroll_x == x) && (_xwin.scroll_y == y))
      return 0;

   XLOCK();
   result = _xdga_private_scroll_screen(x, y);
   XUNLOCK();
   return result;
}
#endif



/*
 * Support for XF86VidMode extension.
 */
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
/* _xvidmode_private_set_fullscreen:
 *  Attempt to switch video mode and make window fullscreen.
 */
static int _xvidmode_private_set_fullscreen(int w, int h, int vw, int vh)
{
   int vid_event_base, vid_error_base;
   int vid_major_version, vid_minor_version;
   XF86VidModeModeInfo *mode;
   int i;

   /* Test that display is local.  */
   if (!_xwin_private_display_is_local()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VidMode extension requires local display"));
      return 0;
   }
   
   /* Test for presence of VidMode extension.  */
   if (!XF86VidModeQueryExtension(_xwin.display, &vid_event_base, &vid_error_base)
       || !XF86VidModeQueryVersion(_xwin.display, &vid_major_version, &vid_minor_version)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VidMode extension is not supported"));
      return 0;
   }

   /* Get list of modelines.  */
   if (!XF86VidModeGetAllModeLines(_xwin.display, _xwin.screen, 
				   &_xwin.num_modes, &_xwin.modesinfo))
      return 0;

   /* Search for a matching video mode.  */
   for (i = 0; i < _xwin.num_modes; i++) {
      mode = _xwin.modesinfo[i];
      if ((mode->hdisplay == w) && (mode->vdisplay == h)
	  && (mode->htotal > vw) && (mode->vtotal > vh)) {
	 /* Switch video mode.  */
	 if (!XF86VidModeSwitchToMode(_xwin.display, _xwin.screen, mode))
	    return 0;

	 /* Lock mode switching.  */
	 XF86VidModeLockModeSwitch(_xwin.display, _xwin.screen, True);

	 _xwin.mode_switched = 1;
	 return 1;
      }
   }

   return 0;
}



/* free_modelines:
 *  Free mode lines.
 */
static void free_modelines(XF86VidModeModeInfo **modesinfo, int num_modes)
{
   int i;

   for (i = 0; i < num_modes; i++)
      if (modesinfo[i]->privsize > 0)
	 XFree(modesinfo[i]->private);
   XFree(modesinfo);
}



/* _xvidmode_private_unset_fullscreen:
 *  Restore original video mode and window attributes.
 */
static void _xvidmode_private_unset_fullscreen(void)
{
   if (_xwin.num_modes > 0) {
      if (_xwin.mode_switched) {
	 /* Unlock mode switching.  */
	 XF86VidModeLockModeSwitch(_xwin.display, _xwin.screen, False);

	 /* Restore the original video mode.  */
	 XF86VidModeSwitchToMode(_xwin.display, _xwin.screen, _xwin.modesinfo[0]);

	 _xwin.mode_switched = 0;
      }

      /* Free modelines.  */
      free_modelines(_xwin.modesinfo, _xwin.num_modes);
      _xwin.num_modes = 0;
      _xwin.modesinfo = 0;
   }
}



/* _xvidmode_private_fetch_mode_list:
 *  Generates a list of valid video modes.
 */
static GFX_MODE_LIST *_xvidmode_private_fetch_mode_list(void)
{
   int vid_event_base, vid_error_base;
   int vid_major_version, vid_minor_version;
   XF86VidModeModeInfo **modesinfo;
   int num_modes, num_bpp;
   GFX_MODE_LIST *mode_list;
   int i, j;

   /* Test that display is local.  */
   if (!_xwin_private_display_is_local())
      return 0;

   /* Test for presence of VidMode extension.  */
   if (!XF86VidModeQueryExtension(_xwin.display, &vid_event_base, &vid_error_base)
       || !XF86VidModeQueryVersion(_xwin.display, &vid_major_version, &vid_minor_version))
      return 0;

   /* Get list of modelines.  */
   if (!XF86VidModeGetAllModeLines(_xwin.display, _xwin.screen, &num_modes, &modesinfo))
      return 0;

   /* Calculate the number of color depths we have to support.  */
   num_bpp = 0;
#ifdef ALLEGRO_COLOR8
   num_bpp++;
#endif
#ifdef ALLEGRO_COLOR16
   num_bpp += 2;     /* 15, 16 */
#endif
#ifdef ALLEGRO_COLOR24
   num_bpp++;
#endif
#ifdef ALLEGRO_COLOR32
   num_bpp++;
#endif
   if (num_bpp == 0) /* ha! */
      return 0;

   /* Allocate space for mode list.  */
   mode_list = malloc(sizeof(GFX_MODE_LIST));
   if (!mode_list) {
      free_modelines(modesinfo, num_modes);
      return 0;
   }

   mode_list->mode = malloc(sizeof(GFX_MODE) * ((num_modes * num_bpp) + 1));
   if (!mode_list->mode) {
      free(mode_list);
      free_modelines(modesinfo, num_modes);
      return 0;
   }

   /* Fill in mode list.  */
   j = 0;
   for (i = 0; i < num_modes; i++) {
#define ADD_MODE(BPP)					    \
      mode_list->mode[j].width = modesinfo[i]->hdisplay;    \
      mode_list->mode[j].height = modesinfo[i]->vdisplay;   \
      mode_list->mode[j].bpp = BPP;			    \
      j++
#ifdef ALLEGRO_COLOR8
      ADD_MODE(8);
#endif
#ifdef ALLEGRO_COLOR16
      ADD_MODE(15);
      ADD_MODE(16);
#endif
#ifdef ALLEGRO_COLOR24
      ADD_MODE(24);
#endif
#ifdef ALLEGRO_COLOR32
      ADD_MODE(32);
#endif
   }

   mode_list->mode[j].width = 0;
   mode_list->mode[j].height = 0;
   mode_list->mode[j].bpp = 0;
   mode_list->num_modes = j;

   free_modelines(modesinfo, num_modes);

   return mode_list;
}
#endif



/* _xwin_fetch_mode_list:
 *  Fetch mode list.
 */
GFX_MODE_LIST *_xwin_fetch_mode_list(void)
{
#ifdef ALLEGRO_XWINDOWS_WITH_XF86VIDMODE
   GFX_MODE_LIST *list;
   XLOCK();
   list = _xvidmode_private_fetch_mode_list();
   XUNLOCK();
   return list;
#else
   return 0;
#endif
}



/* Hook functions */
int (*_xwin_window_creator)(void) = &_xwin_private_create_window;
void (*_xwin_window_defaultor)(void) = &_xwin_private_set_window_defaults;
void (*_xwin_window_redrawer)(int, int, int, int) = &_xwin_private_redraw_window;
void (*_xwin_input_handler)(void) = 0;

void (*_xwin_keyboard_callback)(int, int) = 0;
