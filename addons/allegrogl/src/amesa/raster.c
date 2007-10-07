#include <allegro.h>
#include "GL/amesa.h"
#include "raster.h"

/**********************************************************************/
/*****                   drawing functions                        *****/
/**********************************************************************/

#define FLIP(context, y)  (context->Buffer->Height - (y) - 1)

#include "generic.h"


/**********************************************************************/
/*****              Miscellaneous device driver funcs             *****/
/*****           Note that these functions are mandatory          *****/
/**********************************************************************/

/* Return a string as needed by glGetString().
 * Only the GL_RENDERER token must be implemented.  Otherwise,
 * NULL can be returned.
 */
static const GLubyte* AMesaGetString(GLcontext *ctx, GLenum name)
{
	switch (name) {
		case GL_RENDERER:
			return (const GLubyte *) "Allegro 4.0 Driver for Mesa";
		default:
			return NULL;
	}
}



/*
 * Called whenever glClearIndex() is called.  Set the index for clearing
 * the color buffer when in color index mode.
 */
static void AMesaClearIndex(GLcontext *ctx, GLuint index)
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	context->ClearIndex = index;
}



/*
 * Specifies the current buffer for writing.
 * The following values must be accepted when applicable:
 *    GL_FRONT_LEFT - this buffer always exists
 *    GL_BACK_LEFT - when double buffering
 *    GL_FRONT_RIGHT - when using stereo
 *    GL_BACK_RIGHT - when using stereo and double buffering
 * The folowing values may optionally be accepted.  Return GL_TRUE
 * if accepted, GL_FALSE if not accepted.  In practice, only drivers
 * which can write to multiple color buffers at once should accept
 * these values.
 *    GL_FRONT - write to front left and front right if it exists
 *    GL_BACK - write to back left and back right if it exists
 *    GL_LEFT - write to front left and back left if it exists
 *    GL_RIGHT - write to right left and back right if they exist
 *    GL_FRONT_AND_BACK - write to all four buffers if they exist
 *    GL_NONE - disable buffer write in device driver.
 */
#ifdef MESA_4_0_2
static void AMesaSetDrawBuffer(GLcontext *ctx, GLenum buffer)
#else
static GLboolean AMesaSetDrawBuffer(GLcontext *ctx, GLenum buffer)
#endif
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

	switch(buffer) {
		case GL_FRONT :
		case GL_FRONT_LEFT :
			context->Buffer->Active = context->Buffer->FrontBuffer;
#ifdef MESA_4_0_2
 			return;
#else
 			return GL_TRUE;
#endif

		case GL_BACK :
		case GL_BACK_LEFT :
			if (context->Buffer->BackBuffer) {
				context->Buffer->Active = context->Buffer->BackBuffer;
#ifdef MESA_4_0_2
				return;
#else
				return GL_TRUE;
#endif
		}

		default :
#ifdef MESA_4_0_2
			return;
#else
			return GL_FALSE;
#endif
	}
}



/*
 * Specifies the current buffer for reading.
 * colorBuffer will be one of:
 *    GL_FRONT_LEFT - this buffer always exists
 *    GL_BACK_LEFT - when double buffering
 *    GL_FRONT_RIGHT - when using stereo
 *    GL_BACK_RIGHT - when using stereo and double buffering
 */
#ifdef MESA_5_0
static void AMesaSetReadBuffer(GLcontext *ctx, GLenum buffer)
#else
static void AMesaSetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                                    GLenum buffer)
#endif
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

	switch(buffer) {
		case GL_FRONT :
		case GL_FRONT_LEFT :
			context->Buffer->ReadActive = context->Buffer->FrontBuffer;
			return;

		case GL_BACK :
		case GL_BACK_LEFT :
			if (context->Buffer->BackBuffer) {
				context->Buffer->ReadActive = context->Buffer->BackBuffer;
				return;
		}

		default :
			return;
	}
}



/* 
 * Tell the device driver where to read/write spans.
 */
#ifdef MESA_5_0
static void AMesaSetBuffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
}
#endif

/*
 * Returns the width and height of the current color buffer.
 */
#ifdef MESA_4_0_2
static void AMesaGetBufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
	GET_CURRENT_CONTEXT(ctx);
#else
static void AMesaGetBufferSize(GLcontext *ctx, GLuint *width, GLuint *height)
{
#endif
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

	*width  = context->Buffer->Width;
	*height = context->Buffer->Height;
}

/* Write a horizontal run of CI pixels.  One function is for 32bpp
 * indexes and the other for 8bpp pixels (the common case).  You must
 * implement both for color index mode.
 */
static void AMesaWriteCI32Span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLuint index[], const GLubyte mask[] )
{
	/* Not yet implemented */
}
static void AMesaWriteCI8Span( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         const GLubyte index[], const GLubyte mask[] )
{
	/* Not yet implemented */
}

/* Write a horizontal run of color index pixels using the color index
 * last specified by the Index() function.
 */
static void AMesaWriteMonoCISpan( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            GLuint colorIndex, const GLubyte mask[] )
{
	/* Not yet implemented */
}

/*
 * Write a random array of CI pixels.
 */
static void AMesaWriteCI32Pixels( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLuint index[], const GLubyte mask[] )
{
	/* Not yet implemented */
}

   /* Write a random array of color index pixels using the color index
    * last specified by the Index() function.
    */
static void AMesaWriteMonoCIPixels( const GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint colorIndex, const GLubyte mask[] )
{
	/* Not yet implemented */
}

/* Read a horizontal run of color index pixels.
 */
static void AMesaReadCI32Span( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint index[])
{
	/* Not yet implemented */
}

/* Read a random array of CI pixels.
 */
static void AMesaReadCI32Pixels( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint indx[], const GLubyte mask[] )
{
	/* Not yet implemented */
}

static void AMesaClearDepth(GLcontext *ctx, GLclampd depth)
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

	context->ClearDepth = depth * (2 << context->Visual->GLVisual->depthBits);
}

static void AMesaDepthRange(GLcontext *ctx, GLclampd nearval, GLclampd farval)
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

	context->zFar = farval;
	context->zNear = nearval;
}

/* Write a horizontal span of values into the depth buffer.  Only write
 * depth[i] value if mask[i] is nonzero.
 */
static void AMesaWriteDepthSpan( GLcontext *ctx, GLuint n, GLint x, GLint y,
	const GLdepth depth[], const GLubyte mask[])
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;
	float* zb_addr = (float*)(bmp_write_line(context->Buffer->DepthBuffer, bmp->h - y) + x * sizeof(float));
	
	while (n--) {
		if (mask[0])
			zb_addr[0] = 1. / depth[0];
		zb_addr++;
		mask++;
		depth++;
	}
}

/* Read a horizontal span of values from the depth buffer.
 */
static void AMesaReadDepthSpan( GLcontext *ctx, GLuint n, GLint x, GLint y,
	GLdepth depth[] )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;
	float* zb_addr = (float*)(bmp_write_line(context->Buffer->DepthBuffer, bmp->h - y) + x * sizeof(float));
	
	while (n--) {
		depth[0] = (GLdepth) (1. / zb_addr[0]);
		zb_addr++;
		depth++;
	}
}

/* Write an array of randomly positioned depth values into the
 * depth buffer.  Only write depth[i] value if mask[i] is nonzero.
 */
static void AMesaWriteDepthPixels( GLcontext *ctx, GLuint n,
	const GLint x[], const GLint y[],
	const GLdepth depth[], const GLubyte mask[] )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;
	ZBUFFER *zbuf = context->Buffer->DepthBuffer;
	float* zb_addr;
	
	while (n--) {
		if (mask[0]) {
			zb_addr = (float*)(bmp_write_line(zbuf, bmp->h - y[0]) + x[0] * sizeof(float));
			zb_addr[0] = 1. / depth[0];
		}
		x++; y++;
		mask++;
		depth++;
	}
}

/* Read an array of randomly positioned depth values from the depth buffer.
 */
static void AMesaReadDepthPixels( GLcontext *ctx, GLuint n,
	const GLint x[], const GLint y[],
	GLdepth depth[] )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *bmp = context->Buffer->Active;
	ZBUFFER *zbuf = context->Buffer->DepthBuffer;
	float* zb_addr;
	
	while (n--) {
		zb_addr = (float*)(bmp_write_line(zbuf, bmp->h - y[0]) + x[0] * sizeof(float));
		depth[0] = (GLdepth) (1. / zb_addr[0]);
		x++; y++;
		depth++;
	}
}



/**********************************************************************/
/**********************************************************************/

/* Override for the swrast triangle-selection function.  Try to use one
 * of our internal triangle functions, otherwise fall back to the
 * standard swrast functions.
 */
static void AMesaFlatLine(GLcontext *ctx,
				  const SWvertex *v0,
                                  const SWvertex *v1)
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	BITMAP *buffer = context->Buffer->Active;
	int color_depth = bitmap_color_depth(buffer);
	int color = makecol_depth(color_depth, v0->color[RCOMP], v0->color[GCOMP], v0->color[BCOMP]);
	
	line(buffer, v0->win[0], buffer->h - v0->win[1], v1->win[0], 
		buffer->h - v1->win[1], color);
}

static void AMesaSmoothTriangle(GLcontext *ctx,
				  const SWvertex *v0,
                                  const SWvertex *v1,
                                  const SWvertex *v2 )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	V3D_f vtx1, vtx2, vtx3;
	BITMAP *buffer = context->Buffer->Active;
	int color_depth = bitmap_color_depth(buffer);

	vtx1.x = v0->win[0];
	vtx1.y = buffer->h - v0->win[1];
	vtx1.z = v0->win[2];
	vtx1.c = makecol_depth(color_depth, v0->color[RCOMP], v0->color[GCOMP], v0->color[BCOMP]);
	vtx2.x = v1->win[0];
	vtx2.y = buffer->h - v1->win[1];
	vtx2.z = v1->win[2];
	vtx2.c = makecol_depth(color_depth, v1->color[RCOMP], v1->color[GCOMP], v1->color[BCOMP]);
	vtx3.x = v2->win[0];
	vtx3.y = buffer->h - v2->win[1];
	vtx3.z = v2->win[2];
	vtx3.c = makecol_depth(color_depth, v2->color[RCOMP], v2->color[GCOMP], v2->color[BCOMP]);
	
	triangle3d_f(buffer, POLYTYPE_GCOL, NULL, &vtx1, &vtx2, &vtx3);
}

static void AMesaFlatTriangle(GLcontext *ctx,
				  const SWvertex *v0,
                                  const SWvertex *v1,
                                  const SWvertex *v2 )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	V3D_f vtx1, vtx2, vtx3;
	BITMAP *buffer = context->Buffer->Active;
	int color_depth = bitmap_color_depth(buffer);
	
	vtx1.x = v0->win[0];
	vtx1.y = buffer->h - v0->win[1];
	vtx1.z = v0->win[2];
	vtx1.c = makecol_depth(color_depth, v0->color[RCOMP], v0->color[GCOMP], v0->color[BCOMP]);
	vtx2.x = v1->win[0];
	vtx2.y = buffer->h - v1->win[1];
	vtx2.z = v1->win[2];
	vtx3.x = v2->win[0];
	vtx3.y = buffer->h - v2->win[1];
	vtx3.z = v2->win[2];
	
	triangle3d_f(buffer, POLYTYPE_FLAT, NULL, &vtx1, &vtx2, &vtx3);
}

static void AMesaSmoothZTriangle(GLcontext *ctx,
				  const SWvertex *v0,
                                  const SWvertex *v1,
                                  const SWvertex *v2 )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	V3D_f vtx1, vtx2, vtx3;
	BITMAP *buffer = context->Buffer->Active;
	int color_depth = bitmap_color_depth(buffer);
	
	vtx1.x = v0->win[0];
	vtx1.y = buffer->h - v0->win[1];
	vtx1.z = v0->win[2];
	vtx1.c = makecol_depth(color_depth, v0->color[RCOMP], v0->color[GCOMP], v0->color[BCOMP]);
	vtx2.x = v1->win[0];
	vtx2.y = buffer->h - v1->win[1];
	vtx2.z = v1->win[2];
	vtx2.c = makecol_depth(color_depth, v1->color[RCOMP], v1->color[GCOMP], v1->color[BCOMP]);
	vtx3.x = v2->win[0];
	vtx3.y = buffer->h - v2->win[1];
	vtx3.z = v2->win[2];
	vtx3.c = makecol_depth(color_depth, v2->color[RCOMP], v2->color[GCOMP], v2->color[BCOMP]);
	
	triangle3d_f(buffer, POLYTYPE_GCOL | POLYTYPE_ZBUF, NULL, &vtx1, &vtx2, &vtx3);
}

static void AMesaFlatZTriangle(GLcontext *ctx,
				  const SWvertex *v0,
                                  const SWvertex *v1,
                                  const SWvertex *v2 )
{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);
	V3D_f vtx1, vtx2, vtx3;
	BITMAP *buffer = context->Buffer->Active;
	int color_depth = bitmap_color_depth(buffer);
	
	vtx1.x = v0->win[0];
	vtx1.y = buffer->h - v0->win[1];
	vtx1.z = v0->win[2];
	vtx1.c = makecol_depth(color_depth, v0->color[RCOMP], v0->color[GCOMP], v0->color[BCOMP]);
	vtx2.x = v1->win[0];
	vtx2.y = buffer->h - v1->win[1];
	vtx2.z = v1->win[2];
	vtx3.x = v2->win[0];
	vtx3.y = buffer->h - v2->win[1];
	vtx3.z = v2->win[2];
	
	triangle3d_f(buffer, POLYTYPE_FLAT | POLYTYPE_ZBUF, NULL, &vtx1, &vtx2, &vtx3);
}

static swrast_line_func AMesaChooseLineFunction(GLcontext *ctx)
{
	const SWcontext *swrast = SWRAST_CONTEXT(ctx);

	if (ctx->RenderMode != GL_RENDER)
		return (swrast_line_func) NULL;
	if (ctx->Line.SmoothFlag)
		return (swrast_line_func) NULL;
	if (ctx->Line.StippleFlag)
		return (swrast_line_func) NULL;
	if (ctx->Line.Width != 1.0f)
		return (swrast_line_func) NULL;
#ifdef MESA_5_0
	if (ctx->Texture._EnabledUnits)
		return (swrast_line_func) NULL;
#else
	if (ctx->Texture._ReallyEnabled)
		return (swrast_line_func) NULL;
#endif
	if (ctx->Visual.depthBits != DEFAULT_SOFTWARE_DEPTH_BITS)
		return (swrast_line_func) NULL;
	if (swrast->_RasterMask != DEPTH_BIT &&
	    ctx->Light.ShadeModel != GL_SMOOTH) {
		return AMesaFlatLine;
	}

	return (swrast_line_func) NULL;
}

static swrast_tri_func AMesaChooseTriangleFunction(GLcontext *ctx)
{
	const SWcontext *swrast = SWRAST_CONTEXT(ctx);

	if (ctx->RenderMode != GL_RENDER)
		return (swrast_tri_func) NULL;
	if (ctx->Polygon.SmoothFlag)
		return (swrast_tri_func) NULL;
	if (ctx->Polygon.StippleFlag)
		return (swrast_tri_func) NULL;
#ifdef MESA_5_0
	if (ctx->Texture._EnabledUnits)
		return (swrast_tri_func) NULL;
#else
	if (ctx->Texture._ReallyEnabled)
		return (swrast_tri_func) NULL;
#endif
	if (ctx->Polygon.CullFlag &&
	    ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
	  return (swrast_tri_func) NULL;
	if (swrast->_RasterMask & DEPTH_BIT) {
		if (ctx->Depth.Func == GL_LESS &&
		    ctx->Depth.Mask == GL_TRUE) {
			if (ctx->Light.ShadeModel == GL_SMOOTH)
				return AMesaSmoothZTriangle;
			else
				return AMesaFlatZTriangle;
		}
	}
	else {
		if (ctx->Light.ShadeModel == GL_SMOOTH)
			return AMesaSmoothTriangle;
		else
			return AMesaFlatTriangle;
	}
	return (swrast_tri_func) NULL;
}

void AMesaChooseTriangle( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Triangle = AMesaChooseTriangleFunction( ctx );
   if (!swrast->Triangle)
      _swrast_choose_triangle( ctx );
}

void AMesaChooseLine( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   swrast->Line = AMesaChooseLineFunction( ctx );
   if (!swrast->Line)
      _swrast_choose_line( ctx );
}



/**********************************************************************/
/**********************************************************************/

void AMesaUpdateState(GLcontext *ctx, GLuint new_state)
{
	struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference( ctx );
	TNLcontext *tnl = TNL_CONTEXT(ctx);

	/* Initialize all the pointers in the driver struct. Do this whenever
	 * a new context is made current or we change buffers via set_buffer!
	 */
   
	ctx->Driver.GetString           = AMesaGetString;
	ctx->Driver.UpdateState         = AMesaUpdateState;
#ifdef MESA_5_0
	ctx->Driver.DrawBuffer          = AMesaSetDrawBuffer;
	ctx->Driver.ReadBuffer          = AMesaSetReadBuffer;
	swdd->SetBuffer			= AMesaSetBuffer;
#else
	ctx->Driver.SetDrawBuffer       = AMesaSetDrawBuffer;
	swdd->SetReadBuffer       	= AMesaSetReadBuffer;
#endif
	ctx->Driver.ClearIndex          = AMesaClearIndex;
	ctx->Driver.ClearColor          = clear_color_generic;
	ctx->Driver.Clear               = clear_generic;
	ctx->Driver.GetBufferSize       = AMesaGetBufferSize;

#ifdef MESA_4_0_2
	ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
#else
	ctx->Driver.ResizeBuffersMESA = _swrast_alloc_buffers;
#endif

	ctx->Driver.Accum = _swrast_Accum;
	ctx->Driver.Bitmap = _swrast_Bitmap;
	ctx->Driver.CopyPixels = _swrast_CopyPixels;
	ctx->Driver.DrawPixels = _swrast_DrawPixels;
	ctx->Driver.ReadPixels = _swrast_ReadPixels;

	ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
	ctx->Driver.TexImage1D = _mesa_store_teximage1d;
	ctx->Driver.TexImage2D = _mesa_store_teximage2d;
	ctx->Driver.TexImage3D = _mesa_store_teximage3d;
	ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
	ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
	ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
	ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

	ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
	ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
	ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
	ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
	ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
	ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
	ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
	ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
	ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

#ifdef MESA_5_0
	ctx->Driver.CompressedTexImage1D = _mesa_store_compressed_teximage1d;
	ctx->Driver.CompressedTexImage2D = _mesa_store_compressed_teximage2d;
	ctx->Driver.CompressedTexImage3D = _mesa_store_compressed_teximage3d;
	ctx->Driver.CompressedTexSubImage1D = _mesa_store_compressed_texsubimage1d;
	ctx->Driver.CompressedTexSubImage2D = _mesa_store_compressed_texsubimage2d;
	ctx->Driver.CompressedTexSubImage3D = _mesa_store_compressed_texsubimage3d;
#else
	ctx->Driver.BaseCompressedTexFormat = _mesa_base_compressed_texformat;
	ctx->Driver.CompressedTextureSize = _mesa_compressed_texture_size;
	ctx->Driver.GetCompressedTexImage = _mesa_get_compressed_teximage;
#endif

	swdd->WriteCI32Span       = AMesaWriteCI32Span;
	swdd->WriteCI8Span        = AMesaWriteCI8Span;
	swdd->WriteMonoCISpan     = AMesaWriteMonoCISpan;
	swdd->WriteCI32Pixels     = AMesaWriteCI32Pixels;
	swdd->WriteMonoCIPixels   = AMesaWriteMonoCIPixels;
	swdd->ReadCI32Span        = AMesaReadCI32Span;
	swdd->ReadCI32Pixels      = AMesaReadCI32Pixels;

	swdd->WriteRGBASpan       = write_rgba_span_generic;
	swdd->WriteRGBSpan        = write_rgb_span_generic;
	swdd->WriteMonoRGBASpan   = write_mono_rgba_span_generic;
	swdd->WriteRGBAPixels     = write_rgba_pixels_generic;
	swdd->WriteMonoRGBAPixels = write_mono_rgba_pixels_generic;
	swdd->ReadRGBASpan        = read_rgba_span_generic;
	swdd->ReadRGBAPixels      = read_rgba_pixels_generic;
	
	ctx->Driver.ClearDepth	  = AMesaClearDepth;
	ctx->Driver.DepthRange	  = AMesaDepthRange;
	swdd->WriteDepthSpan	  = AMesaWriteDepthSpan;
	swdd->ReadDepthSpan	  = AMesaReadDepthSpan;
	swdd->WriteDepthPixels	  = AMesaWriteDepthPixels;
	swdd->ReadDepthPixels	  = AMesaReadDepthPixels;

	tnl->Driver.RunPipeline = _tnl_run_pipeline;

	_swrast_InvalidateState( ctx, new_state );
	_swsetup_InvalidateState( ctx, new_state );
	_ac_InvalidateState( ctx, new_state );
	_tnl_InvalidateState( ctx, new_state );
}
