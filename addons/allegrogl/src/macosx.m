/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
#include <string.h>
#define ALLEGRO_SRC
#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <allegro/platform/aintosx.h>

#include "alleggl.h"
#include "glvtable.h"
#include "allglint.h"

#define OSX_GFX_OPENGL           100
#define MAX_ATTRIBUTES           64

#define PREFIX_I "agl-osx INFO: "
#define PREFIX_W "agl-osx WARNING: "


@interface AllegroGLView: NSOpenGLView
- (void)resetCursorRects;
- (id) initWithFrame: (NSRect) frame;
@end

static BITMAP *allegro_gl_macosx_init_windowed(int w, int h, int v_w, int v_h, int color_depth);
static BITMAP *allegro_gl_macosx_init_fullscreen(int w, int h, int v_w, int v_h, int color_depth);
static void allegro_gl_macosx_exit(struct BITMAP *b);
static void macosx_setup_gl(void);
static NSOpenGLPixelFormat *init_pixel_format(int windowed);

static struct allegro_gl_driver allegro_gl_macosx;

static AllegroWindowDelegate *window_delegate = NULL;
static NSOpenGLContext *context;

static CFDictionaryRef old_mode = NULL;

static BITMAP *allegro_gl_screen = NULL;



GFX_DRIVER gfx_allegro_gl_windowed = {
	GFX_OPENGL_WINDOWED,
	EMPTY_STRING,
	EMPTY_STRING,
	"AllegroGL Windowed (MacOS X)",
	allegro_gl_macosx_init_windowed,
	allegro_gl_macosx_exit,
	NULL,						/* scrolling not implemented */
	NULL,						/* vsync, may use for flip? */
	NULL,						/* No h/w pallete, not using indexed mode */
	NULL, NULL,					/* Still no scrolling */
	NULL,						/* No triple buffering */
	allegro_gl_create_video_bitmap,
	allegro_gl_destroy_video_bitmap,
	NULL, NULL,					/* No show/request video bitmaps */
	NULL, NULL,					/* No system bitmaps */
	allegro_gl_set_mouse_sprite,
	allegro_gl_show_mouse,
	allegro_gl_hide_mouse,
	allegro_gl_move_mouse,
	allegro_gl_drawing_mode,
	NULL, NULL,			/* No video state stuff */
	allegro_gl_set_blender_mode,
	NULL,                       /* No fetch_mode_list */
	0, 0,						/* physical (not virtual!) screen size */
	0,							/* true if video memory is linear */
	0,							/* bank size, in bytes */
	0,							/* bank granularity, in bytes */
	0,							/* video memory size, in bytes */
	0,							/* physical address of video memory */
	TRUE                        /* Windowed mode? */
};

GFX_DRIVER gfx_allegro_gl_fullscreen = {
	GFX_OPENGL_FULLSCREEN,
	EMPTY_STRING,
	EMPTY_STRING,
	"AllegroGL Fullscreen (MacOS X)",
	allegro_gl_macosx_init_fullscreen,
	allegro_gl_macosx_exit,
	NULL,						/* scrolling not implemented */
	NULL,						/* vsync, may use for flip? */
	NULL,						/* No h/w pallete, not using indexed mode */
	NULL, NULL,					/* Still no scrolling */
	NULL,						/* No triple buffering */
	allegro_gl_create_video_bitmap,
	allegro_gl_destroy_video_bitmap,
	NULL, NULL,					/* No show/request video bitmaps */
	NULL, NULL,					/* No system bitmaps */
	allegro_gl_set_mouse_sprite,
	allegro_gl_show_mouse,
	allegro_gl_hide_mouse,
	allegro_gl_move_mouse,
	allegro_gl_drawing_mode,
	NULL, NULL,			/* No video state stuff */
	allegro_gl_set_blender_mode,
	NULL,                       /* No fetch_mode_list */
	0, 0,						/* physical (not virtual!) screen size */
	0,							/* true if video memory is linear */
	0,							/* bank size, in bytes */
	0,							/* bank granularity, in bytes */
	0,							/* video memory size, in bytes */
	0,							/* physical address of video memory */
	TRUE                       /* Windowed mode */
};



@implementation AllegroGLView

- (void)resetCursorRects
{
   [super resetCursorRects];
   [self addCursorRect: NSMakeRect(0, 0, gfx_allegro_gl_windowed.w, gfx_allegro_gl_windowed.h)
      cursor: osx_cursor];
   [osx_cursor setOnMouseEntered: YES];
}
/* Custom view: when created, select a suitable pixel format */
- (id) initWithFrame: (NSRect) frame
{
	NSOpenGLPixelFormat* pf = init_pixel_format(TRUE);
	if (pf) {
            self = [super initWithFrame:frame pixelFormat: pf];
            [pf release];
            return self;
	}
	else
	{
            TRACE(PREFIX_W "Unable to find suitable pixel format\n");
	}
	return nil;
}
@end


/* NSOpenGLPixelFormat *build_pixelformat(int format)
 *
 * Given the options in format, attempt to build a pixel format.
 * return nil if no format is suitable
 */
static NSOpenGLPixelFormat *build_pixelformat(int format) {
	NSOpenGLPixelFormatAttribute attribs[MAX_ATTRIBUTES], *attrib;
	attrib=attribs;
	if ((format & AGL_DOUBLEBUFFER) && allegro_gl_display_info.doublebuffered)
		*attrib++ = NSOpenGLPFADoubleBuffer;
	if ((format & AGL_STEREO) && allegro_gl_display_info.stereo)
		*attrib++ = NSOpenGLPFAStereo;
	if (format & AGL_AUX_BUFFERS) {
		*attrib++ = NSOpenGLPFAAuxBuffers;
		*attrib++ = allegro_gl_display_info.aux_buffers;
	}
	if (format & (AGL_COLOR_DEPTH | AGL_RED_DEPTH | AGL_GREEN_DEPTH | AGL_BLUE_DEPTH)) {
		*attrib++ = NSOpenGLPFAColorSize;
		*attrib++ = allegro_gl_display_info.colour_depth;
	}
	if (format & AGL_ALPHA_DEPTH) {
		*attrib++ = NSOpenGLPFAAlphaSize;
		*attrib++ = allegro_gl_display_info.pixel_size.rgba.a;
	}
	if (format & AGL_Z_DEPTH) {
		*attrib++ = NSOpenGLPFADepthSize;
		*attrib++ = allegro_gl_display_info.depth_size;
	}
	if (format & AGL_STENCIL_DEPTH) {
		*attrib++ = NSOpenGLPFAStencilSize;
		*attrib++ = allegro_gl_display_info.stencil_size;
	}
	if (format & (AGL_ACC_RED_DEPTH | AGL_ACC_GREEN_DEPTH | AGL_ACC_BLUE_DEPTH | AGL_ACC_ALPHA_DEPTH)) {
		*attrib++ = NSOpenGLPFAAccumSize;
		*attrib++ = allegro_gl_display_info.accum_size.rgba.r +
			    allegro_gl_display_info.accum_size.rgba.g +
			    allegro_gl_display_info.accum_size.rgba.b +
			    allegro_gl_display_info.accum_size.rgba.a;
	}
	*attrib++ = NSOpenGLPFAMinimumPolicy;
        /* Always request one of fullscreen or windowed */
	if (allegro_gl_display_info.fullscreen) {
		*attrib++ = NSOpenGLPFAFullScreen;
		*attrib++ = NSOpenGLPFAScreenMask;
		*attrib++ = CGDisplayIDToOpenGLDisplayMask(kCGDirectMainDisplay);
	}
	else {
		*attrib++ = NSOpenGLPFAWindow;
		*attrib++ = NSOpenGLPFABackingStore;
	}
	if ((format & AGL_RENDERMETHOD) && (allegro_gl_display_info.rmethod==1))
		*attrib++ = NSOpenGLPFAAccelerated;
        if (format & AGL_SAMPLE_BUFFERS) {
                *attrib++ = NSOpenGLPFASampleBuffers;
                *attrib++ = allegro_gl_display_info.sample_buffers;
		*attrib++ = NSOpenGLPFANoRecovery;
        }
        if (format & AGL_SAMPLES) {
                *attrib++ = NSOpenGLPFASamples;
                *attrib++ = allegro_gl_display_info.samples;
        }
	*attrib = 0;
	
	return [[NSOpenGLPixelFormat alloc] initWithAttributes: attribs];
}

/* NSOpenGLPixelFormat *init_pixel_format(int windowed)
 *
 * Generate a pixel format. First try and get all the 'suggested' settings.
 * If this fails, just get the 'required' settings,
 * or nil if no format can be found
 */
static NSOpenGLPixelFormat *init_pixel_format(int windowed)
{
	NSOpenGLPixelFormat *pf;
	allegro_gl_display_info.fullscreen=!windowed;
	/* First we try an aggressive approach, requiring everything */
	pf=build_pixelformat(__allegro_gl_required_settings | __allegro_gl_suggested_settings);
	/* If not, then just try with what is required */
	if (pf==nil) {
	  pf=build_pixelformat(__allegro_gl_required_settings);
	}
	return pf;
}



static void decode_pixel_format(NSOpenGLPixelFormat *pf, struct allegro_gl_display_info *dinfo)
{
	GLint value;
	
	TRACE(PREFIX_I "Decoding:\n");
	[pf getValues: &value forAttribute: NSOpenGLPFAAccelerated forVirtualScreen: 0];
	dinfo->rmethod = value ? 1 : 0;
	TRACE(PREFIX_I "Acceleration: %s\n", ((dinfo->rmethod == 0) ? "No" : "Yes"));

	[pf getValues: &value forAttribute: NSOpenGLPFADoubleBuffer forVirtualScreen: 0];
	dinfo->doublebuffered = value;
	[pf getValues: &value forAttribute: NSOpenGLPFAStereo forVirtualScreen: 0];
	dinfo->stereo = value;
	[pf getValues: &value forAttribute: NSOpenGLPFAAuxBuffers forVirtualScreen: 0];
	dinfo->aux_buffers = value;
	[pf getValues: &value forAttribute: NSOpenGLPFADepthSize forVirtualScreen: 0];
	dinfo->depth_size = value;
	[pf getValues: &value forAttribute: NSOpenGLPFAStencilSize forVirtualScreen: 0];
	dinfo->stencil_size = value;
	TRACE(PREFIX_I "DblBuf: %i Zbuf: %i Stereo: %i Aux: %i Stencil: %i\n",
		  dinfo->doublebuffered, dinfo->depth_size, dinfo->stereo,
		  dinfo->aux_buffers, dinfo->stencil_size);
	
	[pf getValues: &value forAttribute: NSOpenGLPFASampleBuffers forVirtualScreen: 0];
        dinfo->sample_buffers = value;
	[pf getValues: &value forAttribute: NSOpenGLPFASamples forVirtualScreen: 0];
        dinfo->samples = value;
        TRACE(PREFIX_I "Sample buffers: %i Samples: %i\n", dinfo->sample_buffers, dinfo->samples);

	[pf getValues: &value forAttribute: NSOpenGLPFAColorSize forVirtualScreen: 0];
	dinfo->colour_depth = value;
	[pf getValues: &value forAttribute: NSOpenGLPFAAlphaSize forVirtualScreen: 0];
	dinfo->pixel_size.rgba.a = value;
	switch (dinfo->colour_depth) {
		case 32:
		case 24:
			dinfo->pixel_size.rgba.r = 8;
			dinfo->pixel_size.rgba.g = 8;
			dinfo->pixel_size.rgba.b = 8;
			dinfo->r_shift = 24;
			dinfo->g_shift = 16;
			dinfo->b_shift = 8;
			dinfo->a_shift = 0;
			break;
		case 16:
			dinfo->pixel_size.rgba.r = 5;
			dinfo->pixel_size.rgba.g = 6;
			dinfo->pixel_size.rgba.b = 5;
			dinfo->r_shift = 11;
			dinfo->g_shift = 5;
			dinfo->b_shift = 0;
			dinfo->a_shift = 0;
			break;
		case 15:
			dinfo->pixel_size.rgba.r = 5;
			dinfo->pixel_size.rgba.g = 5;
			dinfo->pixel_size.rgba.b = 5;
			dinfo->r_shift = 11;
			dinfo->g_shift = 6;
			dinfo->b_shift = 1;
			dinfo->a_shift = 0;
			break;
		default:
			TRACE(PREFIX_W "Bad color depth\n");
	}
	TRACE(PREFIX_I "Decoded bpp: %i\n", dinfo->colour_depth);
	dinfo->allegro_format = (dinfo->colour_depth != 0)
		&& (dinfo->g_shift == dinfo->pixel_size.rgba.b)
		&& (dinfo->r_shift * dinfo->b_shift == 0)
		&& (dinfo->r_shift + dinfo->b_shift == dinfo->pixel_size.rgba.b + dinfo->pixel_size.rgba.g);
	[pf getValues: &value forAttribute: NSOpenGLPFAAccumSize forVirtualScreen: 0];
	value /= 4;
	dinfo->accum_size.rgba.r = value;
	dinfo->accum_size.rgba.g = value;
	dinfo->accum_size.rgba.b = value;
	dinfo->accum_size.rgba.a = value;

	dinfo->float_color = 0;
	dinfo->float_depth = 0;
}



static BITMAP *allegro_gl_create_screen (GFX_DRIVER *drv, int w, int h, int depth)
{
	BITMAP *bmp;
	int is_linear = drv->linear;

	drv->linear = 1;
	bmp = _make_bitmap (w, h, 0, drv, depth, 0);
	bmp->id = BMP_ID_VIDEO | 1000;
	drv->linear = is_linear;

	drv->w = w;
	drv->h = h;

	return bmp;
}



void setup_shifts(void)
{
#if TARGET_RT_BIG_ENDIAN
   __allegro_gl_set_allegro_image_format(TRUE);
   #else
   __allegro_gl_set_allegro_image_format(FALSE);
#endif   
   
   __linear_vtable15.mask_color = makecol15(255, 0, 255);
   __linear_vtable16.mask_color = makecol16(255, 0, 255);
   __linear_vtable24.mask_color = makecol24(255, 0, 255);
   __linear_vtable32.mask_color = makecol32(255, 0, 255);
}



static BITMAP *allegro_gl_macosx_init_windowed(int w, int h, int v_w, int v_h, int color_depth)
{
	NSRect rect = NSMakeRect(0, 0, w, h);
	AllegroGLView *view;
	int desktop_depth;
	
	/* virtual screen are not supported */
	if ((v_w != 0 && v_w != w) || (v_h != 0 && v_h != h))
		return NULL;
	
	setup_shifts();
	
	/* Fill in missing color depth info */
	__allegro_gl_fill_in_info();

	/* Be sure the current desktop color depth is at least 15bpp */
	/* We may want to change this, so try to set a better depth, or
	   to at least report an error somehow */
	desktop_depth = desktop_color_depth();

	if (desktop_depth < 15)
		return NULL;

	TRACE(PREFIX_I "Requested color depth: %i  Desktop color depth: %i\n", allegro_gl_display_info.colour_depth, desktop_depth);
	
	_unix_lock_mutex(osx_event_mutex);
	
	osx_window = [[NSWindow alloc] initWithContentRect: rect
		styleMask: NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask
		backing: NSBackingStoreBuffered
		defer: NO];
   
	window_delegate = [[[AllegroWindowDelegate alloc] init] autorelease];
	[osx_window setDelegate: window_delegate];
	[osx_window setOneShot: YES];
	[osx_window setAcceptsMouseMovedEvents: YES];
	[osx_window setViewsNeedDisplay: NO];
	[osx_window setReleasedWhenClosed: YES];
	[osx_window center];
	[osx_window makeKeyAndOrderFront: nil];
	if ((__allegro_gl_required_settings | __allegro_gl_suggested_settings) & (AGL_WINDOW_X | AGL_WINDOW_Y)) {
		/* TODO: set window position */
	}

	set_window_title(osx_window_title);
	view = [[AllegroGLView alloc] initWithFrame: rect];
	[osx_window setContentView: view];

	context=[[view openGLContext] retain];
	[context makeCurrentContext];
	
	decode_pixel_format([view pixelFormat], &allegro_gl_display_info);
	set_color_depth(allegro_gl_display_info.colour_depth);
	allegro_gl_display_info.w = gfx_allegro_gl_windowed.w = w;
	allegro_gl_display_info.h = gfx_allegro_gl_windowed.h = h;

	allegro_gl_screen = allegro_gl_create_screen(&gfx_allegro_gl_windowed, w, h, allegro_gl_get(AGL_COLOR_DEPTH));
	
	TRACE(PREFIX_I "GLScreen: %ix%ix%i\n", w, h,  allegro_gl_get(AGL_COLOR_DEPTH));
	allegro_gl_screen->id |= BMP_ID_VIDEO | 1000;

	__allegro_gl_valid_context = TRUE;
	__allegro_gl_driver = &allegro_gl_macosx;
	
	allegro_gl_info.is_mesa_driver = FALSE;

	/* Print out OpenGL version info */
	TRACE(PREFIX_I "OpenGL Version: %s\n",
	     (AL_CONST char*)glGetString(GL_VERSION));
	TRACE(PREFIX_I "Vendor: %s\n",
	     (AL_CONST char*)glGetString(GL_VENDOR));
	TRACE(PREFIX_I "Renderer: %s\n",
	     (AL_CONST char*)glGetString(GL_RENDERER));

	/* init the GL extensions */
	__allegro_gl_manage_extensions();
	
	/* Update screen vtable in order to use AGL's */
	__allegro_gl__glvtable_update_vtable(&allegro_gl_screen->vtable);
	memcpy(&_screen_vtable, allegro_gl_screen->vtable, sizeof(GFX_VTABLE));
	allegro_gl_screen->vtable = &_screen_vtable;

	osx_mouse_tracking_rect = [view addTrackingRect: rect
				   owner: NSApp
				   userData: nil
				   assumeInside: YES];
	osx_keyboard_focused(FALSE, 0);
	clear_keybuf();
	osx_gfx_mode = OSX_GFX_OPENGL;
	osx_skip_mouse_move = TRUE;

        macosx_setup_gl();
	[context flushBuffer];
	_unix_unlock_mutex(osx_event_mutex);

	return allegro_gl_screen;
}



static BITMAP *allegro_gl_macosx_init_fullscreen(int w, int h, int v_w, int v_h, int color_depth)
{
	NSOpenGLPixelFormat *pf;
	CFDictionaryRef mode = NULL;
	int target_depth;
	boolean_t match = FALSE;
	int refresh_rate;
	
	/* virtual screen are not supported */
	if ((v_w != 0 && v_w != w) || (v_h != 0 && v_h != h))
		return NULL;
	
	setup_shifts();
	
	/* Fill in missing color depth info */
	__allegro_gl_fill_in_info();

	if (allegro_gl_display_info.colour_depth <= 8)
		return NULL;

	TRACE(PREFIX_I "Requested color depth: %i\n", allegro_gl_display_info.colour_depth);
	
	_unix_lock_mutex(osx_event_mutex);
	
	target_depth = allegro_gl_display_info.colour_depth;
	if (target_depth == 16)
		target_depth = 15;
	if (_refresh_rate_request > 0)
		mode = CGDisplayBestModeForParametersAndRefreshRate(kCGDirectMainDisplay, target_depth, w, h,
			(double)_refresh_rate_request, &match);
	if (!match)
		mode = CGDisplayBestModeForParameters(kCGDirectMainDisplay, target_depth, w, h, &match);
	if (!match) {
		TRACE(PREFIX_W "Unsupported mode %dx%dx%d\n", w, h, target_depth);
		_unix_unlock_mutex(osx_event_mutex);
		return NULL;
	}

	osx_init_fade_system();
	old_mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
	CGDisplayHideCursor(kCGDirectMainDisplay);
	osx_fade_screen(FALSE, 0.3);
	if (CGDisplayCapture(kCGDirectMainDisplay) != kCGErrorSuccess) {
		TRACE(PREFIX_W "Cannot capture main display\n");
		old_mode = NULL;
		CGDisplayRestoreColorSyncSettings();
		_unix_unlock_mutex(osx_event_mutex);
		return NULL;
	}
	if (CGDisplaySwitchToMode(kCGDirectMainDisplay, mode) != kCGErrorSuccess) {
		TRACE(PREFIX_W "Cannot switch main display mode\n");
		CGDisplayRelease(kCGDirectMainDisplay);
		old_mode = NULL;
		CGDisplayRestoreColorSyncSettings();
		_unix_unlock_mutex(osx_event_mutex);
		return NULL;
	}
	HideMenuBar();
	CGDisplayRestoreColorSyncSettings();

	CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &refresh_rate);
	_set_current_refresh_rate(refresh_rate);
	
	pf = init_pixel_format(FALSE);
	if (!pf) {
		TRACE(PREFIX_W "Unable to find suitable pixel format\n");
		CGDisplaySwitchToMode(kCGDirectMainDisplay, old_mode);
		CGDisplayRelease(kCGDirectMainDisplay);
		old_mode = NULL;
		CGDisplayRestoreColorSyncSettings();
		_unix_unlock_mutex(osx_event_mutex);
		return NULL;
	}
	context = [[NSOpenGLContext alloc] initWithFormat: pf
					   shareContext: nil];
	if (!context) {
		TRACE(PREFIX_W "Unable to create OpenGL context\n");
		[pf release];
		CGDisplaySwitchToMode(kCGDirectMainDisplay, old_mode);
		CGDisplayRelease(kCGDirectMainDisplay);
		old_mode = NULL;
		CGDisplayRestoreColorSyncSettings();
		_unix_unlock_mutex(&osx_event_mutex);
		return NULL;
	}

	[context makeCurrentContext];
	[context setFullScreen];

	decode_pixel_format(pf, &allegro_gl_display_info);
	set_color_depth(allegro_gl_display_info.colour_depth);
	allegro_gl_display_info.w = gfx_allegro_gl_fullscreen.w = w;
	allegro_gl_display_info.h = gfx_allegro_gl_fullscreen.h = h;

	allegro_gl_screen = allegro_gl_create_screen(&gfx_allegro_gl_fullscreen, w, h, allegro_gl_get(AGL_COLOR_DEPTH));
	
	TRACE(PREFIX_I "GLScreen: %ix%ix%i\n", w, h,  allegro_gl_get(AGL_COLOR_DEPTH));
	allegro_gl_screen->id |= BMP_ID_VIDEO | 1000;

	__allegro_gl_valid_context = TRUE;
	__allegro_gl_driver = &allegro_gl_macosx;
	__allegro_gl__glvtable_update_vtable(&allegro_gl_screen->vtable);
	memcpy(&_screen_vtable, allegro_gl_screen->vtable, sizeof(GFX_VTABLE));
	allegro_gl_screen->vtable = &_screen_vtable;
	
	allegro_gl_info.is_mesa_driver = FALSE;

	/* init the GL extensions */
	__allegro_gl_manage_extensions();
	
	osx_keyboard_focused(FALSE, 0);
	clear_keybuf();
	osx_gfx_mode = OSX_GFX_OPENGL;
	osx_skip_mouse_move = TRUE;

        macosx_setup_gl();
	[context flushBuffer];
	
	_unix_unlock_mutex(osx_event_mutex);

	return allegro_gl_screen;
}



static void allegro_gl_macosx_exit(struct BITMAP *b)
{
        _unix_lock_mutex(osx_event_mutex);

	if (old_mode)
		osx_fade_screen(FALSE, 0.2);

	if (context) {
		[context clearDrawable];
		[context release];
		context = NULL;
	}
	
	if (osx_window) {
		[osx_window close];
		osx_window = NULL;
	}
	
	if (old_mode) {
		CGDisplaySwitchToMode(kCGDirectMainDisplay, old_mode);
		CGDisplayRelease(kCGDirectMainDisplay);
		CGDisplayShowCursor(kCGDirectMainDisplay);
		ShowMenuBar();
		osx_fade_screen(TRUE, 0.3);
		CGDisplayRestoreColorSyncSettings();
		old_mode = NULL;
	}
	
	osx_mouse_tracking_rect = -1;
	
	osx_gfx_mode = OSX_GFX_NONE;
	
	__allegro_gl_unmanage_extensions();
	__allegro_gl_valid_context = FALSE;
	
	_unix_unlock_mutex(osx_event_mutex);
}

/* static void macosx_setup_gl(void)
 *
 * Initialize a reasonable viewport. Those should be OpenGL defaults,
 * but some drivers don't implement this correctly.
 */
static void macosx_setup_gl(void) {
    glViewport(0, 0, SCREEN_W, SCREEN_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    /* For some reason OSX seems to set the scissor test weirdly */
    glScissor(0,0,SCREEN_W,SCREEN_H);

    /* Set up some variables that some GL drivers omit */
    glBindTexture(GL_TEXTURE_2D, 0);

    /* Clear the screen */
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}


/* AllegroGL driver routines */

static void flip(void)
{
	[context flushBuffer];
}

static void gl_on(void)
{
	return;
}

static void gl_off(void)
{
	return;
}


/* AllegroGL driver */

static struct allegro_gl_driver allegro_gl_macosx = {
	flip, gl_on, gl_off, NULL
};

