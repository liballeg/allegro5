#ifndef __al_included_ogl_helpers_h
#define __al_included_ogl_helpers_h

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"

/* Some definitions to smooth out the code in the opengl directory. */

#ifdef ALLEGRO_CFG_OPENGLES
   #define IS_OPENGLES        (true)
#else
   #define IS_OPENGLES        (false)
#endif

#ifdef ALLEGRO_IPHONE
   #define IS_IPHONE          (true)
#else
   #define IS_IPHONE          (false)
#endif

#ifdef ALLEGRO_ANDROID
   #define IS_ANDROID         (true)
   #define IS_ANDROID_AND(x)  (x)
#else
   #define IS_ANDROID         (false)
   #define IS_ANDROID_AND(x)  (false)
#endif

#ifdef ALLEGRO_RASPBERRYPI
   #define IS_RASPBERRYPI     (true)
#else
   #define IS_RASPBERRYPI     (false)
#endif

#if defined(ALLEGRO_ANDROID) || defined(ALLEGRO_RASPBERRYPI)
   #define UNLESS_ANDROID_OR_RPI(x) (0)
#else
   #define UNLESS_ANDROID_OR_RPI(x) (x)
#endif

/* Android uses different functions/symbol names depending on ES version */
#define ANDROID_PROGRAMMABLE_PIPELINE(dpy) \
   IS_ANDROID_AND(al_get_display_flags(dpy) & ALLEGRO_PROGRAMMABLE_PIPELINE)

#if defined ALLEGRO_CFG_OPENGLES2
#ifndef GL_EXT_draw_buffers
   #define GL_COLOR_ATTACHMENT0_EXT     GL_COLOR_ATTACHMENT0
#endif
   #define GL_FRAMEBUFFER_BINDING_EXT   GL_FRAMEBUFFER_BINDING
   #define GL_FRAMEBUFFER_COMPLETE_EXT  GL_FRAMEBUFFER_COMPLETE
   #define GL_FRAMEBUFFER_EXT           GL_FRAMEBUFFER
   #define GL_RENDERBUFFER_EXT          GL_RENDERBUFFER
   #define GL_DEPTH_ATTACHMENT_EXT      GL_DEPTH_ATTACHMENT
   #define glBindFramebufferEXT         glBindFramebuffer
   #define glCheckFramebufferStatusEXT  glCheckFramebufferStatus
   #define glDeleteFramebuffersEXT      glDeleteFramebuffers
   #define glFramebufferTexture2DEXT    glFramebufferTexture2D
   #define glGenFramebuffersEXT         glGenFramebuffers
   #define glGenerateMipmapEXT          glGenerateMipmap
   #define glOrtho                      glOrthof
   #define glGenRenderbuffersEXT        glGenRenderbuffers
   #define glBindRenderbufferEXT        glBindRenderbuffer
   #define glRenderbufferStorageEXT     glRenderbufferStorage
   #define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
   #define glDeleteRenderbuffersEXT     glDeleteRenderbuffers
   #define GL_DEPTH_ATTACHMENT_EXT      GL_DEPTH_ATTACHMENT
#elif defined ALLEGRO_CFG_OPENGLES
   /* Note: This works because all the constants are the same, e.g.
    * GL_FRAMEBUFFER_OES == GL_FRAMEBUFFER_EXT == 0x8D40
    * And so we can use the OpenGL framebuffer extension in the same was
    * as the OpenGL ES framebuffer extension.
    */
#ifndef GL_EXT_draw_buffers
   #define GL_COLOR_ATTACHMENT0_EXT     GL_COLOR_ATTACHMENT0_OES
#endif
   #define GL_FRAMEBUFFER_BINDING_EXT   GL_FRAMEBUFFER_BINDING_OES
   #define GL_FRAMEBUFFER_COMPLETE_EXT  GL_FRAMEBUFFER_COMPLETE_OES
   #define GL_FRAMEBUFFER_EXT           GL_FRAMEBUFFER_OES
   #define GL_RENDERBUFFER_EXT          GL_RENDERBUFFER_OES
   #define GL_DEPTH_ATTACHMENT_EXT      GL_DEPTH_ATTACHMENT_OES

   #define glBindFramebufferEXT         glBindFramebufferOES
   #define glCheckFramebufferStatusEXT  glCheckFramebufferStatusOES
   #define glDeleteFramebuffersEXT      glDeleteFramebuffersOES
   #define glFramebufferTexture2DEXT    glFramebufferTexture2DOES
   #define glGenFramebuffersEXT         glGenFramebuffersOES
   #define glGenerateMipmapEXT          glGenerateMipmapOES
   #define glGenRenderbuffersEXT        glGenRenderbuffersOES
   #define glBindRenderbufferEXT        glBindRenderbufferOES
   #define glRenderbufferStorageEXT     glRenderbufferStorageOES
   #define glFramebufferRenderbufferEXT glFramebufferRenderbufferOES
   #define glOrtho                      glOrthof
   #define glDeleteRenderbuffersEXT     glDeleteRenderbuffersOES
#endif

#endif

/* vim: set sts=3 sw=3 et: */
