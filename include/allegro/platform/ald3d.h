#ifndef _AL_D3D_H
#define _AL_D3D_H

#include "allegro/internal/aintern_display.h"

#include <d3d8.h>
// FIXME: these are for gcc
#define D3DXINLINE static inline
#include <d3dx8.h>


typedef struct AL_DISPLAY_D3D AL_DISPLAY_D3D;


struct AL_DISPLAY_D3D
{
   AL_DISPLAY display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   DWORD thread_handle;
   LPDIRECT3DSWAPCHAIN8 swap_chain;
   LPDIRECT3DSURFACE8 render_target;
   LPDIRECT3DSURFACE8 stencil_buffer;
   unsigned int num_backbuffers;
   bool keyboard_initialized;
};


AL_VAR(HWND, d3d_active_window);
AL_VAR(UINT, msg_call_proc);
AL_VAR(UINT, msg_suicide);


#endif
