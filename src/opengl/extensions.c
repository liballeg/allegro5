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
 *
 *      Based on AllegroGL extensions management.
 */

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/opengl/gl_ext.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_system.h"

/* We need some driver specific details not worth of a vtable entry. */
#if defined A5O_WINDOWS
   #include "../win/wgl.h"
#elif defined A5O_UNIX && !defined A5O_EXCLUDE_GLX
   #include "allegro5/internal/aintern_xdisplay.h"
   #include "allegro5/internal/aintern_xsystem.h"
#endif

#if defined __APPLE__ && !defined A5O_IPHONE
#include <OpenGL/glu.h>
#elif !defined A5O_CFG_OPENGLES
#include <GL/glu.h>
#endif

A5O_DEBUG_CHANNEL("opengl")


#ifdef A5O_HAVE_DYNAMIC_LINK
   #include <dlfcn.h>
   /* Handle for dynamic library libGL.so */
   static void *__libgl_handle = NULL;
   /* Pointer to glXGetProcAddressARB */
   typedef void *(*GLXGETPROCADDRESSARBPROC) (const GLubyte *);
   static GLXGETPROCADDRESSARBPROC alXGetProcAddress;
#else /* #ifdef A5O_HAVE_DYNAMIC_LINK */
   /* Tries static linking */
   /* FIXME: set A5O_GLXGETPROCADDRESSARB on configure time, if
    * glXGetProcAddressARB must be used!
    */
   #if defined A5O_GLXGETPROCADDRESSARB
      #define alXGetProcAddress glXGetProcAddressARB
   #elif defined A5O_RASPBERRYPI
      #define alXGetProcAddress eglGetProcAddress
   #else
      #define alXGetProcAddress glXGetProcAddress
   #endif
#endif /* #ifdef A5O_HAVE_DYNAMIC_LINK */


#ifdef A5O_MACOSX
   #include <CoreFoundation/CoreFoundation.h>
   static CFBundleRef opengl_bundle_ref;
#endif


/* Define the GL API pointers.
 * Example:
 * _A5O_glBlendEquation_t _al_glBlendEquation = NULL;
 */
#define AGL_API(type, name, args) _A5O_gl##name##_t _al_gl##name = NULL;
#	include "allegro5/opengl/GLext/gl_ext_api.h"
#undef AGL_API
#ifdef A5O_WINDOWS
#define AGL_API(type, name, args) _A5O_wgl##name##_t _al_wgl##name = NULL;
#	include "allegro5/opengl/GLext/wgl_ext_api.h"
#undef AGL_API
#elif defined A5O_UNIX
#define AGL_API(type, name, args) _A5O_glX##name##_t _al_glX##name = NULL;
#	include "allegro5/opengl/GLext/glx_ext_api.h"
#undef AGL_API
#endif



static uint32_t parse_opengl_version(const char *s)
{
   char *p = (char *) s;
   int v[4] = {0, 0, 0, 0};
   int n;
   uint32_t ver;

   /* For OpenGL ES version strings have the form:
    * "OpenGL ES-<profile> <major>.<minor>"
    * So for example: "OpenGL ES-CM 2.0". We simply skip any non-digit
    * characters and then parse it like for normal OpenGL.
    */
   while (*p && !isdigit(*p)) {
      p++;
   }

   /* e.g. "4.0.0 Vendor blah blah" */
   for (n = 0; n < 4; n++) {
      char *end;
      long l;

      errno = 0;
      l = strtol(p, &end, 10);
      if (errno)
         break;
      v[n] = _A5O_CLAMP(0, l, 255);
      if (*end != '.')
         break;
      p = end + 1; /* skip dot */
   }

   ver = (v[0] << 24) | (v[1] << 16) | (v[2] << 8) | v[3];
   A5O_DEBUG("Parsed '%s' as 0x%08x\n", s, ver);
   return ver;
}



/* Reads version info out of glGetString(GL_VERSION) */
static uint32_t _al_ogl_version(void)
{
   char const *value = al_get_config_value(al_get_system_config(), "opengl",
      "force_opengl_version");
   if (value) {
      uint32_t v = parse_opengl_version(value);
      A5O_INFO("OpenGL version forced to %d.%d.%d.%d.\n",
         (v >> 24) & 0xff,
         (v >> 16) & 0xff,
         (v >> 8) & 0xff,
         (v & 0xff));
      return v;
   }

   const char *str;

   str = (const char *)glGetString(GL_VERSION);
   if (str) {
      #ifdef A5O_CFG_OPENGLES
      char *str2 = strstr(str, "ES ");
      if (str2)
         str = str2 + 3;
      #endif
      return parse_opengl_version(str);
   }
   else {
      /* The OpenGL driver does not return a version
       * number. However it probably supports at least OpenGL 1.0
       */
      return _A5O_OPENGL_VERSION_1_0;
   }

}



/* print_extensions:
 * Given a string containing extensions (i.e. a NULL terminated string where
 * each extension are separated by a space and which names do not contain any
 * space)
 */
static void print_extensions(char const *extension)
{
   char buf[80];
   char *start;
   ASSERT(extension);

   while (*extension != '\0') {
      start = buf;
      _al_sane_strncpy(buf, extension, 80);
      while ((*start != ' ') && (*start != '\0')) {
         extension++;
         start++;
      }
      *start = '\0';
      if (*extension != '\0')
         extension++;
      A5O_DEBUG("%s\n", buf);
   }
}



#if !defined A5O_CFG_OPENGLES
/* Print all extensions the OpenGL 3.0 way. */ 
static void print_extensions_3_0(void)
{
   int i;
   GLint n;
   GLubyte const *name;
   glGetIntegerv(GL_NUM_EXTENSIONS, &n);
   for (i = 0; i < n; i++) {
      name = glGetStringi(GL_EXTENSIONS, i);
      A5O_DEBUG("%s\n", name);
   }
}
#endif



/* Function: al_get_opengl_version
 */
uint32_t al_get_opengl_version(void)
{
   A5O_DISPLAY *ogl_disp;

   ogl_disp = al_get_current_display();
   if (!ogl_disp || !ogl_disp->ogl_extras)
      return 0x0;

   return ogl_disp->ogl_extras->ogl_info.version;
}


/* NOTE: al_get_opengl_variant is pretty simple right now but may
 * eventually need driver support.
 */

/* Function: al_get_opengl_variant
 */
int al_get_opengl_variant(void)
{
#if defined A5O_CFG_OPENGLES
   return A5O_OPENGL_ES;
#else
   return A5O_DESKTOP_OPENGL;
#endif
}

/* Create the extension list */
static A5O_OGL_EXT_LIST *create_extension_list(void)
{
   A5O_OGL_EXT_LIST *ret = al_calloc(1, sizeof(A5O_OGL_EXT_LIST));

   if (!ret) {
      return NULL;
   }

   return ret;
}



/* Create the extension API table */
static A5O_OGL_EXT_API *create_extension_api_table(void)
{
   A5O_OGL_EXT_API *ret = al_calloc(1, sizeof(A5O_OGL_EXT_API));

   if (!ret) {
      return NULL;
   }

   return ret;
}



typedef void (*VOID_FPTR)(void);
/* GCC 4.8.2 and possibly others are really slow at optimizing the 100's of the
 * if statements in the load_extensions function below, so we extract them to
 * this function.
 */
static VOID_FPTR load_extension(const char* name)
{
   VOID_FPTR fptr = NULL;
#ifdef A5O_WINDOWS
   fptr = (VOID_FPTR)wglGetProcAddress(name);
#elif defined A5O_UNIX
   fptr = (VOID_FPTR)alXGetProcAddress((const GLubyte*)name);
#elif defined A5O_MACOSX
   CFStringRef cfstr = CFStringCreateWithCStringNoCopy(NULL, name,
      kCFStringEncodingUTF8, kCFAllocatorNull);
   if (cfstr) {
      fptr = (VOID_FPTR)CFBundleGetFunctionPointerForName(opengl_bundle_ref, cfstr);
      CFRelease(cfstr);
   }
#elif defined A5O_SDL
   fptr = SDL_GL_GetProcAddress(name);
#endif
   if (fptr) {
      A5O_DEBUG("%s successfully loaded (%p)\n", name, fptr);
   }
   return fptr;
}



/* Load the extension API addresses into the table.
 * Should only be done on context creation.
 */
static void load_extensions(A5O_OGL_EXT_API *ext)
{
   if (!ext) {
      return;
   }
   
#ifdef A5O_UNIX
#ifdef A5O_HAVE_DYNAMIC_LINK
   if (!alXGetProcAddress) {
      return;
   }
#endif
#endif

#ifdef A5O_WINDOWS

   #define AGL_API(type, name, args)                                           \
      ext->name = (_A5O_gl##name##_t)load_extension("gl" #name);

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

   #define AGL_API(type, name, args)                                           \
      ext->name = (_A5O_wgl##name##_t)load_extension("wgl" #name);

      #include "allegro5/opengl/GLext/wgl_ext_api.h"

   #undef AGL_API

#elif defined A5O_UNIX

   #define AGL_API(type, name, args)                                           \
      ext->name = (_A5O_gl##name##_t)load_extension("gl" #name);

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

   #define AGL_API(type, name, args)                                           \
      ext->name = (_A5O_glX##name##_t)load_extension("glX" #name);

      #include "allegro5/opengl/GLext/glx_ext_api.h"

   #undef AGL_API

#elif defined A5O_MACOSX

#define AGL_API(type, name, args)                                              \
      ext->name = (_A5O_gl##name##_t)load_extension("gl" # name);

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

#elif defined A5O_SDL

#define AGL_API(type, name, args)                                              \
      ext->name = (_A5O_gl##name##_t)load_extension("gl" # name);

      #include "allegro5/opengl/GLext/gl_ext_api.h"

   #undef AGL_API

#endif

}



/* Set the GL API pointers to the current table 
 * Should only be called on context switches.
 */
void _al_ogl_set_extensions(A5O_OGL_EXT_API *ext)
{
   if (!ext) {
      return;
   }

#define AGL_API(type, name, args) _al_gl##name = ext->name;
#	include "allegro5/opengl/GLext/gl_ext_api.h"
#undef AGL_API

#ifdef A5O_WINDOWS
#define AGL_API(type, name, args) _al_wgl##name = ext->name;
#	include "allegro5/opengl/GLext/wgl_ext_api.h"
#undef AGL_API

#elif defined A5O_UNIX
#define AGL_API(type, name, args) _al_glX##name = ext->name;
#	include "allegro5/opengl/GLext/glx_ext_api.h"
#undef AGL_API
#endif
}



/* Destroys the extension API table */
static void destroy_extension_api_table(A5O_OGL_EXT_API *ext)
{
   if (ext) {
      al_free(ext);
   }
}



/* Destroys the extension list */
static void destroy_extension_list(A5O_OGL_EXT_LIST *list)
{
   if (list) {
      al_free(list);
   }
}



/* _al_ogl_look_for_an_extension:
 * This function has been written by Mark J. Kilgard in one of his
 * tutorials about OpenGL extensions
 */
int _al_ogl_look_for_an_extension(const char *name, const GLubyte *extensions)
{
   const GLubyte *start;
   GLubyte *where, *terminator;
   ASSERT(extensions);

   /* Extension names should not have spaces. */
   where = (GLubyte *) strchr(name, ' ');
   if (where || *name == '\0')
      return false;
   /* It takes a bit of care to be fool-proof about parsing the
    * OpenGL extensions string. Don't be fooled by sub-strings, etc.
    */
   start = extensions;
   for (;;) {
      where = (GLubyte *) strstr((const char *)start, name);
      if (!where)
         break;
      terminator = where + strlen(name);
      if (where == start || *(where - 1) == ' ')
         if (*terminator == ' ' || *terminator == '\0')
            return true;
      start = terminator;
   }
   return false;
}



static bool _ogl_is_extension_supported(const char *extension,
                                       A5O_DISPLAY *disp)
{
   int ret = 0;
   GLubyte const *ext_str;
#if !defined A5O_CFG_OPENGLES
   int v = al_get_opengl_version();
#endif
   (void)disp;

#if !defined A5O_CFG_OPENGLES
   if (disp->flags & A5O_OPENGL_3_0 || v >= _A5O_OPENGL_VERSION_3_0) {
      int i;
      GLint ext_cnt;
      glGetIntegerv(GL_NUM_EXTENSIONS, &ext_cnt);
      for (i = 0; i < ext_cnt; i++) {
         ext_str = glGetStringi(GL_EXTENSIONS, i);
         if (ext_str && !strcmp(extension, (char*)ext_str)) {
            ret = 1;
            break;
         }
      }
   }
   else
#endif
   {
      ext_str = glGetString(GL_EXTENSIONS);
      if (!ext_str)
         return false;
      ret = _al_ogl_look_for_an_extension(extension, ext_str);
   }

#ifdef A5O_WINDOWS
   if (!ret && strncmp(extension, "WGL", 3) == 0) {
      A5O_DISPLAY_WGL *wgl_disp = (void*)disp;
      _A5O_wglGetExtensionsStringARB_t _wglGetExtensionsStringARB;

      if (!wgl_disp->dc)
         return false;

      _wglGetExtensionsStringARB = (void *)
         wglGetProcAddress("wglGetExtensionsStringARB");
      if (_wglGetExtensionsStringARB) {
         ret = _al_ogl_look_for_an_extension(extension, (const GLubyte *)
                                     _wglGetExtensionsStringARB(wgl_disp->dc));
      }
   }

#elif defined A5O_UNIX && !defined A5O_EXCLUDE_GLX
   if (!ret && strncmp(extension, "GLX", 3) == 0) {
      A5O_SYSTEM_XGLX *sys = (void*)al_get_system_driver();
      A5O_DISPLAY_XGLX *glx_disp = (void*)disp;
      char const *ext;

      if (!sys->gfxdisplay)
         return false;

      ext = glXQueryExtensionsString(sys->gfxdisplay, glx_disp->xscreen);
      if (!ext) {
         /* work around driver bugs? */
         ext = "";
      }
      ret = _al_ogl_look_for_an_extension(extension, (const GLubyte *)ext);
   }
#endif

   return ret;
}



static bool _ogl_is_extension_with_version_supported(
   const char *extension, A5O_DISPLAY *disp, uint32_t ver)
{
   char const *value;

  /* For testing purposes, any OpenGL extension can be disable in
    * the config by using something like:
    * 
    * [opengl_disabled_extensions]
    * GL_ARB_texture_non_power_of_two=0
    * GL_EXT_framebuffer_object=0
    * 
    */
   value = al_get_config_value(al_get_system_config(),
      "opengl_disabled_extensions", extension);
   if (value) {
      A5O_WARN("%s found in [opengl_disabled_extensions].\n",
         extension);
      return false;
   }

   /* If the extension is included in the OpenGL version, there is no
    * need to check the extensions list.
    */
   if (ver > 0 && disp->ogl_extras->ogl_info.version >= ver) {
      return true;
   }
      
   return _ogl_is_extension_supported(extension, disp);
}



/* Function: al_have_opengl_extension
 */
bool al_have_opengl_extension(const char *extension)
{
   A5O_DISPLAY *disp;
   
   disp = al_get_current_display();
   if (!disp)
      return false;

   if (!(disp->flags & A5O_OPENGL))
      return false;

   return _ogl_is_extension_supported(extension, disp);
}



/* Function: al_get_opengl_proc_address
 */
void *al_get_opengl_proc_address(const char *name)
{
   void *symbol = NULL;
#ifdef A5O_MACOSX
   CFStringRef function;
#endif
   A5O_DISPLAY *disp;
   
   disp = al_get_current_display();
   if (!disp)
      return NULL;

   if (!(disp->flags & A5O_OPENGL))
      return NULL;

#if defined A5O_WINDOWS
   /* For once Windows is the easiest platform to use :)
    * It provides a standardized way to get a function address
    * But of course there is a drawback : the symbol is only valid
    * under the current context :P
    */
   {
      A5O_DISPLAY_WGL *wgl_disp = (void*)disp;

      if (!wgl_disp->dc)
         return NULL;

      symbol = wglGetProcAddress(name);
   }
#elif defined A5O_UNIX
#if defined A5O_HAVE_DYNAMIC_LINK
   if (alXGetProcAddress)
#endif
   {
      /* This is definitely the *good* way on Unix to get a GL proc
       * address. Unfortunately glXGetProcAddress is an extension
       * and may not be available on all platforms
       */
#if defined A5O_RASPBERRYPI
      symbol = alXGetProcAddress(name);
#else
      symbol = alXGetProcAddress((const GLubyte *)name);
#endif
   }
#elif defined A5O_HAVE_DYNAMIC_LINK
   else {
      /* Hack if glXGetProcAddress is not available :
       * we try to find the symbol into libGL.so
       */
      if (__al_handle) {
         symbol = dlsym(__al_handle, name);
      }
   }
#elif defined A5O_MACOSX
   function = CFStringCreateWithCString(kCFAllocatorDefault, name,
                                        kCFStringEncodingASCII);
   if (function) {
      symbol =
          CFBundleGetFunctionPointerForName(opengl_bundle_ref, function);
      CFRelease(function);
   }
#endif

   if (!symbol) {

#if defined A5O_HAVE_DYNAMIC_LINK
      if (!alXGetProcAddress) {
         A5O_WARN("get_proc_address: libdl::dlsym: %s\n", dlerror());
      }
#endif

      A5O_WARN("get_proc_address : Unable to load symbol %s\n", name);
   }
   else {
      A5O_DEBUG("get_proc_address : Symbol %s successfully loaded\n", name);
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
   else if (strstr(renderer, "Intel HD Graphics 3000 OpenGL Engine")) {
      info->is_intel_hd_graphics_3000 = 1;
   }

   if ((strncmp(renderer, "3Dfx/Voodoo3 ", 13) == 0)
       || (strncmp(renderer, "3Dfx/Voodoo2 ", 13) == 0)
       || (strncmp(renderer, "3Dfx/Voodoo ", 12) == 0)) {
      info->is_voodoo3_and_under = 1;
   }

   /* Read OpenGL properties */
   info->version = _al_ogl_version();
   A5O_INFO("Assumed OpenGL version: %d.%d.%d.%d\n",
      (info->version >> 24) & 0xff,
      (info->version >> 16) & 0xff,
      (info->version >>  8) & 0xff,
      (info->version      ) & 0xff);

   return;
}



/* _al_ogl_manage_extensions:
 * This functions fills the extensions API table and extension list
 * structures and displays on the log file which extensions are available.
 */
void _al_ogl_manage_extensions(A5O_DISPLAY *gl_disp)
{
   //const GLubyte *buf;
#if defined A5O_MACOSX
   CFURLRef bundle_url;
#endif
   A5O_OGL_EXT_API *ext_api;
   A5O_OGL_EXT_LIST *ext_list;

   /* Some functions depend on knowing the version of opengl in use */
   fill_in_info_struct(glGetString(GL_RENDERER), &(gl_disp->ogl_extras->ogl_info));

   /* Print out OpenGL extensions
    * We should use glGetStringi(GL_EXTENSIONS, i) for OpenGL 3.0+
    * but it doesn't seem to work until later.
    */
   if (gl_disp->ogl_extras->ogl_info.version < _A5O_OPENGL_VERSION_3_0) {
      A5O_DEBUG("OpenGL Extensions:\n");
      print_extensions((char const *)glGetString(GL_EXTENSIONS));
   }

   /* Print out GLU version */
   //buf = gluGetString(GLU_VERSION);
   //A5O_INFO("GLU Version : %s\n", buf);

#ifdef A5O_HAVE_DYNAMIC_LINK
   /* Get glXGetProcAddress entry */
   __libgl_handle = dlopen("libGL.so", RTLD_LAZY);
   if (__libgl_handle) {
      alXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__libgl_handle,
         "glXGetProcAddressARB");
      if (!alXGetProcAddress) {
         alXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__libgl_handle,
            "glXGetProcAddress");
         if (!alXGetProcAddress) {
            alXGetProcAddress = (GLXGETPROCADDRESSARBPROC) dlsym(__libgl_handle,
               "eglGetProcAddress");
         }
      }
   }
   else {
      A5O_WARN("Failed to dlopen libGL.so : %s\n", dlerror());
   }
   A5O_INFO("glXGetProcAddress Extension: %s\n",
         alXGetProcAddress ? "Supported" : "Unsupported");
#elif defined A5O_UNIX
#ifdef ALLEGROGL_GLXGETPROCADDRESSARB
   A5O_INFO("glXGetProcAddressARB Extension: supported\n");
#else
   A5O_INFO("glXGetProcAddress Extension: supported\n");
#endif
#endif

#ifdef A5O_MACOSX
   bundle_url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                              CFSTR
                                              ("/System/Library/Frameworks/OpenGL.framework"),
                                              kCFURLPOSIXPathStyle, true);
   opengl_bundle_ref = CFBundleCreate(kCFAllocatorDefault, bundle_url);
   CFRelease(bundle_url);
#endif

#if defined A5O_UNIX && !defined A5O_EXCLUDE_GLX
   A5O_DEBUG("GLX Extensions:\n");
   A5O_SYSTEM_XGLX *glx_sys = (void*)al_get_system_driver();
   A5O_DISPLAY_XGLX *glx_disp = (void *)gl_disp;
   char const *ext = glXQueryExtensionsString(
      glx_sys->gfxdisplay, glx_disp->xscreen);
   if (!ext) {
      /* work around driver bugs? */
      ext = "";
   }
   print_extensions(ext);
#endif

   /* Create & load extension API table */
   ext_api = create_extension_api_table();
   load_extensions(ext_api);
   gl_disp->ogl_extras->extension_api = ext_api;
   
#if !defined A5O_CFG_OPENGLES
   /* Need that symbol already so can't wait until it is assigned later. */
   glGetStringi = ext_api->GetStringi;

   if (gl_disp->ogl_extras->ogl_info.version >= _A5O_OPENGL_VERSION_3_0) {
      A5O_DEBUG("OpenGL Extensions:\n");
      print_extensions_3_0();
   }
#endif

   /* Create the list of supported extensions. */
   ext_list = create_extension_list();
   gl_disp->ogl_extras->extension_list = ext_list;

   /* Fill the list. */
#define AGL_EXT(name, ver) { \
      ext_list->A5O_GL_##name = \
         _ogl_is_extension_with_version_supported("GL_" #name, gl_disp, \
            _A5O_OPENGL_VERSION_##ver); \
   }
   #include "allegro5/opengl/GLext/gl_ext_list.h"
#undef AGL_EXT

#ifdef A5O_UNIX
#define AGL_EXT(name, ver) { \
      ext_list->A5O_GLX_##name = \
         _ogl_is_extension_with_version_supported("GLX_" #name, gl_disp, \
            _A5O_OPENGL_VERSION_##ver); \
   }
   #include "allegro5/opengl/GLext/glx_ext_list.h"
#undef AGL_EXT
#elif defined A5O_WINDOWS
#define AGL_EXT(name, ver) { \
      ext_list->A5O_WGL_##name = \
         _ogl_is_extension_with_version_supported("WGL_" #name, gl_disp, \
            _A5O_OPENGL_VERSION_##ver); \
   }
   #include "allegro5/opengl/GLext/wgl_ext_list.h"
#undef AGL_EXT
#endif

   /* Get max texture size */
   glGetIntegerv(GL_MAX_TEXTURE_SIZE,
                 (GLint *) & gl_disp->ogl_extras->ogl_info.max_texture_size);

   /* Note: Voodoo (even V5) don't seem to correctly support
    * packed pixel formats. Disabling them for those cards.
    */
   ext_list->A5O_GL_EXT_packed_pixels &= !gl_disp->ogl_extras->ogl_info.is_voodoo;


   if (ext_list->A5O_GL_EXT_packed_pixels) {

      A5O_INFO("Packed Pixels formats available\n");

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
         if (!ext_list->A5O_GL_NV_fragment_program2
          || !ext_list->A5O_GL_NV_vertex_program3) {
            ext_list->A5O_GL_ARB_texture_non_power_of_two = 0;
         }
      }
      else if (strstr(vendor, "ATI Technologies")) {
         if (gl_disp->ogl_extras->ogl_info.version >= _A5O_OPENGL_VERSION_3_0) {
            /* Assume okay. */
         }
         else if (!strstr((const char *)glGetString(GL_EXTENSIONS),
               "GL_ARB_texture_non_power_of_two")
             && gl_disp->ogl_extras->ogl_info.version >= _A5O_OPENGL_VERSION_2_0) {
            ext_list->A5O_GL_ARB_texture_non_power_of_two = 0;
         }
      }
   }
   
   {
      int *s = gl_disp->extra_settings.settings;
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, s + A5O_MAX_BITMAP_SIZE);
   
      if (gl_disp->ogl_extras->ogl_info.version >= _A5O_OPENGL_VERSION_2_0)
         s[A5O_SUPPORT_SEPARATE_ALPHA] = 1;

      s[A5O_SUPPORT_NPOT_BITMAP] =
         ext_list->A5O_GL_ARB_texture_non_power_of_two ||
         ext_list->A5O_GL_OES_texture_npot;
   A5O_INFO("Use of non-power-of-two textures %s.\n",
      s[A5O_SUPPORT_NPOT_BITMAP] ? "enabled" : "disabled");
#if defined A5O_CFG_OPENGLES
   if (gl_disp->flags & A5O_PROGRAMMABLE_PIPELINE) {
      s[A5O_CAN_DRAW_INTO_BITMAP] = true;
   }
   else {
      s[A5O_CAN_DRAW_INTO_BITMAP] =
         ext_list->A5O_GL_OES_framebuffer_object;
   }
   A5O_INFO("Use of FBO to draw to textures %s.\n",
      s[A5O_CAN_DRAW_INTO_BITMAP] ? "enabled" :
      "disabled");
#else
   s[A5O_CAN_DRAW_INTO_BITMAP] =
      ext_list->A5O_GL_EXT_framebuffer_object;
   A5O_INFO("Use of FBO to draw to textures %s.\n",
      s[A5O_CAN_DRAW_INTO_BITMAP] ? "enabled" :
      "disabled");
#endif
   }
}



/* Function: al_get_opengl_extension_list
 */
A5O_OGL_EXT_LIST *al_get_opengl_extension_list(void)
{
   A5O_DISPLAY *disp;

   disp = al_get_current_display();
   ASSERT(disp);

   if (!(disp->flags & A5O_OPENGL))
      return NULL;

   ASSERT(disp->ogl_extras);
   return disp->ogl_extras->extension_list;
}



void _al_ogl_unmanage_extensions(A5O_DISPLAY *gl_disp)
{
   destroy_extension_api_table(gl_disp->ogl_extras->extension_api);
   destroy_extension_list(gl_disp->ogl_extras->extension_list);
   gl_disp->ogl_extras->extension_api = NULL;
   gl_disp->ogl_extras->extension_list = NULL;

#ifdef A5O_MACOSX
   CFRelease(opengl_bundle_ref);
#endif
#ifdef A5O_HAVE_DYNAMIC_LINK
   if (__libgl_handle) {
      dlclose(__libgl_handle);
      __libgl_handle = NULL;
   }
#endif
}

/* vim: set sts=3 sw=3 et: */
