/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
#include <string.h>
#include <allegro.h>
#include <allegro/internal/aintern.h>


#include "alleggl.h"
#include "glvtable.h"
#include "allglint.h"


static BITMAP *allegro_gl_win_init_windowed(int w, int h, int v_w, int v_h,
                                            int color_depth);
static BITMAP *allegro_gl_win_init_fullscreen(int w, int h, int v_w, int v_h,
                                              int color_depth);
static void allegro_gl_win_exit(struct BITMAP *b);
static GFX_MODE_LIST* allegro_gl_win_fetch_mode_list(void);

static struct allegro_gl_driver allegro_gl_win;

#define PREFIX_I                "agl-win INFO: "
#define PREFIX_W                "agl-win WARNING: "
#define PREFIX_E                "agl-win ERROR: "


static BITMAP *allegro_gl_screen = NULL;


/* Windowed mode driver */
GFX_DRIVER gfx_allegro_gl_windowed = {
	GFX_OPENGL_WINDOWED,
	EMPTY_STRING,
	EMPTY_STRING,
	"AllegroGL Windowed (Win32)",
	allegro_gl_win_init_windowed,
	allegro_gl_win_exit,
	NULL,                       /* scrolling not implemented */
	NULL,                       /* vsync, may use for flip? */
	NULL,                       /* No h/w pallete, not using indexed mode */
	NULL, NULL,                 /* Still no scrolling */
	NULL,                       /* No triple buffering */
	allegro_gl_create_video_bitmap,
	allegro_gl_destroy_video_bitmap,
	NULL, NULL,                 /* No show/request video bitmaps */
	NULL, NULL,                 /* No system bitmaps */
	allegro_gl_set_mouse_sprite,
	allegro_gl_show_mouse,
	allegro_gl_hide_mouse,
	allegro_gl_move_mouse,
	allegro_gl_drawing_mode,
	NULL, NULL,           /* No video state stuff */
	allegro_gl_set_blender_mode,
	NULL,                       /* No fetch_mode_list */
	0,0,                        /* physical (not virtual!) screen size */
	0,                          /* true if video memory is linear */
	0,                          /* bank size, in bytes */
	0,                          /* bank granularity, in bytes */
	0,                          /* video memory size, in bytes */
	0,                          /* physical address of video memory */
	TRUE                        /* Windowed mode */
};


/* Fullscreen driver */
GFX_DRIVER gfx_allegro_gl_fullscreen = {
	GFX_OPENGL_FULLSCREEN,
	EMPTY_STRING,
	EMPTY_STRING,
	"AllegroGL Fullscreen (Win32)",
	allegro_gl_win_init_fullscreen,
	allegro_gl_win_exit,
	NULL,                       /* scrolling not implemented */
	NULL,                       /* vsync, may use for flip? */
	NULL,                       /* No h/w pallete, not using indexed mode */
	NULL, NULL,                 /* Still no scrolling */
	NULL,                       /* No triple buffering */
	allegro_gl_create_video_bitmap,
	allegro_gl_destroy_video_bitmap,
	NULL, NULL,                 /* No show/request video bitmaps */
	NULL, NULL,                 /* No system bitmaps */
	allegro_gl_set_mouse_sprite,
	allegro_gl_show_mouse,
	allegro_gl_hide_mouse,
	allegro_gl_move_mouse,
	allegro_gl_drawing_mode,
	NULL, NULL,           /* No video state stuff */
	allegro_gl_set_blender_mode,
	allegro_gl_win_fetch_mode_list, /* fetch_mode_list */
	0,0,                        /* physical (not virtual!) screen size */
	0,                          /* true if video memory is linear */
	0,                          /* bank size, in bytes */
	0,                          /* bank granularity, in bytes */
	0,                          /* video memory size, in bytes */
	0,                          /* physical address of video memory */
	FALSE                       /* Windowed mode */
};


/* XXX <rohannessian> We should move those variable definitions into a struct,
 * for when multiple windows end up being supported.
 */

/* Device Context used for the Allegro window. Note that only one window
 * is supported, so only onyl HDC is needed. This is shared by the AGL
 * extension code.
 */
HDC __allegro_gl_hdc = NULL;

/* Render Context used by AllegroGL, once screen mode was set. Note that only
 * a single window is supported.
 */
static HGLRC allegro_glrc = NULL;

/* Full-screen flag, for the current context. */
static int fullscreen = 0;

/* Current window handle */
static HWND wnd = NULL;

/* If AGL was initialized */
static int initialized = 0;

/* XXX <rohannessian> Put those globals as function parameters */
/* Note - these globals should really end up as parameters to functions.
 */
static DWORD style_saved, exstyle_saved;
static DEVMODE dm_saved;
static int test_windows_created = 0;
static int new_w = 0, new_h = 0;

static PIXELFORMATDESCRIPTOR pfd = {
	sizeof(PIXELFORMATDESCRIPTOR),	/* size of this pfd */
	1,							/* version number */
	PFD_DRAW_TO_WINDOW			/* support window */
		| PFD_SUPPORT_OPENGL	/* support OpenGL */
		| PFD_DOUBLEBUFFER,		/* double buffered */
	PFD_TYPE_RGBA,				/* RGBA type */
	24,							/* 24-bit color depth */
	0, 0, 0, 0, 0, 0,			/* color bits ignored */
	0,							/* no alpha buffer */
	0,							/* shift bit ignored */
	0,							/* no accumulation buffer */
	0, 0, 0, 0,					/* accum bits ignored */
	0,							/* z-buffer */
	0,							/* no stencil buffer */
	0,							/* no auxiliary buffer */
	PFD_MAIN_PLANE,				/* main layer */
	0,							/* reserved */
	0, 0, 0						/* layer masks ignored */
};



/* Logs a Win32 error/warning message in the log file.
 */
static void log_win32_msg(const char *prefix, const char *func,
                          const char *error_msg, DWORD err) {

	char *err_msg = NULL;
	BOOL free_msg = TRUE;

	/* Get the formatting error string from Windows. Note that only the
	 * bottom 14 bits matter - the rest are reserved for various library
	 * IDs and type of error.
	 */
	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
		             | FORMAT_MESSAGE_FROM_SYSTEM
	                 | FORMAT_MESSAGE_IGNORE_INSERTS,
	                 NULL, err & 0x3FFF,
	                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	                 (LPTSTR) &err_msg, 0, NULL)) {
		err_msg = "(Unable to decode error code)  ";
		free_msg = FALSE;
	}

	/* Remove two trailing characters */
	if (err_msg && strlen(err_msg) > 1)
		*(err_msg + strlen(err_msg) - 2) = '\0';

	TRACE("%s%s(): %s %s (0x%08lx)\n", prefix, func,
		  error_msg ? error_msg : "",
		  err_msg ? err_msg : "(null)",
		  (unsigned long)err);

	if (free_msg) {
		LocalFree(err_msg);
	}

	return;
}



/* Logs an error */
static void log_win32_error(const char *func, const char *error_msg,
                            DWORD err) {
	log_win32_msg(PREFIX_E, func, error_msg, err);
}



/* Logs a warning */
static void log_win32_warning(const char *func, const char *error_msg,
                              DWORD err) {
	log_win32_msg(PREFIX_W, func, error_msg, err);
}



/* Logs a note */
static void log_win32_note(const char *func, const char *error_msg, DWORD err) {
	log_win32_msg(PREFIX_I, func, error_msg, err);
}



/* Define the AllegroGL Test window class */
#define ALLEGROGL_TEST_WINDOW_CLASS "AllegroGLTestWindow"


/* Registers the test window
 * Returns 0 on success, non-zero on failure.
 */
static int register_test_window()
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = ALLEGROGL_TEST_WINDOW_CLASS;

	if (!RegisterClass(&wc)) {
		DWORD err = GetLastError();

		if (err != ERROR_CLASS_ALREADY_EXISTS) {
			log_win32_error("register_test_window",
		    	            "Unable to register the window class!", err);
			return -1;
		}
	}

	return 0;
}




/* Creates the test window.
 * The window class must have already been registered.
 * Returns the window handle, or NULL on failure.
 */
static HWND create_test_window()
{
	HWND wnd = CreateWindow(ALLEGROGL_TEST_WINDOW_CLASS,
							"AllegroGL Test Window",
							WS_POPUP | WS_CLIPCHILDREN,
							0, 0, new_w, new_h,
							NULL, NULL,
							GetModuleHandle(NULL),
							NULL);

	if (!wnd) {
		log_win32_error("create_test_window",
		                "Unable to create a test window!", GetLastError());
		return NULL;
	}		

	test_windows_created++;
	return wnd;
}



/* Print the pixel format info */
static void print_pixel_format(struct allegro_gl_display_info *dinfo) {

	if (!dinfo) {
		return;
	}
	
	TRACE(PREFIX_I "Acceleration: %s\n", ((dinfo->rmethod == 0) ? "No"
	                         : ((dinfo->rmethod == 1) ? "Yes" : "Unknown")));
	TRACE(PREFIX_I "RGBA: %i.%i.%i.%i\n", dinfo->pixel_size.rgba.r,
	      dinfo->pixel_size.rgba.g, dinfo->pixel_size.rgba.b,
	      dinfo->pixel_size.rgba.a);
	
	TRACE(PREFIX_I "Accum: %i.%i.%i.%i\n", dinfo->accum_size.rgba.r,
	      dinfo->accum_size.rgba.g, dinfo->accum_size.rgba.b,
	      dinfo->accum_size.rgba.a);
	
	TRACE(PREFIX_I "DblBuf: %i Zbuf: %i Stereo: %i Aux: %i Stencil: %i\n",
		  dinfo->doublebuffered, dinfo->depth_size, dinfo->stereo,
		  dinfo->aux_buffers, dinfo->stencil_size);
	
	TRACE(PREFIX_I "Shift: %i.%i.%i.%i\n", dinfo->r_shift, dinfo->g_shift,
		  dinfo->b_shift, dinfo->a_shift);

	TRACE(PREFIX_I "Sample Buffers: %i Samples: %i\n",
	      dinfo->sample_buffers, dinfo->samples);
	
	TRACE(PREFIX_I "Decoded bpp: %i\n", dinfo->colour_depth);	
}



/* Decodes the pixel format into an agl_display_info struct and logs the pixel
 * format in the trace file.
 */
static int decode_pixel_format(PIXELFORMATDESCRIPTOR * pfd, HDC hdc, int format,
                               struct allegro_gl_display_info *dinfo,
                               int desktop_depth)
{
	TRACE(PREFIX_I "Decoding: \n");
	/* Not interested if it doesn't support OpenGL and RGBA */
	if (!(pfd->dwFlags & PFD_SUPPORT_OPENGL)) {
		TRACE(PREFIX_I "OpenGL Unsupported\n");
		return -1;
	}
	if (pfd->iPixelType != PFD_TYPE_RGBA) {
		TRACE(PREFIX_I "Not RGBA mode\n");
		return -1;
	}

	if ((pfd->cColorBits != desktop_depth)
	 && (pfd->cColorBits != 32 || desktop_depth < 24)) {
		TRACE(PREFIX_I "Current color depth != "
			  "pixel format color depth\n");
		//return -1;  /* XXX <rohannessian> Why is this a bad thing? */
	}
	

	/* hardware acceleration */
	if (((pfd->dwFlags & PFD_GENERIC_ACCELERATED)
		 && (pfd->dwFlags & PFD_GENERIC_FORMAT))
		|| (!(pfd->dwFlags & PFD_GENERIC_ACCELERATED)
			&& !(pfd->dwFlags & PFD_GENERIC_FORMAT)))
		dinfo->rmethod = 1;
	else
		dinfo->rmethod = 0;


	/* Depths of colour buffers */
	dinfo->pixel_size.rgba.r = pfd->cRedBits;
	dinfo->pixel_size.rgba.g = pfd->cGreenBits;
	dinfo->pixel_size.rgba.b = pfd->cBlueBits;
	dinfo->pixel_size.rgba.a = pfd->cAlphaBits;

	/* Depths of accumulation buffer */
	dinfo->accum_size.rgba.r = pfd->cAccumRedBits;
	dinfo->accum_size.rgba.g = pfd->cAccumGreenBits;
	dinfo->accum_size.rgba.b = pfd->cAccumBlueBits;
	dinfo->accum_size.rgba.a = pfd->cAccumAlphaBits;

	/* Miscellaneous settings */
	dinfo->doublebuffered = pfd->dwFlags & PFD_DOUBLEBUFFER;
	dinfo->stereo = pfd->dwFlags & PFD_STEREO;
	dinfo->aux_buffers = pfd->cAuxBuffers;
	dinfo->depth_size = pfd->cDepthBits;
	dinfo->stencil_size = pfd->cStencilBits;

	/* These are the component shifts, like Allegro's _rgb_*_shift_*. */
	dinfo->r_shift = pfd->cRedShift;
	dinfo->g_shift = pfd->cGreenShift;
	dinfo->b_shift = pfd->cBlueShift;
	dinfo->a_shift = pfd->cAlphaShift;

	/* Multisampling isn't supported under Windows if we don't also use
	 * WGL_ARB_pixel_format or WGL_EXT_pixel_format.
	 */
	dinfo->sample_buffers = 0;
	dinfo->samples = 0;

	/* Float depth/color isn't supported under Windows if we don't also use
	 * AGL_ARB_pixel_format or WGL_EXT_pixel_format.
	 */
	dinfo->float_color = 0;
	dinfo->float_depth = 0;

	/* This bit is the same as the X code, setting some things based on
	 * what we've read out of the PFD. */
	dinfo->colour_depth = 0;
	if (dinfo->pixel_size.rgba.r == 5 && dinfo->pixel_size.rgba.b == 5) {
		if (dinfo->pixel_size.rgba.g == 5)
			dinfo->colour_depth = 15;
		if (dinfo->pixel_size.rgba.g == 6)
			dinfo->colour_depth = 16;
	}
	if (dinfo->pixel_size.rgba.r == 8
		&& dinfo->pixel_size.rgba.g == 8 && dinfo->pixel_size.rgba.b == 8) {
		if (dinfo->pixel_size.rgba.a == 8)
			dinfo->colour_depth = 32;
		else
			dinfo->colour_depth = 24;
	}


	dinfo->allegro_format = (dinfo->colour_depth != 0)
		&& (dinfo->g_shift == dinfo->pixel_size.rgba.b)
		&& (dinfo->r_shift * dinfo->b_shift == 0)
		&& (dinfo->r_shift + dinfo->b_shift ==
			dinfo->pixel_size.rgba.b + dinfo->pixel_size.rgba.g);

	return 0;
}



/* Decodes the pixel format into an agl_display_info struct and logs the pixel
 * format in the trace file.
 */
static int decode_pixel_format_attrib(struct allegro_gl_display_info *dinfo,
                          int num_attribs, const int *attrib, const int *value,
                          int desktop_depth) {
	int i;
	
	TRACE(PREFIX_I "Decoding: \n");

	dinfo->samples = 0;
	dinfo->sample_buffers = 0;
	dinfo->float_depth = 0;
	dinfo->float_color = 0;

	for (i = 0; i < num_attribs; i++) {

		/* Not interested if it doesn't support OpenGL or window drawing or
		 * RGBA.
		 */
		if (attrib[i] == WGL_SUPPORT_OPENGL_ARB && value[i] == 0) {	
			TRACE(PREFIX_I "OpenGL Unsupported\n");
			return -1;
		}
		else if (attrib[i] == WGL_DRAW_TO_WINDOW_ARB && value[i] == 0) {	
			TRACE(PREFIX_I "Can't draw to window\n");
			return -1;
		}
		else if (attrib[i] == WGL_PIXEL_TYPE_ARB &&
				(value[i] != WGL_TYPE_RGBA_ARB
				 && value[i] != WGL_TYPE_RGBA_FLOAT_ARB)) {	
			TRACE(PREFIX_I "Not RGBA mode\n");
			return -1;
		}
		/* Check for color depth matching */
		else if (attrib[i] == WGL_COLOR_BITS_ARB) {
			if ((value[i] != desktop_depth)
			 && (value[i] != 32 || desktop_depth < 24)) {
				TRACE(PREFIX_I "Current color depth != "
					  "pixel format color depth\n");
				//return -1; /* XXX <rohannessian> Why is this a bad thing? */
			}
		}
		/* hardware acceleration */
		else if (attrib[i] == WGL_ACCELERATION_ARB) {
			dinfo->rmethod = (value[i] == WGL_NO_ACCELERATION_ARB) ? 0 : 1;
		}
		/* Depths of colour buffers */
		else if (attrib[i] == WGL_RED_BITS_ARB) {
			dinfo->pixel_size.rgba.r = value[i];
		}
		else if (attrib[i] == WGL_GREEN_BITS_ARB) {
			dinfo->pixel_size.rgba.g = value[i];
		}
		else if (attrib[i] == WGL_BLUE_BITS_ARB) {
			dinfo->pixel_size.rgba.b = value[i];
		}
		else if (attrib[i] == WGL_ALPHA_BITS_ARB) {
			dinfo->pixel_size.rgba.a = value[i];
		}
		/* Shift of color components */
		else if (attrib[i] == WGL_RED_SHIFT_ARB) {
			dinfo->r_shift = value[i];
		}
		else if (attrib[i] == WGL_GREEN_SHIFT_ARB) {
			dinfo->g_shift = value[i];
		}
		else if (attrib[i] == WGL_BLUE_SHIFT_ARB) {
			dinfo->b_shift = value[i];
		}
		else if (attrib[i] == WGL_ALPHA_SHIFT_ARB) {
			dinfo->a_shift = value[i];
		}

		/* Depths of accumulation buffer */
		else if (attrib[i] == WGL_ACCUM_RED_BITS_ARB) {
			dinfo->accum_size.rgba.r = value[i];
		}
		else if (attrib[i] == WGL_ACCUM_GREEN_BITS_ARB) {
			dinfo->accum_size.rgba.g = value[i];
		}
		else if (attrib[i] == WGL_ACCUM_BLUE_BITS_ARB) {
			dinfo->accum_size.rgba.b = value[i];
		}
		else if (attrib[i] == WGL_ACCUM_ALPHA_BITS_ARB) {
			dinfo->accum_size.rgba.a = value[i];
		}	
		/* Miscellaneous settings */
		else if (attrib[i] == WGL_DOUBLE_BUFFER_ARB) {
			dinfo->doublebuffered = value[i];
		}
		else if (attrib[i] == WGL_STEREO_ARB) {
			dinfo->stereo = value[i];
		}
		else if (attrib[i] == WGL_AUX_BUFFERS_ARB) {
			dinfo->aux_buffers = value[i];
		}
		else if (attrib[i] == WGL_DEPTH_BITS_ARB) {
			dinfo->depth_size = value[i];
		}
		else if (attrib[i] == WGL_STENCIL_BITS_ARB) {
			dinfo->stencil_size = value[i];
		}
		/* Multisampling bits */
		else if (attrib[i] == WGL_SAMPLE_BUFFERS_ARB) {
			dinfo->sample_buffers = value[i];
		}
		else if (attrib[i] == WGL_SAMPLES_ARB) {
			dinfo->samples = value[i];
		}
		/* Float color */
		if (attrib[i] == WGL_PIXEL_TYPE_ARB
		  && value[i] == WGL_TYPE_RGBA_FLOAT_ARB) {
			dinfo->float_color = TRUE;
		}
		/* Float depth */
		else if (attrib[i] == WGL_DEPTH_FLOAT_EXT) {
			dinfo->float_depth = value[i];
		}
	}

	/* This bit is the same as the X code, setting some things based on
	 * what we've read out of the PFD. */
	dinfo->colour_depth = 0;
	if (dinfo->pixel_size.rgba.r == 5 && dinfo->pixel_size.rgba.b == 5) {
		if (dinfo->pixel_size.rgba.g == 5)
			dinfo->colour_depth = 15;
		if (dinfo->pixel_size.rgba.g == 6)
			dinfo->colour_depth = 16;
	}
	if (dinfo->pixel_size.rgba.r == 8
		&& dinfo->pixel_size.rgba.g == 8 && dinfo->pixel_size.rgba.b == 8) {
		if (dinfo->pixel_size.rgba.a == 8)
			dinfo->colour_depth = 32;
		else
			dinfo->colour_depth = 24;
	}

	dinfo->allegro_format = (dinfo->colour_depth != 0)
		&& (dinfo->g_shift == dinfo->pixel_size.rgba.b)
		&& (dinfo->r_shift * dinfo->b_shift == 0)
		&& (dinfo->r_shift + dinfo->b_shift ==
			dinfo->pixel_size.rgba.b + dinfo->pixel_size.rgba.g);

	return 0;
}



typedef struct format_t {
	int score;
	int format;
} format_t;



/* Helper function for sorting pixel formats by score */
static int select_pixel_format_sorter(const void *p0, const void *p1) {
	format_t *f0 = (format_t*)p0;
	format_t *f1 = (format_t*)p1;

	if (f0->score == f1->score) {
		return 0;
	}
	else if (f0->score > f1->score) {
		return -1;
	}
	else {
		return 1;
	}
}



/* Describes the pixel format and assigns it a score */
int describe_pixel_format_old(HDC dc, int fmt, int desktop_depth,
                               format_t *formats, int *num_formats,
                               struct allegro_gl_display_info *pdinfo) {

	struct allegro_gl_display_info dinfo;
	PIXELFORMATDESCRIPTOR pfd;
	int score = -1;
	
	int result = DescribePixelFormat(dc, fmt, sizeof(pfd), &pfd);

	/* Remember old settings */
	if (pdinfo) {
		dinfo = *pdinfo;
	}

	if (!result) {
		log_win32_warning("describe_pixel_format_old",
		                  "DescribePixelFormat() failed!", GetLastError());
		return -1;
	}
	
	result = !decode_pixel_format(&pfd, dc, fmt, &dinfo, desktop_depth);
	
	if (result) {
		print_pixel_format(&dinfo);
		score = __allegro_gl_score_config(fmt, &dinfo);
	}
			
	if (score < 0) {
		return -1; /* Reject non-compliant pixel formats */
	}

	if (formats && num_formats) {
		formats[*num_formats].score  = score;
		formats[*num_formats].format = fmt;
		(*num_formats)++;
	}

	if (pdinfo) {
		*pdinfo = dinfo;
	}

	return 0;
}



static AGL_GetPixelFormatAttribivARB_t __wglGetPixelFormatAttribivARB = NULL;
static AGL_GetPixelFormatAttribivEXT_t __wglGetPixelFormatAttribivEXT = NULL;



/* Describes the pixel format and assigns it a score */
int describe_pixel_format_new(HDC dc, int fmt, int desktop_depth,
                              format_t *formats, int *num_formats,
                              struct allegro_gl_display_info *pdinfo) {

	struct allegro_gl_display_info dinfo;
	int score = -1;

	/* Note: Even though we use te ARB suffix, all those enums are compatible
	 * with EXT_pixel_format.
	 */
	int attrib[] = {
		WGL_SUPPORT_OPENGL_ARB,
		WGL_DRAW_TO_WINDOW_ARB,
		WGL_PIXEL_TYPE_ARB,
		WGL_ACCELERATION_ARB,
		WGL_DOUBLE_BUFFER_ARB,
		WGL_DEPTH_BITS_ARB,
		WGL_COLOR_BITS_ARB,
		WGL_RED_BITS_ARB,
		WGL_GREEN_BITS_ARB,
		WGL_BLUE_BITS_ARB,
		WGL_ALPHA_BITS_ARB,
		WGL_RED_SHIFT_ARB,
		WGL_GREEN_SHIFT_ARB,
		WGL_BLUE_SHIFT_ARB,
		WGL_ALPHA_SHIFT_ARB,
		WGL_STENCIL_BITS_ARB,
		WGL_STEREO_ARB,
		WGL_ACCUM_BITS_ARB,
		WGL_ACCUM_RED_BITS_ARB,
		WGL_ACCUM_GREEN_BITS_ARB,
		WGL_ACCUM_BLUE_BITS_ARB,
		WGL_ACCUM_ALPHA_BITS_ARB,
		WGL_AUX_BUFFERS_ARB,

		/* The following are used by extensions that add to WGL_pixel_format.
		 * If WGL_p_f isn't supported though, we can't use the (then invalid)
		 * enums. We can't use any magic number either, so we settle for 
		 * replicating one. The pixel format decoder
		 * (decode_pixel_format_attrib()) doesn't care about duplicates.
		 */
		WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_SAMPLE_BUFFERS_ARB */
		WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_SAMPLES_ARB        */
		WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_DEPTH_FLOAT_EXT    */
	};

	const int num_attribs = sizeof(attrib) / sizeof(attrib[0]);
	int *value = (int*)malloc(sizeof(int) * num_attribs);
	int result;
	BOOL ret;
	int old_valid = __allegro_gl_valid_context;

	/* Can't allocate mem? */
	if (!value) {
		TRACE(PREFIX_E "describe_pixel_format_new(): Unable to allocate "
		      "memory for pixel format descriptor!\n");
		return -1;
	}

	/* Remember old settings */
	if (pdinfo) {
		dinfo = *pdinfo;
	}
	

	/* If multisampling is supported, query for it. Note - we need to tell
	 * allegro_gl_is_extension_supported() that we have a valid context,
	 * even though AGL is not initialized yet.
	 */
	__allegro_gl_valid_context = 1;
	if (allegro_gl_is_extension_supported("WGL_ARB_multisample")) {
		attrib[num_attribs - 3] = WGL_SAMPLE_BUFFERS_ARB;
		attrib[num_attribs - 2] = WGL_SAMPLES_ARB;
	}
	if (allegro_gl_is_extension_supported("WGL_EXT_depth_float")) {
		attrib[num_attribs - 1] = WGL_DEPTH_FLOAT_EXT;
	}
	__allegro_gl_valid_context = old_valid;

	
	/* Get the pf attributes */
	if (__wglGetPixelFormatAttribivARB) {
		ret = __wglGetPixelFormatAttribivARB(dc, fmt, 0, num_attribs,
		                                     attrib, value);
	}
	else if (__wglGetPixelFormatAttribivEXT) {
		ret = __wglGetPixelFormatAttribivEXT(dc, fmt, 0, num_attribs,
		                                     attrib, value);
	}
	else {
		ret = 0;
	}	

	/* wglGetPixelFormatAttrib() failed? Abort and revert to old path */
	if (!ret) {
		log_win32_error("describe_pixel_format_new",
		                "wglGetPixelFormatAttrib failed!", GetLastError());
		free(value);
		return -1;
	}

	/* Convert to AllegroGL format for scoring */
	result = !decode_pixel_format_attrib(&dinfo, num_attribs, attrib, value,
	                                     desktop_depth);
	free(value);

	if (result) {
		print_pixel_format(&dinfo);	
		score = __allegro_gl_score_config(fmt, &dinfo);
	}

	if (score < 0) {
		return 0; /* Reject non-compliant pixel formats */
	}

	if (formats && num_formats) {
		formats[*num_formats].score  = score;
		formats[*num_formats].format = fmt;
		(*num_formats)++;
	}

	if (pdinfo) {
		*pdinfo = dinfo;
	}

	return 0;
}



/* Returns the number of pixel formats we should investigate */
int get_num_pixel_formats(HDC dc, int *new_pf_code) {
	
	/* DescribePixelFormat() returns maximum pixel format index in the old
	 * code. wglGetPixelFormatAttribivARB() does it in the new code.
	 */
	if (new_pf_code && *new_pf_code) {
		int attrib[1];
		int value[1];
		
		TRACE(PREFIX_I "get_num_pixel_formats(): Attempting to use WGL_pf.\n");
		attrib[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
		if ((__wglGetPixelFormatAttribivARB
		  && __wglGetPixelFormatAttribivARB(dc, 0, 0, 1, attrib, value)
		                                                         == GL_FALSE)
		 || (__wglGetPixelFormatAttribivEXT
		  && __wglGetPixelFormatAttribivEXT(dc, 0, 0, 1, attrib, value)
		                                                         == GL_FALSE)) {
			log_win32_note("get_num_pixel_formats",
		                "WGL_ARB/EXT_pixel_format use failed!", GetLastError());
			*new_pf_code = 0;
		}
		else {
			return value[0];
		}
	}

	if (!new_pf_code || !*new_pf_code) {
		PIXELFORMATDESCRIPTOR pfd;
		int ret;
		
		TRACE(PREFIX_I "get_num_pixel_formats(): Using DescribePixelFormat.\n");
		ret = DescribePixelFormat(dc, 1, sizeof(pfd), &pfd);

		if (!ret) {
			log_win32_error("get_num_pixel_formats",
		                "DescribePixelFormat failed!", GetLastError());
		}
		
		return ret;
	}

	return 0;
}



/* Pick the best matching pixel format */
static int select_pixel_format(PIXELFORMATDESCRIPTOR * pfd)
{
	int i;
	int result, maxindex;
	int desktop_depth;

	HWND testwnd = NULL;
	HDC testdc   = NULL;
	HGLRC testrc = NULL;
	
	format_t *format = NULL;
	int num_formats = 0;
	int new_pf_code = 0;

	
	__allegro_gl_reset_scorer();

	/* Read again the desktop depth */
	desktop_depth = desktop_color_depth();
 
	if (register_test_window() < 0) {
		return 0;
	}

	testwnd = create_test_window();

	if (!testwnd) {
		return 0;
	}

	testdc = GetDC(testwnd);

	/* Check if we can support new pixel format code */
	TRACE(PREFIX_I "select_pixel_format(): Trying to set up temporary RC\n");
	{
		HDC old_dc = __allegro_gl_hdc;
		int old_valid = __allegro_gl_valid_context;
		PIXELFORMATDESCRIPTOR pfd;
		int pf;
		
		new_pf_code = 0;
		
		/* We need to create a dummy window with a pixel format to get the
		 * list of valid PFDs
		 */
		memset(&pfd, 0, sizeof(pfd));
		pfd.nSize = sizeof(pfd);
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
			        | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.cColorBits = 32;

		TRACE(PREFIX_I "select_pixel_format(): ChoosePixelFormat()\n");
		pf = ChoosePixelFormat(testdc, &pfd);

		if (!pf) {
			log_win32_warning("select_pixel_format",
		                "Unable to chose a temporary pixel format!",
			            GetLastError());
			goto fail_pf;
		}

		/* Set up a GL context there */
		TRACE(PREFIX_I "select_pixel_format(): SetPixelFormat()\n");
		memset(&pfd, 0, sizeof(pfd));
		if (!SetPixelFormat(testdc, pf, &pfd)) {
			log_win32_warning("select_pixel_format",
		                "Unable to set a temporary pixel format!",
			            GetLastError());
			goto fail_pf;
		}

		TRACE(PREFIX_I "select_pixel_format(): CreateContext()\n");
		testrc = wglCreateContext(testdc);

		if (!testrc) {
			log_win32_warning("select_pixel_format",
		                "Unable to create a render context!",
			            GetLastError());
			goto fail_pf;
		}
		
		TRACE(PREFIX_I "select_pixel_format(): MakeCurrent()\n");
		if (!wglMakeCurrent(testdc, testrc)) {
			log_win32_warning("select_pixel_format",
		                "Unable to set the render context as current!",
			            GetLastError());
			goto fail_pf;
		}

		__allegro_gl_hdc = testdc;
		__allegro_gl_valid_context = TRUE;


		/* This is a workaround for a bug in old NVidia drivers. We need to
		 * call wglGetExtensionsStringARB() for it to properly initialize.
		 */
		TRACE(PREFIX_I "select_pixel_format(): GetExtensionsStringARB()\n");
		if (strstr((AL_CONST char*)glGetString(GL_VENDOR), "NVIDIA")) {
			AGL_GetExtensionsStringARB_t __wglGetExtensionsStringARB = NULL;
			
			__wglGetExtensionsStringARB = (AGL_GetExtensionsStringARB_t)
			               wglGetProcAddress("wglGetExtensionsStringARB");

			TRACE(PREFIX_I "select_pixel_format(): Querying for "
			      "WGL_ARB_extension_string\n");
			
			if (__wglGetExtensionsStringARB) {
				TRACE(PREFIX_I "select_pixel_format(): Calling "
				      "__wglGetExtensionsStringARB\n");
				__wglGetExtensionsStringARB(testdc);
			}
		}

		
		/* Check that we support ARB/EXT_pixel_format */
		if (!allegro_gl_is_extension_supported("WGL_ARB_pixel_format")
		 && !allegro_gl_is_extension_supported("WGL_EXT_pixel_format")) {
			TRACE(PREFIX_I "select_pixel_format(): WGL_ARB/EXT_pf unsupported.\n");
			goto fail_pf;
		}
		
		/* Load the ARB_p_f symbol - Note, we shouldn't use the AGL extension
		 * mechanism here, because AGL hasn't been initialized yet!
		 */
		TRACE(PREFIX_I "select_pixel_format(): GetProcAddress()\n");		
		__wglGetPixelFormatAttribivARB = (AGL_GetPixelFormatAttribivARB_t)
		               wglGetProcAddress("wglGetPixelFormatAttribivARB");
		__wglGetPixelFormatAttribivEXT = (AGL_GetPixelFormatAttribivEXT_t)
		               wglGetProcAddress("wglGetPixelFormatAttribivEXT");

		if (!__wglGetPixelFormatAttribivARB
		 && !__wglGetPixelFormatAttribivEXT) {
			TRACE(PREFIX_E "select_pixel_format(): WGL_ARB/EXT_pf not "
			      "correctly supported!\n");
			goto fail_pf;
		}

		new_pf_code = 1;
		goto exit_pf;

fail_pf:
		wglMakeCurrent(NULL, NULL);
		if (testrc) {
			wglDeleteContext(testrc);
		}
		testrc = NULL;

		__wglGetPixelFormatAttribivARB = NULL;
		__wglGetPixelFormatAttribivEXT = NULL;
exit_pf:
		__allegro_gl_hdc = old_dc;
		__allegro_gl_valid_context = old_valid;
	}

	maxindex = get_num_pixel_formats(testdc, &new_pf_code);

	/* Check if using the new pf code failed. Likely due to driver bug.
	 * maxindex is still valid though, so we can continue.
	 */
	if (!new_pf_code && testrc) {
		TRACE(PREFIX_W "select_pixel_format(): WGL_ARB_pf call failed - "
		      "reverted to plain old WGL.\n");
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(testrc);
		testrc  = NULL;
		__wglGetPixelFormatAttribivARB = NULL;
		__wglGetPixelFormatAttribivEXT = NULL;
	}

	TRACE(PREFIX_I "select_pixel_format(): %i formats.\n", maxindex);

	if (maxindex < 1) {
		TRACE(PREFIX_E "select_pixel_format(): Didn't find any pixel "
		      "formats at all!\n");
		goto bail;
	}
	
	format = malloc((maxindex + 1) * sizeof(format_t));
	
	if (!format) {
		TRACE(PREFIX_E "select_pixel_format(): Unable to allocate memory for "
		      "pixel format scores!\n");
		goto bail;
	}

	/* First, pixel formats are sorted by decreasing order */
	TRACE(PREFIX_I "select_pixel_format(): Testing pixel formats:\n");
	for (i = 1; i <= maxindex; i++) {

		int use_old = !new_pf_code;
		
		TRACE(PREFIX_I "Format %i:\n", i);
		
		if (new_pf_code) {
			if (describe_pixel_format_new(testdc, i, desktop_depth,
			                              format, &num_formats, NULL) < 0) {
				TRACE(PREFIX_W "select_pixel_format(): Wasn't able to use "
				      "WGL_PixelFormat - reverting to old WGL code.\n");
				use_old = 1;
			}
		}

		if (use_old) {
			if (describe_pixel_format_old(testdc, i, desktop_depth,
			                          format, &num_formats, NULL) < 0) {
				TRACE(PREFIX_W "select_pixel_format(): Unable to rely on "
				      "unextended WGL to describe this pixelformat.\n");
			}
		}
 	}

	if (new_pf_code) {
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(testrc);
		testrc = NULL;
	}
	if (testwnd) {
		ReleaseDC(testwnd, testdc);
		testdc = NULL;
		DestroyWindow(testwnd);
		testwnd = NULL;
	}

	if (num_formats < 1) {
		TRACE(PREFIX_E "select_pixel_format(): Didn't find any available "
		      "pixel formats!\n");
		goto bail;
	}

	qsort(format, num_formats, sizeof(format_t), select_pixel_format_sorter);


	/* Sorted pixel formats are tested until one of them succeeds to
	 * make a GL context current */
 	for (i = 0; i < num_formats ; i++) {
		HGLRC rc;

		/* Recreate our test windows */
		testwnd = create_test_window();
		testdc = GetDC(testwnd);
		
		if (SetPixelFormat(testdc, format[i].format, pfd)) {
			rc = wglCreateContext(testdc);
			if (!rc) {
				TRACE(PREFIX_I "select_pixel_format(): Unable to create RC!\n");
			}
			else {
				if (wglMakeCurrent(testdc, rc)) {
					wglMakeCurrent(NULL, NULL);
					wglDeleteContext(rc);
					rc = NULL;

					TRACE(PREFIX_I "select_pixel_format(): Best config is: %i"
					      "\n", format[i].format);

					/* XXX <rohannessian> DescribePixelFormat may fail on 
					 * extended pixel format (WGL_ARB_p_f)
					 */
					if (!DescribePixelFormat(testdc, format[i].format,
										sizeof *pfd, pfd)) {
						TRACE(PREFIX_E "Cannot describe this pixel format\n");
						ReleaseDC(testwnd, testdc);
						DestroyWindow(testwnd);
						testdc = NULL;
						testwnd = NULL;
						continue;
					}

					ReleaseDC(testwnd, testdc);
					DestroyWindow(testwnd);

					result = format[i].format;
					
					free(format);
					return result;
				}
				else {
					wglMakeCurrent(NULL, NULL);
					wglDeleteContext(rc);
					rc = NULL;
					log_win32_warning("select_pixel_format",
		    	            "Couldn't make the temporary render context "
					        "current for the this pixel format.",
				            GetLastError());
				}
			}
		}
		else {
			log_win32_note("select_pixel_format",
		                "Unable to set pixel format!", GetLastError());
		}
		
		ReleaseDC(testwnd, testdc);
		DestroyWindow(testwnd);
		testdc = NULL;
		testwnd = NULL;
	}

	TRACE(PREFIX_E "select_pixel_format(): All modes have failed...\n");
bail:
	if (format) {
		free(format);
	}
	if (new_pf_code) {
		wglMakeCurrent(NULL, NULL);
		if (testrc) {
			wglDeleteContext(testrc);
		}
	}
	if (testwnd) {
		ReleaseDC(testwnd, testdc);
		DestroyWindow(testwnd);
	}
	
 	return 0;
}



static void allegrogl_init_window(int w, int h, DWORD style, DWORD exstyle)
{
	RECT rect;

#define req __allegro_gl_required_settings
#define sug __allegro_gl_suggested_settings

	int x = 32, y = 32;
	
	if (req & AGL_WINDOW_X || sug & AGL_WINDOW_X)
		x = allegro_gl_display_info.x;
	if (req & AGL_WINDOW_Y || sug & AGL_WINDOW_Y)
		y = allegro_gl_display_info.y;

#undef req
#undef sug
	
	if (!fullscreen) {
		rect.left = x;
		rect.right = x + w;
		rect.top = y;
		rect.bottom = y + h;
	}
	else {
		rect.left = 0;
		rect.right = w;
		rect.top  = 0;
		rect.bottom = h;
	}

	/* save original Allegro styles */
	style_saved = GetWindowLong(wnd, GWL_STYLE);
	exstyle_saved = GetWindowLong(wnd, GWL_EXSTYLE);

	/* set custom AllegroGL style */
	SetWindowLong(wnd, GWL_STYLE, style);
	SetWindowLong(wnd, GWL_EXSTYLE, exstyle);

	if (!fullscreen) {
		AdjustWindowRectEx(&rect, style, FALSE, exstyle);
	}

	/* make the changes visible */
	SetWindowPos(wnd, 0, rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		SWP_NOZORDER | SWP_FRAMECHANGED);
	
	return;
}



static BITMAP *allegro_gl_create_screen (GFX_DRIVER *drv, int w, int h,
                                         int depth)
{
	BITMAP *bmp;
	int is_linear = drv->linear;

	drv->linear = 1;
	bmp = _make_bitmap (w, h, 0, drv, depth, 0);
	
	if (!bmp) {
		return NULL;
	}

	bmp->id = BMP_ID_VIDEO | 1000;
	drv->linear = is_linear;

	drv->w = w;
	drv->h = h;

	return bmp;
}


static LRESULT CALLBACK dummy_wnd_proc(HWND wnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(wnd, message, wparam, lparam);
}

static HWND dummy_wnd;

static void dummy_window(void)
{
	WNDCLASS wnd_class;

	wnd_class.style = CS_HREDRAW | CS_VREDRAW;
	wnd_class.lpfnWndProc = dummy_wnd_proc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = GetModuleHandle(NULL);
	wnd_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd_class.hbrBackground = NULL;
	wnd_class.lpszMenuName = NULL;
	wnd_class.lpszClassName = "allegro focus";

	RegisterClass(&wnd_class);

	dummy_wnd = CreateWindow("allegro focus", "Allegro", WS_POPUP | WS_VISIBLE,
		    0, 0, 200, 200,
		    NULL, NULL, GetModuleHandle(NULL), NULL);

	ShowWindow(dummy_wnd, SW_SHOWNORMAL);
	SetForegroundWindow(dummy_wnd);
}

static void remove_dummy_window(void)
{
	DestroyWindow(dummy_wnd);
	UnregisterClass("allegro focus", GetModuleHandle(NULL));
}


static BITMAP *allegro_gl_win_init(int w, int h, int v_w, int v_h)
{
	static int first_time = 1;
	
	DWORD style=0, exstyle=0;
	int refresh_rate = _refresh_rate_request;
	int desktop_depth;
	int pf=0;

	new_w = w;
	new_h = h;

	/* virtual screen are not supported */
	if ((v_w != 0 && v_w != w) || (v_h != 0 && v_h != h)) {
		TRACE(PREFIX_E "win_init(): Virtual screens are not supported in "
		      "AllegroGL!\n");
		return NULL;
	}
		
	/* Fill in missing color depth info */
	__allegro_gl_fill_in_info();

	/* Be sure the current desktop color depth is at least 15bpp */
	/* We may want to change this, so try to set a better depth, or
	   to at least report an error somehow */
	desktop_depth = desktop_color_depth();

	if (desktop_depth < 15)
		return NULL;

	TRACE(PREFIX_I "win_init(): Requested color depth: %i  "
	      "Desktop color depth: %i\n", allegro_gl_display_info.colour_depth,
	      desktop_depth);

        /* In the moment the main window is destroyed, Allegro loses focus, and
         * focus can only be returned by actual user input under Windows XP. So
         * we need to create a dummy window which retains focus for us, until
         * the new window is up.
         */
        if (fullscreen) dummy_window();

	/* Need to set the w and h driver members at this point to avoid assertion
	 * failure in set_mouse_range() when win_set_window() is called.
	 */
	if (fullscreen) {
		gfx_allegro_gl_fullscreen.w = w;
		gfx_allegro_gl_fullscreen.h = h;
	}
	else {
		gfx_allegro_gl_windowed.w = w;
		gfx_allegro_gl_windowed.h = h;
	}

	/* request a fresh new window from Allegro... */
	/* Set a NULL window to get Allegro to generate a new HWND. This is needed
	 * because we can only set the pixel format once per window. Thus, calling
	 * set_gfx_mode() multiple times will fail without this code.
	 */
	if (!first_time) {
		win_set_window(NULL);
	}
	first_time = 0;

	/* ...and retrieve its handle */
	wnd = win_get_window();
	if (!wnd)
		return NULL;

	/* set up the AllegroGL window */
	if (fullscreen) {
		style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		exstyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
	}
	else {
		style = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN
		      | WS_CLIPSIBLINGS;
		exstyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	}

	TRACE(PREFIX_I "win_init(): Setting up window.\n");
	allegrogl_init_window(w, h, style, exstyle);

	__allegro_gl_hdc = GetDC(wnd); /* get the device context of our window */
	if (!__allegro_gl_hdc) {
		goto Error;
	}

	TRACE(PREFIX_I "win_init(): Driver selected fullscreen: %s\n",
	      fullscreen ? "Yes" : "No");

	if (fullscreen)
	{
		DEVMODE dm;
		DEVMODE fallback_dm;
		int fallback_dm_valid = 0;

		int bpp_to_check[] = {16, 32, 24, 15, 0};
		int bpp_checked[] = {0, 0, 0, 0, 0};
		int bpp_index = 0;
		int i, j, result, modeswitch, done = 0;

		for (j = 0; j < 4; j++)
		{
			if (bpp_to_check[j] == allegro_gl_get(AGL_COLOR_DEPTH))
			{
				bpp_index = j;
				break;
			}
		}

		dm.dmSize = sizeof(DEVMODE);
		dm_saved.dmSize = sizeof(DEVMODE);
		
		/* Save old mode */
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm_saved);
		dm.dmBitsPerPel = desktop_depth; /* Go around Win95's bug */

		do
		{
			if (!bpp_to_check[bpp_index])
			{
				TRACE(PREFIX_E "win_init(): No more color depths to test.\n"
				      "\tUnable to find appropriate full screen mode and pixel "
				      "format.\n");
				goto Error;
			}

			TRACE(PREFIX_I "win_init(): Testing color depth: %i\n",
			      bpp_to_check[bpp_index]);
			
			memset(&dm, 0, sizeof(DEVMODE));
			dm.dmSize = sizeof(DEVMODE);
            
			i = 0;
			do 
			{
				modeswitch = EnumDisplaySettings(NULL, i, &dm);
				if (!modeswitch)
					break;

				if ((dm.dmPelsWidth  == (unsigned) w)
				 && (dm.dmPelsHeight == (unsigned) h)
				 && (dm.dmBitsPerPel == (unsigned) bpp_to_check[bpp_index])
				 && (dm.dmDisplayFrequency != (unsigned) refresh_rate)) {
					/* Keep it as fallback if refresh rate request could not
					 * be satisfied. Try to get as close to 60Hz as possible though,
					 * it's a bit better for a fallback than just blindly picking
					 * something like 47Hz or 200Hz.
					 */
					if (!fallback_dm_valid) {
						fallback_dm = dm;
						fallback_dm_valid = 1;
					}
					else if (dm.dmDisplayFrequency >= 60) {
						if (dm.dmDisplayFrequency < fallback_dm.dmDisplayFrequency) {
							fallback_dm = dm;
						}
					}
				}
		
				i++;
			}
			while ((dm.dmPelsWidth  != (unsigned) w)
			    || (dm.dmPelsHeight != (unsigned) h)
			    || (dm.dmBitsPerPel != (unsigned) bpp_to_check[bpp_index])
			    || (dm.dmDisplayFrequency != (unsigned) refresh_rate));

			if (!modeswitch && !fallback_dm_valid) {
				TRACE(PREFIX_I "win_init(): Unable to set mode, continuing "
				      "with next color depth\n");
			}
			else {
				if (!modeswitch && fallback_dm_valid)
					dm = fallback_dm;

				TRACE(PREFIX_I "win_init(): bpp_to_check[bpp_index] = %i\n",
				      bpp_to_check[bpp_index]);
				TRACE(PREFIX_I "win_init(): dm.dmBitsPerPel = %i\n",
				      (int)dm.dmBitsPerPel);

				dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL
				            | DM_DISPLAYFREQUENCY;

				result = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

				if (result == DISP_CHANGE_SUCCESSFUL) 
				{
					TRACE(PREFIX_I "win_init(): Setting pixel format.\n");
					pf = select_pixel_format(&pfd);
					if (pf) {
						TRACE(PREFIX_I "mode found\n");
						_set_current_refresh_rate(dm.dmDisplayFrequency);
						done = 1;
					}
					else {
						TRACE(PREFIX_I "win_init(): Couldn't find compatible "
						      "GL context. Trying another screen mode.\n");
					}
				}
			}

			fallback_dm_valid = 0;
			bpp_checked[bpp_index] = 1;

			bpp_index = 0;
			while (bpp_checked[bpp_index]) {
				bpp_index++;
			}
		} while (!done);
	}
	else {
		DEVMODE dm;

		memset(&dm, 0, sizeof(DEVMODE));
		dm.dmSize = sizeof(DEVMODE);
		if (EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm) != 0) {
			_set_current_refresh_rate(dm.dmDisplayFrequency);
		}
	}

	if (!fullscreen) {
		TRACE(PREFIX_I "win_init(): Setting pixel format.\n");
		pf = select_pixel_format(&pfd);
		if (pf == 0)
			goto Error;
	}

	/* set the pixel format */
	if (!SetPixelFormat(__allegro_gl_hdc, pf, &pfd)) { 
		log_win32_error("win_init",
	                "Unable to set pixel format.",
		            GetLastError());
		goto Error;
	}

	/* create an OpenGL context */
	allegro_glrc = wglCreateContext(__allegro_gl_hdc);
	
	if (!allegro_glrc) { /* make the context the current one */
		log_win32_error("win_init",
	                "Unable to create a render context!",
		            GetLastError());
		goto Error;
	}
	if (!wglMakeCurrent(__allegro_gl_hdc, allegro_glrc)) {
		log_win32_error("win_init",
	                "Unable to make the context current!",
		            GetLastError());
		goto Error;
	}


	if (__wglGetPixelFormatAttribivARB || __wglGetPixelFormatAttribivEXT) {
		describe_pixel_format_new(__allegro_gl_hdc, pf, desktop_depth,
		                          NULL, NULL, &allegro_gl_display_info);
	}
	else {
		describe_pixel_format_old(__allegro_gl_hdc, pf, desktop_depth,
		                          NULL, NULL, &allegro_gl_display_info);
	}
	
	
	__allegro_gl_set_allegro_image_format(FALSE);
	set_color_depth(allegro_gl_display_info.colour_depth);
	allegro_gl_display_info.w = w;
	allegro_gl_display_info.h = h;

	
	/* <rohannessian> Win98/2k/XP's window forground rules don't let us
	 * make our window the topmost window on launch. This causes issues on 
	 * full-screen apps, as DInput loses input focus on them.
	 * We use this trick to force the window to be topmost, when switching
	 * to full-screen only. Note that this only works for Win98 and greater.
	 * Win95 will ignore our SystemParametersInfo() calls.
	 * 
	 * See http://support.microsoft.com:80/support/kb/articles/Q97/9/25.asp
	 * for details.
	 */
	{
		DWORD lock_time;

#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
		if (fullscreen) {
			SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,
			                     0, (LPVOID)&lock_time, 0);
			SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
			                     0, (LPVOID)0,
			                     SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
		}

		ShowWindow(wnd, SW_SHOWNORMAL);
		SetForegroundWindow(wnd);
		/* In some rare cases, it doesn't seem to work without the loop. And we
		 * absolutely need this to succeed, else we trap the user in a
		 * fullscreen window without input.
		 */
		while (GetForegroundWindow() != wnd) {
			rest(100);
			SetForegroundWindow(wnd);
		}
		UpdateWindow(wnd);

		if (fullscreen) {
			SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
			                     0, (LPVOID)lock_time,
			                     SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
		}
#undef SPI_GETFOREGROUNDLOCKTIMEOUT
#undef SPI_SETFOREGROUNDLOCKTIMEOUT
	}
		
	win_grab_input();
	
	if (fullscreen)	{
		allegro_gl_screen= allegro_gl_create_screen(&gfx_allegro_gl_fullscreen,
		                                w, h, allegro_gl_get(AGL_COLOR_DEPTH));
	}
	else {
		allegro_gl_screen= allegro_gl_create_screen(&gfx_allegro_gl_windowed,
		                                w, h, allegro_gl_get(AGL_COLOR_DEPTH));
	}

	if (!allegro_gl_screen) {
		ChangeDisplaySettings(NULL, 0);
		goto Error;
	}
	

	TRACE(PREFIX_I "win_init(): GLScreen: %ix%ix%i\n",
	      w, h, allegro_gl_get(AGL_COLOR_DEPTH));

	allegro_gl_screen->id |= BMP_ID_VIDEO | BMP_ID_MASK;

	__allegro_gl_valid_context = TRUE;
	__allegro_gl_driver = &allegro_gl_win;
	initialized = 1;

	/* Print out OpenGL version info */
	TRACE(PREFIX_I "OpenGL Version: %s\n", (AL_CONST char*)glGetString(GL_VERSION));
	TRACE(PREFIX_I "Vendor: %s\n", (AL_CONST char*)glGetString(GL_VENDOR));
	TRACE(PREFIX_I "Renderer: %s\n\n", (AL_CONST char*)glGetString(GL_RENDERER));

	/* Detect if the GL driver is based on Mesa */
	allegro_gl_info.is_mesa_driver = FALSE;
	if (strstr((AL_CONST char*)glGetString(GL_VERSION),"Mesa")) {
		AGL_LOG(1, "OpenGL driver based on Mesa\n");
		allegro_gl_info.is_mesa_driver = TRUE;
	}

	/* init the GL extensions */
	__allegro_gl_manage_extensions();
	
	/* Update screen vtable in order to use AGL's */
	__allegro_gl__glvtable_update_vtable(&allegro_gl_screen->vtable);
	memcpy(&_screen_vtable, allegro_gl_screen->vtable, sizeof(GFX_VTABLE));
	allegro_gl_screen->vtable = &_screen_vtable;

	/* Print out WGL extension info */
	if (wglGetExtensionsStringARB) {
		AGL_LOG(1, "WGL Extensions :\n");
#if LOGLEVEL >= 1
		__allegro_gl_print_extensions((AL_CONST char*)wglGetExtensionsStringARB(wglGetCurrentDC()));
#endif
	}
	else {
		TRACE(PREFIX_I "win_init(): No WGL Extensions available\n");
	}

	gfx_capabilities |= GFX_HW_CURSOR;

	/* Initialize a reasonable viewport. Those should be OpenGL defaults,
	 * but some drivers don't implement this correctly.
	 */	
	glViewport(0, 0, SCREEN_W, SCREEN_H);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (allegro_gl_extensions_GL.ARB_multisample) {
		/* Workaround some "special" drivers that do not export the extension
		 * once it was promoted to core.*/
		if (allegro_gl_opengl_version() >= 1.3)
			glSampleCoverage(1.0, GL_FALSE);
		else
			glSampleCoverageARB(1.0, GL_FALSE);
	}
	
	/* Set up some variables that some GL drivers omit */
	glBindTexture(GL_TEXTURE_2D, 0);
	
	screen = allegro_gl_screen;

	if (fullscreen)
		remove_dummy_window();

	return allegro_gl_screen;
	
Error:
	if (allegro_glrc) {
		wglDeleteContext(allegro_glrc);
	}
	if (__allegro_gl_hdc) {
		ReleaseDC(wnd, __allegro_gl_hdc);
	}
	__allegro_gl_hdc = NULL;
	ChangeDisplaySettings(NULL, 0);
	allegro_gl_win_exit(NULL);

	return NULL;
}



static BITMAP *allegro_gl_win_init_windowed(int w, int h, int v_w, int v_h,
											int color_depth)
{
	fullscreen = 0;
	return allegro_gl_win_init(w, h, v_w, v_h);
}



static BITMAP *allegro_gl_win_init_fullscreen(int w, int h, int v_w, int v_h,
											  int color_depth)
{
	fullscreen = 1;
	return allegro_gl_win_init(w, h, v_w, v_h);
}



static void allegro_gl_win_exit(struct BITMAP *b)
{
	/* XXX <rohannessian> For some reason, uncommenting this line will blank
	 * out the log file.
	 */
	//TRACE(PREFIX_I "allegro_gl_win_exit: Shutting down.\n");
	__allegro_gl_unmanage_extensions();
	
	if (allegro_glrc) {
		wglDeleteContext(allegro_glrc);
		allegro_glrc = NULL;
	}
		
	if (__allegro_gl_hdc) {
		ReleaseDC(wnd, __allegro_gl_hdc);
		__allegro_gl_hdc = NULL;
	}

	if (fullscreen && initialized) {
		/* Restore screen */
		ChangeDisplaySettings(NULL, 0);
		_set_current_refresh_rate(0);
 	}
	initialized = 0;

	/* Note: Allegro will destroy screen (== allegro_gl_screen),
	 * so don't destroy it here.
	 */
	//destroy_bitmap(allegro_gl_screen);
	allegro_gl_screen = NULL;
	
	/* hide the window */
	system_driver->restore_console_state();

	/* restore original Allegro styles */
	SetWindowLong(wnd, GWL_STYLE, style_saved);
	SetWindowLong(wnd, GWL_EXSTYLE, exstyle_saved);
	SetWindowPos(wnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
	                               | SWP_FRAMECHANGED);

	__allegro_gl_valid_context = FALSE;
	
	return;
}


/* 
   Returns TRUE is dm doesn't match any mode in mode_list, FALSE otherwise.
*/
static int is_mode_entry_unique(GFX_MODE_LIST *mode_list, DEVMODE *dm) {
	int i;
	
	for (i = 0; i < mode_list->num_modes; ++i) {
		if (mode_list->mode[i].width == (int)dm->dmPelsWidth
			&& mode_list->mode[i].height == (int)dm->dmPelsHeight
			&& mode_list->mode[i].bpp == (int)dm->dmBitsPerPel)
			return FALSE;
	}
	
	return TRUE;
}



/* Returns a list of valid video modes */
static GFX_MODE_LIST* allegro_gl_win_fetch_mode_list(void)
{
	int c, modes_count;
	GFX_MODE_LIST *mode_list;
	DEVMODE dm;

	dm.dmSize = sizeof(DEVMODE);

	/* Allocate space for mode list. */
	mode_list = malloc(sizeof(GFX_MODE_LIST));
	if (!mode_list) {
		return NULL;
	}

	/* Allocate and fill the first mode in case EnumDisplaySettings fails at
	 * first call.
	 */
	mode_list->mode = malloc(sizeof(GFX_MODE));
	if (!mode_list->mode) {
		free(mode_list);
		return NULL;
	}
	mode_list->mode[0].width = 0;
	mode_list->mode[0].height = 0;
	mode_list->mode[0].bpp = 0;
	mode_list->num_modes = 0;

	modes_count = 0;
	c = 0;
	while (EnumDisplaySettings(NULL, c, &dm)) {
	   	mode_list->mode = realloc(mode_list->mode,
								sizeof(GFX_MODE) * (modes_count + 2));
		if (!mode_list->mode) {
			free(mode_list);
			return NULL;
		}

		/* Filter modes with bpp lower than 9, and those which are already
		 * in there.
		 */
		if (dm.dmBitsPerPel > 8 && is_mode_entry_unique(mode_list, &dm)) {
			mode_list->mode[modes_count].width = dm.dmPelsWidth;
			mode_list->mode[modes_count].height = dm.dmPelsHeight;
			mode_list->mode[modes_count].bpp = dm.dmBitsPerPel;
			++modes_count;
			mode_list->mode[modes_count].width = 0;
			mode_list->mode[modes_count].height = 0;
			mode_list->mode[modes_count].bpp = 0;
			mode_list->num_modes = modes_count;
 		}
		++c;
	};

	return mode_list;
}




/* AllegroGL driver routines */

static void flip(void)
{
	SwapBuffers(__allegro_gl_hdc);
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

static struct allegro_gl_driver allegro_gl_win = {
	flip, gl_on, gl_off, NULL
};

