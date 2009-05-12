/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file alleggl.c
  * \brief Allegro-OpenGL interfacing
  *
  * This sorts out having Allegro and the OpenGL library sharing a window, 
  * and generally cooperating. */


#include <string.h>
#include <stdlib.h>

#include "alleggl.h"
#include "allglint.h"

#include <allegro/internal/aintern.h>
#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#define PREFIX_I                "agl INFO: "
#define PREFIX_E                "agl ERROR: "
#define PREFIX_L                "agl LOG: "


/* Structs containing the current driver state */
struct allegro_gl_driver *__allegro_gl_driver = NULL;
struct allegro_gl_display_info allegro_gl_display_info;

/* Settings required/suggested */
int __allegro_gl_required_settings, __allegro_gl_suggested_settings;

/* Valid context state */
int __allegro_gl_valid_context = 0;


/* Operation to enable while blitting. */
int __allegro_gl_blit_operation;


char allegro_gl_error[AGL_ERROR_SIZE] = EMPTY_STRING;

BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats8;
BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats15;
BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats16;
BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats24;
BLIT_BETWEEN_FORMATS_FUNC __blit_between_formats32;



/**
 * \ingroup updatemodes
 * \brief Direct-mode GL `screen' bitmap
 *
 * If you draw to this bitmap (using supported Allegro functions) your
 * calls will be converted into OpenGL equivalents and appear on the 
 * screen (or backbuffer).
 */
BITMAP *allegro_gl_screen;



/* Allegro GFX_DRIVER list handling */
static _DRIVER_INFO our_driver_list[] = {
#ifdef GFX_OPENGL_WINDOWED
	{GFX_OPENGL_WINDOWED, &gfx_allegro_gl_windowed, FALSE},
#endif
#ifdef GFX_OPENGL_FULLSCREEN
	{GFX_OPENGL_FULLSCREEN, &gfx_allegro_gl_fullscreen, FALSE},
#endif
	{GFX_OPENGL, &gfx_allegro_gl_default, FALSE},
	{0, NULL, FALSE}
};



static _DRIVER_INFO *our_gfx_drivers(void)
{
	return our_driver_list;
}



_DRIVER_INFO *(*saved_gfx_drivers) (void) = NULL;



static _DRIVER_INFO *list_saved_gfx_drivers(void)
{
	return _gfx_driver_list;
}



static BITMAP *allegro_gl_default_gfx_init(int w, int h, int vw, int vh, int depth);



GFX_DRIVER gfx_allegro_gl_default =
{
   GFX_OPENGL,
   EMPTY_STRING,
   EMPTY_STRING,
   "AllegroGL Default Driver",
   allegro_gl_default_gfx_init,
   NULL,
   NULL,
   NULL,               //_xwin_vsync,
   NULL,
   NULL, NULL, NULL,
   NULL,        /* create_video_bitmap */
   NULL,
   NULL, NULL,	/* No show/request video bitmaps */
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   NULL,        /* set_blender_mode */
   NULL,        /* No fetch_mode_list */
   0, 0,
   0,
   0, 0,
   0,
   0,
   FALSE        /* Windowed mode */
};



/** \addtogroup settings Option settings
  *
  * OpenGL has many options controlling the way a video mode is selected.
  * AllegroGL has functions to set particular options, and to state whether
  * choices are merely preferences or are essential.
  *
  * Use allegro_gl_set() to set options. All options are integers. The value
  * you set is, by default, ignored; you must tell AllegroGL how important it
  * is.  There are three levels of importance:
  *
  * - #AGL_DONTCARE:  ignore this setting
  * - #AGL_SUGGEST:   this setting is a preference but not essential
  * - #AGL_REQUIRE:   this setting is important, so don't set a mode without it
  *
  * To specify the importance of a setting,
  * set #AGL_REQUIRE, #AGL_SUGGEST, or #AGL_DONTCARE to contain that setting's 
  * flag, e.g.
  *
  * <pre>
  *   allegro_gl_set(#AGL_COLOR_DEPTH, 16);
  *   allegro_gl_set(#AGL_REQUIRE, #AGL_COLOR_DEPTH);
  * </pre>
  *
  * Rather than calling allegro_gl_set once per setting you want to mark as
  * required, you can OR the setting flags together:
  *
  * <pre>
  *   allegro_gl_set(#AGL_REQUIRE, #AGL_COLOR_DEPTH | #AGL_DOUBLEBUFFER);
  * </pre>
  *
  * This has the same effect as marking the two settings separately.
  *
  * After saying that you #AGL_REQUIRE a particular setting, you
  * can still go back and #AGL_DONTCARE or #AGL_SUGGEST it instead -- nothing
  * happens until you try to set a graphics mode.  At that stage, a mode is
  * set (or set_gfx_mode() returns failure, if not all required settings could
  * be satisfied), and you can then retrieve the actual settings used by
  * calling allegro_gl_get().  Note that this is largely untested; some settings
  * may not be retrieved properly.  Please do let us know if you find any!
  */



/* void allegro_gl_clear_settings(void) */
/** \ingroup settings
 *  Clear the option settings
 *  All settings are set to their default values, and marked as
 *  neither suggested not required.  The mode setting routines will
 *  now ignore all of the settings other than those which you
 *  explicitly mark with #AGL_SUGGEST or #AGL_REQUIRE.
 *
 *  \note You should not rely on what the default values actually
 *  are - don't mark settings unless you've also assigned something
 *  to them.
 *
 *  \note Some settings are turned on by default. #AGL_DOUBLEBUFFER,
 *  #AGL_RENDERMETHOD and #AGL_FULLSCREEN are set to #AGL_SUGGEST.
 *
 * \sa allegro_gl_set(), allegro_gl_get()
 */
void allegro_gl_clear_settings(void)
{
	memset(&allegro_gl_display_info, 0, sizeof allegro_gl_display_info);

	__allegro_gl_required_settings = __allegro_gl_suggested_settings = 0;
	
	/* Pick sensible defaults */
	allegro_gl_display_info.fullscreen = 1;
	allegro_gl_display_info.rmethod = 1;
	allegro_gl_display_info.doublebuffered = 1;
	allegro_gl_display_info.vidmem_policy = AGL_KEEP;
	__allegro_gl_suggested_settings =
	                       AGL_FULLSCREEN | AGL_RENDERMETHOD | AGL_DOUBLEBUFFER;
}



/* void allegro_gl_set(int option, int value) */
/** \ingroup settings
 *  Sets a configuration option.
 *  Use this routine to configure the framebuffer, *before*
 *  setting a graphics mode.  Options are integer constants, and all
 *  values are effectively integers.
 *
 *  Three of the options are special. #AGL_SUGGEST and
 *  #AGL_REQUIRE are used to mark which of the other options
 *  are merely suggestions and which are absolute requirements.  If
 *  the OpenGL implementation can't provide a feature which you mark
 *  with #AGL_REQUIRE, the call to set_gfx_mode will fail. 
 *  If you don't mark an option as either suggested or required, that
 *  option will be ignored (#AGL_DONTCARE).
 *  You can OR (|) together the other constants when using one of
 *  these three options to indicate your preferences for several
 *  settings at one time. Selecting an option as one of the suggestion
 *  modes will remove it from the others. For example, if you first set
 *  the color depth to be required, but then decide that you want it to be
 *  suggested instead, then the option will be removed from the required
 *  settings. Setting any option to #AGL_DONTCARE will remove any previous
 *  setting attributed to it, and default values will be used if necessary.
 *
 *  The remaining options are:
 *  <pre>
 *		#AGL_ALLEGRO_FORMAT,
 *		#AGL_RED_DEPTH,
 *		#AGL_GREEN_DEPTH,
 *		#AGL_BLUE_DEPTH,
 *		#AGL_ALPHA_DEPTH,
 *		#AGL_COLOR_DEPTH,
 *		#AGL_ACC_RED_DEPTH,
 *		#AGL_ACC_GREEN_DEPTH,
 *		#AGL_ACC_BLUE_DEPTH,
 *		#AGL_ACC_ALPHA_DEPTH,
 *		#AGL_DOUBLEBUFFER,
 *		#AGL_STEREO,
 *		#AGL_AUX_BUFFERS,
 *		#AGL_Z_DEPTH,
 *		#AGL_STENCIL_DEPTH,
 *		#AGL_WINDOW_X,
 *		#AGL_WINDOW_Y,
 *		#AGL_RENDERMETHOD
 *		#AGL_FULLSCREEN
 *		#AGL_WINDOWED
 *		#AGL_VIDEO_MEMORY_POLICY
 *		#AGL_SAMPLE_BUFFERS
 *		#AGL_SAMPLES
 *		#AGL_FLOAT_COLOR
 *		#AGL_FLOAT_Z
 *   </pre>
 *
 * \param option Selects which option to change.
 * \param value  The new option value.
 * 
 * \b Example:
 *  <pre>
 *    allegro_gl_set(#AGL_COLOR_DEPTH, 32);
 *    allegro_gl_set(#AGL_RENDERMETHOD, 1);
 *    allegro_gl_set(#AGL_REQUIRE, #AGL_COLOR_DEPTH | #AGL_RENDERMETHOD);
 *  </pre>
 *
 * \sa allegro_gl_get(), allegro_gl_clear_settings()
 */
void allegro_gl_set(int option, int value)
{
	switch (option) {
			/* Options stating importance of other options */
		case AGL_REQUIRE:
			__allegro_gl_required_settings |= value;
			__allegro_gl_suggested_settings &= ~value;
			break;
		case AGL_SUGGEST:
			__allegro_gl_suggested_settings |= value;
			__allegro_gl_required_settings &= ~value;
			break;
		case AGL_DONTCARE:
			__allegro_gl_required_settings &= ~value;
			__allegro_gl_suggested_settings &= ~value;
			break;

			/* Options configuring the mode set */
		case AGL_ALLEGRO_FORMAT:
			allegro_gl_display_info.allegro_format = value;
			break;
		case AGL_RED_DEPTH:
			allegro_gl_display_info.pixel_size.rgba.r = value;
			break;
		case AGL_GREEN_DEPTH:
			allegro_gl_display_info.pixel_size.rgba.g = value;
			break;
		case AGL_BLUE_DEPTH:
			allegro_gl_display_info.pixel_size.rgba.b = value;
			break;
		case AGL_ALPHA_DEPTH:
			allegro_gl_display_info.pixel_size.rgba.a = value;
			break;
		case AGL_COLOR_DEPTH:
			switch (value) {
				case 8:
					allegro_gl_set(AGL_RED_DEPTH, 3);
					allegro_gl_set(AGL_GREEN_DEPTH, 3);
					allegro_gl_set(AGL_BLUE_DEPTH, 2);
					allegro_gl_set(AGL_ALPHA_DEPTH, 0);
					break;
				case 15:
					allegro_gl_set(AGL_RED_DEPTH, 5);
					allegro_gl_set(AGL_GREEN_DEPTH, 5);
					allegro_gl_set(AGL_BLUE_DEPTH, 5);
					allegro_gl_set(AGL_ALPHA_DEPTH, 1);
					break;
				case 16:
					allegro_gl_set(AGL_RED_DEPTH, 5);
					allegro_gl_set(AGL_GREEN_DEPTH, 6);
					allegro_gl_set(AGL_BLUE_DEPTH, 5);
					allegro_gl_set(AGL_ALPHA_DEPTH, 0);
					break;
				case 24:
				case 32:
					allegro_gl_set(AGL_RED_DEPTH, 8);
					allegro_gl_set(AGL_GREEN_DEPTH, 8);
					allegro_gl_set(AGL_BLUE_DEPTH, 8);
					allegro_gl_set(AGL_ALPHA_DEPTH, value-24);
					break;
			}
			allegro_gl_display_info.colour_depth = value;
			break;
		case AGL_ACC_RED_DEPTH:
			allegro_gl_display_info.accum_size.rgba.r = value;
			break;
		case AGL_ACC_GREEN_DEPTH:
			allegro_gl_display_info.accum_size.rgba.g = value;
			break;
		case AGL_ACC_BLUE_DEPTH:
			allegro_gl_display_info.accum_size.rgba.b = value;
			break;
		case AGL_ACC_ALPHA_DEPTH:
			allegro_gl_display_info.accum_size.rgba.a = value;
			break;

		case AGL_DOUBLEBUFFER:
			allegro_gl_display_info.doublebuffered = value;
			break;
		case AGL_STEREO:
			allegro_gl_display_info.stereo = value;
			break;
		case AGL_AUX_BUFFERS:
			allegro_gl_display_info.aux_buffers = value;
			break;
		case AGL_Z_DEPTH:
			allegro_gl_display_info.depth_size = value;
			break;
		case AGL_STENCIL_DEPTH:
			allegro_gl_display_info.stencil_size = value;
			break;

		case AGL_WINDOW_X:
			allegro_gl_display_info.x = value;
			break;
		case AGL_WINDOW_Y:
			allegro_gl_display_info.y = value;
			break;

		case AGL_RENDERMETHOD:
			allegro_gl_display_info.rmethod = value;
			break;

		case AGL_FULLSCREEN:
			allegro_gl_display_info.fullscreen = value;
			break;

		case AGL_WINDOWED:
			allegro_gl_display_info.fullscreen = !value;
			break;
		case AGL_VIDEO_MEMORY_POLICY:
			if ((value == AGL_KEEP) || (value == AGL_RELEASE))
				allegro_gl_display_info.vidmem_policy = value;
			break;
		case AGL_SAMPLE_BUFFERS:
			allegro_gl_display_info.sample_buffers = value;
			break;
		case AGL_SAMPLES:
			allegro_gl_display_info.samples = value;
			break;
		case AGL_FLOAT_COLOR:
			allegro_gl_display_info.float_color = value;
			break;
		case AGL_FLOAT_Z:
			allegro_gl_display_info.float_depth = value;
			break;
	}
}



/** \ingroup settings
 *  Reads the setting of a configuration option.
 *
 *  This routine can be used to read back the configuration of
 *  the framebuffer.  You can do this either before setting a graphics
 *  mode to check what configuration you are requesting, or afterwards
 *  to find out what settings were actually used.
 *  For a list of option constants, see documentation for allegro_gl_set().
 *
 * \param option The option to have its value returned.
 *
 * \return The value of the option selected by the parameter,
 *         or -1 if the option is invalid.
 *
 * \sa allegro_gl_set(), allegro_gl_clear_settings()
 */
int allegro_gl_get(int option)
{
	switch (option) {
			/* Options stating importance of other options */
		case AGL_REQUIRE:
			return __allegro_gl_required_settings;
		case AGL_SUGGEST:
			return __allegro_gl_suggested_settings;
		case AGL_DONTCARE:
			return ~0 & ~(__allegro_gl_required_settings |
						  __allegro_gl_suggested_settings);

			/* Options configuring the mode set */
		case AGL_ALLEGRO_FORMAT:
			return allegro_gl_display_info.allegro_format;
		case AGL_RED_DEPTH:
			return allegro_gl_display_info.pixel_size.rgba.r;
		case AGL_GREEN_DEPTH:
			return allegro_gl_display_info.pixel_size.rgba.g;
		case AGL_BLUE_DEPTH:
			return allegro_gl_display_info.pixel_size.rgba.b;
		case AGL_ALPHA_DEPTH:
			return allegro_gl_display_info.pixel_size.rgba.a;
		case AGL_COLOR_DEPTH:
			return allegro_gl_display_info.pixel_size.rgba.r
				+ allegro_gl_display_info.pixel_size.rgba.g
				+ allegro_gl_display_info.pixel_size.rgba.b
				+ allegro_gl_display_info.pixel_size.rgba.a;
		case AGL_ACC_RED_DEPTH:
			return allegro_gl_display_info.accum_size.rgba.r;
		case AGL_ACC_GREEN_DEPTH:
			return allegro_gl_display_info.accum_size.rgba.g;
		case AGL_ACC_BLUE_DEPTH:
			return allegro_gl_display_info.accum_size.rgba.b;
		case AGL_ACC_ALPHA_DEPTH:
			return allegro_gl_display_info.accum_size.rgba.a;
		case AGL_DOUBLEBUFFER:
			return allegro_gl_display_info.doublebuffered;
		case AGL_STEREO:
			return allegro_gl_display_info.stereo;
		case AGL_AUX_BUFFERS:
			return allegro_gl_display_info.aux_buffers;
		case AGL_Z_DEPTH:
			return allegro_gl_display_info.depth_size;
		case AGL_STENCIL_DEPTH:
			return allegro_gl_display_info.stencil_size;
		case AGL_WINDOW_X:
			return allegro_gl_display_info.x;
		case AGL_WINDOW_Y:
			return allegro_gl_display_info.y;
		case AGL_FULLSCREEN:
			return allegro_gl_display_info.fullscreen;
		case AGL_WINDOWED:
			return !allegro_gl_display_info.fullscreen;
		case AGL_VIDEO_MEMORY_POLICY:
			return allegro_gl_display_info.vidmem_policy;
		case AGL_SAMPLE_BUFFERS:
			return allegro_gl_display_info.sample_buffers;
		case AGL_SAMPLES:
			return allegro_gl_display_info.samples;
		case AGL_FLOAT_COLOR:
			return allegro_gl_display_info.float_color;
		case AGL_FLOAT_Z:
			return allegro_gl_display_info.float_depth;
	}
	return -1;
}



/* Builds a string corresponding to the options set in 'opt'
 * and writes in the config file
 */
static void build_settings(int opt, char *section, char *name) {
	char buf[2048];

	usetc(buf, 0);

	if (opt & AGL_ALLEGRO_FORMAT)
		ustrcat(buf, "allegro_format ");
	if (opt & AGL_RED_DEPTH)
		ustrcat(buf, "red_depth ");
	if (opt & AGL_GREEN_DEPTH)
		ustrcat(buf, "green_depth ");
	if (opt & AGL_BLUE_DEPTH)
		ustrcat(buf, "blue_depth ");
	if (opt & AGL_ALPHA_DEPTH)
		ustrcat(buf, "alpha_depth ");
	if (opt & AGL_COLOR_DEPTH)
		ustrcat(buf, "color_depth ");
	if (opt & AGL_ACC_RED_DEPTH)
		ustrcat(buf, "accum_red_depth ");
	if (opt & AGL_ACC_GREEN_DEPTH)
		ustrcat(buf, "accum_green_depth ");
	if (opt & AGL_ACC_BLUE_DEPTH)
		ustrcat(buf, "accum_blue_depth ");
	if (opt & AGL_ACC_ALPHA_DEPTH)
		ustrcat(buf, "accum_alpha_depth ");
	if (opt & AGL_DOUBLEBUFFER)
		ustrcat(buf, "double_buffer ");
	if (opt & AGL_STEREO)
		ustrcat(buf, "stereo_display ");
	if (opt & AGL_AUX_BUFFERS)
		ustrcat(buf, "aux_buffers ");
	if (opt & AGL_Z_DEPTH)
		ustrcat(buf, "z_depth ");
	if (opt & AGL_STENCIL_DEPTH)
		ustrcat(buf, "stencil_depth ");
	if (opt & AGL_WINDOW_X)
		ustrcat(buf, "window_x ");
	if (opt & AGL_WINDOW_Y)
		ustrcat(buf, "window_y ");
	if (opt & AGL_FULLSCREEN)
		ustrcat(buf, "fullscreen ");
	if (opt & AGL_WINDOWED)
		ustrcat(buf, "windowed ");
	if (opt & AGL_VIDEO_MEMORY_POLICY)
		ustrcat(buf, "video_memory_policy ");
	if (opt & AGL_SAMPLE_BUFFERS)
		ustrcat(buf, "sample_buffers ");
	if (opt & AGL_SAMPLES)
		ustrcat(buf, "samples ");
	if (opt & AGL_FLOAT_COLOR)
		ustrcat(buf, "float_color ");
	if (opt & AGL_FLOAT_Z)
		ustrcat(buf, "float_depth ");
		
	set_config_string(section, name, buf);
}



/* void allegro_gl_save_settings() */
/** \ingroup settings
 *  Saves the current settings (as specified by allegro_gl_set()) to
 *  the current config file, in the section [OpenGL].
 *
 *  \sa allegro_gl_load_settings()
 */
void allegro_gl_save_settings() {

	char *section = "OpenGL";
	int save = allegro_gl_get(AGL_REQUIRE) | allegro_gl_get(AGL_SUGGEST);
	
	if (save & AGL_ALLEGRO_FORMAT)
		set_config_int(section, "allegro_format",
			allegro_gl_get(AGL_ALLEGRO_FORMAT));
	if (save & AGL_RED_DEPTH)
		set_config_int(section, "red_depth",
			allegro_gl_get(AGL_RED_DEPTH));
	if (save & AGL_GREEN_DEPTH)
		set_config_int(section, "green_depth",
			allegro_gl_get(AGL_GREEN_DEPTH));
	if (save & AGL_BLUE_DEPTH)
		set_config_int(section, "blue_depth",
			allegro_gl_get(AGL_BLUE_DEPTH));
	if (save & AGL_ALPHA_DEPTH)
		set_config_int(section, "alpha_depth",
			allegro_gl_get(AGL_ALPHA_DEPTH));
	if (save & AGL_COLOR_DEPTH)
		set_config_int(section, "color_depth",
			allegro_gl_get(AGL_COLOR_DEPTH));
	if (save & AGL_ACC_RED_DEPTH)
		set_config_int(section, "accum_red_depth",
			allegro_gl_get(AGL_ACC_RED_DEPTH));
	if (save & AGL_ACC_GREEN_DEPTH)
		set_config_int(section, "accum_green_depth",
			allegro_gl_get(AGL_ACC_GREEN_DEPTH));
	if (save & AGL_ACC_BLUE_DEPTH)
		set_config_int(section, "accum_blue_depth",
			allegro_gl_get(AGL_ACC_BLUE_DEPTH));
	if (save & AGL_ACC_ALPHA_DEPTH)
		set_config_int(section, "accum_alpha_depth",
			allegro_gl_get(AGL_ACC_ALPHA_DEPTH));
	if (save & AGL_DOUBLEBUFFER)
		set_config_int(section, "double_buffer",
			allegro_gl_get(AGL_DOUBLEBUFFER));
	if (save & AGL_STEREO)
		set_config_int(section, "stereo_display",
			allegro_gl_get(AGL_STEREO));
	if (save & AGL_AUX_BUFFERS)
		set_config_int(section, "aux_buffers",
			allegro_gl_get(AGL_AUX_BUFFERS));
	if (save & AGL_Z_DEPTH)
		set_config_int(section, "z_depth",
			allegro_gl_get(AGL_Z_DEPTH));
	if (save & AGL_STENCIL_DEPTH)
		set_config_int(section, "stencil_depth",
			allegro_gl_get(AGL_STENCIL_DEPTH));
	if (save & AGL_WINDOW_X)
		set_config_int(section, "window_x",
			allegro_gl_get(AGL_WINDOW_X));
	if (save & AGL_WINDOW_Y)
		set_config_int(section, "window_y",
			allegro_gl_get(AGL_WINDOW_Y));
	if (save & AGL_FULLSCREEN)
		set_config_int(section, "fullscreen",
			allegro_gl_get(AGL_FULLSCREEN));
	if (save & AGL_WINDOWED)
		set_config_int(section, "windowed",
			allegro_gl_get(AGL_WINDOWED));
	if (save & AGL_VIDEO_MEMORY_POLICY)
		set_config_int(section, "video_memory_policy",
			allegro_gl_get(AGL_VIDEO_MEMORY_POLICY));
	if (save & AGL_SAMPLE_BUFFERS)
		set_config_int(section, "sample_buffers",
		               allegro_gl_get(AGL_SAMPLE_BUFFERS));
	if (save & AGL_SAMPLES)
		set_config_int(section, "samples",
		               allegro_gl_get(AGL_SAMPLES));
	if (save & AGL_FLOAT_COLOR)
		set_config_int(section, "float_color",
		               allegro_gl_get(AGL_FLOAT_COLOR));
	if (save & AGL_FLOAT_Z)
		set_config_int(section, "float_depth",
		               allegro_gl_get(AGL_FLOAT_Z));

	if (save & AGL_REQUIRE)
		build_settings(allegro_gl_get(AGL_REQUIRE), section, "require");
	if (save & AGL_SUGGEST)
		build_settings(allegro_gl_get(AGL_SUGGEST), section, "suggest");
}



/* Parses an input string to read settings */
static void agl_parse_section(int sec, char *section, char *name) {
	const char *end;
	char *buf;
	char *ptr;
	int strsize;
	int opt = 0;

	end = get_config_string(section, name, "");
	strsize = ustrsizez(end);
	
	buf = (char*)malloc(sizeof(char) * strsize);
	
	if (!buf) {
		TRACE(PREFIX_E "parse_section: Ran out of memory "
		      "while trying to allocate %i bytes!",
		      (int)sizeof(char) * strsize);
		return;
	}

	memcpy(buf, end, strsize);
	end = buf + strsize;
	ptr = buf;

	while (ptr < end) {
		char *s = ustrtok_r(ptr, " ;|+", &ptr);
		
		if (!ustrcmp(s, "allegro_format"))
			opt |= AGL_ALLEGRO_FORMAT;
		if (!ustrcmp(s, "red_depth"))
			opt |= AGL_RED_DEPTH;
		if (!ustrcmp(s, "green_depth"))
			opt |= AGL_GREEN_DEPTH;
		if (!ustrcmp(s, "blue_depth"))
			opt |= AGL_BLUE_DEPTH;
		if (!ustrcmp(s, "alpha_depth"))
			opt |= AGL_ALPHA_DEPTH;
		if (!ustrcmp(s, "color_depth"))
			opt |= AGL_COLOR_DEPTH;
		if (!ustrcmp(s, "accum_red_depth"))
			opt |= AGL_ACC_RED_DEPTH;
		if (!ustrcmp(s, "accum_green_depth"))
			opt |= AGL_ACC_GREEN_DEPTH;
		if (!ustrcmp(s, "accum_blue_depth"))
			opt |= AGL_ACC_BLUE_DEPTH;
		if (!ustrcmp(s, "accum_alpha_depth"))
			opt |= AGL_ACC_ALPHA_DEPTH;
		if (!ustrcmp(s, "double_buffer"))
			opt |= AGL_DOUBLEBUFFER;
		if (!ustrcmp(s, "stereo_display"))
			opt |= AGL_STEREO;
		if (!ustrcmp(s, "aux_buffers"))
			opt |= AGL_AUX_BUFFERS;
		if (!ustrcmp(s, "z_depth"))
			opt |= AGL_Z_DEPTH;
		if (!ustrcmp(s, "stencil_depth"))
			opt |= AGL_STENCIL_DEPTH;
		if (!ustrcmp(s, "window_x"))
			opt |= AGL_WINDOW_X;
		if (!ustrcmp(s, "window_y"))
			opt |= AGL_WINDOW_Y;
		if (!ustrcmp(s, "fullscreen"))
			opt |= AGL_FULLSCREEN;
		if (!ustrcmp(s, "windowed"))
			opt |= AGL_WINDOWED;
		if (!ustrcmp(s, "video_memory_policy"))
			opt |= AGL_VIDEO_MEMORY_POLICY;
		if (!ustrcmp(s, "sample_buffers"))
			opt |= AGL_SAMPLE_BUFFERS;
		if (!ustrcmp(s, "samples"))
			opt |= AGL_SAMPLES;
		if (!ustrcmp(s, "float_color"))
			opt |= AGL_FLOAT_COLOR;
		if (!ustrcmp(s, "float_depth"))
			opt |= AGL_FLOAT_Z;
	}
	
	free(buf);
	
	allegro_gl_set(sec, opt);
}



/* void allegro_gl_load_settings() */
/** \ingroup settings
 *
 *  Loads the settings from the current config file, in the
 *  section [OpenGL].
 *  Note that this function will not clear any settings currently
 *  set, but will add them up, as if each of the setting were set
 *  manually.
 *
 *  \sa allegro_gl_save_settings()
 */
void allegro_gl_load_settings() {

	int set;
	char *section = "OpenGL";
	
	set = get_config_int(section, "allegro_format", -1);
	if (set != -1)
		allegro_gl_set(AGL_ALLEGRO_FORMAT, set);
	set = get_config_int(section, "red_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_RED_DEPTH, set);
	set = get_config_int(section, "green_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_GREEN_DEPTH, set);
	set = get_config_int(section, "blue_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_BLUE_DEPTH, set);
	set = get_config_int(section, "alpha_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_ALPHA_DEPTH, set);
	set = get_config_int(section, "color_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_COLOR_DEPTH, set);
	set = get_config_int(section, "accum_red_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_ACC_RED_DEPTH, set);
	set = get_config_int(section, "accum_green_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_ACC_GREEN_DEPTH, set);
	set = get_config_int(section, "accum_blue_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_ACC_BLUE_DEPTH, set);
	set = get_config_int(section, "accum_alpha_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_ACC_ALPHA_DEPTH, set);
	set = get_config_int(section, "double_buffer", -1);
	if (set != -1)
		allegro_gl_set(AGL_DOUBLEBUFFER, set);
	set = get_config_int(section, "stereo_display", -1);
	if (set != -1)
		allegro_gl_set(AGL_STEREO, set);
	set = get_config_int(section, "aux_buffers", -1);
	if (set != -1)
		allegro_gl_set(AGL_AUX_BUFFERS, set);
	set = get_config_int(section, "z_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_Z_DEPTH, set);
	set = get_config_int(section, "stencil_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_STENCIL_DEPTH, set);
	set = get_config_int(section, "window_x", -1);
	if (set != -1)
		allegro_gl_set(AGL_WINDOW_X, set);
	set = get_config_int(section, "window_y", -1);
	if (set != -1)
		allegro_gl_set(AGL_WINDOW_Y, set);
	set = get_config_int(section, "fullscreen", -1);
	if (set != -1)
		allegro_gl_set(AGL_FULLSCREEN, set);
	set = get_config_int(section, "windowed", -1);
	if (set != -1)
		allegro_gl_set(AGL_WINDOWED, set);
	set = get_config_int(section, "video_memory_policy", -1);
	if (set != -1)
		allegro_gl_set(AGL_VIDEO_MEMORY_POLICY, set);
	set = get_config_int(section, "sample_buffers", -1);
	if (set != -1)
		allegro_gl_set(AGL_SAMPLE_BUFFERS, set);
	set = get_config_int(section, "samples", -1);
	if (set != -1)
		allegro_gl_set(AGL_SAMPLES, set);
	set = get_config_int(section, "float_color", -1);
	if (set != -1)
		allegro_gl_set(AGL_FLOAT_COLOR, set);
	set = get_config_int(section, "float_depth", -1);
	if (set != -1)
		allegro_gl_set(AGL_FLOAT_Z, set);
	
	agl_parse_section(AGL_REQUIRE, section, "require");
	agl_parse_section(AGL_SUGGEST, section, "suggest");
}



/* int install_allegro_gl(void) */
/** \ingroup core
 *
 *  Installs the AllegroGL addon to Allegro.
 *  Allegro should already be initialized with allegro_init()
 *  or install_allegro(). 
 *
 *  \return 0 on success; -1 on failure.
 *
 *  \see remove_allegro_gl()
 */
int install_allegro_gl(void)
{
	if (!system_driver)
		return -1;

	if (atexit(remove_allegro_gl))
		return -1;
	
	if (system_driver->gfx_drivers)
		saved_gfx_drivers = system_driver->gfx_drivers;
	else
		saved_gfx_drivers = list_saved_gfx_drivers;
	
	system_driver->gfx_drivers = our_gfx_drivers;

	allegro_gl_clear_settings();

	/* Save and replace old blit_between_formats methods */
#ifdef ALLEGRO_COLOR8
	__blit_between_formats8 = __linear_vtable8.blit_between_formats;
	__linear_vtable8.blit_between_formats =
	                                     allegro_gl_memory_blit_between_formats;
#endif
#ifdef ALLEGRO_COLOR16
	__blit_between_formats15 = __linear_vtable15.blit_between_formats;
	__linear_vtable15.blit_between_formats =
	                                     allegro_gl_memory_blit_between_formats;
	__blit_between_formats16 = __linear_vtable16.blit_between_formats;
	__linear_vtable16.blit_between_formats
	                                   = allegro_gl_memory_blit_between_formats;
#endif
#ifdef ALLEGRO_COLOR24
	__blit_between_formats24 = __linear_vtable24.blit_between_formats;
	__linear_vtable24.blit_between_formats
	                                   = allegro_gl_memory_blit_between_formats;
#endif
#ifdef ALLEGRO_COLOR32
	__blit_between_formats32 = __linear_vtable32.blit_between_formats;
	__linear_vtable32.blit_between_formats
	                                   = allegro_gl_memory_blit_between_formats;
#endif

	usetc(allegro_gl_error, 0);
	
	return 0;
}



/* void remove_allegro_gl(void) */
/** \ingroup core
 *  Removes the AllegroGL addon. You should not call any more OpenGL
 *  or AllegroGL functions after calling this function. Note that
 *  it will be called automatically at program exit, so you don't need
 *  to explicitly do it.
 *
 * \see install_allegro_gl()
 */
void remove_allegro_gl(void)
{
	if ((!system_driver) || (!saved_gfx_drivers))
		return;

	if (saved_gfx_drivers == &list_saved_gfx_drivers)
		system_driver->gfx_drivers = NULL;
	else
		system_driver->gfx_drivers = saved_gfx_drivers;

	/* This function may be called twice (once by a user explicit call
	 * and once again at exit since the function is registered with at_exit)
	 * In order to prevent crashes, 'saved_gfx_drivers' is set to NULL
	 */
	saved_gfx_drivers = NULL;

	/* Restore the blit_between_formats methods */
	#ifdef ALLEGRO_COLOR8
	__linear_vtable8.blit_between_formats = __blit_between_formats8;
	#endif
	#ifdef ALLEGRO_COLOR16
	__linear_vtable15.blit_between_formats = __blit_between_formats15;
	__linear_vtable16.blit_between_formats = __blit_between_formats16;
	#endif
	#ifdef ALLEGRO_COLOR24
	__linear_vtable24.blit_between_formats = __blit_between_formats24;
	#endif
	#ifdef ALLEGRO_COLOR32
	__linear_vtable32.blit_between_formats = __blit_between_formats32;
	#endif
}



/* void allegro_gl_flip(void) */
/** \ingroup core
 *  Flips the front and back framebuffers.
 *
 *  If you chose, or were given, a double buffered OpenGL mode, you have
 *  access to a front buffer, which is visible on the screen, and also a
 *  back buffer, which is not visible.  This routine flips the buffers,
 *  so that the contents of the back buffer is now the contents of the
 *  (visible) front buffer. The contents of the backbuffer is undefined
 *  after the operation.
 * 
 *  Normally in these modes you would do all your drawing to the back
 *  buffer, without the user seeing the image while it's partially drawn,
 *  and then call this function to flip the buffers, allowing the user
 *  to see what you've drawn, now that it's finished, while you proceed to
 *  draw the next frame.
 * 
 *  When drawing to the screen bitmap, you may not be drawing to what user
 *  currently sees on his monitor. It is recommended that you rebuild the
 *  screen every frame, then flip, then draw again.
 * 
 * \sa allegro_gl_set(), AGL_DOUBLEBUFFER
 */
void allegro_gl_flip(void)
{
	__allegro_gl_driver->flip();
}



/* float allegro_gl_opengl_version() */
/** \ingroup core
 *
 *  Returns the OpenGL version number of the client
 *  (the computer the program is running on).
 *  "1.0" is returned as 1.0, "1.2.1" is returned as 1.21,
 *  and "1.2.2" as 1.22, etc.
 *
 *  A valid OpenGL context must exist for this function to work, which
 *  means you may \b not call it before set_gfx_mode(GFX_OPENGL)
 *
 *  \return The OpenGL ICD/MCD version number.
 */
float allegro_gl_opengl_version() {
	
	const char *str;
	
	if (!__allegro_gl_valid_context)
		return 0.0f;
	
	str = (const char*)glGetString(GL_VERSION);

	if ((strncmp(str, "1.0 ", 4) == 0) || (strncmp(str, "1.0.0 ", 6) == 0))
		return 1.0;
	if ((strncmp(str, "1.1 ", 4) == 0) || (strncmp(str, "1.1.0 ", 6) == 0))
		return 1.1;
	if ((strncmp(str, "1.2 ", 4) == 0) || (strncmp(str, "1.2.0 ", 6) == 0))
		return 1.2;
	if ((strncmp(str, "1.2.1 ", 6) == 0))
		return 1.21;
	if ((strncmp(str, "1.2.2 ", 6) == 0))
		return 1.22;
	if ((strncmp(str, "1.3 ", 4) == 0) || (strncmp(str, "1.3.0 ", 6) == 0))
		return 1.3;
	if ((strncmp(str, "1.4 ", 4) == 0) || (strncmp(str, "1.4.0 ", 6) == 0))
		return 1.4;
	if ((strncmp(str, "1.5 ", 4) == 0) || (strncmp(str, "1.5.0 ", 6) == 0))
		return 1.5;
	if ((strncmp(str, "2.0 ", 4) == 0) || (strncmp(str, "2.0.0 ", 6) == 0))
		return 2.0;
	if ((strncmp(str, "2.1 ", 4) == 0) || (strncmp(str, "2.1.0 ", 6) == 0))
		return 2.1;
	if ((strncmp(str, "3.0 ", 4) == 0) || (strncmp(str, "3.0.0 ", 6) == 0))
		return 3.0;

	/* The OpenGL driver does not return a version
	 * number. However it probably supports at least OpenGL 1.0
	 */	
	if (!str) {
		return 1.0;
	}
	
	return atof(str);
}



void __allegro_gl_set_allegro_image_format(int big_endian)
{
	/* Sets up Allegro to use OpenGL formats */
	_rgb_r_shift_15 = 11;
	_rgb_g_shift_15 = 6;
	_rgb_b_shift_15 = 1;

	_rgb_r_shift_16 = 11;
	_rgb_g_shift_16 = 5;
	_rgb_b_shift_16 = 0;

	if (big_endian) {
		_rgb_r_shift_24 = 16;
		_rgb_g_shift_24 = 8;
		_rgb_b_shift_24 = 0;

		_rgb_a_shift_32 = 0;
		_rgb_r_shift_32 = 24;
		_rgb_g_shift_32 = 16;
		_rgb_b_shift_32 = 8;
	}
	else {
		_rgb_r_shift_24 = 0;
		_rgb_g_shift_24 = 8;
		_rgb_b_shift_24 = 16;

		_rgb_r_shift_32 = 0;
		_rgb_g_shift_32 = 8;
		_rgb_b_shift_32 = 16;
		_rgb_a_shift_32 = 24;
	}

	return;
}



/* allegro_gl_default_init:
 *  Sets a graphics mode according to the mode (fullscreen or windowed)
 *  requested by the user. If it fails to set up the mode then it tries
 *  (if available) the other one unless the user has "AGL_REQUIRED" the mode.
 */
static BITMAP *allegro_gl_default_gfx_init(int w, int h, int vw, int vh,
                                                                     int depth)
{
	BITMAP* bmp = NULL;
	
	if (allegro_gl_display_info.fullscreen) {
		TRACE(PREFIX_I "default_gfx_init: Trying to set up fullscreen mode.\n");
		
#ifdef GFX_OPENGL_FULLSCREEN
		/* Looks for fullscreen mode in our_driver_list */
		gfx_driver = &gfx_allegro_gl_fullscreen;

		if (__allegro_gl_required_settings & AGL_FULLSCREEN)
			/* Fullscreen mode required and found */
			return gfx_allegro_gl_fullscreen.init(w, h, vw, vh, depth);
		else
			bmp = gfx_allegro_gl_fullscreen.init(w, h, vw, vh, depth);
		
		if (bmp)
			/* Fullscreen mode found but not required (probably suggested) */
			return bmp;

#endif /*GFX_OPENGL_FULLSCREEN*/

		/* Fullscreen mode not available but not required :
		 * let's try windowed mode :
		 */
		TRACE(PREFIX_I "default_gfx_init: Failed to set up fullscreen mode!\n");
#ifdef GFX_OPENGL_WINDOWED
		TRACE(PREFIX_I "default_gfx_init: Trying windowed mode...\n");
		allegro_gl_display_info.fullscreen = FALSE;
		gfx_driver = &gfx_allegro_gl_windowed;
		return gfx_allegro_gl_windowed.init(w, h, vw, vh, depth);
#else
		return NULL;
#endif /* GFX_OPENGL_WINDOWED */
	}
	else {
		TRACE(PREFIX_I "default_gfx_init: Trying to set up windowed mode...\n");
		
#ifdef GFX_OPENGL_WINDOWED
		/* Looks for windowed mode in our_driver_list */
		gfx_driver = &gfx_allegro_gl_windowed;

		if (__allegro_gl_required_settings & AGL_WINDOWED)
			/* Windowed mode required and found */
			return gfx_allegro_gl_windowed.init(w, h, vw, vh, depth);
		else
			bmp = gfx_allegro_gl_windowed.init(w, h, vw, vh, depth);
		
		if (bmp)
			/* Windowed mode found but not required (probably suggested) */
			return bmp;

#endif /* GFX_OPENGL_WINDOWED */

		/* Windowed mode not available but not required :
		 * let's try fullscreen mode :
		 */
		TRACE(PREFIX_I "default_gfx_init: Failed to set up windowed mode...\n");
#ifdef GFX_OPENGL_FULLSCREEN
		TRACE(PREFIX_I "default_gfx_init: Trying fullscreen mode...\n");
		allegro_gl_display_info.fullscreen = TRUE;
		gfx_driver = &gfx_allegro_gl_fullscreen;
		return gfx_allegro_gl_fullscreen.init(w, h, vw, vh, depth);
#else
		return NULL;
#endif /*GFX_OPENGL_FULLSCREEN*/
	}
}



/* allegro_gl_set_blender_mode (GFX_DRIVER vtable entry):
 * Sets the blending mode. Same implementation to all GFX vtables.
 */
void allegro_gl_set_blender_mode(int mode, int r, int g, int b, int a) {
	__allegro_gl_blit_operation = AGL_OP_BLEND;
	/* These blenders do not need any special extensions. 
	 * We specify only pixel arithmetic here. Blend equation and blend
	 * color (if available) are reset to defualt later.*/
	switch (mode) {
		case blender_mode_none:
			glBlendFunc(GL_ONE, GL_ZERO);
		break;
		case blender_mode_alpha:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
		case blender_mode_invert:
			glLogicOp(GL_COPY_INVERTED);
			__allegro_gl_blit_operation = AGL_OP_LOGIC_OP;
		break;
		case blender_mode_multiply:
			glBlendFunc(GL_DST_COLOR, GL_ZERO);
		break;
	}
	
	if (allegro_gl_opengl_version() >= 1.4 ||
	   (allegro_gl_opengl_version() >= 1.2 &&
		allegro_gl_is_extension_supported("GL_ARB_imaging"))) {
		/* We're running a recent version of OpenGL and everything needed is here. */
		
		glBlendColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

		switch (mode) {
			case blender_mode_none:
				glBlendEquation(GL_FUNC_ADD);
			break;
			case blender_mode_alpha:
				glBlendEquation(GL_FUNC_ADD);
			break;
			case blender_mode_trans:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
			break;
			case blender_mode_add:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
			break;
			case blender_mode_burn:
				glBlendEquation(GL_FUNC_SUBTRACT);
				glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA);
			break;
			case blender_mode_dodge:
				glBlendEquation(GL_FUNC_ADD);
				glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA);
			break;
			case blender_mode_multiply:
				glBlendEquation(GL_FUNC_ADD);
			break;
		}

		return;
	}
	
	/* Check for presence of glBlendColor() and special parameters to
	 * glBlendFunc(). */
	if (allegro_gl_is_extension_supported("GL_EXT_blend_color")) {
		glBlendColorEXT(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

		switch (mode) {
			case blender_mode_trans:
				glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
			break;
			case blender_mode_add:
				glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE);
			break;
			case blender_mode_burn:
				glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA_EXT);
			break;
			case blender_mode_dodge:
				glBlendFunc(GL_ONE, GL_CONSTANT_ALPHA_EXT);
			break;
		}
	}
	else if (mode == blender_mode_trans ||
			 mode == blender_mode_add ||
			 mode == blender_mode_burn ||
			 mode == blender_mode_dodge) {
		/* glBlendColor() is not available and it is needed by the selected
		 * bledner. Bail out.*/
		return;
	}

	/* Check for presence of glBlendEquation(). */
	if (allegro_gl_is_extension_supported("GL_EXT_blend_minmax")) {
		switch (mode) {
			case blender_mode_none:
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			break;
			case blender_mode_alpha:
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			break;
			case blender_mode_trans:
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			break;
			case blender_mode_add:
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			break;
			case blender_mode_dodge:
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			break;
			case blender_mode_multiply:
				glBlendEquationEXT(GL_FUNC_ADD_EXT);
			break;
			case blender_mode_burn:
				if (allegro_gl_is_extension_supported("GL_EXT_blend_subtract")) {
					glBlendEquationEXT(GL_FUNC_SUBTRACT_EXT);
				}
				else {
					/* GL_FUNC_SUBTRACT is not supported and it is needed by the
					 * selected blender. Bail out. */
					return;
				}
			break;
		}
	}
}


#ifdef DEBUGMODE
#ifdef LOGLEVEL

void __allegro_gl_log(int level, const char *str)
{
	if (level <= LOGLEVEL)
		TRACE(PREFIX_L "[%d] %s", level, str);
}

#endif
#endif

