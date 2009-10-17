/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/*----------------------------------------------------------------
 * x.c -- Allegro-GLX interfacing
 *----------------------------------------------------------------
 *  This is the interface module for use under X.
 */
#include <string.h>
#include <stdio.h>

#include <allegro.h>
#include <xalleg.h>

#include <allegro/platform/aintunix.h>

#include "alleggl.h"
#include "allglint.h"
#include "glvtable.h"


#ifndef XLOCK
	#define OLD_ALLEGRO
	#define XLOCK() DISABLE()
	#undef XUNLOCK
	#define XUNLOCK() ENABLE()
#endif

#define PREFIX_I                "agl-x INFO: "
#define PREFIX_E                "agl-x ERROR: "


#ifdef ALLEGRO_XWINDOWS_WITH_XPM
#include <X11/xpm.h>
extern void *allegro_icon;
#endif


static BITMAP *allegro_gl_x_windowed_init(int w, int h, int vw, int vh,
                                          int color_depth);
static void allegro_gl_x_exit(BITMAP *bmp);
#ifdef ALLEGROGL_HAVE_XF86VIDMODE
static GFX_MODE_LIST* allegro_gl_x_fetch_mode_list(void);
#endif
static void allegro_gl_x_vsync(void);
static void allegro_gl_x_hide_mouse(void);

static BITMAP *allegro_gl_screen = NULL;

/* TODO: Revamp the whole window handling under X11 in Allegro and
 * AllegroGL. We really shouldn't have to duplicate code or hack on
 * Allegro internals in AllegroGL - the *only* difference for the AllegroGL
 * window should be the GLX visual (e.g. cursor, icon, VidModeExtension and
 * so on should never have been touched in this file).
 */
#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
static Window backup_allegro_window = None;
static Colormap backup_allegro_colormap = None;
#endif


static BITMAP *allegro_gl_x_fullscreen_init(int w, int h, int vw, int vh,
                                            int color_depth);

GFX_DRIVER gfx_allegro_gl_fullscreen =
{
   GFX_OPENGL_FULLSCREEN,
   empty_string,
   empty_string,
   "AllegroGL Fullscreen (X)",
   allegro_gl_x_fullscreen_init,
   allegro_gl_x_exit,
   NULL,
   allegro_gl_x_vsync,
   NULL,
   NULL, NULL, NULL,
   allegro_gl_create_video_bitmap,
   allegro_gl_destroy_video_bitmap,
   NULL, NULL,					/* No show/request video bitmaps */
   NULL, NULL,
   allegro_gl_set_mouse_sprite,
   allegro_gl_show_mouse,
   allegro_gl_x_hide_mouse,
   allegro_gl_move_mouse,
   allegro_gl_drawing_mode,
   NULL, NULL,
   allegro_gl_set_blender_mode,
#ifdef ALLEGROGL_HAVE_XF86VIDMODE
   allegro_gl_x_fetch_mode_list,
#else
   NULL,
#endif
   0, 0,
   0,
   0, 0,
   0,
   0,
   FALSE                        /* Windowed mode */
};



GFX_DRIVER gfx_allegro_gl_windowed =
{
   GFX_OPENGL_WINDOWED,
   empty_string,
   empty_string,
   "AllegroGL Windowed (X)",
   allegro_gl_x_windowed_init,
   allegro_gl_x_exit,
   NULL,
   allegro_gl_x_vsync,
   NULL,
   NULL, NULL, NULL,
   allegro_gl_create_video_bitmap,
   allegro_gl_destroy_video_bitmap,
   NULL, NULL,					/* No show/request video bitmaps */
   NULL, NULL,
   allegro_gl_set_mouse_sprite,
   allegro_gl_show_mouse,
   allegro_gl_x_hide_mouse,
   allegro_gl_move_mouse,
   allegro_gl_drawing_mode,
   NULL, NULL,
   allegro_gl_set_blender_mode,
   NULL,                        /* No fetch_mode_list */
   0, 0,
   0,
   0, 0,
   0,
   0,
   TRUE                         /* Windowed mode */
};



static struct allegro_gl_driver allegro_gl_x;

static XVisualInfo *allegro_gl_x_windowed_choose_visual (void);
static int allegro_gl_x_create_window (int fullscreen);
static BITMAP *allegro_gl_x_windowed_create_screen (GFX_DRIVER *drv, int w, int h, int depth);

static int decode_visual (XVisualInfo *v, struct allegro_gl_display_info *i);
struct {
	int fullscreen;
	GLXContext ctx;
	int major, minor;	/* Major and minor GLX version */
	int error_base, event_base;
	int use_glx_window;
	GLXWindow window;
} _glxwin;

static void (*old_window_redrawer)(int, int, int, int);
extern void (*_xwin_window_redrawer)(int, int, int, int);
static int (*old_x_error_handler)(Display*, XErrorEvent*);



/* allegro_gl_redraw_window :
 *  Redraws the window when an Expose event is processed
 *  Important note : no GL commands should be processed in this function
 *  since it may be called by a thread which is different from the main thread.
 *  In order to be able to process GL commands, we should create another context
 *  and make it current to the other thread. IMHO it would be overkill.
 */
static void allegro_gl_redraw_window(int x, int y, int w, int h)
{
	/* Does nothing */
	return;
}



#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
/* _xwin_hide_x_mouse:
 * Create invisible X cursor
 */
static void _xwin_hide_x_mouse(void)
{
	unsigned long gcmask;
	XGCValues gcvalues;
	Pixmap pixmap;

	XUndefineCursor(_xwin.display, _xwin.window);

	if (_xwin.cursor != None) {
		XFreeCursor(_xwin.display, _xwin.cursor);
		_xwin.cursor = None;
	}

	if (_xwin.xcursor_image != None) {
		XcursorImageDestroy(_xwin.xcursor_image);
		_xwin.xcursor_image = None;
	}

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
}
#endif



/* hide_mouse:
 *  Hide the custom X cursor (if supported)
 */
static void hide_mouse(void)
{
   #ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
   if (_xwin.support_argb_cursor) {
      XLOCK();
      _xwin_hide_x_mouse();
      XUNLOCK();
   }
   #endif
   return;
}



/* If Allegro's X11 mouse driver enabled hw cursors, we shouldn't use
 * allegro_gl_hide_mouse();
 */
static void allegro_gl_x_hide_mouse(void)
{
	if (_xwin.hw_cursor_ok) {
		hide_mouse();
	}
	else {
		allegro_gl_hide_mouse();
	}
}



/* allegro_gl_x_windowed_init:
 *  Creates screen bitmap.
 */
static BITMAP *allegro_gl_x_create_screen(int w, int h, int vw, int vh,
                                          int depth, int fullscreen)
{
	int _keyboard_was_installed = FALSE;
	int _mouse_was_installed = FALSE;
	int create_window_ret;

	/* test if Allegro have pthread support enabled */
	if (!_unix_bg_man->multi_threaded) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		       get_config_text("Fatal Error : pthread support is not enabled"));
		return NULL;
	}
	
	if (keyboard_driver) {
		_keyboard_was_installed = TRUE;
		remove_keyboard();
		TRACE(PREFIX_I "x_create_screen: Removing Keyboard...\n");
	}
	
	if (mouse_driver) {
		_mouse_was_installed = TRUE;
		remove_mouse();
		TRACE(PREFIX_I "x_create_screen: Removing Mouse...\n");
	}
	
	XLOCK();

	if (!glXQueryExtension(_xwin.display, &_glxwin.error_base,
	                      &_glxwin.event_base)) {

		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
		             get_config_text("GLX Extension not supported by display"));
		XUNLOCK();
		goto failure;
	}

	sscanf(glXQueryServerString(_xwin.display, _xwin.screen, GLX_VERSION),
			"%i.%i", &_glxwin.major, &_glxwin.minor);

	if ((w == 0) && (h == 0)) {
		w = 640;
		h = 480;
	}

	if ((vw > w) || (vh > h)) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
		     get_config_text ("OpenGL drivers do not support virtual screens"));
		XUNLOCK();
		goto failure;
	}

	allegro_gl_display_info.w = w;
	allegro_gl_display_info.h = h;

	old_window_redrawer = _xwin_window_redrawer;
	_xwin_window_redrawer = allegro_gl_redraw_window;
	_glxwin.fullscreen = FALSE;
	_glxwin.use_glx_window = FALSE;

	create_window_ret = allegro_gl_x_create_window(fullscreen);
	if (create_window_ret) {
		if (fullscreen && create_window_ret == -2) {
			ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
			          get_config_text ("Unable to switch in GLX fullscreen"));
		}
		else if (create_window_ret == -2) {
			ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
			          get_config_text ("Unable to create GLX window"));
		}
		XUNLOCK();
		allegro_gl_x_exit(NULL);
		goto failure;
	}

	/* If pixel format is Allegro compatible, set up Allegro correctly. */
	set_color_depth(allegro_gl_display_info.colour_depth);

	/* XXX <rohannessian> X can run on Big-Endian systems. We need to 
	 * make a check for that and pass TRUE to
	 * __allegro_gl_set_allegro_image_format() in that case.
	 */
	__allegro_gl_set_allegro_image_format(FALSE);

	if (fullscreen) {
		allegro_gl_screen =
		       allegro_gl_x_windowed_create_screen (&gfx_allegro_gl_fullscreen,
		               allegro_gl_display_info.w, allegro_gl_display_info.h,
		               _color_depth);
	}
	else {
		allegro_gl_screen =
		        allegro_gl_x_windowed_create_screen (&gfx_allegro_gl_windowed,
		                allegro_gl_display_info.w, allegro_gl_display_info.h,
		                _color_depth);
	}

	if (!allegro_gl_screen) {
		ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
		          get_config_text ("Error creating screen bitmap"));
		XUNLOCK();
		allegro_gl_x_exit(NULL);
		goto failure;
	}
	
	__allegro_gl_valid_context = TRUE;
	__allegro_gl_driver = &allegro_gl_x;

	/* Print out OpenGL version info */
	TRACE(PREFIX_I "OpenGL Version: %s\n", (AL_CONST char*)glGetString(GL_VERSION));
	TRACE(PREFIX_I "OpenGL Vendor: %s\n", (AL_CONST char*)glGetString(GL_VENDOR));
	TRACE(PREFIX_I "OpenGL Renderer: %s\n", (AL_CONST char*)glGetString(GL_RENDERER));
	
	/* Detect if the GL driver is based on Mesa */
	allegro_gl_info.is_mesa_driver = FALSE;
	if (strstr((AL_CONST char*)glGetString(GL_VERSION),"Mesa")) {
		AGL_LOG(1, "OpenGL driver based on Mesa\n");
		allegro_gl_info.is_mesa_driver = TRUE;
	}

	/* Print out GLX version info */
	TRACE(PREFIX_I "GLX Version: %d.%d\n", _glxwin.major, _glxwin.minor);

#ifdef LOGLEVEL
	if (glXIsDirect(_xwin.display, _glxwin.ctx)) {
		AGL_LOG(1, "GLX Direct Rendering is enabled\n");
	}
	else {
		AGL_LOG(1, "GLX Direct Rendering is disabled\n");
	}
#endif

	/* Prints out GLX extensions info */
	AGL_LOG(1, "glX Extensions:\n");
#ifdef LOGLEVEL
	__allegro_gl_print_extensions(
		(AL_CONST char*)glXQueryExtensionsString(_xwin.display, _xwin.screen));
#endif
	/* Prints out OpenGL extensions info and activates needed extensions */
	__allegro_gl_manage_extensions();
	
	/* Update screen vtable in order to use AGL's */
	__allegro_gl__glvtable_update_vtable (&allegro_gl_screen->vtable);
	memcpy(&_screen_vtable, allegro_gl_screen->vtable, sizeof(GFX_VTABLE));
	allegro_gl_screen->vtable = &_screen_vtable;

	XUNLOCK();

	if (_keyboard_was_installed) {
		TRACE(PREFIX_I "x_create_screen: Installing Keyboard...\n");
		install_keyboard();
	}

	if (_mouse_was_installed) {
		TRACE(PREFIX_I "x_create_screen: Installing Mouse...\n");
		install_mouse();
	}
	gfx_capabilities |= GFX_HW_CURSOR;

	return allegro_gl_screen;

failure:
	if (_keyboard_was_installed) {
		install_keyboard();
	}

	if (_mouse_was_installed) {
		install_mouse();
	}

	return NULL;
}



/* allegro_gl_x_windowed_init:
 *  Creates screen bitmap for windowed driver.
 */
static BITMAP *allegro_gl_x_windowed_init(int w, int h, int vw, int vh,
                                          int depth)
{
	return allegro_gl_x_create_screen(w, h, vw, vh, depth, FALSE);
}



/* allegro_gl_x_fullscreen_init:
 *  Creates screen bitmap for fullscreen driver.
 */
static BITMAP *allegro_gl_x_fullscreen_init(int w, int h, int vw, int vh,
                                            int depth)
{
	return allegro_gl_x_create_screen(w, h, vw, vh, depth, TRUE);
}



#ifdef ALLEGROGL_HAVE_XF86VIDMODE
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
#endif



/* allegro_gl_x_exit:
 *  Shuts down the driver (shared between windowed and full-screen)
 */
static void allegro_gl_x_exit(BITMAP *bmp)
{
#ifdef ALLEGROGL_HAVE_XF86VIDMODE
	XSetWindowAttributes setattr;
#endif

	XLOCK();
	/* We politely wait for OpenGL to finish its current operations before
	   shutting down the driver */
	glXWaitGL();

	__allegro_gl_unmanage_extensions();	

	if (_glxwin.ctx) {
		if (!allegro_gl_info.is_ati_r200_chip) {
			/* The DRI drivers for ATI cards with R200 chip
			 * seem to be broken since they crash at this point.
			 * As a workaround AGL does not release the GLX context
			 * here. This should not hurt since the GLX specs don't
			 * require the context to be released before the program
			 * ends or before another context is made current to the
			 * thread.
			 */
			if (!glXMakeCurrent(_xwin.display, None, NULL)) {
				ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE,
				          get_config_text ("Could not release drawing context.\n"));
			}
		}

		glXDestroyContext(_xwin.display, _glxwin.ctx);
		_glxwin.ctx = NULL;
	}

	if (_xwin.mouse_grabbed) {
		XUngrabPointer(_xwin.display, CurrentTime);
		_xwin.mouse_grabbed = 0;
	}

	if (_xwin.keyboard_grabbed) {
		XUngrabKeyboard(_xwin.display, CurrentTime);
		_xwin.keyboard_grabbed = 0;
	}

#ifdef ALLEGROGL_HAVE_XF86VIDMODE
	if (_glxwin.fullscreen) {
		if (_xwin.mode_switched) {
			XF86VidModeLockModeSwitch(_xwin.display, _xwin.screen, False);
			XF86VidModeSwitchToMode(_xwin.display, _xwin.screen,
			                        _xwin.modesinfo[0]);
			XF86VidModeSetViewPort(_xwin.display, _xwin.screen, 0, 0);
			_xwin.mode_switched = 0;
		}
		if (_xwin.override_redirected) {
			setattr.override_redirect = False;
			XChangeWindowAttributes(_xwin.display, _xwin.window,
			                        CWOverrideRedirect, &setattr);
			_xwin.override_redirected = 0;
		}

		/* Free modelines.  */
		free_modelines(_xwin.modesinfo, _xwin.num_modes);
		_xwin.num_modes = 0;
		_xwin.modesinfo = NULL;
	}
#endif

	/* Note: Allegro will destroy screen (== allegro_gl_screen),
	 * so don't destroy it here.
	 */
	//destroy_bitmap(allegro_gl_screen);
	ASSERT(allegro_gl_screen == screen);
 	allegro_gl_screen = NULL;

	/* Unmap the window in order not to see the cursor when quitting.
	   The window *must not* be destroyed and _xwin.visual must be left
	   to its current value otherwise the program will crash when exiting */
	if (_xwin.window != None)
		XUnmapWindow(_xwin.display, _xwin.window);

	if (_glxwin.use_glx_window) {
		glXDestroyWindow(_xwin.display, _glxwin.window);
		_glxwin.window = 0;
		_glxwin.use_glx_window = FALSE;
	}

	__allegro_gl_valid_context = FALSE;

	_xwin_window_redrawer = old_window_redrawer;
	XSetErrorHandler(old_x_error_handler);


#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
	/* Hack: Allegro 4.2.1 uses a persistent window, we need to restore it. */
	if (backup_allegro_window != None) {
		if (_xwin.colormap != None) {
			XUninstallColormap(_xwin.display, _xwin.colormap);
			XFreeColormap(_xwin.display, _xwin.colormap);
		}
		_xwin.colormap = backup_allegro_colormap;

		if (_xwin.window != None)
			XDestroyWindow(_xwin.display, _xwin.window);
		_xwin.window = backup_allegro_window;
		backup_allegro_window = None;
		XMapWindow(_xwin.display, _xwin.window);
	}
#endif

	XUNLOCK();
}



/* get_shift:
 *  Returns the shift value for a given mask.
 */
static int get_shift (int mask)
{
	int i = 0, j = 1;
	if (!mask) return -1;
	while (!(j & mask)) {
		i++;
		j <<= 1;
	}
	return i;
}



static int decode_fbconfig (GLXFBConfig fbc, struct allegro_gl_display_info *i) {
	int render_type, visual_type, buffer_size, sbuffers, samples;
	int drawable_type, renderable;
	XVisualInfo *v;

	TRACE(PREFIX_I "decode_fbconfig: Decoding:\n");
	i->rmethod = 2;

	if (glXGetFBConfigAttrib (_xwin.display, fbc, GLX_RENDER_TYPE,
							&render_type)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_X_RENDERABLE,
							&renderable)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_DRAWABLE_TYPE,
							&drawable_type)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_X_VISUAL_TYPE,
							&visual_type)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_BUFFER_SIZE,
							&buffer_size)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_DEPTH_SIZE,
							&i->depth_size)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_STEREO,
							&i->stereo)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_RED_SIZE,
							&i->pixel_size.rgba.r)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_GREEN_SIZE,
							&i->pixel_size.rgba.g)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_BLUE_SIZE,
							&i->pixel_size.rgba.b)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_ALPHA_SIZE,
							&i->pixel_size.rgba.a)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_DOUBLEBUFFER,
							&i->doublebuffered)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_AUX_BUFFERS,
							&i->aux_buffers)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_STENCIL_SIZE,
			 				&i->stencil_size)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_ACCUM_RED_SIZE,
							&i->accum_size.rgba.r)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_ACCUM_GREEN_SIZE,
							&i->accum_size.rgba.g)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_ACCUM_BLUE_SIZE,
							&i->accum_size.rgba.b)
	 || glXGetFBConfigAttrib (_xwin.display, fbc, GLX_ACCUM_ALPHA_SIZE,
	                  &i->accum_size.rgba.a)) {
		TRACE(PREFIX_I "decode_fbconfig: Incomplete glX mode ...\n");
		return -1;
	}

	if (!(render_type & GLX_RGBA_BIT) && !(render_type & GLX_RGBA_FLOAT_BIT)) {
		TRACE(PREFIX_I "decode_fbconfig: Not RGBA mode\n");
		return -1;
	}

	if (!(drawable_type & GLX_WINDOW_BIT)) {
		TRACE(PREFIX_I "decode_fbconfig: Cannot render to a window.\n");
		return -1;
	}
	
	if (renderable == False) {
		TRACE(PREFIX_I "decode_fbconfig: GLX windows not supported.\n");
		return -1;
	}

	if (visual_type != GLX_TRUE_COLOR && visual_type != GLX_DIRECT_COLOR) {
		TRACE(PREFIX_I "decode_fbconfig: visual type other than TrueColor and "
						"DirectColor.\n");
		return -1;
	}

	/* Floating-point depth is not supported as glx extension (yet). */
	i->float_depth = 0;

	i->float_color = (render_type & GLX_RGBA_FLOAT_BIT);

	v = glXGetVisualFromFBConfig(_xwin.display, fbc);
	if (!v) {
		TRACE(PREFIX_I "decode_fbconfig: Cannot get associated visual for the "
						"FBConfig.\n");
		return -1;
	}
	i->r_shift = get_shift (v->red_mask);
	i->g_shift = get_shift (v->green_mask);
	i->b_shift = get_shift (v->blue_mask);
	i->a_shift = 0;

	/* If we are going to need to setup a palette we need bit shifts */
	if ((visual_type == GLX_DIRECT_COLOR)
		&& ((i->r_shift == -1) || (i->g_shift == -1) || (i->b_shift == -1))
		&& (i->pixel_size.rgba.r + i->pixel_size.rgba.g + i->pixel_size.rgba.b
		   <= 12)) {
		/* XXX <rohannessian> Report something here? */
		XFree(v);
		return -1;
	}

	i->colour_depth = 0;

	if (i->pixel_size.rgba.r == 3
	 && i->pixel_size.rgba.g == 3
	 && i->pixel_size.rgba.b == 2) {
		i->colour_depth = 8;
	}

	if (i->pixel_size.rgba.r == 5
	 && i->pixel_size.rgba.b == 5) {
		if (i->pixel_size.rgba.g == 5) {
			i->colour_depth = 15;
		}
		if (i->pixel_size.rgba.g == 6) {
			i->colour_depth = 16;
		}
	}

	if (i->pixel_size.rgba.r == 8
	 && i->pixel_size.rgba.g == 8
	 && i->pixel_size.rgba.b == 8) {
	 	if (i->pixel_size.rgba.a == 0) {
			i->colour_depth = 24;
		}
		if (i->pixel_size.rgba.a == 8) {
			i->colour_depth = 32;
			/* small hack that tries to guess alpha shifting */
			i->a_shift = 48 - i->r_shift - i->g_shift - i->b_shift;
		}
	}

	i->allegro_format = (i->colour_depth != 0)
	                 && (i->g_shift == i->pixel_size.rgba.b)
	                 && (i->r_shift * i->b_shift == 0)
	                 && (i->r_shift + i->b_shift
	                            == i->pixel_size.rgba.b + i->pixel_size.rgba.g);

	if (glXGetConfig(_xwin.display, v, GLX_SAMPLE_BUFFERS, &sbuffers)) {
		/* Multisample extension is not supported */
		i->sample_buffers = 0;
	}
	else {
		i->sample_buffers = sbuffers;
	}
	if (glXGetConfig(_xwin.display, v, GLX_SAMPLES, &samples)) {
		/* Multisample extension is not supported */
		i->samples = 0;
	}
	else {
		i->samples = samples;
	}

	XFree(v);

	TRACE(PREFIX_I "Color Depth: %i\n", buffer_size);
	TRACE(PREFIX_I "RGBA Type: %s point\n", i->float_color ? "floating" : "fixed");
	TRACE(PREFIX_I "RGBA: %i.%i.%i.%i\n", i->pixel_size.rgba.r, i->pixel_size.rgba.g,
	      i->pixel_size.rgba.b, i->pixel_size.rgba.a);
	TRACE(PREFIX_I "Accum: %i.%i.%i.%i\n", i->accum_size.rgba.r, i->accum_size.rgba.g,
	      i->accum_size.rgba.b, i->accum_size.rgba.a);
	TRACE(PREFIX_I "DblBuf: %i Zbuf: %i Stereo: %i Aux: %i Stencil: %i\n",
	      i->doublebuffered, i->depth_size, i->stereo,
	      i->aux_buffers, i->stencil_size);
	TRACE(PREFIX_I "Shift: %i.%i.%i.%i\n", i->r_shift, i->g_shift, i->b_shift,
	      i->a_shift);
	TRACE(PREFIX_I "Sample Buffers: %i Samples: %i\n", i->sample_buffers, i->samples);
	TRACE(PREFIX_I "Decoded bpp: %i\n", i->colour_depth);

	return 0;
}



int allegro_gl_x_windowed_choose_fbconfig (GLXFBConfig *ret_fbconfig) {
	int num_fbconfigs, i;
	GLXFBConfig *fbconfig;
	struct allegro_gl_display_info dinfo;

	fbconfig = glXGetFBConfigs (_xwin.display, _xwin.screen, &num_fbconfigs);
	if (!fbconfig || !num_fbconfigs)
		return FALSE;

	TRACE(PREFIX_I "x_windowed_choose_fbconfig: %i formats.\n", num_fbconfigs);
	__allegro_gl_reset_scorer();

	for (i = 0; i < num_fbconfigs; i++) {
		TRACE(PREFIX_I "x_windowed_choose_fbconfig: Mode %i\n", i);
		if (decode_fbconfig (*(fbconfig + i), &dinfo) != -1) {
			__allegro_gl_score_config (i, &dinfo);
		}
	}

	i = __allegro_gl_best_config();
	TRACE(PREFIX_I "x_windowed_choose_fbconfig: Best FBConfig is: %i\n", i);

	if (i < 0) {
		XFree(fbconfig);
		return FALSE;
	}

	*ret_fbconfig = *(fbconfig + i);
	XFree(fbconfig);

	return TRUE;
}



/* windowed_choose_visual:
 *  Chooses a visual to use.
 */
static XVisualInfo *allegro_gl_x_windowed_choose_visual (void)
{
	int num_visuals, i;
	XVisualInfo *vinfo;
	struct allegro_gl_display_info dinfo;
	static XVisualInfo ret_vinfo;

	vinfo = XGetVisualInfo (_xwin.display, 0, NULL, &num_visuals);
	if (!vinfo) return NULL;
	
	TRACE(PREFIX_I "x_windowed_choose_visual: %i formats.\n", num_visuals);
	__allegro_gl_reset_scorer();

	for (i = 0; i < num_visuals; i++) {
		TRACE(PREFIX_I "x_windowed_choose_visual: Mode %i\n", i);
		if (decode_visual (vinfo + i, &dinfo) != -1) {
			__allegro_gl_score_config (i, &dinfo);
		}
	}

	i = __allegro_gl_best_config();
	TRACE(PREFIX_I "x_windowed_choose_visual: Best config is: %i\n", i);

	if (i < 0) return NULL;

	memcpy (&ret_vinfo, vinfo+i, sizeof ret_vinfo);
	XFree (vinfo);

	return &ret_vinfo;
}



#ifdef ALLEGROGL_HAVE_XF86VIDMODE
/* get_xf86_modes:
 *  Test if the XF86VidMode extension is available and get the gfx modes
 *  that can be queried.
 */
static int get_xf86_modes(XF86VidModeModeInfo ***modesinfo, int *num_modes)
{
	int vid_event_base, vid_error_base;
	int vid_major_version, vid_minor_version;

	/* Test for presence of VidMode extension.  */
	if (!XF86VidModeQueryExtension(_xwin.display, &vid_event_base,
	                               &vid_error_base)
	 || !XF86VidModeQueryVersion(_xwin.display, &vid_major_version,
	                             &vid_minor_version)) {

		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("VidMode extension is not supported"));
		return -1;
	}

	if (!XF86VidModeGetAllModeLines(_xwin.display, _xwin.screen, num_modes,
	                                modesinfo)) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Can not Get ModeLines"));
		return -1;
	}

	return 0;
}
#endif


static int allegro_gl_x_error_handler(Display *display, XErrorEvent *err_event)
{
	char buffer[256];

	XGetErrorText(display, err_event->error_code, buffer, 256);
	TRACE(PREFIX_E "%s\n", buffer);
	return 0;
}


/* create_window:
 *  Based on Michael's `_xwin[_private]_create_window' and the xdemos
 *  from the Mesa distribution (I don't remember which one).
 */
static int allegro_gl_x_create_window (int fullscreen)
{
	Window root;
	XVisualInfo *visinfo;
	XSetWindowAttributes setattr;
	unsigned long valuemask = CWBackPixel | CWBorderPixel | CWColormap
	                        | CWEventMask;
	XSizeHints *hints;
	GLXFBConfig fbconfig;
	int use_fbconfig;

	if (_xwin.display == 0) {
		return -2;
	}

	old_x_error_handler = XSetErrorHandler(allegro_gl_x_error_handler);

	/* Fill in missing color depth info */
	__allegro_gl_fill_in_info();

	use_fbconfig = (_glxwin.major > 1 || (_glxwin.major == 1 && _glxwin.minor >= 3));

	if (use_fbconfig) {
		TRACE(PREFIX_I "x_create_window: using FBConfig routines\n");

		if (!allegro_gl_x_windowed_choose_fbconfig(&fbconfig)) {
			TRACE(PREFIX_I "x_create_window: Failed using FBConfig, switching "
							"back to VisualInfo routines\n");
			use_fbconfig = FALSE;
			goto old_choose_visual;
		}

		/* Query back FBConfig components */
		if (decode_fbconfig(fbconfig, &allegro_gl_display_info)) {
			TRACE(PREFIX_E "x_create_window: Cannot decode FBConfig, switching "
							"back to VisualInfo routines\n");
			use_fbconfig = FALSE;
			goto old_choose_visual;
		}

		visinfo = glXGetVisualFromFBConfig(_xwin.display, fbconfig);
		if (!visinfo) {
			TRACE(PREFIX_I "x_create_window: Failed to convert FBConfig to "
						"visual, switching back to VisualInfo routines\n");
			use_fbconfig = FALSE;
			goto old_choose_visual;
		}
	}
	else {
old_choose_visual:
		TRACE(PREFIX_I "x_create_window: using VisualInfo routines\n");

		/* Find best visual */
		visinfo = allegro_gl_x_windowed_choose_visual();
		if (!visinfo) {
			TRACE(PREFIX_E "x_create_window: Can not get visual.\n");
			XSetErrorHandler(old_x_error_handler);
			return -2;
		}

		/* Query back visual components */
		if (decode_visual (visinfo, &allegro_gl_display_info)) {
			TRACE(PREFIX_E "x_create_window: Can not decode visual.\n");
			XSetErrorHandler(old_x_error_handler);
			return -2;
		}
	}

	/* Log some information about it */
	switch (visinfo->class) {
		case TrueColor:
			AGL_LOG (1, "x.c: visual class: TrueColor\n");
			break;
		case DirectColor:
			AGL_LOG (1, "x.c: visual class: DirectColor\n");
			break;
		default:
			AGL_LOG (1, "x.c: visual class: invalid(!)\n");
	}


	/* Begin window creation. */

#if GET_ALLEGRO_VERSION() >= MAKE_VER(4, 2, 1)
	/* Hack: For Allegro 4.2.1, we need to keep the existing window. */
	if (backup_allegro_window == None) {
		backup_allegro_window = _xwin.window;
		backup_allegro_colormap = _xwin.colormap;
		_xwin.colormap = None;
		XUnmapWindow(_xwin.display, _xwin.window);
	}
	else
#endif
		XDestroyWindow (_xwin.display, _xwin.window);

	_xwin.window = None;

	root = RootWindow (_xwin.display, _xwin.screen);

	/* Recreate window. */
	setattr.background_pixel = XBlackPixel (_xwin.display, _xwin.screen);
	setattr.border_pixel = XBlackPixel (_xwin.display, _xwin.screen);
	setattr.colormap = XCreateColormap (_xwin.display, root, visinfo->visual, AllocNone);
	setattr.event_mask =
		( KeyPressMask | KeyReleaseMask
		| EnterWindowMask | LeaveWindowMask
		| FocusChangeMask | ExposureMask
		| ButtonPressMask | ButtonReleaseMask | PointerMotionMask
		/*| MappingNotifyMask (SubstructureRedirectMask?)*/
	);

#ifdef ALLEGROGL_HAVE_XF86VIDMODE
	if (fullscreen) {
		int i;
		int bestmode = 0;
		_xwin.num_modes = 0;
		_xwin.modesinfo = NULL;
		_glxwin.fullscreen = TRUE;

		if (get_xf86_modes(&_xwin.modesinfo, &_xwin.num_modes)) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("x_create_window: Can't get"
			                          "XF86VidMode info.\n"));
			XSetErrorHandler(old_x_error_handler);
			return -2;
		}

		/* look for mode with requested resolution */
		for (i = 0; i < _xwin.num_modes; i++)
		{
			if ((_xwin.modesinfo[i]->hdisplay == allegro_gl_display_info.w)
			 && (_xwin.modesinfo[i]->vdisplay == allegro_gl_display_info.h))
				bestmode = i;
		}

		setattr.override_redirect = True;
		if (!XF86VidModeSwitchToMode(_xwin.display, _xwin.screen,
		                             _xwin.modesinfo[bestmode])) {

			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Can not set XF86VidMode mode"));
			XSetErrorHandler(old_x_error_handler);
			return -1;
		}

		XF86VidModeSetViewPort(_xwin.display, _xwin.screen, 0, 0);

		/* Lock Mode switching */
		XF86VidModeLockModeSwitch(_xwin.display, _xwin.screen, True);
		_xwin.mode_switched = 1;

		allegro_gl_display_info.x = 0;
		allegro_gl_display_info.y = 0;
		allegro_gl_display_info.w = _xwin.modesinfo[bestmode]->hdisplay;
		allegro_gl_display_info.h = _xwin.modesinfo[bestmode]->vdisplay;

		valuemask |= CWOverrideRedirect;
		_xwin.override_redirected = 1;
	}

	_xwin.window = XCreateWindow (
		_xwin.display, root,
		allegro_gl_display_info.x, allegro_gl_display_info.y,
		allegro_gl_display_info.w, allegro_gl_display_info.h, 0,
		visinfo->depth,
		InputOutput,
		visinfo->visual,
		valuemask, &setattr
	);

#else //ALLEGROGL_HAVE_XF86VIDMODE
	if (fullscreen) {
		/* Without Xf86VidMode extension we support only fullscreen modes which
		 * match current resolution. */
		int fs_width  = DisplayWidth(_xwin.display, _xwin.screen);
		int fs_height = DisplayHeight(_xwin.display, _xwin.screen);

		if (fs_width  != allegro_gl_display_info.w
		 || fs_height != allegro_gl_display_info.h) {
			TRACE(PREFIX_E "Only desktop resolution fullscreen available.");
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
				get_config_text("Compiled without Xf86VidMode extension support.\n"
								"Only desktop resolution fullscreen available."));
			XSetErrorHandler(old_x_error_handler);
			return -1;
		}

		_glxwin.fullscreen = TRUE;

		/* Create the fullscreen window.  */
		_xwin.window = XCreateWindow(_xwin.display, root,
									allegro_gl_display_info.x, allegro_gl_display_info.y,
									fs_width, fs_height, 0,
									visinfo->depth,
									InputOutput,
									visinfo->visual,
									valuemask, &setattr);

		/* Map the fullscreen window.  */
		XMapRaised(_xwin.display, _xwin.window);

		/* Make sure we got to the top of the window stack.  */
		XRaiseWindow(_xwin.display, _xwin.fs_window);
	}
	else {
		_xwin.window = XCreateWindow (
			_xwin.display, root,
			allegro_gl_display_info.x, allegro_gl_display_info.y,
			allegro_gl_display_info.w, allegro_gl_display_info.h, 0,
			visinfo->depth,
			InputOutput,
			visinfo->visual,
			valuemask, &setattr
		);
	}
#endif //ALLEGROGL_HAVE_XF86VIDMODE

	/* Set size and position hints for Window Manager :
	 * prevents the window to be resized
	 */
	hints = XAllocSizeHints();
	if (hints) {
		/* This code chunk comes from Allegro's src/x/xwin.c */
		hints->flags = PMinSize | PMaxSize | PBaseSize;
		hints->min_width  = hints->max_width  = hints->base_width
		                  = allegro_gl_display_info.w;
		hints->min_height = hints->max_height = hints->base_height
		                  = allegro_gl_display_info.h;

		XSetWMNormalHints(_xwin.display, _xwin.window, hints);
		XFree(hints);
	}
	

	/* Set WM_DELETE_WINDOW atom in WM_PROTOCOLS property
	 * (to get window_delete requests).
	 */
	{
		Atom wm_delete_window = XInternAtom(_xwin.display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(_xwin.display, _xwin.window, &wm_delete_window, 1);
	}

	/* Finish off the GLX setup */
	if (use_fbconfig)
		_glxwin.ctx = glXCreateNewContext (_xwin.display, fbconfig, GLX_RGBA_TYPE, NULL, True);
	else
		_glxwin.ctx = glXCreateContext (_xwin.display, visinfo, NULL, True);

	if (use_fbconfig) {
		_glxwin.window = glXCreateWindow(_xwin.display, fbconfig, _xwin.window, 0);	
		if (!_glxwin.window) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Cannot create GLX window."));
			XSetErrorHandler(old_x_error_handler);
			return -1;
		}
		_glxwin.use_glx_window = TRUE;
	}

	if (!_glxwin.ctx) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Can not create GLX context."));
		XSetErrorHandler(old_x_error_handler);
		return -1;
	}
	else {
		Bool ret;

		if (use_fbconfig)
			ret = glXMakeContextCurrent(_xwin.display, _glxwin.window, _glxwin.window, _glxwin.ctx);
		else
			ret = glXMakeCurrent (_xwin.display, _xwin.window, _glxwin.ctx);

		if (!ret) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Cannot make GLX context current."));
			XSetErrorHandler(old_x_error_handler);
			return -1;
		}
	}

	/* Finish off the Allegro setup */

	/* Get associated visual and window depth (bits per pixel), and
	 * store window size  */
	{
		XWindowAttributes getattr;
		XGetWindowAttributes(_xwin.display, _xwin.window, &getattr);
		_xwin.visual = getattr.visual;
		_xwin.window_depth = getattr.depth;
		_xwin.window_width = allegro_gl_display_info.w;
		_xwin.window_height = allegro_gl_display_info.h;
		_xwin.screen_depth = getattr.depth;
		_xwin.screen_width = allegro_gl_display_info.w;
		_xwin.screen_height = allegro_gl_display_info.h;
	}

	/* Destroy the current colormap (if any) */
	if (_xwin.colormap != None) {
		XUninstallColormap(_xwin.display, _xwin.colormap);
		XFreeColormap(_xwin.display, _xwin.colormap);
	}

	/* Create and install colormap.  */
	if (_xwin.visual->class == DirectColor) {
		_xwin.colormap = XCreateColormap(_xwin.display, _xwin.window,
		                                 _xwin.visual, AllocAll);
	}
	else { /* must be TrueColor */
		_xwin.colormap = XCreateColormap(_xwin.display, _xwin.window,
		                                 _xwin.visual, AllocNone);
	}
	XSetWindowColormap(_xwin.display, _xwin.window, _xwin.colormap);
	XInstallColormap(_xwin.display, _xwin.colormap);

	/* Setup a palette if needed */
	if (_xwin.visual->class == DirectColor) {
		XColor color;
		int rsize, gsize, bsize;
		int rmax, gmax, bmax;
		int rshift, gshift, bshift;
		int r, g, b;

		AGL_LOG (1, "x.c: Using DirectColor visual, setting palette...\n");

		rsize = 1 << allegro_gl_display_info.pixel_size.rgba.r;
		gsize = 1 << allegro_gl_display_info.pixel_size.rgba.g;
		bsize = 1 << allegro_gl_display_info.pixel_size.rgba.b;

		rshift = allegro_gl_display_info.r_shift;
		bshift = allegro_gl_display_info.b_shift;
		gshift = allegro_gl_display_info.g_shift;

		rmax = rsize - 1;
		gmax = gsize - 1;
		bmax = bsize - 1;

		color.flags = DoRed | DoGreen | DoBlue;
		for (r = 0; r < rsize; r++) {
			for (g = 0; g < gsize; g++) {
				for (b = 0; b < bsize; b++) {
					color.pixel = (r << rshift) | (g << gshift) | (b << bshift);
					color.red = ((rmax <= 0) ? 0 : ((r * 65535L) / rmax));
					color.green = ((gmax <= 0) ? 0 : ((g * 65535L) / gmax));
					color.blue = ((bmax <= 0) ? 0 : ((b * 65535L) / bmax));
					XStoreColor(_xwin.display, _xwin.colormap, &color);
				}
			}
		}
	}

	/* Configure the window a bit */
	{
		XClassHint hint;
		XWMHints wm_hints;

		/* Set title.  */
		XStoreName(_xwin.display, _xwin.window, _xwin.window_title);

		/* Set hints.  */
		hint.res_name = _xwin.application_name;
		hint.res_class = _xwin.application_class;
		XSetClassHint(_xwin.display, _xwin.window, &hint);

		wm_hints.flags = InputHint | StateHint;
		wm_hints.input = True;
		wm_hints.initial_state = NormalState;

#ifdef ALLEGRO_XWINDOWS_WITH_XPM
		if (allegro_icon) {
			wm_hints.flags |= IconPixmapHint | IconMaskHint  | WindowGroupHint;
			XpmCreatePixmapFromData(_xwin.display, _xwin.window, allegro_icon,&wm_hints.icon_pixmap, &wm_hints.icon_mask, NULL);
		}
#endif

		XSetWMHints(_xwin.display, _xwin.window, &wm_hints);
	}

	/* Map window.  */
	XMapWindow(_xwin.display, _xwin.window);


	if (fullscreen) {
		AL_CONST char *fc = NULL;
		char tmp1[64], tmp2[128];
		int c = 0;
		int h = allegro_gl_display_info.h;
		int w = allegro_gl_display_info.w;
		
		/* This chunk is disabled by default because of problems on KDE
		   desktops.  */
		fc = get_config_string(uconvert_ascii("graphics", tmp1),
			uconvert_ascii("force_centering", tmp2), NULL);
		if ((fc) && ((c = ugetc(fc)) != 0) && ((c == 'y') || (c == 'Y')
			|| (c == '1'))) {
			/* Hack: make the window fully visible and center cursor.  */
			XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0, 0, 0);
			XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0,
			             w - 1, 0);
			XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0,
			             0, h - 1);
			XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0,
			             w - 1, h - 1);
		}
		XWarpPointer(_xwin.display, None, _xwin.window, 0, 0, 0, 0,
		             w / 2, h / 2);
		XSync(_xwin.display, False);
		
		/* Grab keyboard and mouse.  */
		if (XGrabKeyboard(_xwin.display, _xwin.window, False, GrabModeAsync,
	    	GrabModeAsync, CurrentTime) != GrabSuccess) {
			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Can not grab keyboard"));
			XSetErrorHandler(old_x_error_handler);
			return -1;
		}
		_xwin.keyboard_grabbed = 1;
		
		if (XGrabPointer(_xwin.display, _xwin.window, False, 
	    	PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
	    	GrabModeAsync, GrabModeAsync, _xwin.window, None, CurrentTime)
		 != GrabSuccess) {

			ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
			          get_config_text("Can not grab mouse"));
			XSetErrorHandler(old_x_error_handler);
			return -1;
		}
		_xwin.mouse_grabbed = 1;
	}


	/* Destroy current cursor (if any) */
	if (_xwin.cursor != None) {
		XUndefineCursor(_xwin.display, _xwin.window);
		XFreeCursor(_xwin.display, _xwin.cursor);
	}

	{
		/* Create invisible X cursor.  */
		Pixmap pixmap = XCreatePixmap(_xwin.display, _xwin.window, 1, 1, 1);
		if (pixmap != None) {
			GC temp_gc;
			XColor color;
			XGCValues gcvalues;

			int gcmask = GCFunction | GCForeground | GCBackground;
			gcvalues.function = GXcopy;
			gcvalues.foreground = 0;
			gcvalues.background = 0;
			temp_gc = XCreateGC(_xwin.display, pixmap, gcmask, &gcvalues);
			XDrawPoint(_xwin.display, pixmap, temp_gc, 0, 0);
			XFreeGC(_xwin.display, temp_gc);
			color.pixel = 0;
			color.red = color.green = color.blue = 0;
			color.flags = DoRed | DoGreen | DoBlue;
			_xwin.cursor = XCreatePixmapCursor(_xwin.display, pixmap, pixmap,
			                                   &color, &color, 0, 0);
			XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
			XFreePixmap(_xwin.display, pixmap);
		}
		else {
			_xwin.cursor = XCreateFontCursor(_xwin.display, _xwin.cursor_shape);
			XDefineCursor(_xwin.display, _xwin.window, _xwin.cursor);
		}
	}

	/* Wait for the first exposure event. See comment in Allegro's
     * xwin.c about why no blocking X11 function is used.
     */
    while (1) {
       XEvent e;
       if (XCheckTypedEvent(_xwin.display, Expose, &e)) {
          if (e.xexpose.count == 0) break;
       }
       rest(1);
   }

	return 0;
}



static BITMAP *allegro_gl_x_windowed_create_screen (GFX_DRIVER *drv, int w, int h, int depth)
{
	BITMAP *bmp;
	int is_linear = drv->linear;

	drv->linear = 1;
	bmp = _make_bitmap (w, h, 0, drv, depth, 0);
	bmp->id = BMP_ID_VIDEO | BMP_ID_MASK;
	drv->linear = is_linear;

	if (bmp == 0) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("Not enough memory"));
		return NULL;
	}
	
	drv->w = w;
	drv->h = h;

	return bmp;
}



/* decode_visual:
 *  Used to read back the information in the visual.  0 = ok.
 */
static int decode_visual (XVisualInfo *v, struct allegro_gl_display_info *i)
{
	int rgba, buffer_size, use_gl, sbuffers, samples;

	TRACE(PREFIX_I "decode_visual: Decoding:\n");
	i->rmethod = 2;

	/* We can only support TrueColor and DirectColor visuals --
	 * we only support RGBA mode */
	if (v->class != TrueColor && v->class != DirectColor)
		return -1;

	if (glXGetConfig (_xwin.display, v, GLX_RGBA, &rgba)
	 || glXGetConfig (_xwin.display, v, GLX_USE_GL,       &use_gl)
	 || glXGetConfig (_xwin.display, v, GLX_BUFFER_SIZE,  &buffer_size)
	 || glXGetConfig (_xwin.display, v, GLX_RED_SIZE,     &i->pixel_size.rgba.r)
	 || glXGetConfig (_xwin.display, v, GLX_GREEN_SIZE,   &i->pixel_size.rgba.g)
	 || glXGetConfig (_xwin.display, v, GLX_BLUE_SIZE,    &i->pixel_size.rgba.b)
	 || glXGetConfig (_xwin.display, v, GLX_ALPHA_SIZE,   &i->pixel_size.rgba.a)
	 || glXGetConfig (_xwin.display, v, GLX_DOUBLEBUFFER, &i->doublebuffered)
	 || glXGetConfig (_xwin.display, v, GLX_STEREO,       &i->stereo)
	 || glXGetConfig (_xwin.display, v, GLX_AUX_BUFFERS,  &i->aux_buffers)
	 || glXGetConfig (_xwin.display, v, GLX_DEPTH_SIZE,   &i->depth_size)
	 || glXGetConfig (_xwin.display, v, GLX_STENCIL_SIZE, &i->stencil_size)
	 || glXGetConfig (_xwin.display, v, GLX_ACCUM_RED_SIZE,
	                  &i->accum_size.rgba.r)
	 || glXGetConfig (_xwin.display, v, GLX_ACCUM_GREEN_SIZE,
	                  &i->accum_size.rgba.g)
	 || glXGetConfig (_xwin.display, v, GLX_ACCUM_BLUE_SIZE,
	                  &i->accum_size.rgba.b)
	 || glXGetConfig (_xwin.display, v, GLX_ACCUM_ALPHA_SIZE,
	                  &i->accum_size.rgba.a)) {
		TRACE(PREFIX_I "x_create_window: Incomplete glX mode ...\n");
		return -1;
	}

	if (!rgba) {
		TRACE(PREFIX_I "x_create_window: Not RGBA mode\n");
		return -1;
	}
	
	if (!use_gl) {
		ustrzcpy (allegro_gl_error, AGL_ERROR_SIZE,
		          get_config_text("OpenGL Unsupported"));
		return -1;
	}
	
	i->r_shift = get_shift (v->red_mask);
	i->g_shift = get_shift (v->green_mask);
	i->b_shift = get_shift (v->blue_mask);
	i->a_shift = 0;
	
	/* If we are going to need to setup a palette we need bit shifts */
	if ((v->class == DirectColor)
		&& ((i->r_shift == -1) || (i->g_shift == -1) || (i->b_shift == -1))
		&& (i->pixel_size.rgba.r + i->pixel_size.rgba.g + i->pixel_size.rgba.b
		   <= 12)) {
		/* XXX <rohannessian> Report something here? */
		return -1;
	}

	i->float_color = 0;
	i->float_depth = 0;

	i->colour_depth = 0;

	if (i->pixel_size.rgba.r == 3
	 && i->pixel_size.rgba.g == 3
	 && i->pixel_size.rgba.b == 2) {
		i->colour_depth = 8;
	}

	if (i->pixel_size.rgba.r == 5
	 && i->pixel_size.rgba.b == 5) {
		if (i->pixel_size.rgba.g == 5) {
			i->colour_depth = 15;
		}
		if (i->pixel_size.rgba.g == 6) {
			i->colour_depth = 16;
		}
	}

	if (i->pixel_size.rgba.r == 8
	 && i->pixel_size.rgba.g == 8
	 && i->pixel_size.rgba.b == 8) {
	 	if (i->pixel_size.rgba.a == 0) {
			i->colour_depth = 24;
		}
		if (i->pixel_size.rgba.a == 8) {
			i->colour_depth = 32;
			/* small hack that tries to guess alpha shifting */
			i->a_shift = 48 - i->r_shift - i->g_shift - i->b_shift;
		}
	}

	i->allegro_format = (i->colour_depth != 0)
	                 && (i->g_shift == i->pixel_size.rgba.b)
	                 && (i->r_shift * i->b_shift == 0)
	                 && (i->r_shift + i->b_shift
	                            == i->pixel_size.rgba.b + i->pixel_size.rgba.g);
	
	if (glXGetConfig(_xwin.display, v, GLX_SAMPLE_BUFFERS, &sbuffers)
	                                                     == GLX_BAD_ATTRIBUTE) {
		/* Multisample extension is not supported */
		i->sample_buffers = 0;
	}
	else {
		i->sample_buffers = sbuffers;
	}
	if (glXGetConfig(_xwin.display, v, GLX_SAMPLES, &samples)
	                                                     == GLX_BAD_ATTRIBUTE) {
		/* Multisample extension is not supported */
		i->samples = 0;
	}
	else {
		i->samples = samples;
	}

	
	TRACE(PREFIX_I "Color Depth: %i\n", buffer_size);
	TRACE(PREFIX_I "RGBA: %i.%i.%i.%i\n", i->pixel_size.rgba.r, i->pixel_size.rgba.g,
	      i->pixel_size.rgba.b, i->pixel_size.rgba.a);
	TRACE(PREFIX_I "Accum: %i.%i.%i.%i\n", i->accum_size.rgba.r, i->accum_size.rgba.g,
	      i->accum_size.rgba.b, i->accum_size.rgba.a);
	TRACE(PREFIX_I "DblBuf: %i Zbuf: %i Stereo: %i Aux: %i Stencil: %i\n",
	      i->doublebuffered, i->depth_size, i->stereo,
	      i->aux_buffers, i->stencil_size);
	TRACE(PREFIX_I "Shift: %i.%i.%i.%i\n", i->r_shift, i->g_shift, i->b_shift,
	      i->a_shift);
	TRACE(PREFIX_I "Sample Buffers: %i Samples: %i\n", i->sample_buffers, i->samples);
	TRACE(PREFIX_I "Decoded bpp: %i\n", i->colour_depth);
	
	return 0;
}



#ifdef ALLEGROGL_HAVE_XF86VIDMODE
/* allegro_gl_x_fetch_mode_list:
 *  Generates a list of valid video modes (made after 
 *  _xvidmode_private_fetch_mode_list of Allegro)
 */
static GFX_MODE_LIST* allegro_gl_x_fetch_mode_list(void)
{
	int num_modes = 0;
	XF86VidModeModeInfo **modesinfo = NULL;
	GFX_MODE_LIST *mode_list;
	int i;

	XLOCK();

	if (get_xf86_modes(&modesinfo, &num_modes)) {
		XUNLOCK();
		return NULL;
	}

	/* Allocate space for mode list.  */
	mode_list = malloc(sizeof(GFX_MODE_LIST));
	if (!mode_list) {
		free_modelines(modesinfo, num_modes);
		XUNLOCK();
		return NULL;
	}

	mode_list->mode = malloc(sizeof(GFX_MODE) * (num_modes + 1));
	if (!mode_list->mode) {
		free(mode_list);
		free_modelines(modesinfo, num_modes);
		XUNLOCK();
		return NULL;
	}

	/* Fill in mode list.  */
	for (i = 0; i < num_modes; i++) {
		mode_list->mode[i].width = modesinfo[i]->hdisplay;
		mode_list->mode[i].height = modesinfo[i]->vdisplay;
		/* Since XF86VidMode can not change the color depth of
		 * the screen, there is no need to define modes for other
		 * color depth than the desktop's.
		 */
		mode_list->mode[i].bpp = desktop_color_depth();
	}

	mode_list->mode[num_modes].width = 0;
	mode_list->mode[num_modes].height = 0;
	mode_list->mode[num_modes].bpp = 0;
	mode_list->num_modes = num_modes;

	free_modelines(modesinfo, num_modes);

	XUNLOCK();
	return mode_list;
}
#endif



/* allegro_gl_x_vsync:
 *  Wait for a vertical retrace. GLX_SGI_video_sync is needed.
 */
static void allegro_gl_x_vsync(void)
{
	XLOCK();
	if (allegro_gl_extensions_GLX.SGI_video_sync) {
		unsigned int count;

		glXGetVideoSyncSGI(&count);
		glXWaitVideoSyncSGI(2, (count+1) & 1, &count);
	}
	XUNLOCK();
}



/******************************/
/* AllegroGL driver functions */
/******************************/

/* flip:
 *  Does a page flip / double buffer copy / whatever it really is.
 */
static void flip (void)
{
	XLOCK();
	if (_glxwin.use_glx_window)
		glXSwapBuffers (_xwin.display, _glxwin.window);
	else
		glXSwapBuffers (_xwin.display, _xwin.window);
	XUNLOCK();
}



/* gl_on, gl_off:
 *  Switches to/from GL mode.
 */
static void gl_on (void)
{
#ifdef OLD_ALLEGRO
	DISABLE();
#endif
}



static void gl_off (void)
{
#ifdef OLD_ALLEGRO
	ENABLE();
	_xwin_handle_input();
#endif
}



/*****************/
/* Driver struct */
/*****************/

static struct allegro_gl_driver allegro_gl_x = {
	flip,
	gl_on,
	gl_off,
	NULL
};

