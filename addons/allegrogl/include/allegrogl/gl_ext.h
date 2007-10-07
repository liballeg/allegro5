
#ifndef INCLUDE_ALLEGRO_GL_GL_EXT_H_GUARD
#define INCLUDE_ALLEGRO_GL_GL_EXT_H_GUARD


#ifdef __cplusplus
extern "C" {
#endif

#include "allegrogl/gl_header_detect.h"

#include "allegrogl/GLext/gl_ext_defs.h"
#ifdef ALLEGRO_WINDOWS
#include "allegrogl/GLext/wgl_ext_defs.h"
#elif defined ALLEGRO_UNIX
#include "allegrogl/GLext/glx_ext_defs.h"
#endif


/* GL extension types */
#ifndef APIENTRY
#define APIENTRY
#define APIENTRY_defined
#endif

#define AGL_API(type, name, args) typedef type (APIENTRY * AGL_##name##_t) args;
#	include "allegrogl/GLext/gl_ext_api.h"
#ifdef ALLEGRO_WINDOWS
#	include "allegrogl/GLext/wgl_ext_api.h"
#elif defined ALLEGRO_UNIX
#	include "allegrogl/GLext/glx_ext_api.h"
#endif
#undef AGL_API

#ifdef APIENTRY_defined
#undef APIENTRY
#undef APIENTRY_defined
#endif


/* GL extension declarations */
#define AGL_API(type, name, args) extern _AGL_DLL AGL_##name##_t __agl##name;
# 	include "allegrogl/GLext/gl_ext_alias.h"
#	include "allegrogl/GLext/gl_ext_api.h"
#undef AGL_API
#ifdef ALLEGRO_WINDOWS
#define AGL_API(type, name, args) extern _AGL_DLL AGL_##name##_t __awgl##name;
# 	include "allegrogl/GLext/wgl_ext_alias.h"
#	include "allegrogl/GLext/wgl_ext_api.h"
#undef AGL_API
#elif defined ALLEGRO_UNIX
#define AGL_API(type, name, args) extern AGL_##name##_t __aglX##name;
# 	include "allegrogl/GLext/glx_ext_alias.h"
#	include "allegrogl/GLext/glx_ext_api.h"
#undef AGL_API
#endif

	
typedef struct AGL_EXTENSION_LIST_GL {
#    define AGL_EXT(name, ver) int name;
#    include "allegrogl/GLext/gl_ext_list.h"
#    undef  AGL_EXT
} AGL_EXTENSION_LIST_GL;

#ifdef ALLEGRO_UNIX
typedef struct AGL_EXTENSION_LIST_GLX {
#    define AGL_EXT(name, ver) int name;
#    include "allegrogl/GLext/glx_ext_list.h"
#    undef  AGL_EXT
} AGL_EXTENSION_LIST_GLX;
#elif defined ALLEGRO_WINDOWS
typedef struct AGL_EXTENSION_LIST_WGL {
#    define AGL_EXT(name, ver) int name;
#    include "allegrogl/GLext/wgl_ext_list.h"
#    undef  AGL_EXT
} AGL_EXTENSION_LIST_WGL;
#endif



AGL_VAR(AGL_EXTENSION_LIST_GL, allegro_gl_extensions_GL);
#ifdef ALLEGRO_UNIX
extern AGL_EXTENSION_LIST_GLX allegro_gl_extensions_GLX;
#elif defined ALLEGRO_WINDOWS
AGL_VAR(AGL_EXTENSION_LIST_WGL, allegro_gl_extensions_WGL);
#endif


#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ALLEGRO_GL_GL_EXT_H_GUARD */

