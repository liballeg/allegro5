#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_display.h"
#include "d3d.h"

/* Note: synched to A5O_RENDER_FUNCTION values as array indices */
static int _d3d_funcs[] = {
   D3DCMP_NEVER,
   D3DCMP_ALWAYS,
   D3DCMP_LESS, 
   D3DCMP_EQUAL,     
   D3DCMP_LESSEQUAL,       
   D3DCMP_GREATER,        
   D3DCMP_NOTEQUAL, 
   D3DCMP_GREATEREQUAL
};

void _al_d3d_update_render_state(A5O_DISPLAY *display)
{
   _A5O_RENDER_STATE *r = &display->render_state;
   A5O_DISPLAY_D3D *disp = (A5O_DISPLAY_D3D *)display;

   if (!disp->device) return;

   /* TODO: We could store the previous state and/or mark updated states to
    * avoid so many redundant SetRenderState calls.
    */

   disp->device->SetRenderState(D3DRS_ALPHATESTENABLE,
      r->alpha_test ? TRUE : FALSE);
   disp->device->SetRenderState(D3DRS_ALPHAFUNC, _d3d_funcs[r->alpha_function]);
   disp->device->SetRenderState(D3DRS_ALPHAREF, r->alpha_test_value);

   disp->device->SetRenderState(D3DRS_ZENABLE,
      r->depth_test ? D3DZB_TRUE : D3DZB_FALSE);
   disp->device->SetRenderState(D3DRS_ZFUNC, _d3d_funcs[r->depth_function]);
  
   disp->device->SetRenderState(D3DRS_ZWRITEENABLE,
      (r->write_mask & A5O_MASK_DEPTH) ? TRUE : FALSE);

   disp->device->SetRenderState(D3DRS_COLORWRITEENABLE,
      ((r->write_mask & A5O_MASK_RED) ? D3DCOLORWRITEENABLE_RED : 0) |
      ((r->write_mask & A5O_MASK_GREEN) ? D3DCOLORWRITEENABLE_GREEN : 0) |
      ((r->write_mask & A5O_MASK_BLUE) ? D3DCOLORWRITEENABLE_BLUE : 0) |
      ((r->write_mask & A5O_MASK_ALPHA) ? D3DCOLORWRITEENABLE_ALPHA : 0));

}
