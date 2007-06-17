#ifndef _AL_D3D_H
#define _AL_D3D_H

#include "allegro/internal/aintern_display.h"

#include <d3d9.h>
// FIXME: these are for gcc
#define D3DXINLINE static inline
#include <d3dx9.h>


typedef struct AL_DISPLAY_D3D AL_DISPLAY_D3D;


struct AL_DISPLAY_D3D
{
   AL_DISPLAY display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   DWORD thread_handle;
   LPDIRECT3DSWAPCHAIN9 swap_chain;
   LPDIRECT3DSURFACE9 render_target;
   bool keyboard_initialized;
   //LPDIRECT3DSURFACE8 stencil_buffer;
};


#endif /* _AL_D3D_H */
