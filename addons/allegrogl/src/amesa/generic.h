/* These are the functions for interfacing Mesa with Allegro.
 */

#ifdef MESA_5_0
static void clear_color_generic(GLcontext *ctx, const GLfloat color[4])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

	context->ClearColor = makecol(color[0]*255, color[1]*255, color[2]*255);
}
#else
static void clear_color_generic(GLcontext *ctx, const GLchan color[4])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

#if CHAN_TYPE == GL_FLOAT
	context->ClearColor = makecol(color[0]*255, color[1]*255, color[2]*255);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
	context->ClearColor = makecol(color[0]<<8, color[1]<<8, color[2]<<8);
#else
	context->ClearColor = makecol(color[0], color[1], color[2]);
#endif
}
#endif


static void clear_generic(GLcontext *ctx,
                                GLbitfield mask, GLboolean all,
                                GLint x, GLint y,
                                GLint width, GLint height)
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	int color = context->ClearColor;
	BITMAP *buffer = context->Buffer->Active;
	union {
		float zf;
		long zi;
	} _zbuf_clip;

	if (mask & DD_FRONT_LEFT_BIT) {
		buffer = context->Buffer->FrontBuffer;
		mask &= ~ DD_FRONT_LEFT_BIT;
	}

	if (mask & DD_BACK_LEFT_BIT) {
		buffer = context->Buffer->BackBuffer;
		mask &= ~ DD_BACK_LEFT_BIT;
	}
	
	if (mask & DD_DEPTH_BIT) {
		ZBUFFER *depthBuffer = context->Buffer->DepthBuffer;
		
		if (depthBuffer) {
			_zbuf_clip.zf = 1. / (context->ClearDepth * context->zFar);
			if (all)
				clear_to_color(depthBuffer, _zbuf_clip.zi);
			else
				rectfill(depthBuffer, x, y, x+width-1, y+height-1, _zbuf_clip.zi);
			mask &= ~ DD_DEPTH_BIT;
		}
	}

	if (all)
		clear_to_color(buffer, color);
	else
		rectfill(buffer, x, y, x+width-1, y+height-1, color);

	/* Call swrast if there is anything left to clear (like DEPTH) */
	if (mask)
		_swrast_Clear( ctx, mask, all, x, y, width, height );

	return;
}


static void write_rgba_span_generic(const GLcontext *ctx,
                                    GLuint n, GLint x, GLint y,
                                    const GLubyte rgba[][4],
                                    const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;

	y = FLIP(context, y);

	if (mask) {
		while (n--) {
			if (mask[0])
				putpixel(bmp, x, y, makecol(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]));
			x++; mask++; rgba++;
		}
	}
	else {
		while (n--) {
			putpixel(bmp, x, y, makecol(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]));
			x++; rgba++;
		}
	}
}


static void write_rgb_span_generic(const GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLubyte rgb[][3],
                                   const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;

	y = FLIP(context, y);

	if (mask) {
		while(n--) {
			if (mask[0])
				putpixel(bmp, x, y, makecol(rgb[0][RCOMP], rgb[0][GCOMP], rgb[0][BCOMP]));
			x++; mask++; rgb++;
		}
	}
	else {
		while (n--) {
			putpixel(bmp, x, y, makecol(rgb[0][RCOMP], rgb[0][GCOMP], rgb[0][BCOMP]));
			x++; rgb++;
		}
	}
}


static void write_mono_rgba_span_generic(const GLcontext *ctx,
                                         GLuint n, GLint x, GLint y,
                                         const GLchan colour[4],
					 const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;
#if CHAN_TYPE == GL_FLOAT
	int color = makecol(colour[0]*255, colour[1]*255, colour[2]*255);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
	int color = makecol(colour[0]<<8, colour[1]<<8, colour[2]<<8);
#else
	int color = makecol(colour[0], colour[1], colour[2]);
#endif

	y = FLIP(context, y);

	if (mask) {
		while(n--) {
			if (mask[0])
				putpixel(bmp, x, y, color);
			x++; mask++;
		}
	}
	else
		hline(bmp, x, y, x+n-1, color);
}


static void read_rgba_span_generic(const GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   GLubyte rgba[][4])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->ReadActive;

	y = FLIP(context, y);

	while (n--) {
		int color = getpixel(bmp, x, y);

		rgba[0][RCOMP] = getr(color);
		rgba[0][GCOMP] = getg(color);
		rgba[0][BCOMP] = getb(color);
		rgba[0][ACOMP] = 255;

		x++; rgba++;
	}
}


static void write_rgba_pixels_generic(const GLcontext *ctx,
                                      GLuint n,
                                      const GLint x[],
                                      const GLint y[],
                                      const GLubyte rgba[][4],
                                      const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;

	while (n--) {
		if (mask[0])
			putpixel(bmp, x[0], FLIP(context, y[0]), makecol(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]));
		x++; y++; mask++; rgba++;
	}
}


static void write_mono_rgba_pixels_generic(const GLcontext *ctx,
                                           GLuint n,
                                           const GLint x[],
                                           const GLint y[],
					   const GLchan colour[4],
                                           const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;
#if CHAN_TYPE == GL_FLOAT
	int color = makecol(colour[0]*255, colour[1]*255, colour[2]*255);
#elif CHAN_TYPE == GL_UNSIGNED_SHORT
	int color = makecol(colour[0]<<8, colour[1]<<8, colour[2]<<8);
#else
	int color = makecol(colour[0], colour[1], colour[2]);
#endif

	while (n--) {
		if (mask[0])
			putpixel(bmp, x[0], FLIP(context, y[0]), color);
		x++; y++; mask++;
	}
}


static void read_rgba_pixels_generic(const GLcontext *ctx,
                                     GLuint n,
                                     const GLint x[],
                                     const GLint y[],
                                     GLubyte rgba[][4],
                                     const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->ReadActive;

	while (n--) {
		if (mask[0]) {
			int color = getpixel(bmp, x[0], FLIP(context, y[0]));

			rgba[0][RCOMP] = getr(color);
			rgba[0][GCOMP] = getg(color);
			rgba[0][BCOMP] = getb(color);
			rgba[0][ACOMP] = 255;
		}

		x++; y++; mask++; rgba++;
	}
}
