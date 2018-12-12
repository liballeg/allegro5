/** \file alleggl.h
  * \brief Main header file for AllegroGL
  */

#ifndef _allegrogl_included_alleggl_h
#define _allegrogl_included_alleggl_h

#include <allegro.h>

#ifdef ALLEGRO_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN_defined
#endif /* WIN32_LEAN_AND_MEAN */

#ifdef ALLEGRO_DMC
typedef int32_t INT32;
typedef __int64 INT64;
#endif

#include <winalleg.h>

#ifdef WIN32_LEAN_AND_MEAN_defined
#undef WIN32_LEAN_AND_MEAN_defined
#undef WIN32_LEAN_AND_MEAN
#endif /* WIN32_LEAN_AND_MEAN_defined */

#endif /* ALLEGRO_WINDOWS */


#if (defined ALLEGRO_GL_DYNAMIC) && (defined ALLEGRO_WINDOWS)
	#ifdef ALLEGRO_GL_SRC_BUILD
		#define _AGL_DLL __declspec(dllexport)
	#else
		#define _AGL_DLL __declspec(dllimport)
	#endif /* ALLEGRO_GL_SRC_BUILD */
#else
	#define _AGL_DLL
#endif /* (defined ALLEGRO_GL_DYNAMIC) && (defined ALLEGRO_WINDOWS) */

#define AGL_VAR(type, name) extern _AGL_DLL type name

#if (defined ALLEGRO_GL_DYNAMIC) && (defined ALLEGRO_WINDOWS)
	#define AGL_FUNC(type, name, args) extern _AGL_DLL type __cdecl name args
#else
	#define AGL_FUNC(type, name, args) extern type name args
#endif /* (defined ALLEGRO_GL_DYNAMIC) && (defined ALLEGRO_WINDOWS) */


#ifdef ALLEGRO_MACOSX

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#else /* ALLEGRO_MACOSX */

/* HACK: Prevent both Mesa and SGI's broken headers from screwing us */
#define __glext_h_
#define __gl_glext_h_
#define __glxext_h_
#define __glx_glxext_h_
#include <GL/gl.h>
#undef  __glext_h_
#undef  __gl_glext_h_
#undef  __glxext_h_
#undef  __glx_glxext_h_

#endif /* ALLEGRO_MACOSX */


#include "allegrogl/gl_ext.h"

#ifdef ALLEGRO_WITH_XWINDOWS
#if (ALLEGRO_SUB_VERSION == 2) && (ALLEGRO_WIP_VERSION < 2)
#   ifndef HAVE_LIBPTHREAD
#      error AllegroGL requires Allegro to have pthread support enabled!
#   endif
#else
#   ifndef ALLEGRO_HAVE_LIBPTHREAD
#      error AllegroGL requires Allegro to have pthread support enabled!
#   endif
#endif
#include "allegrogl/alleggl_config.h"
#endif


/** \defgroup version Version information
  * \{ */
/** \name Version Information
  * \{ */
#define AGL_VERSION     0            ///< Major version number
#define AGL_SUB_VERSION 4            ///< Minor version number
#define AGL_WIP_VERSION 4            ///< Work-In-Progress version number
#define AGL_VERSION_STR "0.4.4"      ///< Version description string
/** \} */
/** \} */

/* Version Check */
#if (ALLEGRO_VERSION < 4 || (ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION < 2))
	#error AllegroGL requires Allegro 4.2.0 or later to compile!
#endif
#ifndef GL_VERSION_1_1
	#error AllegroGL requires OpenGL 1.1 libraries or later to compile!
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef ALLEGRO_WINDOWS
	
/* Missing #defines from Mingw */
#ifndef PFD_SWAP_LAYER_BUFFERS
#define PFD_SWAP_LAYER_BUFFERS  0x00000800
#endif

#ifndef PFD_GENERIC_ACCELERATED
#define PFD_GENERIC_ACCELERATED 0x00001000
#endif

#ifndef PFD_SUPPORT_DIRECTDRAW
#define PFD_SUPPORT_DIRECTDRAW  0x00002000
#endif

#ifndef CDS_FULLSCREEN
#define CDS_FULLSCREEN      0x00000004
#endif

#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#endif

#endif


#define AGL_ERROR_SIZE 256
AGL_VAR(char, allegro_gl_error[AGL_ERROR_SIZE]);


/** \defgroup core Core routines
 *  Basic AllegroGL functions. These functions allow you to initialize
 *  AllegroGL, set up a rendering context via set_gfx_mode(), and allow
 *  access to regular OpenGL functions.
 *
 */
/** \name Core Functions
 *  \{
 */
AGL_FUNC(int, install_allegro_gl, (void));
AGL_FUNC(void, remove_allegro_gl, (void));

AGL_FUNC(void, allegro_gl_flip, (void));
AGL_FUNC(float, allegro_gl_opengl_version, (void));

/** \ingroup core
 *  Backward compatibility \#define for programs written
 *  prior to AGL 0.0.22.
 *  It isn't defined as anything meaningful, so you don't
 *  need to put them in your program.
 *
 * \see allegro_gl_end()
 */
#define allegro_gl_begin() ;

/** \ingroup core
 *  Backward compatibility \#define for programs written
 *  prior to AGL 0.0.22.
 *  It isn't defined as anything meaningful, so you don't
 *  need to put them in your program.
 *
 * \see allegro_gl_begin()
 */
#define allegro_gl_end() ;

/** \} */


/** \addtogroup settings 
 *  \{ */
/** \name Frame Buffer Settings
 *  \{ */

/** Use Allegro-compatible framebuffer
 * \deprecated This define is ignored.
 */
#define AGL_ALLEGRO_FORMAT  0x00000001

/** Select the red depth of the frame buffer. This defines the number of
 *  bits for the red component you'd like. The driver may or may not find
 *  a suitable mode
 */
#define AGL_RED_DEPTH       0x00000002

/** Select the green depth of the frame buffer. This defines the number of
 *  bits for the green component you'd like. The driver may or may not find
 *  a suitable mode
 */
#define AGL_GREEN_DEPTH     0x00000004

/** Select the blue depth of the frame buffer. This defines the number of
 *  bits for the blue component you'd like. The driver may or may not find
 *  a suitable mode
 */
#define AGL_BLUE_DEPTH      0x00000008

/** Select the alpha depth of the frame buffer. This defines the number of
 *  bits for the alpha component you'd like. Not many video cards support
 *  destination alpha, so be aware that the you may not get an alpha depth
 *  at all.
 */
#define AGL_ALPHA_DEPTH     0x00000010

/** Specify the total color depth of the frame buffer. The driver is free to
 *  select any combination of red, green, blue and alpha bits.
 */
#define AGL_COLOR_DEPTH     0x00000020


/** Select the red depth of the accumulator buffer. This defines the number
 *  of bits to use for the red component. The driver may or may not find a
 *  suitable mode. Note that on many video cards, the accumulator buffer is
 *  not accelerated.
 */
#define AGL_ACC_RED_DEPTH   0x00000040

/** Select the green depth of the accumulator buffer. This defines the number
 *  of bits to use for the green component. The driver may or may not find a
 *  suitable mode. Note that on many video cards, the accumulator buffer is
 *  not accelerated.
 */
#define AGL_ACC_GREEN_DEPTH 0x00000080


/** Select the blue depth of the accumulator buffer. This defines the number
 *  of bits to use for the blue component. The driver may or may not find a
 *  suitable mode. Note that on many video cards, the accumulator buffer is
 *  not accelerated.
 */
#define AGL_ACC_BLUE_DEPTH  0x00000100


/** Select the alpha depth of the accumulator buffer. This defines the number
 *  of bits to use for the alpha component. The driver may or may not find a
 *  suitable mode. Note that on many video cards, the accumulator buffer is
 *  not accelerated. Not many cards support destination alpha either.
 */
#define AGL_ACC_ALPHA_DEPTH 0x00000200

/** Creates a backbuffer if set. The buffering mode may be double buffering
  * or page flipping, depending on the driver settings. OpenGL programs cannot
  * chose the buffering mode themselves.
  */
#define AGL_DOUBLEBUFFER    0x00000400

/** Creates seperate left/right buffers for stereo display. Stereo display
 *  is used with special hardware (tipically glasses) for giving the illusion
 *  of depth by drawing the left and right buffers with a slight horizontal
 *  displacement. This makes the display appear to heavily flicker without
 *  the special hardware.
 *  Set to TRUE for enabling stereo viewing.
 */
#define AGL_STEREO          0x00000800


/** Creates additional auxiliary buffers. This allows you to have more than
 *  one rendering context. Few video cards support this feature.
 */
#define AGL_AUX_BUFFERS     0x00001000


/** Select the depth of the z-buffer. The z-buffer is used to properly
 *  display polygons in 3D without recurring to sorting. The higher the
 *  depth of the z-buffer, the more precise it is.
 */
#define AGL_Z_DEPTH         0x00002000


/** Select the depth of the stencil buffer. The stencil buffer is used to
 *  to do per-pixel testing (like the z-buffer), but of an arbitrary
 *  pattern instead of depth. Please see the OpenGL documentation for details.
 *  Newer cards support stenciling in hardware, but older cards (TNT2s,
 *  Voodoos, ATI Rage) do not.
 */
#define AGL_STENCIL_DEPTH   0x00004000

/** Requests a placement of the window to a specified pixel location.
 *  The driver may or may not honnor the request.
 */
#define AGL_WINDOW_X        0x00008000

/** Same as AGL_WINDOW_X, but for the y-axis.
 */
#define AGL_WINDOW_Y        0x00010000

/** Set it if you'd like AllegroGL to pay special attention on whether
 *  hardware acceleration is present or not. Notice however this
 *  isn't a guarentee that the OpenGL operations will be accelerated, but
 *  rather a request that the operations go through the video card's
 *  drivers instead of Microsoft's software renderer. The video card
 *  driver is free to drop back to software mode if it deems it necessary.
 *  This setting has no effect in X.
 */
#define AGL_RENDERMETHOD    0x00020000

/** Set if you'd like a full screen mode. Full screen may or may not be
 *  available on the current platform.
 */
#define AGL_FULLSCREEN      0x00040000

/** Set if you'd like a windowed mode. Windowed modes may or may not be
 *  available on the current platform.
 */
#define AGL_WINDOWED        0x00080000

/** Define AllegroGL's policy relative to video memory usage.
 *  Sometimes AllegroGL needs to create an internal 256x256 texture in order to
 *  perform graphics operations like masked_blit, draw_sprite and so on.
 *  This parameter defines the policy of AllegroGL relative to the management
 *  of this texture.
 *  Several options are available :
 *  - AGL_RELEASE : internal texture is released in order to free video memory.
 *  - AGL_KEEP : internal texture is kept in video memory. This option generally
 *  accelerate graphics operations when allegro_gl_set_allegro_mode() and 
 *  allegro_gl_unset_allegro_mode() are often called.
 *  System with few video memory should use AGL_RELEASE while others should use
 *  AGL_KEEP since it allows the internal texture to be created once.
 *  Default value is AGL_KEEP.
 *  
 */
#define AGL_VIDEO_MEMORY_POLICY		0x00100000

/** Define multisample parameters
 *  Some OpenGL ICDs expose an extension called GL_ARB_multisample which
 *  provides a mechanism to anti-alias all GL primitives: points, lines,
 *  polygons, bitmaps and pixel rectangles.
 *
 *  In order to get an AllegroGL mode which supports multisample, you have to
 *  set both #AGL_SAMPLE_BUFFERS to 1 and #AGL_SAMPLES to the number of desired
 *  samples for multisampling.
 *
 *  Notice however that since this feature relies on several extensions
 *  (GL_ARB_multisample and GLX_ARB_multisample or WGL_ARB_multisample),
 *  it isn't guaranteed that AllegroGL will find a graphics mode that supports
 *  multisample : many not-so-old video cards, like the GeForce 2, do not
 *  support multisampling
 *
 *  Hence, if you want your app to be able to run on most platforms, you
 *  should not require this parameter.
 *
 *  Set this value to 1 to enable multisampling.
 *
 *  \see AGL_SAMPLES
 */
#define AGL_SAMPLE_BUFFERS	0x00200000

/** Define multisample samples
 *  Set this value to the number of samples that can be accepted in the
 *  multisample buffers.
 *
 * \see AGL_SAMPLE_BUFFERS
 */
#define AGL_SAMPLES        0x00400000
/** \} */


/** Floating-point Color buffer.
 */
#define AGL_FLOAT_COLOR    0x00800000

/** Floating-point Depth buffer.
 */
#define AGL_FLOAT_Z        0x01000000



/* XXX <rohannessian> I'm reserving 2 bits here for later expansion. DO NOT USE
 * without consulting me first.
 */
#define AGL_CONFIG_RESRVED 0xA000000


/** \name Mode selection priority options
 *  \{ */
#define AGL_DONTCARE   0 ///< Ignore these settings
#define AGL_SUGGEST   -1 ///< Prefer the assigned values for these settings
#define AGL_REQUIRE   -2 ///< Reject other values for these settings
/** \} */


/** \name Video memory policy options
 *  \{ */
#define AGL_KEEP	1 ///< Keep internal texture in video memory
#define AGL_RELEASE	2 ///< Release video memory occupied by internal texture
/** \} */


/** \name Mode selection functions
 *  \{ */
AGL_FUNC(void, allegro_gl_clear_settings, (void));
AGL_FUNC(void, allegro_gl_set, (int option, int value));
AGL_FUNC(int,  allegro_gl_get, (int option));
AGL_FUNC(void, allegro_gl_save_settings, (void));
AGL_FUNC(void, allegro_gl_load_settings, (void));
/** \} */
/** \} */


/** \defgroup gfxdrv Graphics drivers
  *
  * Use set_gfx_mode to select an OpenGL mode as normal, but using e.g.
  * GFX_OPENGL as the driver.  The virtual width and height are ignored.
  * To set the colour depth, use allegro_gl_set (AGL_COLOR_DEPTH, xxx).
  * However if the color depth is not set by allegro_gl_set(), AllegroGL
  * will refer to the value set by the last call to set_color_depth().
  *
  * Allegro modes are still available. Use of GFX_AUTODETECT or
  * GFX_AUTODETECT_WINDOWED will select Allegro modes, and not OpenGL modes.
  */

#if defined DOXYGEN  /* Is this a documentation scan? */
  /** \ingroup gfxdrv
   * \name Graphics drivers
   * \{
   */
  /** Windowed OpenGL graphics driver for Allegro */
# define GFX_OPENGL_WINDOWED
  /** Fullscreen OpenGL graphics driver for Allegro */
# define GFX_OPENGL_FULLSCREEN
  /** Non-specific OpenGL graphics driver for Allegro */
  #define GFX_OPENGL
  /** \}
   */

#else

#if defined ALLEGROGL_GENERIC_DRIVER
  /* Allegro is able to determine at run-time if windowed or fullscreen modes
     are available */
  #define GFX_OPENGL_WINDOWED               AL_ID('O','G','L','W')
  #define GFX_OPENGL_FULLSCREEN             AL_ID('O','G','L','F')

#else
#if defined _WIN32
  /* Windows always supports fullscreen */
  #define GFX_OPENGL_WINDOWED               AL_ID('O','G','L','W')
  #define GFX_OPENGL_FULLSCREEN             AL_ID('O','G','L','F')

#elif defined ALLEGRO_WITH_XWINDOWS
  /* X always supports fullscreen */
  #define GFX_OPENGL_WINDOWED               AL_ID('O','G','L','W')
  #define GFX_OPENGL_FULLSCREEN             AL_ID('O','G','L','F')
  
#elif defined ALLEGRO_MACOSX
  /* MacOS X always supports fullscreen */
  #define GFX_OPENGL_WINDOWED               AL_ID('O','G','L','W')
  #define GFX_OPENGL_FULLSCREEN             AL_ID('O','G','L','F')

#else
  #warning Unknown or unsupported platform.
#endif
#endif

#define GFX_OPENGL      		    AL_ID('O','G','L','D')

#endif


/* Declare graphics driver objects */
extern GFX_DRIVER gfx_allegro_gl_default;
#ifdef GFX_OPENGL_WINDOWED
extern GFX_DRIVER gfx_allegro_gl_windowed;
#endif
#ifdef GFX_OPENGL_FULLSCREEN
extern GFX_DRIVER gfx_allegro_gl_fullscreen;
#endif


/** \defgroup bitmap Bitmap Routines
 *  AllegroGL provides a function to set color depth for video
 *  bitmaps.
 */
/** \{ */
/** \name Video Bitmap Rountines
 *  \{ */
AGL_FUNC(GLint, allegro_gl_set_video_bitmap_color_depth, (int bpp));
/** \} */
/** \} */


/** \defgroup texture Texture Routines
 *  AllegroGL provides functions to allow using Allegro BITMAP objects to
 *  be used as OpenGL textures. 
 */
/** \{ */
/** \name Texture routines
 *  \{ */
AGL_FUNC(int, allegro_gl_use_mipmapping, (int enable));
AGL_FUNC(int, allegro_gl_use_alpha_channel, (int enable));
AGL_FUNC(int, allegro_gl_flip_texture, (int enable));
AGL_FUNC(int, allegro_gl_check_texture, (BITMAP *bmp));
AGL_FUNC(int, allegro_gl_check_texture_ex, (int flags, BITMAP *bmp,
                                       GLint internal_format));
AGL_FUNC(GLint, allegro_gl_get_texture_format, (BITMAP *bmp));
AGL_FUNC(GLint, allegro_gl_set_texture_format, (GLint format));
AGL_FUNC(GLenum, allegro_gl_get_bitmap_type, (BITMAP *bmp));
AGL_FUNC(GLenum, allegro_gl_get_bitmap_color_format, (BITMAP *bmp));
AGL_FUNC(GLuint, allegro_gl_make_texture, (BITMAP *bmp));
AGL_FUNC(GLuint, allegro_gl_make_masked_texture, (BITMAP *bmp));
AGL_FUNC(GLuint, allegro_gl_make_texture_ex,(int flags, BITMAP *bmp,
                                         GLint internal_format));


/** AllegroGL will generate mipmaps for this texture.
 */
#define AGL_TEXTURE_MIPMAP      0x01

/** Tell AllegroGL that the bitmap had an alpha channel, so it should be
 *  preserved when generating the texture.
 */
#define AGL_TEXTURE_HAS_ALPHA   0x02

/** Flip the texture on the x-axis. OpenGL uses the bottom-left corner of
 *  the texture as (0,0), so if you need your texture to be flipped to make
 *  (0,0) the top-left corner, you need to use this flag.
 */
#define AGL_TEXTURE_FLIP        0x04

/** Generate an alpha channel for this texture, based on the Allegro mask color.
 *  Make sure the target format supports an alpha channel.
 */
#define AGL_TEXTURE_MASKED      0x08

/** Tell AllegroGL to allow rescaling of the bitmap. By default, AllegroGL
 *  will not rescale the bitmap to fit into a texture. You can override this
 *  behavior by using this flag.
 */
#define AGL_TEXTURE_RESCALE     0x10


/** Tell AllegroGL that the specified BITMAP is an 8-bpp alpha-only BITMAP.
 */
#define AGL_TEXTURE_ALPHA_ONLY  0x20

/** \} */
/** \} */



/** \addtogroup allegro Allegro Interfacing
 *
 * \{ */
/** \name Allegro Interfacing routines
 *  \{ */
AGL_FUNC(void, allegro_gl_set_allegro_mode, (void));
AGL_FUNC(void, allegro_gl_unset_allegro_mode, (void));
AGL_FUNC(void, allegro_gl_set_projection, (void));
AGL_FUNC(void, allegro_gl_unset_projection, (void));
/** \} */
/** \} */



/** \defgroup math Math conversion routines
  * Routines to convert between OpenGL and Allegro math types
  */
/** \{ */

/** \name Matrix conversion routines
 *  \{ */

AGL_FUNC(void, allegro_gl_MATRIX_to_GLfloat, (MATRIX *m, GLfloat gl[16]));
AGL_FUNC(void, allegro_gl_MATRIX_to_GLdouble, (MATRIX *m, GLdouble gl[16]));
AGL_FUNC(void, allegro_gl_MATRIX_f_to_GLfloat, (MATRIX_f *m, GLfloat gl[16]));
AGL_FUNC(void, allegro_gl_MATRIX_f_to_GLdouble, (MATRIX_f *m, GLdouble gl[16]));

AGL_FUNC(void, allegro_gl_GLfloat_to_MATRIX, (GLfloat gl[16], MATRIX *m));
AGL_FUNC(void, allegro_gl_GLdouble_to_MATRIX, (GLdouble gl[16], MATRIX *m));
AGL_FUNC(void, allegro_gl_GLfloat_to_MATRIX_f, (GLfloat gl[16], MATRIX_f *m));
AGL_FUNC(void, allegro_gl_GLdouble_to_MATRIX_f, (GLdouble gl[16], MATRIX_f *m));

/** \} */

/** \name Quaternion conversion routines
 *  \{ */
AGL_FUNC(void, allegro_gl_apply_quat, (QUAT *q));
AGL_FUNC(void, allegro_gl_quat_to_glrotatef, (QUAT *q, float *angle,
                                         float *x, float *y, float *z));
AGL_FUNC(void, allegro_gl_quat_to_glrotated, (QUAT *q, double *angle,
                                         double *x, double *y, double *z));
/** \} */
/** \} */


/** \defgroup Text Text drawing and fonts.
 *
 *  AllegroGL now provides mechanisms for converting Allegro FONTs to
 *  a format which is more usable in OpenGL. You can also load system
 *  fonts (such as Arial and Courrier) and draw using those.
 *  Allegro FONTs should still work if you use textout, textprintf, etc
 *  on them. However, converted Allegro FONTs will not work with these
 *  functions. You will have to use AllegroGL functions to work with
 *  AllegroGL FONTs.
 *
 * \{ */

/** \name Text Drawing and Font conversion
 * \{
 */


/* These define the supported font types */
/** Indicates that you don't really care how a font will be converted.
 *  AGL will pick the best format for you.
 */
#define AGL_FONT_TYPE_DONT_CARE     -1

/** Indicates that you want fonts to be converted to a bitmap format.
 *  Bitmaps are used to represent characters with one bit per pixel,
 *  and are usually very slow to draw, as they stall the 3D pipeline.
 *  Note that you can't scale, rotate or place text with a z coordinate
 *  if you use this style.
 *  A display list will be created.
 *
 *  \sa allegro_gl_convert_allegro_font(), allegro_gl_printf()
 */
#define AGL_FONT_TYPE_BITMAP         0

/** Indicates that you want fonts to be converted to an outline format.
 *  Outlined mode is a vector-style font. Characters are represented by a
 *  set of polygons and lines. This style of fonts is fast to draw, and can
 *  be scaled, rotated, etc, since they are just vectors.
 *  A display list will be created.
 *
 *  \deprecated Non-textured fonts will be dropped from AllegroGL.
 *
 *  \sa allegro_gl_convert_allegro_font(), allegro_gl_printf()
 */
#define AGL_FONT_TYPE_OUTLINE        1

/** Indicates that you want fonts to be converted to a texture format.
 *  Each character is represented by a textured quad. The texture is
 *  common for all characters of the same font, and it will automatically be
 *  uploaded to the video card when needed. Drawing text with this
 *  type of font is the fastest since only two triangles are needed per
 *  character. Textured text can also be scaled and rotated.
 *  A display list will be created.
 *
 *  \sa allegro_gl_convert_allegro_font(), allegro_gl_printf()
 */
#define AGL_FONT_TYPE_TEXTURED       2


/* These defines the font styles for system generated fonts */
/** Creates a font with bold characters. System fonts only.
 *
 *  \deprecated Non-textured fonts will be dropped from AllegroGL
 */
#define AGL_FONT_STYLE_BOLD          1

/** Creates a font with black (strong bold) characters. System fonts only. 
 *  \deprecated Non-textured fonts will be dropped from AllegroGL.
 */
#define AGL_FONT_STYLE_BLACK         2

/** Creates a font with italicized characters. System fonts only.
 * \deprecated Non-textured fonts will be dropped from AllegroGL.
 */
#define AGL_FONT_STYLE_ITALIC        4

/** Creates a font with underlined characters. System fonts only.
 * \deprecated Non-textured fonts will be dropped from AllegroGL.
 */
#define AGL_FONT_STYLE_UNDERLINE     8

/** Creates a font with striked out characters. System fonts only.
 * \deprecated Non-textured fonts will be dropped from AllegroGL.
 */
#define AGL_FONT_STYLE_STRIKEOUT    16

/** Creates a font with anti-aliased characters. System fonts only.
 * Anti-aliasing may not be available, and no error will be reported
 * if such is the case.
 * \deprecated Non-textured fonts will be dropped from AllegroGL.
 */
#define AGL_FONT_STYLE_ANTI_ALIASED 32

/** Font generation mode. System fonts only.
 * Indicates that you want outline system fonts to be generated using polygons.
 * \deprecated Non-textured fonts will be dropped from AllegroGL.
 *
 * \sa allegro_gl_load_system_font(), allegro_gl_load_system_font_ex()
 */
#define AGL_FONT_POLYGONS 1


/** Font generation mode. System fonts only.
 * Indicates that you want outline system fonts to be generated using lines.
 * \deprecated Non-textured fonts will be dropped from AllegroGL.
 *
 * \sa allegro_gl_load_system_font(), allegro_gl_load_system_font_ex()
 */
#define AGL_FONT_LINES    2


AGL_FUNC(int, allegro_gl_printf, (AL_CONST FONT *f, float x, float y, float z,
                             int color, AL_CONST char *format, ...));
AGL_FUNC(int, allegro_gl_printf_ex, (AL_CONST FONT *f, float x, float y, float z,
                             AL_CONST char *format, ...));
AGL_FUNC(FONT*, allegro_gl_convert_allegro_font, (FONT *f, int type, float scale));
AGL_FUNC(FONT*, allegro_gl_convert_allegro_font_ex, (FONT *f, int type, float scale,
                                                GLint format));

AGL_FUNC(void, allegro_gl_set_font_generation_mode, (int mode));
AGL_FUNC(FONT*, allegro_gl_load_system_font, (char *name, int style, int w, int h));
AGL_FUNC(FONT*, allegro_gl_load_system_font_ex, (char *name, int type, int style,
                                 int w, int h, float depth, int start, int end));
AGL_FUNC(void, allegro_gl_destroy_font, (FONT *f));
AGL_FUNC(size_t, allegro_gl_list_font_textures, (FONT *f, GLuint *ids, size_t max_num_id));
/** \} */
/** \} */


/** \defgroup extensions OpenGL Extensions
 * Management of the OpenGL extensions mechanism.
 * AllegroGL provides two ways to access OpenGL extensions: It's native
 * extension library and some versatile portable routines.
 *
 * If you want to get more control on extensions or if you want to use an
 * extension that is not supported by AllegroGL then you can use the
 * routines : allegro_gl_is_extension_supported() and
 * allegro_gl_get_proc_address(). They provide a way to determine if an
 * extension is available and to get its address. These routines are available
 * on every platforms that AllegroGL supports
 *
 */
/** \{ */
/** \name OpenGL Extensions Management Functions
 *  \{
 */
#if defined DOXYGEN  /* Is this a documentation scan? */
/** OpenGL extensions handlers helper.
 * Defines a function pointer type. This macro is almost equivalent to a
 * \b typedef. It is intended to hide some platform-specific machinery in
 * order to keep code portable.
 *
 * \sa allegro_gl_get_proc_address()
 */
#define AGL_DEFINE_PROC_TYPE

#else

AGL_FUNC(int, allegro_gl_is_extension_supported, (const char *));
AGL_FUNC(void*, allegro_gl_get_proc_address, (const char *));

#if defined ALLEGRO_WINDOWS
	#define AGL_DEFINE_PROC_TYPE(type, name, args) \
		typedef type (APIENTRY * name) args;
#else
	#define AGL_DEFINE_PROC_TYPE(type, name, args) \
		typedef type (*name) args;
#endif

#endif
/** \} */
/** \} */


/** \defgroup gui Allegro-compatible GUI routines
 *  AllegroGL GUI wrappers.
 *  Due to several specificities of OpenGL, some of the Allegro's GUI routines
 *  can not be used "as is". Indeed they are not designed to natively support
 *  double-buffered graphics mode. Hence AllegroGL provides wrapper routines
 *  of do_dialog, alert and so on...
 *
 *  AllegroGL GUI routines internally call allegro_gl_set_allegro_mode() and its
 *  counterpart allegro_gl_unset_allegro_mode(). So the default drawing mode in
 *  the GUI routines is the "2D Allegro mode" and functions like line() or
 *  rect() can safely be called.
 *
 *  Additionnaly AllegroGL provides a new GUI object d_algl_viewport_proc()
 *  which allows to have a 3D viewport that can safely coexist with other
 *  "classical" 2D GUI objects : no need to call allegro_gl_set_allegro_mode()
 *  or to save the current state of OpenGL.
 */
/** \{ */
AGL_FUNC(int, algl_do_dialog, (DIALOG *dialog, int focus_obj));
AGL_FUNC(int, algl_popup_dialog, (DIALOG *dialog, int focus_obj));
AGL_FUNC(void, algl_draw_mouse, (void));
AGL_FUNC(void, algl_set_mouse_drawer, (void (*user_draw_mouse)(void)));
AGL_FUNC(int, algl_alert, (AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3,
                      AL_CONST char *b1, AL_CONST char *b2, int c1, int c2));
AGL_FUNC(int, algl_alert3, (AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3,
                       AL_CONST char *b1, AL_CONST char *b2, AL_CONST char *b3,
                       int c1, int c2, int c3));
AGL_FUNC(int, d_algl_viewport_proc, (int msg, DIALOG *d, int c));

/** \} */



#ifdef __cplusplus
}
#endif


/* Fixes to MS's (or SGI?) broken GL headers */
#ifdef GL_VERSION_1_1
#ifndef GL_TEXTURE_BINDING_2D

#ifdef GL_TEXTURE_2D_BINDING
#define GL_TEXTURE_BINDING_2D GL_TEXTURE_2D_BINDING
#endif

#else

#ifdef GL_TEXTURE_BINDING_2D
#define GL_TEXTURE_2D_BINDING GL_TEXTURE_BINDING_2D
#endif

#endif

#ifndef GL_TEXTURE_BINDING_2D
#warning "GL_TEXTURE_BINDING_2D or GL_TEXTURE_2D_BINDING isn't defined by your"
#warning "OpenGL headers. Make sure you have a genuine set of headers for"
#warning "OpenGL 1.1 (or greater)"
#endif
#endif

#endif

