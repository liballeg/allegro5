/* $Id$ */

/*
 * Mesa 3-D graphics library
 * Version:  3.4.2
 * 
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/* Allegro driver by Bernhard Tschirren (bernie-t@geocities.com)
 * Updated to Mesa 3.4.2 by Bertrand Coconnier (bcoconni@club-internet.fr)
 */


#ifndef AMESA_H
#define AMESA_H

#include <allegro.h>

/* Names collision : We don't need them to build AMesa so let's undefine... */
#undef ASSERT    /* Used by Mesa */
#undef INLINE    /* Also used by Mesa */

#include <GL/gl.h>


#ifdef __cplusplus
extern "C" {
#endif


#define AMESA_MAJOR_VERSION 3
#define AMESA_MINOR_VERSION 3

#define AMESA_FRONT  0x1
#define AMESA_BACK   0x2
#define AMESA_ACTIVE 0x3


typedef struct amesa_visual*  AMesaVisual;
typedef struct amesa_buffer*  AMesaBuffer;
typedef struct amesa_context* AMesaContext;


AMesaVisual AMesaCreateVisual(GLboolean dbFlag, GLint depth,
                                               GLint rgba, GLint depthSize,
                                               GLint stencilSize,
                                               GLint accumSizeRed,
					       GLint accumSizeGreen,
					       GLint accumSizeBlue,
					       GLint accumSizeAlpha);

void AMesaDestroyVisual(AMesaVisual visual);

AMesaBuffer AMesaCreateBuffer(AMesaVisual visual, BITMAP* bmp);

void AMesaDestroyBuffer(AMesaBuffer buffer);


AMesaContext AMesaCreateContext(AMesaVisual visual,
                                                 AMesaContext sharelist);

void AMesaDestroyContext(AMesaContext context);

GLboolean AMesaMakeCurrent(AMesaContext context,
                                            AMesaBuffer buffer);

void AMesaSwapBuffers(AMesaBuffer buffer);

AMesaContext AMesaGetCurrentContext(void);

BITMAP* AMesaGetColorBuffer(AMesaBuffer buffer, GLenum mode);

void* AMesaGetProcAddress(AL_CONST char *name);

#ifdef __cplusplus
}
#endif


#endif /* AMESA_H */
