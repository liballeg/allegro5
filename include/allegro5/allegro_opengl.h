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
 *      Main header file for all OpenGL drivers.
 *
 *      By Milan Mimica.
 *
 */


#ifndef __al_included_allegro5_allegro_opengl_h
#define __al_included_allegro5_allegro_opengl_h

#ifdef __cplusplus
   extern "C" {
#endif

#if defined(ALLEGRO_WINDOWS)
#include <windows.h>
#endif

#if defined ALLEGRO_IPHONE

#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

/* Apple defines OES versions for these - however the separated alpha ones
 * don't seem to work on the device and just crash.
 */
#define glBlendEquation glBlendEquationOES
#define glBlendFuncSeparate glBlendFuncSeparateOES
#define glBlendEquationSeparate glBlendEquationSeparateOES
#ifdef GL_FUNC_ADD
#undef GL_FUNC_ADD
#undef GL_FUNC_SUBTRACT
#undef GL_FUNC_REVERSE_SUBTRACT
#endif
#define GL_FUNC_ADD GL_FUNC_ADD_OES
#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES

#elif defined ALLEGRO_MACOSX

#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#elif defined ALLEGRO_ANDROID || defined ALLEGRO_RASPBERRYPI

#include <GLES/gl.h>
#include <GLES/glext.h>

#ifdef ALLEGRO_CFG_OPENGLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#define GL_FUNC_ADD GL_FUNC_ADD_OES
#define GL_FUNC_SUBTRACT GL_FUNC_SUBTRACT_OES
#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES
#endif

#define GL_RGBA8 GL_RGBA8_OES

#ifndef ALLEGRO_RASPBERRYPI
#define GL_FRAMEBUFFER_BINDING_EXT GL_FRAMEBUFFER_BINDING_OES
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
#define glBlendEquation glBlendEquationOES
#define glBlendFuncSeparate glBlendFuncSeparateOES
#define glBlendEquationSeparate glBlendEquationSeparateOES
#define glGenerateMipmapEXT glGenerateMipmapOES
#define glBindFramebufferEXT glBindFramebufferOES
#define glDeleteFramebuffersEXT glDeleteFramebuffersOES
#else
#define GL_FRAMEBUFFER_BINDING_EXT GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
#define glBlendEquation glBlendEquation
#define glBlendFuncSeparate glBlendFuncSeparate
#define glBlendEquationSeparate glBlendEquationSeparate
#define glGenerateMipmapEXT glGenerateMipmap
#define glBindFramebufferEXT glBindFramebuffer
#define glDeleteFramebuffersEXT glDeleteFramebuffers
#endif

#else /* ALLEGRO_MACOSX */

/* HACK: Prevent both Mesa and SGI's broken headers from screwing us */
#define __glext_h_
#define __glxext_h_
#include <GL/gl.h>
#undef  __glext_h_
#undef  __glxext_h_

#endif /* ALLEGRO_MACOSX */

#ifdef ALLEGRO_RASPBERRYPI
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#include "allegro5/opengl/gl_ext.h"

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

#endif /* ALLEGRO_WINDOWS */

#if defined ALLEGRO_WINDOWS
	#define ALLEGRO_DEFINE_PROC_TYPE(type, name, args) \
		typedef type (APIENTRY * name) args;
#else
	#define ALLEGRO_DEFINE_PROC_TYPE(type, name, args) \
		typedef type (*name) args;
#endif

/*
 *  Public OpenGL-related API
 */

/* Enum: ALLEGRO_OPENGL_VARIANT
 */
typedef enum ALLEGRO_OPENGL_VARIANT {
   ALLEGRO_DESKTOP_OPENGL = 0,
   ALLEGRO_OPENGL_ES
} ALLEGRO_OPENGL_VARIANT;

AL_FUNC(uint32_t,              al_get_opengl_version,            (void));
AL_FUNC(bool,                  al_have_opengl_extension,         (const char *extension));
AL_FUNC(void*,                 al_get_opengl_proc_address,       (const char *name));
AL_FUNC(ALLEGRO_OGL_EXT_LIST*, al_get_opengl_extension_list,     (void));
AL_FUNC(GLuint,                al_get_opengl_texture,            (ALLEGRO_BITMAP *bitmap));
AL_FUNC(void,                  al_remove_opengl_fbo,             (ALLEGRO_BITMAP *bitmap));
AL_FUNC(GLuint,                al_get_opengl_fbo,                (ALLEGRO_BITMAP *bitmap));
AL_FUNC(bool,                  al_get_opengl_texture_size,       (ALLEGRO_BITMAP *bitmap,
                                                                  int *w, int *h));
AL_FUNC(void,                  al_get_opengl_texture_position,   (ALLEGRO_BITMAP *bitmap,
                                                                  int *u, int *v));
AL_FUNC(void,                  al_set_current_opengl_context,    (ALLEGRO_DISPLAY *display));
AL_FUNC(int,                   al_get_opengl_variant,            (void));

#ifdef __cplusplus
   }
#endif

#endif
