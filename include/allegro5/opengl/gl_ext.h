
#ifndef GL_EXT_ALLEGRO_H
#define GL_EXT_ALLEGRO_H


struct allegro_gl_info {
	float version;          /* OpenGL version */
	int num_texture_units;  /* Number of texture units */
	int max_texture_size;   /* Maximum texture size */
	int is_voodoo3_and_under; /* Special cases for Voodoo 1-3 */
	int is_voodoo;          /* Special cases for Voodoo cards */
	int is_matrox_g200;     /* Special cases for Matrox G200 boards */
	int is_ati_rage_pro;    /* Special cases for ATI Rage Pro boards */
	int is_ati_radeon_7000; /* Special cases for ATI Radeon 7000 */
	int is_ati_r200_chip;	/* Special cases for ATI card with chip R200 */
	int is_mesa_driver;     /* Special cases for MESA */
};

extern int __allegro_gl_valid_context;
extern GLint __allegro_gl_texture_read_format[5];
extern GLint __allegro_gl_texture_components[5];
extern struct allegro_gl_info allegro_gl_info;


/* GL extension definitions. */

/* For example:
 *
 * #define GL_BGRA 0x80E1
 *
 */

#include "allegro5/opengl/GLext/gl_ext_defs.h"
#ifdef ALLEGRO_WINDOWS
#include "allegro5/opengl/GLext/wgl_ext_defs.h"
#elif defined ALLEGRO_UNIX
#include "allegro5/opengl/GLext/glx_ext_defs.h"
#endif

/* GL extension types */

/* For example:
 *
 * typedef void (APIENTRY * AGL_BlendEquation_t (GLenum);
 *
 */
 
#ifndef APIENTRY
#define APIENTRY
#define APIENTRY_defined
#endif

#define AGL_API(type, name, args) typedef type (APIENTRY * AGL_##name##_t) args;
#	include "allegro5/opengl/GLext/gl_ext_api.h"
#ifdef ALLEGRO_WINDOWS
#	include "allegro5/opengl/GLext/wgl_ext_api.h"
#elif defined ALLEGRO_UNIX
#	include "allegro5/opengl/GLext/glx_ext_api.h"
#endif
#undef AGL_API

#ifdef APIENTRY_defined
#undef APIENTRY
#undef APIENTRY_defined
#endif

/* GL extension declarations */

/* For example:
 *
 * #define glBlendEquation __aglBlendEquation
 * extern AGL_BlendEquation_t __aglBlendEquation;
 *
 */

#define AGL_API(type, name, args) extern AGL_##name##_t __agl##name;
# 	include "allegro5/opengl/GLext/gl_ext_alias.h"
#	include "allegro5/opengl/GLext/gl_ext_api.h"
#undef AGL_API
#ifdef ALLEGRO_WINDOWS
#define AGL_API(type, name, args) extern _AL_DLL AGL_##name##_t __awgl##name;
# 	include "allegro5/opengl/GLext/wgl_ext_alias.h"
#	include "allegro5/opengl/GLext/wgl_ext_api.h"
#undef AGL_API
#elif defined ALLEGRO_UNIX
#define AGL_API(type, name, args) extern AGL_##name##_t __aglX##name;
# 	include "allegro5/opengl/GLext/glx_ext_alias.h"
#	include "allegro5/opengl/GLext/glx_ext_api.h"
#undef AGL_API
#endif

/* For example:
 *
 * int ALLEGRO_GL_ARB_imaging;
 *
 */

typedef struct AGL_EXTENSION_LIST {
#    define AGL_EXT(name, ver) int ALLEGRO_GL_##name;
#    include "allegro5/opengl/GLext/gl_ext_list.h"
#    undef  AGL_EXT

#ifdef ALLEGRO_UNIX
#    define AGL_EXT(name, ver) int ALLEGRO_GLX_##name;
#    include "allegro5/opengl/GLext/glx_ext_list.h"
#    undef  AGL_EXT
#elif defined ALLEGRO_WINDOWS
#    define AGL_EXT(name, ver) int ALLEGRO_WGL_##name;
#    include "allegro5/opengl/GLext/wgl_ext_list.h"
#    undef  AGL_EXT

#endif
} AGL_EXTENSION_LIST;

AGL_EXTENSION_LIST allegro_gl_extensions;

int __allegro_gl_look_for_an_extension(AL_CONST char *name, AL_CONST GLubyte *extensions);
int al_is_opengl_extension_supported(AL_CONST char *extension);


#endif /* #ifndef GL_EXT_ALLEGRO_H */

