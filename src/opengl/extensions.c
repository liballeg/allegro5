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
 *      OpenGL extensions management subsystem
 *
 *      By Elias Pschernig and Milan Mimica.
 *      Based on AllegroGL extensions management.
 */


#include "allegro5/allegro5.h"
#include "allegro5/display_new.h"
#include "allegro5/opengl/gl_ext.h"
#include "allegro5/internal/aintern_opengl.h"


/* We need some driver specific details not worth of a vtable entry. */
#if defined ALLEGRO_WINDOWS
   #include "../win/wgl.h"
#elif defined ALLEGRO_UNIX
   #include "../xglx/xglx.h"
#elif defined ALLEGRO_MACOSX
   #include "../macosx/osxgl.h"
#endif

#include <string.h>
#ifdef ALLEGRO_MACOSX
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif


#define PREFIX_I                "al-ext INFO: "
#define PREFIX_W                "al-ext WARNING: "
#define PREFIX_E                "al-ext ERROR: "


#ifdef ALLEGRO_HAVE_DYNAMIC_LINK
   #include <dlfcn.h>
   /* Handle for dynamic library libGL.so */
   static void *__libgl_handle = NULL;
   /* Pointer to glXGetProcAddressARB */
   typedef void *(*GLXGETPROCADDRESSARBPROC) (const GLubyte *);
   static GLXGETPROCADDRESSARBPROC alXGetProcAddress;
#else /* #ifdef ALLEGRO_HAVE_DYNAMIC_LINK */
   /* Tries static linking */
   /* FIXME: set ALLEGRO_GLXGETPROCADDRESSARB on configure time, if
    * glXGetProcAddressARB must be used!
    */
   #ifdef ALLEGRO_GLXGETPROCADDRESSARB
      #define alXGetProcAddress glXGetProcAddressARB
   #else
      #define alXGetProcAddress glXGetProcAddress
   #endif
#endif /* #ifdef ALLEGRO_HAVE_DYNAMIC_LINK */


#ifdef ALLEGRO_MACOSX
   #undef TRUE
   #undef FALSE
   #include <CoreFoundation/CoreFoundation.h>
   #undef TRUE
   #undef FALSE
   #define TRUE  -1
   #define FALSE 0
   static CFBundleRef opengl_bundle_ref;
#endif


/* Define the GL API pointers.
 * Example:
 * ALLEGRO_BlendEquation_t __aglBlendEquation = NULL;
 */
#define AGL_API(type, name, args) ALLEGRO_##name##_t __agl##name = NULL;
#	include "allegro5/opengl/GLext/gl_ext_api.h"
#undef AGL_API
#ifdef ALLEGRO_WINDOWS
#define AGL_API(type, name, args) ALLEGRO_##name##_t __awgl##name = NULL;
#	include "allegro5/opengl/GLext/wgl_ext_api.h"
#undef AGL_API
#elif defined ALLEGRO_UNIX
#define AGL_API(type, name, args) ALLEGRO_##name##_t __aglX##name = NULL;
#	include "allegro5/opengl/GLext/glx_ext_api.h"
#undef AGL_API
#endif



/* Reads version info out of glGetString(GL_VERSION) */
static float _al_ogl_version(void)
{
   const char *str;

   str = (const char *)glGetString(GL_VERSION);

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

   /* The OpenGL driver does not return a version
    * number. However it probably supports at least OpenGL 1.0
    */
   if (!str) {
      return 1.0;
   }

   return atof(str);
}



/* Function: al_opengl_version
 *
 *  Returns the OpenGL version number of the client
 *  (the computer the program is running on), for the current DISPLAY.
 *  "1.0" is returned as 1.0, "1.2.1" is returned as 1.21,
 *  and "1.2.2" as 1.22, etc.
 *
 *  A valid OpenGL context must exist for this function to work, which
 *  means you may \b not call it before al_create_display().
 *
 */
float al_opengl_version(void)
{
   ALLEGRO_DISPLAY_OGL *ogl_disp;

   ogl_disp = (ALLEGRO_DISPLAY_OGL*)al_get_current_display();
   if (!ogl_disp)
      return 0.0f;

   return ogl_disp->ogl_info.version;
}



/* Create the extension list */
static ALLEGRO_OGL_EXT_LIST *create_extension_list(void)
{
   ALLEGRO_OGL_EXT_LIST *ret = malloc(sizeof(ALLEGRO_OGL_EXT_LIST));

   if (!ret) {
      return NULL;
   }

   memset(ret, 0, sizeof(ALLEGRO_OGL_EXT_LIST));

   return ret;
}



/* Create the extension API table */
static ALLEGRO_OGL_EXT_API *create_extension_api_table(void)
{
   ALLEGRO_OGL_EXT_API *ret = malloc(sizeof(ALLEGRO_OGL_EXT_API));

   if (!ret) {
      return NULL;
   }

   memset(ret, 0, sizeof(ALLEGRO_OGL_EXT_API));

   return ret;
}



/* Load the extension API addresses into the table.
 * Should only be done on context creation.
 */
static void load_extensions(ALLEGRO_OGL_EXT_API *ext)
{
   if (!ext) {
      return;
   }
#ifdef ALLEGRO_UNIX
#ifdef ALLEGRO_HAVE_DYNAMIC_LINK
   if (!alXGetProcAddress) {
      return;
   }
#endif
#endif

#ifdef ALLEGRO_WINDOWS

   #define AGL_API(type, name, args)                                 \
      ext->name = (ALLEGRO_##name##_t)wglGetProcAddress("gl" #name); \
      if (ext->name) { TRACE("gl" #name " successfully loaded\n"); }

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

   #define AGL_API(type, name, args)                                  \
      ext->name = (ALLEGRO_##name##_t)wglGetProcAddress("wgl" #name); \
      if (ext->name) { TRACE("wgl" #name " successfully loaded\n"); }

      #include "allegro5/opengl/GLext/wgl_ext_api.h"

   #undef AGL_API

#elif defined ALLEGRO_UNIX

   #define AGL_API(type, name, args)                                               \
      ext->name = (ALLEGRO_##name##_t)alXGetProcAddress((const GLubyte*)"gl" #name);  \
      if (ext->name) { TRACE("gl" #name " successfully loaded\n"); }

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

   #define AGL_API(type, name, args)                                               \
      ext->name = (ALLEGRO_##name##_t)alXGetProcAddress((const GLubyte*)"glX" #name); \
      if (ext->name) { TRACE("glX" #name " successfully loaded\n"); }

      #include "allegro5/opengl/GLext/glx_ext_api.h"

   #undef AGL_API

#elif defined ALLEGRO_MACOSX

#define AGL_API(type, name, args)                                                                 \
      ext->name = (ALLEGRO_##name##_t)CFBundleGetFunctionPointerForName(opengl_bundle_ref, CFSTR("gl" # name)); \
      if (ext->name) { TRACE("gl" #name " successfully loaded\n"); }

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

#endif
}



/* Set the GL API pointers to the current table 
 * Should only be called on context switches.
 */
void _al_ogl_set_extensions(ALLEGRO_OGL_EXT_API *ext)
{
   if (!ext) {
      return;
   }

#define AGL_API(type, name, args) __agl##name = ext->name;
#	include "allegro5/opengl/GLext/gl_ext_api.h"
#undef AGL_API

#ifdef ALLEGRO_WINDOWS
#define AGL_API(type, name, args) __awgl##name = ext->name;
#	include "allegro5/opengl/GLext/wgl_ext_api.h"
#undef AGL_API

#elif defined ALLEGRO_UNIX
#define AGL_API(type, name, args) __aglX##name = ext->name;
#	include "allegro5/opengl/GLext/glx_ext_api.h"
#undef AGL_API
#endif
}



/* Destroys the extension API table */
static void destroy_extension_api_table(ALLEGRO_OGL_EXT_API *ext)
{
   if (ext) {
      free(ext);
   }
}



/* Destroys the extension list */
static void destroy_extension_list(ALLEGRO_OGL_EXT_LIST *list)
{
   if (list) {
      free(list);
   }
}



/* _al_ogl_look_for_an_extension:
 * This function has been written by Mark J. Kilgard in one of his
 * tutorials about OpenGL extensions
 */
int _al_ogl_look_for_an_extension(AL_CONST char *name, AL_CONST GLubyte *extensions)
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
      where = (GLubyte *) strstr((AL_CONST char *)start, name);
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



static int _ogl_is_extension_supported(AL_CONST char *extension,
                                       ALLEGRO_DISPLAY_OGL *disp)
{
   int ret;
   
   if (!glGetString(GL_EXTENSIONS))
      return FALSE;

   ret = _al_ogl_look_for_an_extension(extension, glGetString(GL_EXTENSIONS));

#ifdef ALLEGRO_WINDOWS
   if (!ret && strncmp(extension, "WGL", 3) == 0) {
      ALLEGRO_DISPLAY_WGL *wgl_disp = (void*)disp;
      ALLEGRO_GetExtensionsStringARB_t __wglGetExtensionsStringARB;

      if (!wgl_disp->dc)
         return FALSE;

      __wglGetExtensionsStringARB =
         (ALLEGRO_GetExtensionsStringARB_t)wglGetProcAddress("wglGetExtensionsStringARB");
      if (__wglGetExtensionsStringARB) {
         ret = _al_ogl_look_for_an_extension(extension, (const GLubyte *)
                                     __wglGetExtensionsStringARB(wgl_disp->dc));
      }
   }

#elif defined ALLEGRO_UNIX
   if (!ret && strncmp(extension, "GLX", 3) == 0) {
      ALLEGRO_SYSTEM_XGLX *sys = (void*)al_system_driver();
      ALLEGRO_DISPLAY_XGLX *glx_disp = (void*)disp;

      if (!sys->xdisplay || !glx_disp->xscreen)
         return FALSE;

      ret = _al_ogl_look_for_an_extension(extension, (const GLubyte *)
                    glXQueryExtensionsString(sys->xdisplay, glx_disp->xscreen));
   }
#endif

   return ret;
}


/* Function: al_is_opengl_extension_supported
 *  This function is a helper to determine whether an OpenGL extension is
 *  available on the current display or not.
 *
 *  Example:
 *
 *  >int packedpixels = al_is_opengl_extension_supported("GL_EXT_packed_pixels");
 *
 *  If _packedpixels_ is TRUE then you can safely use the constants related
 *  to the packed pixels extension.
 *
 * Parameters:
 * extension - The name of the extension that is needed
 *
 * Returns:
 * TRUE if the extension is available FALSE otherwise.
 */
int al_is_opengl_extension_supported(AL_CONST char *extension)
{
   ALLEGRO_DISPLAY *disp;
   
   disp = al_get_current_display();
   if (!disp)
      return FALSE;

   if (!(disp->flags & ALLEGRO_OPENGL))
      return FALSE;

   return _ogl_is_extension_supported(extension, (ALLEGRO_DISPLAY_OGL*)disp);
}



/* Function: al_get_opengl_proc_address
 *  Helper to get the address of an OpenGL symbol
 *
 *  Example:
 *  How to get the function _glMultiTexCoord3fARB_ that comes
 *  with ARB's Multitexture extension :
 *   
 *  >// define the type of the function
 *  >   ALLEGRO_DEFINE_PROC_TYPE(void, MULTI_TEX_FUNC,
 *  >      (GLenum, GLfloat, GLfloat, GLfloat));
 *  >// declare the function pointer
 *  >   MULTI_TEX_FUNC glMultiTexCoord3fARB;
 *  >// get the address of the function
 *  >   glMultiTexCoord3fARB = (MULTI_TEX_FUNC) al_get_opengl_proc_address(
 *  >      "glMultiTexCoord3fARB");
 *
 *  If _glMultiTexCoord3fARB_ is not NULL then it can be used as if it has 
 *  been defined in the OpenGL core library. Note that the use of the
 *  ALLEGRO_DEFINE_PROC_TYPE macro is mandatory if you want your program to be
 *  portable.
 *
 *  Parameters:
 *  name - The name of the symbol you want to link to.
 *
 *  Returns:
 *  A pointer to the symbol if available or NULL otherwise.
 */
void *al_get_opengl_proc_address(AL_CONST char *name)
{
   void *symbol = NULL;
#ifdef ALLEGRO_MACOSX
   CFStringRef function;
#endif
   ALLEGRO_DISPLAY *disp;
   
   disp = al_get_current_display();
   if (!disp)
      return FALSE;

   if (!(disp->flags & ALLEGRO_OPENGL))
      return FALSE;

#if defined ALLEGRO_WINDOWS
   /* For once Windows is the easiest platform to use :)
    * It provides a standardized way to get a function address
    * But of course there is a drawback : the symbol is only valid
    * under the current context :P
    */
   {
      ALLEGRO_DISPLAY_WGL *wgl_disp = (void*)disp;

      if (!wgl_disp->dc)
         return FALSE;

      symbol = wglGetProcAddress(name);
   }
#elif defined ALLEGRO_UNIX
#if defined ALLEGRO_HAVE_DYNAMIC_LINK
   if (alXGetProcAddress)
#endif
   {
      /* This is definitely the *good* way on Unix to get a GL proc
       * address. Unfortunately glXGetProcAddress is an extension
       * and may not be available on all platforms
       */
      symbol = alXGetProcAddress((const GLubyte *)name);
   }
#elif defined ALLEGRO_HAVE_DYNAMIC_LINK
   else {
      /* Hack if glXGetProcAddress is not available :
       * we try to find the symbol into libGL.so
       */
      if (__al_handle) {
         symbol = dlsym(__al_handle, name);
      }
   }
#elif defined ALLEGRO_MACOSX
   function = CFStringCreateWithCString(kCFAllocatorDefault, name,
                                        kCFStringEncodingASCII);
   if (function) {
      symbol =
          CFBundleGetFunctionPointerForName(opengl_bundle_ref, function);
      CFRelease(function);
   }
#endif

   if (!symbol) {

#if defined ALLEGRO_HAVE_DYNAMIC_LINK
      if (!alXGetProcAddress) {
         TRACE(PREFIX_W "get_proc_address: libdl::dlsym: %s\n", dlerror());
      }
#endif

      TRACE(PREFIX_W "get_proc_address : Unable to load symbol %s\n", name);
   }
   else {
      TRACE(PREFIX_I "get_proc_address : Symbol %s successfully loaded\n", name);
   }
   return symbol;
}



/* fill_in_info_struct:
 *  Fills in the OPENGL_INFO info struct for blacklisting video cards.
 */
static void fill_in_info_struct(const GLubyte *rendereru, OPENGL_INFO *info)
{
   const char *renderer = (const char *)rendereru;
   ASSERT(renderer);

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
   info->version = _al_ogl_version();

   return;
}



/* _al_ogl_manage_extensions:
 * This functions fills the extensions API table and extension list
 * structures and displays on the log file which extensions are available.
 */
void _al_ogl_manage_extensions(ALLEGRO_DISPLAY_OGL *gl_disp)
{
   //AL_CONST GLubyte *buf;
#if defined ALLEGRO_MACOSX
   CFURLRef bundle_url;
#endif
   ALLEGRO_OGL_EXT_API *ext_api;
   ALLEGRO_OGL_EXT_LIST *ext_list;

   /* Print out OpenGL extensions */
#if LOGLEVEL >= 1
   TRACE("OpenGL Extensions:\n");
   print_extensions((AL_CONST char *)glGetString(GL_EXTENSIONS));
#endif

   /* Print out GLU version */
   //buf = gluGetString(GLU_VERSION);
   //TRACE(PREFIX_I "GLU Version : %s\n", buf);

#ifdef ALLEGRO_HAVE_DYNAMIC_LINK
   /* Get glXGetProcAddress entry */
   __libgl_handle = dlopen("libGL.so", RTLD_LAZY);
   if (__libgl_handle) {
      alXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__libgl_handle,
                                                            "glXGetProcAddressARB");
      if (!alXGetProcAddress) {
         alXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__libgl_handle,
                                                             "glXGetProcAddress");
      }
   }
   else {
      TRACE(PREFIX_W "Failed to dlopen libGL.so : %s\n", dlerror());
   }
   TRACE(PREFIX_I "glXGetProcAddress Extension: %s\n",
         alXGetProcAddress ? "Supported" : "Unsupported");
#elif defined ALLEGRO_UNIX
#ifdef ALLEGROGL_GLXGETPROCADDRESSARB
   TRACE(PREFIX_I "glXGetProcAddressARB Extension: supported\n");
#else
   TRACE(PREFIX_I "glXGetProcAddress Extension: supported\n");
#endif
#endif

#ifdef ALLEGRO_MACOSX
   bundle_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                              CFSTR
                                              ("/System/Library/Frameworks/OpenGL.framework"),
                                              kCFURLPOSIXPathStyle, true);
   opengl_bundle_ref = CFBundleCreate(kCFAllocatorDefault, bundle_url);
   CFRelease(bundle_url);
#endif

   fill_in_info_struct(glGetString(GL_RENDERER), &(gl_disp->ogl_info));

   /* Create & load extension API table */
   ext_api = create_extension_api_table();
   load_extensions(ext_api);
   gl_disp->extension_api = ext_api;

   /* Create the list of supported extensions. */
   ext_list = create_extension_list();
   gl_disp->extension_list = ext_list;

   /* Fill the list. */
#define AGL_EXT(name, ver) {                                                             \
		ext_list->ALLEGRO_GL_##name =  _ogl_is_extension_supported("GL_" #name, gl_disp) \
		                      || (gl_disp->ogl_info.version >= ver && ver > 0);          \
	}
   #include "allegro5/opengl/GLext/gl_ext_list.h"
#undef AGL_EXT

#ifdef ALLEGRO_UNIX
#define AGL_EXT(name, ver) {                                                              \
		ext_list->ALLEGRO_GLX_##name = _ogl_is_extension_supported("GLX_" #name, gl_disp) \
		                      || (gl_disp->ogl_info.version >= ver && ver > 0);           \
	}
   #include "allegro5/opengl/GLext/glx_ext_list.h"
#undef AGL_EXT
#elif defined ALLEGRO_WINDOWS
#define AGL_EXT(name, ver) {                                                              \
		ext_list->ALLEGRO_WGL_##name = _ogl_is_extension_supported("WGL_" #name, gl_disp) \
		                      || (gl_disp->ogl_info.version >= ver && ver > 0);           \
	}
   #include "allegro5/opengl/GLext/wgl_ext_list.h"
#undef AGL_EXT
#endif

    /* TODO: use these somewhere */
#if 0
   for (i = 0; i < 5; i++) {
      __allegro_gl_texture_read_format[i] = -1;
      __allegro_gl_texture_components[i] = GL_RGB;
   }
   __allegro_gl_texture_read_format[3] = GL_UNSIGNED_BYTE;
   __allegro_gl_texture_read_format[4] = GL_UNSIGNED_BYTE;
   __allegro_gl_texture_components[4] = GL_RGBA;
#endif /* #if 0 */

   /* Get number of texture units */
   if (ext_list->ALLEGRO_GL_ARB_multitexture) {
      glGetIntegerv(GL_MAX_TEXTURE_UNITS,
                    (GLint *) & gl_disp->ogl_info.num_texture_units);
   }
   else {
      gl_disp->ogl_info.num_texture_units = 1;
   }

   /* Get max texture size */
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,
                 (GLint *) & gl_disp->ogl_info.max_texture_size);

   /* Note: Voodoo (even V5) don't seem to correctly support
    * packed pixel formats. Disabling them for those cards.
    */
   ext_list->ALLEGRO_GL_EXT_packed_pixels &= !gl_disp->ogl_info.is_voodoo;


   if (ext_list->ALLEGRO_GL_EXT_packed_pixels) {

      TRACE("Packed Pixels formats available\n");

      /* XXX On NV cards, we want to use BGRA instead of RGBA for speed */
      /* Fills the __allegro_gl_texture_format array */
      /* TODO: use these somewhere */
#if 0
      __allegro_gl_texture_read_format[0] = GL_UNSIGNED_BYTE_3_3_2;
      __allegro_gl_texture_read_format[1] = GL_UNSIGNED_SHORT_5_5_5_1;
      __allegro_gl_texture_read_format[2] = GL_UNSIGNED_SHORT_5_6_5;
#endif /* #if 0 */
   }

   /* NVidia and ATI cards expose OpenGL 2.0 but often don't accelerate
    * non-power-of-2 textures. This check is how you verify that NP2
    * textures are hardware accelerated or not.
    * We should clobber the NPOT support if it's not accelerated.
    */
   {
      const char *vendor = (const char *)glGetString(GL_VENDOR);
      if (strstr(vendor, "NVIDIA Corporation")) {
         if (!ext_list->ALLEGRO_GL_NV_fragment_program2
          || !ext_list->ALLEGRO_GL_NV_vertex_program3) {
            ext_list->ALLEGRO_GL_ARB_texture_non_power_of_two = 0;
         }
      }
      else if (strstr(vendor, "ATI Technologies")) {
         if (!strstr((const char *)glGetString(GL_EXTENSIONS),
                     "GL_ARB_texture_non_power_of_two")
             && gl_disp->ogl_info.version >= 2.0f) {
            ext_list->ALLEGRO_GL_ARB_texture_non_power_of_two = 0;
         }
      }
   }
}



/* Function: al_get_opengl_extension_list
 * Returns the list of OpenGL extensions supported by Allegro, for
 * the current display.
 *
 * Allegro will keep information about all extensions it knows about in a
 * structure returned by <al_get_opengl_extension_list>.
 *
 * For example:
 * > if (al_get_opengl_extension_list()->ALLEGRO_GL_ARB_multitexture) { use it }
 *
 * The extension will be set to true if available for the current display and
 * false otherwise. This means to use the definitions and functions from an
 * OpenGL extension, all you need to do is to check for it as above at
 * run time, after acquiring the OpenGL display from Allegro.
 *
 * Under Windows, this will also work with WGL extensions, and under Unix
 * with GLX extensions.
 *
 * In case you want to manually check for extensions and load function pointers
 * yourself (say, in case the Allegro developers did not include it yet), you
 * can use the <al_is_opengl_extension_supported> and
 * <al_get_opengl_proc_address> functions instead.
 */
ALLEGRO_OGL_EXT_LIST *al_get_opengl_extension_list(void)
{
   ALLEGRO_DISPLAY *disp;

   disp = al_get_current_display();
   ASSERT(disp);
   return ((ALLEGRO_DISPLAY_OGL*)disp)->extension_list;
}



/* print_extensions:
 * Given a string containing extensions (i.e. a NULL terminated string where
 * each extension are separated by a space and which names do not contain any
 * space)
 */
static void print_extensions(AL_CONST char *extension)
{
   char buf[80];
   char *start;

   while (*extension != '\0') {
      start = buf;
      strncpy(buf, extension, 80);
      while ((*start != ' ') && (*start != '\0')) {
         extension++;
         start++;
      }
      *start = '\0';
      extension++;
      TRACE(PREFIX_I "%s\n", buf);
   }
}



void _al_ogl_unmanage_extensions(ALLEGRO_DISPLAY_OGL *gl_disp)
{
   destroy_extension_api_table(gl_disp->extension_api);
   destroy_extension_list(gl_disp->extension_list);

#ifdef ALLEGRO_MACOSX
   CFRelease(opengl_bundle_ref);
#endif
#ifdef ALLEGRO_HAVE_DYNAMIC_LINK
   if (__libgl_handle) {
      dlclose(__libgl_handle);
      __libgl_handle = NULL;
   }
#endif
}
