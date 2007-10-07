/*
 * This the Mesa driver for Allegro 4.0
 * It is based on the work of Bernhard Tsirren
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glext.h"
#include "GL/amesa.h"
#include "raster.h"


typedef struct _amesa_extension_data {
	void* proc;
	char* name;
} amesa_extension_data;

amesa_extension_data amesa_extension[] = {
#ifdef GL_EXT_polygon_offset
   { (void*)glPolygonOffsetEXT,			"glPolygonOffsetEXT"		},
#endif
   { (void*)glBlendEquationEXT,			"glBlendEquationEXT"		},
   { (void*)glBlendColorEXT,			"glBlendColorExt"		},
   { (void*)glVertexPointerEXT,			"glVertexPointerEXT"		},
   { (void*)glNormalPointerEXT,			"glNormalPointerEXT"		},
   { (void*)glColorPointerEXT,			"glColorPointerEXT"		},
   { (void*)glIndexPointerEXT,			"glIndexPointerEXT"		},
   { (void*)glTexCoordPointerEXT,		"glTexCoordPointer"		},
   { (void*)glEdgeFlagPointerEXT,		"glEdgeFlagPointerEXT"		},
   { (void*)glGetPointervEXT,			"glGetPointervEXT"		},
   { (void*)glArrayElementEXT,			"glArrayElementEXT"		},
   { (void*)glDrawArraysEXT,			"glDrawArrayEXT"		},
   { (void*)glAreTexturesResidentEXT,		"glAreTexturesResidentEXT"	},
   { (void*)glBindTextureEXT,			"glBindTextureEXT"		},
   { (void*)glDeleteTexturesEXT,		"glDeleteTexturesEXT"		},
   { (void*)glGenTexturesEXT,			"glGenTexturesEXT"		},
   { (void*)glIsTextureEXT,			"glIsTextureEXT"		},
   { (void*)glPrioritizeTexturesEXT,		"glPrioritizeTexturesEXT"	},
   { (void*)glCopyTexSubImage3DEXT,		"glCopyTexSubImage3DEXT"	},
   { (void*)glTexImage3DEXT,			"glTexImage3DEXT"		},
   { (void*)glTexSubImage3DEXT,			"glTexSubImage3DEXT"		},
   { (void*)glColorTableEXT,			"glColorTableEXT"		},
   { (void*)glColorSubTableEXT,			"glColorSubTableEXT"		},
   { (void*)glGetColorTableEXT,			"glGetColorTableEXT"		},
   { (void*)glGetColorTableParameterfvEXT,	"glGetColorTableParameterfvEXT"	},
   { (void*)glGetColorTableParameterivEXT,	"glGetColorTableParameterivEXT"	},
   { (void*)glPointParameterfEXT,		"glPointParameterfEXT"		},
   { (void*)glPointParameterfvEXT,		"glPointParameterfvEXT"		},
   { (void*)glBlendFuncSeparateEXT,		"glBlendFuncSeparateEXT"	},
   { (void*)glLockArraysEXT,			"glLockArraysEXT"		},
   { (void*)glUnlockArraysEXT,			"glUnlockArraysEXT"		},
   { NULL,					'\0'				}
};

#define AMESA_NEW_LINE   (_NEW_LINE | \
                           _NEW_TEXTURE | \
                           _NEW_LIGHT | \
                           _NEW_DEPTH | \
                           _NEW_RENDERMODE | \
                           _SWRAST_NEW_RASTERMASK)

#define AMESA_NEW_TRIANGLE (_NEW_POLYGON | \
                             _NEW_TEXTURE | \
                             _NEW_LIGHT | \
                             _NEW_DEPTH | \
                             _NEW_RENDERMODE | \
                             _SWRAST_NEW_RASTERMASK)

AMesaContext AMesa = NULL;    /* the current context */

/**********************************************************************/
/*****                AMesa Public API Functions                  *****/
/**********************************************************************/

AMesaVisual GLAPIENTRY
AMesaCreateVisual(GLboolean dbFlag, GLint depth, GLint rgba,
                  GLint depthSize, GLint stencilSize, GLint accumSizeRed,
		  GLint accumSizeGreen, GLint accumSizeBlue, GLint accumSizeAlpha)
{
	AMesaVisual visual;
	GLbyte redBits, greenBits, blueBits, alphaBits, indexBits;

	if ((!rgba) && (depthSize != 8))
		return NULL;

	visual = (AMesaVisual) malloc(sizeof(struct amesa_visual));
	if (!visual)
		return NULL;

	switch (depth) {

		case 8:
			if (rgba) {
				redBits   = 3;
				greenBits = 3;
				blueBits  = 2;
				alphaBits = 0;
				indexBits = 0;
			}
			else {
				redBits   = 0;
				greenBits = 0;
				blueBits  = 0;
				alphaBits = 0;
				indexBits = 8;
			}
			break;

		case 15:
			redBits   = 5;
			greenBits = 5;
			blueBits  = 5;
			alphaBits = 0;
			indexBits = 0;
			break;

		case 16:
			redBits   = 5;
			greenBits = 6;
			blueBits  = 5;
			alphaBits = 0;
			indexBits = 0;
			break;

		case 24:
			redBits   = 8;
			greenBits = 8;
			blueBits  = 8;
			alphaBits = 0;
			indexBits = 0;
			break;

		case 32:
			redBits   = 8;
			greenBits = 8;
			blueBits  = 8;
			alphaBits = 8;
			indexBits = 0;
		break;

		default:
			free(visual);
			return NULL;
	}

	if (depthSize)
		depthSize = 16;
	
	visual->DBFlag = dbFlag;
	visual->Depth = depth;
	visual->GLVisual = _mesa_create_visual(rgba,    /* rgb mode       */
   					dbFlag,		/* double buffer */
					GL_FALSE,	/* stereo*/
					redBits,
					greenBits,
					blueBits,
					alphaBits,
					indexBits,
					depthSize,
					stencilSize,
					accumSizeBlue,
					accumSizeRed,
					accumSizeGreen,
					alphaBits ? accumSizeAlpha : 0,
					1);
	if (!visual->GLVisual) {
		free(visual);
		return NULL;
	}

	return visual;
}



void GLAPIENTRY AMesaDestroyVisual(AMesaVisual visual)
{
	_mesa_destroy_visual(visual->GLVisual);
	free(visual);
}



AMesaBuffer GLAPIENTRY AMesaCreateBuffer(AMesaVisual visual, BITMAP* bmp)
{
	AMesaBuffer buffer;

	/* Buffer and visual must share the same color depth */
	if (bitmap_color_depth(bmp) != visual->Depth)
		return NULL;

	buffer = (AMesaBuffer) malloc(sizeof(struct amesa_buffer));
	if (!buffer)
		return NULL;

	buffer->FrontBuffer    = bmp;
	buffer->BackBuffer     = NULL;
	buffer->Active         = NULL;
	buffer->ReadActive     = NULL;
	buffer->DepthBuffer    = NULL;

	if (visual->DBFlag) {
		buffer->BackBuffer = create_bitmap_ex(visual->Depth, bmp->w, bmp->h);
		if (!buffer->BackBuffer) {
			free(buffer);
			return NULL;
		}
	}
	
	if (visual->GLVisual->depthBits > 0) {
		buffer->DepthBuffer = create_zbuffer(bmp);
		if (!buffer->DepthBuffer) {
			if (buffer->BackBuffer)
				destroy_bitmap(buffer->BackBuffer);
			free(buffer);
			return NULL;
		}
		set_zbuffer(buffer->DepthBuffer);
	}

	buffer->GLBuffer = _mesa_create_framebuffer(visual->GLVisual,
                         0,   				    /*depth buffer is handled by Allegro */
                         visual->GLVisual->stencilBits > 0, /*software stencil buffer*/
                         visual->GLVisual->accumRedBits > 0,/*software accum buffer*/
                         GL_FALSE ); /*software alpha buffer*/

	if (!buffer->GLBuffer) {
		if (buffer->BackBuffer)
			destroy_bitmap(buffer->BackBuffer);
		if (buffer->DepthBuffer)
			destroy_zbuffer(buffer->DepthBuffer);
		free(buffer);
      		return NULL;
	}

	buffer->Width  = bmp->w;
	buffer->Height = bmp->h;

	return buffer;
}



void GLAPIENTRY AMesaDestroyBuffer(AMesaBuffer buffer)
{
	if (buffer->BackBuffer)
		destroy_bitmap(buffer->BackBuffer);
	if (buffer->DepthBuffer)
		destroy_zbuffer(buffer->DepthBuffer);

	_mesa_destroy_framebuffer(buffer->GLBuffer);
	free(buffer);
}



/* Extend the software rasterizer with our line and triangle
 * functions.
 */
static void AMesaRegisterSwrastFunctions( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );

   swrast->choose_line = AMesaChooseLine;
   swrast->choose_triangle = AMesaChooseTriangle;

   swrast->invalidate_line |= AMESA_NEW_LINE;
   swrast->invalidate_triangle |= AMESA_NEW_TRIANGLE;
}



AMesaContext GLAPIENTRY AMesaCreateContext(AMesaVisual visual, AMesaContext share)
{
	AMesaContext context;
	GLboolean direct = GL_FALSE;
	GLcontext *ctx;

	context = (AMesaContext) malloc(sizeof(struct amesa_context));
	if (!context)
		return NULL;

	context->GLContext = _mesa_create_context(visual->GLVisual,
                                             share ? share->GLContext : NULL,
                                             (void*)context,
                                             direct);
	if (!context->GLContext) {
		free(context);
		return NULL;
	}

	context->Visual = visual;

	ctx = context->GLContext;
	_mesa_enable_sw_extensions(ctx);
	_mesa_enable_1_3_extensions(ctx);
   
	/* Initialize the software rasterizer and helper modules. */

	_swrast_CreateContext( ctx );
	_ac_CreateContext( ctx );
	_tnl_CreateContext( ctx );
	_swsetup_CreateContext( ctx );
	
	_swsetup_Wakeup( ctx );
	/*AMesaRegisterSwrastFunctions(ctx);*/

	return context;
}



void GLAPIENTRY AMesaDestroyContext(AMesaContext context)
{
	if (context) {
		_swsetup_DestroyContext(context->GLContext);
		_tnl_DestroyContext(context->GLContext);
		_ac_DestroyContext(context->GLContext);
		_swrast_DestroyContext(context->GLContext);

		_mesa_free_context_data(context->GLContext);
		free(context);
	}
}



/*
 * Make the specified context and buffer the current one.
 */
GLboolean GLAPIENTRY AMesaMakeCurrent(AMesaContext context, AMesaBuffer buffer)
{
	if (context && buffer) {
		context->Buffer = buffer;
		buffer->Active  = buffer->BackBuffer ? buffer->BackBuffer : buffer->FrontBuffer;
		buffer->ReadActive = buffer->Active;
		context->ClearDepth =  2 << context->Visual->GLVisual->depthBits;
		context->zNear = 0.;
		context->zFar = 1.;
		context->ClearColor = 0;
		context->ClearIndex = 0;
		context->ColorIndex = 0;
        
		AMesaUpdateState(context->GLContext, ~0);
		_mesa_make_current(context->GLContext, buffer->GLBuffer);
		if (context->GLContext->Viewport.Width == 0 || 
		    context->GLContext->Viewport.Height == 0) {
			/* initialize viewport and scissor box to buffer size */
			_mesa_Viewport(0, 0, buffer->Width, buffer->Height);
			context->GLContext->Scissor.Width = buffer->Width;
			context->GLContext->Scissor.Height = buffer->Height;
		}
		AMesa = context;
	}
	else {
		if (context) {
			/* Detach */
			context->Buffer->Active = NULL;
			context->Buffer->ReadActive = NULL;
			context->Buffer = NULL;
			_mesa_make_current(NULL, NULL);
			AMesa = NULL;
		}
		else
			return GL_FALSE;
	}

	return GL_TRUE;
}



void GLAPIENTRY AMesaSwapBuffers(AMesaBuffer buffer)
{
	if (buffer->BackBuffer)
		blit(buffer->BackBuffer, buffer->FrontBuffer,
		0, 0, 0, 0, buffer->Width, buffer->Height);
}



AMesaContext GLAPIENTRY AMesaGetCurrentContext (void)
{
	return AMesa;
}



BITMAP* GLAPIENTRY AMesaGetColorBuffer(AMesaBuffer buffer, GLenum mode)
{
	switch (mode) {
		case AMESA_FRONT:
			return buffer->FrontBuffer;

		case AMESA_BACK:
			if (buffer->BackBuffer)
				return buffer->BackBuffer;
			else
				return NULL;

		case AMESA_ACTIVE:
			return buffer->Active;

		default:
			return NULL;
	}
}

void* GLAPIENTRY AMesaGetProcAddress(AL_CONST char *name)
{
	int i = 0;
	
	while (amesa_extension[i].proc) {
		if (ustrcmp(amesa_extension[i].name, name) == 0)
			return amesa_extension[i].proc;
		i++;
	}
	
	return NULL;
}
