/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#define DESTINATION(BMP, X, Y, TYPE)                                \
    ({                                                              \
    BITMAP *_bmp = BMP;                                             \
                                                                    \
    (TYPE*)(_bmp->line[_bmp->h - (Y) - 1]) + (X);                   \
    })


#define IMPLEMENT_WRITE_RGBA_SPAN(DEPTH, TYPE)                                                   \
static void write_rgba_span_##DEPTH (const GLcontext *ctx,                                       \
                                     GLuint n, GLint x, GLint y,                                 \
                                     const GLubyte rgba[][4],                                    \
                                     const GLubyte mask[])                                       \
    {                                                                                            \
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);                                       \
    TYPE              *d = DESTINATION(context->Buffer->Active, x, y, TYPE);                     \
                                                                                                 \
    if (mask)                                                                                    \
        {                                                                                        \
        while (n--)                                                                              \
            {                                                                                    \
            if (mask[0]) d[0] = makecol##DEPTH(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]);  \
            d++; rgba++; mask++;                                                                 \
            }                                                                                    \
        }                                                                                        \
    else                                                                                         \
        {                                                                                        \
        while (n--)                                                                              \
            {                                                                                    \
            d[0] = makecol##DEPTH(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]);               \
            d++; rgba++;                                                                         \
            }                                                                                    \
        }                                                                                        \
    }


#define IMPLEMENT_WRITE_RGB_SPAN(DEPTH, TYPE)                                                    \
static void write_rgb_span_##DEPTH (const GLcontext *ctx,                                        \
                                    GLuint n, GLint x, GLint y,                                  \
                                    const GLubyte rgb[][3],                                      \
                                    const GLubyte mask[])                                        \
    {                                                                                            \
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);                                       \
    TYPE              *d = DESTINATION(context->Buffer->Active, x, y, TYPE);                     \
                                                                                                 \
    if (mask)                                                                                    \
        {                                                                                        \
        while (n--)                                                                              \
            {                                                                                    \
            if (mask[0]) d[0] = makecol##DEPTH(rgb[0][RCOMP], rgb[0][GCOMP], rgb[0][BCOMP]);     \
            d++; rgb++; mask++;                                                                  \
            }                                                                                    \
        }                                                                                        \
    else                                                                                         \
        {                                                                                        \
        while (n--)                                                                              \
            {                                                                                    \
            d[0] = makecol##DEPTH(rgb[0][RCOMP], rgb[0][GCOMP], rgb[0][BCOMP]);                  \
            d++; rgb++;                                                                          \
            }                                                                                    \
        }                                                                                        \
    }


#define IMPLEMENT_WRITE_MONO_RGBA_SPAN(DEPTH, TYPE)                                              \
static void write_mono_rgba_span_##DEPTH (const GLcontext *ctx,                                  \
                                          GLuint n, GLint x, GLint y,                            \
                                          const GLubyte mask[])                                  \
    {                                                                                            \
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);                                       \
    TYPE           color = context->CurrentColor;                                                \
    TYPE              *d = DESTINATION(context->Buffer->Active, x, y, TYPE);                     \
                                                                                                 \
    while (n--)                                                                                  \
        {                                                                                        \
        if (mask[0]) d[0] = color;                                                               \
        d++; mask++;                                                                             \
        }                                                                                        \
    }


#define IMPLEMENT_READ_RGBA_SPAN(DEPTH, TYPE)                           \
static void read_rgba_span_##DEPTH (const GLcontext *ctx,				\
                                    GLuint n, GLint x, GLint y,			\
                                    GLubyte rgba[][4])					\
    {																	\
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);				\
    BITMAP          *bmp = context->Buffer->ReadActive;                     \
    TYPE              *d = DESTINATION(bmp, x, y, TYPE);                \
                                                                        \
    while (n--)                                                         \
        {                                                               \
        rgba[0][RCOMP] = getr##DEPTH(d[0]);                             \
        rgba[0][GCOMP] = getg##DEPTH(d[0]);                             \
        rgba[0][BCOMP] = getb##DEPTH(d[0]);                             \
        rgba[0][ACOMP] = 255;                                           \
                                                                        \
        d++; rgba++;                                                    \
        }                                                               \
    }


#define IMPLEMENT_WRITE_RGBA_PIXELS(DEPTH, TYPE)                                                                                 \
static void write_rgba_pixels_##DEPTH (const GLcontext *ctx,                                                                     \
                                       GLuint n,                                                                                 \
                                       const GLint x[],                                                                          \
                                       const GLint y[],                                                                          \
                                       const GLubyte rgba[][4],                                                                  \
                                       const GLubyte mask[])                                                                     \
    {                                                                                                                            \
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);                                                                       \
    BITMAP          *bmp = context->Buffer->Active;                                                                              \
                                                                                                                                 \
    while (n--)                                                                                                                  \
        {                                                                                                                        \
        if (mask[0]) *DESTINATION(bmp, x[0], y[0], TYPE) = makecol##DEPTH(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]);       \
        rgba++; x++; y++; mask++;                                                                                                \
        }                                                                                                                        \
    }



#define IMPLEMENT_WRITE_MONO_RGBA_PIXELS(DEPTH, TYPE)                                                                            \
static void write_mono_rgba_pixels_##DEPTH (const GLcontext *ctx,                                                                \
                                            GLuint n,                                                                            \
                                            const GLint x[],                                                                     \
                                            const GLint y[],                                                                     \
                                            const GLubyte mask[])                                                                \
    {                                                                                                                            \
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);                                                                       \
    TYPE          color  = context->CurrentColor;                                                                                \
    BITMAP          *bmp = context->Buffer->Active;                                                                              \
                                                                                                                                 \
    while (n--)                                                                                                                  \
        {                                                                                                                        \
        if (mask[0]) *DESTINATION(bmp, x[0], y[0], TYPE) = color;                                                                \
        x++; y++; mask++;                                                                                                        \
        }                                                                                                                        \
    }


#define IMPLEMENT_READ_RGBA_PIXELS(DEPTH, TYPE)                         \
static void read_rgba_pixels_##DEPTH (const GLcontext *ctx,				\
                              		  GLuint n,							\
                              		  const GLint x[],					\
                              		  const GLint y[],					\
                              		  GLubyte rgba[][4],				\
                              		  const GLubyte mask[])				\
    {																	\
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);				\
    BITMAP          *bmp = context->Buffer->ReadActive;                     \
                                                                        \
    while (n--)                                                         \
        {                                                               \
        if (mask[0])                                                    \
            {                                                           \
            int color = *DESTINATION(bmp, x[0], y[0], TYPE);            \
                                                                        \
            rgba[0][RCOMP] = getr##DEPTH(color);                        \
            rgba[0][GCOMP] = getg##DEPTH(color);                        \
            rgba[0][BCOMP] = getb##DEPTH(color);                        \
            rgba[0][ACOMP] = 255;                                       \
            }                                                           \
                                                                        \
        x++; y++; rgba++; mask++;                                       \
        }                                                               \
    }

