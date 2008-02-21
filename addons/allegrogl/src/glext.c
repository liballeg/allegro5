/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file glext.c
  * \brief OpenGL extension management.
  */

#include "alleggl.h"
#include "allglint.h"
#include <string.h>
#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif

#include <allegro/internal/aintern.h>


/* GL extension Structure. Holds the extension pointers for a single context */
#define AGL_API(type, name, args) AGL_##name##_t name;
typedef struct AGL_EXT {
#	include "allegrogl/GLext/gl_ext_api.h"
#ifdef ALLEGRO_WINDOWS
#	include "allegrogl/GLext/wgl_ext_api.h"
#elif defined ALLEGRO_UNIX
#	include "allegrogl/GLext/glx_ext_api.h"
#endif
} AGL_EXT;
#undef AGL_API

#define PREFIX_I                "agl-ext INFO: "
#define PREFIX_W                "agl-ext WARNING: "
#define PREFIX_E                "agl-ext ERROR: "


/* Current driver info */
struct allegro_gl_info allegro_gl_info;



/** \ingroup extensions
 *  List of OpenGL extensions supported by AllegroGL. Each entry of this
 *  structure is an int which is either set to 1, if the corresponding
 *  extension is available on the host system, or 0 otherwise.
 *
 *  Extension names use only the base name. For example, GL_ARB_multitexture
 *  should be refered to by:
 *  <pre>
 *   allegro_gl_extensions_GL.ARB_multitexture
 *  </pre>
 *
 *  \sa allegro_gl_extensions_GLX allegro_gl_extensions_WGL
 */
struct AGL_EXTENSION_LIST_GL allegro_gl_extensions_GL;



/** \ingroup extensions
 * List of GLX extensions supported by AllegroGL.
 *
 *  \sa allegro_gl_extensions_GL allegro_gl_extensions_WGL
 */
#ifdef ALLEGRO_UNIX
struct AGL_EXTENSION_LIST_GLX allegro_gl_extensions_GLX;
#endif



/** \ingroup extensions
 * List of WGL extensions supported by AllegroGL.
 *
 *  \sa allegro_gl_extensions_GL allegro_gl_extensions_GLX
 */
#ifdef ALLEGRO_WINDOWS
struct AGL_EXTENSION_LIST_WGL allegro_gl_extensions_WGL;
#endif



/* Current context */
AGL_EXT *agl_extension_table = NULL;


#ifdef ALLEGROGL_GENERIC_DRIVER
#include "GL/amesa.h"
#endif


#ifdef ALLEGROGL_HAVE_DYNAMIC_LINK
#include <dlfcn.h>

/* Handle for dynamic library libGL.so */
static void* __agl_handle = NULL;
/* Pointer to glXGetProcAddressARB */
typedef void* (*GLXGETPROCADDRESSARBPROC) (const GLubyte*);
static GLXGETPROCADDRESSARBPROC aglXGetProcAddress;
#else
/* Tries static linking */
#ifdef ALLEGROGL_GLXGETPROCADDRESSARB
#define aglXGetProcAddress glXGetProcAddressARB
#else
#define aglXGetProcAddress glXGetProcAddress
#endif
#endif


#ifdef ALLEGRO_MACOSX
#undef TRUE
#undef FALSE
#include <Carbon/Carbon.h>
#undef TRUE
#undef FALSE
#define TRUE  -1
#define FALSE 0

static CFBundleRef opengl_bundle_ref;
#endif



/* Define the GL API pointers */
#define AGL_API(type, name, args) AGL_##name##_t __agl##name = NULL;
#	include "allegrogl/GLext/gl_ext_api.h"
#undef AGL_API
#ifdef ALLEGRO_WINDOWS
#define AGL_API(type, name, args) AGL_##name##_t __awgl##name = NULL;
#	include "allegrogl/GLext/wgl_ext_api.h"
#undef AGL_API
#elif defined ALLEGRO_UNIX
#define AGL_API(type, name, args) AGL_##name##_t __aglX##name = NULL;
#	include "allegrogl/GLext/glx_ext_api.h"
#undef AGL_API
#endif



/* Create the extension table */
AGL_EXT* __allegro_gl_create_extensions() {

	AGL_EXT *ret = malloc(sizeof(AGL_EXT));

	if (!ret) {
		return NULL;
	}

	memset(ret, 0, sizeof(AGL_EXT));

	return ret;
}



/* Load the extension addresses into the table.
 * Should only be done on context creation.
 */
void __allegro_gl_load_extensions(AGL_EXT *ext) {

#ifdef ALLEGRO_MACOSX
	CFStringRef function;
#endif

	if (!ext) {
		return;
	}
#ifdef ALLEGRO_UNIX
	if (!aglXGetProcAddress) {
		return;
	}
#endif

#	ifdef ALLEGRO_WINDOWS
#	define AGL_API(type, name, args) \
		ext->name = (AGL_##name##_t)wglGetProcAddress("gl" #name); \
		if (ext->name) { AGL_LOG(2,"gl" #name " successfully loaded\n"); }
#	include "allegrogl/GLext/gl_ext_api.h"
#	undef AGL_API
#	define AGL_API(type, name, args) \
		ext->name = (AGL_##name##_t)wglGetProcAddress("wgl" #name); \
		if (ext->name) { AGL_LOG(2,"wgl" #name " successfully loaded\n"); }
#	include "allegrogl/GLext/wgl_ext_api.h"
#	undef AGL_API
#	elif defined ALLEGRO_UNIX
#	define AGL_API(type, name, args) \
		ext->name = (AGL_##name##_t)aglXGetProcAddress((const GLubyte*)"gl" #name); \
		if (ext->name) { AGL_LOG(2,"gl" #name " successfully loaded\n"); }
#	include "allegrogl/GLext/gl_ext_api.h"
#	undef AGL_API
#	define AGL_API(type, name, args) \
		ext->name = (AGL_##name##_t)aglXGetProcAddress((const GLubyte*)"glX" #name); \
		if (ext->name) { AGL_LOG(2,"glX" #name " successfully loaded\n"); }
#	include "allegrogl/GLext/glx_ext_api.h"
#	undef AGL_API
#	elif defined ALLEGRO_MACOSX
#	define AGL_API(type, name, args)                                          \
		function = CFStringCreateWithCString(kCFAllocatorDefault, "gl" #name, \
		                                             kCFStringEncodingASCII); \
		if (function) {                                                       \
			ext->name = (AGL_##name##_t)CFBundleGetFunctionPointerForName(    \
			                                 opengl_bundle_ref, function);    \
			CFRelease(function);                                              \
		}                                                                     \
		if (ext->name) { AGL_LOG(2,"gl" #name " successfully loaded\n"); }
#	include "allegrogl/GLext/gl_ext_api.h"
#	undef AGL_API
#	endif
}



/* Set the GL API pointers to the current table 
 * Should only be called on context switches.
 */
void __allegro_gl_set_extensions(AGL_EXT *ext) {

	if (!ext) {
		return;
	}

#define AGL_API(type, name, args) __agl##name = ext->name;
#	include "allegrogl/GLext/gl_ext_api.h"
#undef AGL_API
#ifdef ALLEGRO_WINDOWS
#define AGL_API(type, name, args) __awgl##name = ext->name;
#	include "allegrogl/GLext/wgl_ext_api.h"
#undef AGL_API
#elif defined ALLEGRO_UNIX
#define AGL_API(type, name, args) __aglX##name = ext->name;
#	include "allegrogl/GLext/glx_ext_api.h"
#undef AGL_API
#endif
}



/* Destroys the extension table */
void __allegro_gl_destroy_extensions(AGL_EXT *ext) {

	if (ext) {
		if (ext == agl_extension_table) {
			agl_extension_table = NULL;
		}
		free(ext);
	}
}



/* __allegro_gl_look_for_an_extension:
 * This function has been written by Mark J. Kilgard in one of his
 * tutorials about OpenGL extensions
 */
int __allegro_gl_look_for_an_extension(AL_CONST char *name,
                                                  AL_CONST GLubyte * extensions)
{
	AL_CONST GLubyte *start;
	GLubyte *where, *terminator;

	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(name, ' ');
	if (where || *name == '\0')
		return FALSE;
	/* It takes a bit of care to be fool-proof about parsing the
	 * OpenGL extensions string. Don't be fooled by sub-strings, etc.
	 */
	start = extensions;
	for (;;) {
		where = (GLubyte *) strstr((AL_CONST char *) start, name);
		if (!where)
		break;
		terminator = where + strlen(name);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return TRUE;
		start = terminator;
	}
	return FALSE;
}



#ifdef ALLEGRO_WINDOWS
static AGL_GetExtensionsStringARB_t __wglGetExtensionsStringARB = NULL;
static HDC __hdc = NULL;
#elif defined ALLEGRO_UNIX
#include <xalleg.h>
#endif


/* int allegro_gl_is_extension_supported(AL_CONST char *extension) */
/** \ingroup extensions
 *  This function is an helper to determine whether an OpenGL extension is
 *  available or not.
 *
 *  \b Example:
 *    <pre>
 *       int packedpixels =
 *                  allegro_gl_is_extension_supported("GL_EXT_packed_pixels");
 *    </pre>
 *  If \c packedpixels is TRUE then you can safely use the constants related
 *  to the packed pixels extension.
 *
 *  \param extension The name of the extension that is needed
 *  \return TRUE if the extension is available FALSE otherwise.
 */
int allegro_gl_is_extension_supported(AL_CONST char *extension)
{
	int ret;
	
	if (!__allegro_gl_valid_context)
		return FALSE;

	if (!glGetString(GL_EXTENSIONS))
		return FALSE;

	ret = __allegro_gl_look_for_an_extension(extension,
	                                              glGetString(GL_EXTENSIONS));

#ifdef ALLEGRO_WINDOWS
	if (!ret && strncmp(extension, "WGL", 3) == 0) {
		if (!__wglGetExtensionsStringARB || __hdc != __allegro_gl_hdc) {
			__wglGetExtensionsStringARB = (AGL_GetExtensionsStringARB_t)
			                   wglGetProcAddress("wglGetExtensionsStringARB");
			__hdc = __allegro_gl_hdc;
		}
		if (__wglGetExtensionsStringARB) {
			ret = __allegro_gl_look_for_an_extension(extension,
			           (AL_CONST GLubyte*)__wglGetExtensionsStringARB(__allegro_gl_hdc));
		}
	}
#elif defined ALLEGRO_UNIX
	if (!ret && strncmp(extension, "GLX", 3) == 0) {
		XLOCK();
		ret = __allegro_gl_look_for_an_extension(extension,
		        (const GLubyte*)glXQueryExtensionsString(_xwin.display,
		                                                 _xwin.screen));
		XUNLOCK();
	}
#endif

	return ret;
}



/* void * allegro_gl_get_proc_address(AL_CONST char *name) */
/** \ingroup extensions
 *  Helper to get the address of an OpenGL symbol
 *
 *  \b Example:
 *  How to get the function \c glMultiTexCoord3fARB that comes
 *  with ARB's Multitexture extension :
 *    <pre>
 *  // define the type of the function
 *	AGL_DEFINE_PROC_TYPE(void, MULTI_TEX_FUNC,
 *	                                      (GLenum, GLfloat, GLfloat, GLfloat));
 *  // declare the function pointer
 *	MULTI_TEX_FUNC glMultiTexCoord3fARB;
 *  // get the address of the function
 * 	glMultiTexCoord3fARB = (MULTI_TEX_FUNC) allegro_gl_get_proc_address(
 * 	                                                   "glMultiTexCoord3fARB");
 *    </pre>
 *
 *  If \c glMultiTexCoord3fARB is not \c NULL then it can be used as if it has 
 *  been defined in the OpenGL core library. Note that the use of the
 *  AGL_DEFINE_PROC_TYPE macro is mandatory if you want your program to be
 *  portable.
 *
 *  \param name The name of the symbol you want to link to.
 *  \return A pointer to the symbol if available or NULL otherwise.
 */
void *allegro_gl_get_proc_address(AL_CONST char *name)
{
	void *symbol = NULL;
#ifdef ALLEGRO_MACOSX
	CFStringRef function;
#endif

	if (!__allegro_gl_valid_context)
		return NULL;

#ifdef ALLEGROGL_GENERIC_DRIVER
	/* AMesa provides a function to get a proc address. It does
	 * not emulate dynamic linking of course...
	 */
	symbol = AMesaGetProcAddress(name);

#elif defined ALLEGRO_WINDOWS
	/* For once Windows is the easiest platform to use :)
	 * It provides a standardized way to get a function address
	 * But of course there is a drawback : the symbol is only valid
	 * under the current context :P
	 */
	symbol = wglGetProcAddress(name);
#elif defined ALLEGRO_UNIX
	if (aglXGetProcAddress) {
		/* This is definitely the *good* way on Unix to get a GL proc
		 * address. Unfortunately glXGetProcAddress is an extension
		 * and may not be available on all platforms
		 */
		symbol = aglXGetProcAddress((const GLubyte*)name);
	}
#elif defined ALLEGROGL_HAVE_DYNAMIC_LINK
	else {
		/* Hack if glXGetProcAddress is not available :
		 * we try to find the symbol into libGL.so
		 */
		if (__agl_handle) {
			symbol = dlsym(__agl_handle, name);
		}
	}
#elif defined ALLEGRO_MACOSX
	function = CFStringCreateWithCString(kCFAllocatorDefault, name,
	                                     kCFStringEncodingASCII);
	if (function) {
		symbol = CFBundleGetFunctionPointerForName(opengl_bundle_ref, function);
		CFRelease(function);
	}
#else
	/* DOS does not support dynamic linking. If the function is not
	 * available at build-time then it will not be available at run-time
	 * Therefore we do not need to look for it...
	 */
#endif

	if (!symbol) {

#if defined ALLEGROGL_HAVE_DYNAMIC_LINK
		if (!aglXGetProcAddress) {
			TRACE(PREFIX_W "get_proc_address: libdl::dlsym: %s\n",
			      dlerror());
		}
#endif
		
		TRACE(PREFIX_W "get_proc_address : Unable to load symbol %s\n",
		      name);
	}
	else {
		TRACE(PREFIX_I "get_proc_address : Symbol %s successfully loaded\n",
		      name);
	}
	return symbol;
}



/* Fills in the AllegroGL info struct for blacklisting video cards.
 */
static void __fill_in_info_struct(const GLubyte *rendereru,
                                  struct allegro_gl_info *info) {
	const char *renderer = (const char*)rendereru;
	
	/* Some cards are "special"... */
	if (strstr(renderer, "3Dfx/Voodoo")) {
		info->is_voodoo = 1;
	}
	else if (strstr(renderer, "Matrox G200")) {
		info->is_matrox_g200 = 1;
	}
	else if (strstr(renderer, "RagePRO")) {
		info->is_ati_rage_pro = 1;
	}
	else if (strstr(renderer, "RADEON 7000")) {
		info->is_ati_radeon_7000 = 1;
	}
	else if (strstr(renderer, "Mesa DRI R200")) {
		info->is_ati_r200_chip = 1;
	}

	if ((strncmp(renderer, "3Dfx/Voodoo3 ", 13) == 0)
	 || (strncmp(renderer, "3Dfx/Voodoo2 ", 13) == 0) 
	 || (strncmp(renderer, "3Dfx/Voodoo ", 12) == 0)) {
		info->is_voodoo3_and_under = 1;
	}

	/* Read OpenGL properties */	
	info->version = allegro_gl_opengl_version();

	return;
}



/* __allegro_gl_manage_extensions:
 * This functions fills the __allegro_gl_extensions structure and displays
 * on the log file which extensions are available
 */
void __allegro_gl_manage_extensions(void)
{
	AL_CONST GLubyte *buf;
	int i;

#ifdef ALLEGRO_MACOSX
	CFURLRef bundle_url;
#endif

	/* Print out OpenGL extensions */
#if LOGLEVEL >= 1
	AGL_LOG(1, "OpenGL Extensions:\n");
             __allegro_gl_print_extensions((AL_CONST char*)
            glGetString(GL_EXTENSIONS));
#endif
	
	/* Print out GLU version */
	buf = gluGetString(GLU_VERSION);
	TRACE(PREFIX_I "GLU Version : %s\n", buf);

#ifdef ALLEGROGL_HAVE_DYNAMIC_LINK
	/* Get glXGetProcAddress entry */
	__agl_handle = dlopen("libGL.so", RTLD_LAZY);
	if (__agl_handle) {
		aglXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__agl_handle,
	                                                    "glXGetProcAddressARB");
		if (!aglXGetProcAddress) {
			aglXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__agl_handle,
	                                                       "glXGetProcAddress");
		}
	}
	else {
		TRACE(PREFIX_W "Failed to dlopen libGL.so : %s\n", dlerror());
	}
	TRACE(PREFIX_I "glXGetProcAddress Extension: %s\n",
	                       aglXGetProcAddress ? "Supported" : "Unsupported");
#elif defined ALLEGRO_UNIX
#ifdef ALLEGROGL_GLXGETPROCADDRESSARB
	TRACE(PREFIX_I "glXGetProcAddressARB Extension: supported\n");
#else
	TRACE(PREFIX_I "glXGetProcAddress Extension: supported\n");
#endif
#endif
	
#ifdef ALLEGRO_MACOSX
	bundle_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
	                     CFSTR("/System/Library/Frameworks/OpenGL.framework"),
	                     kCFURLPOSIXPathStyle, true);
	opengl_bundle_ref = CFBundleCreate(kCFAllocatorDefault, bundle_url);
	CFRelease(bundle_url);
#endif

	__fill_in_info_struct(glGetString(GL_RENDERER), &allegro_gl_info);

	/* Load extensions */
	agl_extension_table = __allegro_gl_create_extensions();
	__allegro_gl_load_extensions(agl_extension_table);
	__allegro_gl_set_extensions(agl_extension_table);
	
	for (i = 0; i < 5; i++) {
		__allegro_gl_texture_read_format[i] = -1;
		__allegro_gl_texture_components[i] = GL_RGB;
	}
	__allegro_gl_texture_read_format[3] = GL_UNSIGNED_BYTE;
	__allegro_gl_texture_read_format[4] = GL_UNSIGNED_BYTE;
	__allegro_gl_texture_components[4] = GL_RGBA;

	
	/* Get extension info for the rest of the lib */
#    define AGL_EXT(name, ver) {                               \
		allegro_gl_extensions_GL.name =                        \
		      allegro_gl_is_extension_supported("GL_" #name)   \
		  || (allegro_gl_info.version >= ver && ver > 0);      \
	}
#   include "allegrogl/GLext/gl_ext_list.h"
#   undef AGL_EXT

#ifdef ALLEGRO_UNIX
#    define AGL_EXT(name, ver) {                               \
		allegro_gl_extensions_GLX.name =                       \
		      allegro_gl_is_extension_supported("GLX_" #name)  \
		  || (allegro_gl_info.version >= ver && ver > 0);      \
	}
#   include "allegrogl/GLext/glx_ext_list.h"
#   undef AGL_EXT
#elif defined ALLEGRO_WINDOWS
#    define AGL_EXT(name, ver) {                               \
		allegro_gl_extensions_WGL.name =                       \
		      allegro_gl_is_extension_supported("WGL_" #name)  \
		  || (allegro_gl_info.version >= ver && ver > 0);      \
	}
#   include "allegrogl/GLext/wgl_ext_list.h"
#   undef AGL_EXT
#endif
	
	/* Get number of texture units */
	if (allegro_gl_extensions_GL.ARB_multitexture) {
		glGetIntegerv(GL_MAX_TEXTURE_UNITS,
		              (GLint*)&allegro_gl_info.num_texture_units);
	}
	else {
		allegro_gl_info.num_texture_units = 1;
	}

	/* Get max texture size */
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,
	              (GLint*)&allegro_gl_info.max_texture_size);

	/* Note: Voodoo (even V5) don't seem to correctly support
	 * packed pixel formats. Disabling them for those cards.
	 */
	allegro_gl_extensions_GL.EXT_packed_pixels &= !allegro_gl_info.is_voodoo;

	
	if (allegro_gl_extensions_GL.EXT_packed_pixels) {

		AGL_LOG(1, "Packed Pixels formats available\n");

		/* XXX On NV cards, we want to use BGRA instead of RGBA for speed */
		/* Fills the __allegro_gl_texture_format array */
		__allegro_gl_texture_read_format[0] = GL_UNSIGNED_BYTE_3_3_2;
		__allegro_gl_texture_read_format[1] = GL_UNSIGNED_SHORT_5_5_5_1;
		__allegro_gl_texture_read_format[2] = GL_UNSIGNED_SHORT_5_6_5;
	}

	/* NVidia and ATI cards expose OpenGL 2.0 but often don't accelerate
	 * non-power-of-2 textures. This check is how you verify that NP2
	 * textures are hardware accelerated or not.
	 * We should clobber the NPOT support if it's not accelerated.
	 */
	{	const char *vendor = (const char*)glGetString(GL_VENDOR);
		if (strstr(vendor, "NVIDIA Corporation")) {
			if (!allegro_gl_extensions_GL.NV_fragment_program2
			 || !allegro_gl_extensions_GL.NV_vertex_program3) {
				allegro_gl_extensions_GL.ARB_texture_non_power_of_two = 0;
			}
		}
		else if (strstr(vendor, "ATI Technologies")) {
			if (!strstr((const char*)glGetString(GL_EXTENSIONS),
			            "GL_ARB_texture_non_power_of_two")
			 && allegro_gl_info.version >= 2.0f) {
				allegro_gl_extensions_GL.ARB_texture_non_power_of_two = 0;
			}
		}
	}
}



/* __allegro_gl_print_extensions:
 * Given a string containing extensions (i.e. a NULL terminated string where
 * each extension are separated by a space and which names do not contain any
 * space)
 */
void __allegro_gl_print_extensions(AL_CONST char * extension)
{
        char buf[80];
        char* start;

        while (*extension != '\0') {
                start = buf;
                strncpy(buf, extension, 80);
                while ((*start != ' ') && (*start != '\0')) {
                        extension++;
                        start++;
                }
                *start = '\0';
                extension ++;
                TRACE(PREFIX_I "%s\n", buf);
        }
}



void __allegro_gl_unmanage_extensions() {
	__allegro_gl_destroy_extensions(agl_extension_table);
#ifdef ALLEGRO_MACOSX
	CFRelease(opengl_bundle_ref);
#endif
#ifdef ALLEGROGL_HAVE_DYNAMIC_LINK
	if (__agl_handle) {
		dlclose(__agl_handle);
		__agl_handle = NULL;
	}
#endif
}

