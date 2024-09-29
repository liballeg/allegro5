#ifndef __al_included_allegro5_aintern_direct3d_h
#define __al_included_allegro5_aintern_direct3d_h

#include "allegro5/platform/alplatf.h"
#include "allegro5/allegro_direct3d.h"
#include "allegro5/platform/aintwin.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct A5O_BITMAP_EXTRA_D3D
{
   /* Driver specifics. */

   unsigned int texture_w;
   unsigned int texture_h;

   LPDIRECT3DTEXTURE9 video_texture;
   LPDIRECT3DTEXTURE9 system_texture;
   int system_format;

   bool initialized;
   bool is_backbuffer;

   D3DLOCKED_RECT locked_rect;

   struct A5O_DISPLAY_D3D *display;

   IDirect3DSurface9 *render_target;

   bool dirty;
} A5O_BITMAP_EXTRA_D3D;

typedef struct A5O_DISPLAY_D3D
{
   A5O_DISPLAY_WIN win_display; /* This must be the first member. */
   bool es_inited;

   /* Driver specifics */
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DSURFACE9 render_target;

   bool do_reset;
   bool reset_done;
   bool reset_success;

   A5O_BITMAP backbuffer_bmp;
   A5O_BITMAP_EXTRA_D3D backbuffer_bmp_extra;

   /* Contains the target video bitmap for this display. */
   A5O_BITMAP* target_bitmap;

   bool device_lost;
   bool suppress_lost_events;

   bool faux_fullscreen;

   bool supports_separate_alpha_blend;

   TCHAR *device_name;

   int format;
   D3DFORMAT depth_stencil_format;
   int samples;
   bool single_buffer;
   bool vsync;

   int blender_state_op;
   int blender_state_src;
   int blender_state_dst;
   int blender_state_alpha_op;
   int blender_state_alpha_src;
   int blender_state_alpha_dst;

   RECT scissor_state;

#ifdef A5O_CFG_SHADER_HLSL
   LPD3DXEFFECT effect;
#endif
} A5O_DISPLAY_D3D;


AL_FUNC(void, _al_d3d_set_blender, (A5O_DISPLAY_D3D *disp));
AL_FUNC(void, _al_set_d3d_sampler_state, (IDirect3DDevice9* device,
   int sampler, A5O_BITMAP* bitmap, bool prim_default));

void _al_d3d_destroy_bitmap(A5O_BITMAP *bitmap);
void _al_d3d_update_render_state(A5O_DISPLAY *display);

#ifdef A5O_CFG_SHADER_HLSL
   bool _al_hlsl_set_projview_matrix(LPD3DXEFFECT effect,
      const A5O_TRANSFORM *t);
#endif

#ifdef A5O_CFG_D3DX9
   typedef HRESULT (WINAPI *_A5O_D3DXLSFLSPROC)(LPDIRECT3DSURFACE9, const PALETTEENTRY*,
      const RECT*, LPDIRECT3DSURFACE9, const PALETTEENTRY*, const RECT*,
      DWORD, D3DCOLOR);

   typedef HRESULT (WINAPI *_A5O_D3DXCREATEEFFECTPROC)(LPDIRECT3DDEVICE9, LPCVOID, UINT,
      CONST D3DXMACRO*, LPD3DXINCLUDE, DWORD, LPD3DXEFFECTPOOL, LPD3DXEFFECT*,
      LPD3DXBUFFER*);

   bool _al_load_d3dx9_module();
   void _al_unload_d3dx9_module();

   extern _A5O_D3DXLSFLSPROC _al_imp_D3DXLoadSurfaceFromSurface;
   extern _A5O_D3DXCREATEEFFECTPROC _al_imp_D3DXCreateEffect;
#endif

#ifdef __cplusplus
}
#endif

#endif
