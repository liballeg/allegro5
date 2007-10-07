#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "texstore.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast/s_context.h"
#include "swrast/s_depth.h"
#include "swrast/s_lines.h"
#include "swrast/s_triangle.h"
#include "swrast/s_trispan.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"


typedef struct amesa_visual
{
   GLvisual*   GLVisual;       /* inherit from GLvisual      */
   GLboolean   DBFlag;         /* double buffered?           */
   GLint       Depth;          /* bits per pixel ( >= 15 )   */
} amesa_visual;


typedef struct amesa_buffer
{
   GLframebuffer* GLBuffer;    /* inherit from GLframebuffer */
   GLint          Width;
   GLint          Height;

   BITMAP*        FrontBuffer;
   BITMAP*        BackBuffer;
   BITMAP*        Active;
   BITMAP*        ReadActive;
   ZBUFFER*	  DepthBuffer;
} amesa_buffer;


typedef struct amesa_context
{
   GLcontext*   GLContext;     /* inherit from GLcontext     */
   AMesaVisual  Visual;
   AMesaBuffer  Buffer;

   GLint        ClearColor;
   GLuint       ClearIndex;
   GLuint       ColorIndex;
   float	ClearDepth;
   float	zNear;
   float	zFar;
} amesa_context;


extern AMesaContext AMesa;
void AMesaUpdateState(GLcontext *ctx, GLuint new_state);
void AMesaChooseLine(GLcontext *ctx);
void AMesaChooseTriangle(GLcontext *ctx);
