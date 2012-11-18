#ifndef __al_included_allegro5_aintern_direct3d_h
#define __al_included_allegro5_aintern_direct3d_h

#include "allegro5/platform/alplatf.h"
#include "allegro5/allegro_direct3d.h"
#include "allegro5/platform/aintwin.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_BITMAP_EXTRA_D3D
{
   /* Driver specifics. */

   unsigned int texture_w;
   unsigned int texture_h;

   LPDIRECT3DTEXTURE9 video_texture;
   LPDIRECT3DTEXTURE9 system_texture;

   bool initialized;
   bool is_backbuffer;

   D3DLOCKED_RECT locked_rect;

   struct ALLEGRO_DISPLAY_D3D *display;

   IDirect3DSurface9 *render_target;

   bool dirty;
} ALLEGRO_BITMAP_EXTRA_D3D;

typedef struct ALLEGRO_DISPLAY_D3D
{
   ALLEGRO_DISPLAY_WIN win_display; /* This must be the first member. */
   bool es_inited;

   /* Driver specifics */
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DSURFACE9 render_target;

   bool do_reset;
   bool reset_done;
   bool reset_success;

   ALLEGRO_BITMAP backbuffer_bmp;
   ALLEGRO_BITMAP_EXTRA_D3D backbuffer_bmp_extra;

   bool device_lost;

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

#ifdef ALLEGRO_CFG_HLSL_SHADERS
   LPD3DXEFFECT effect;
#endif
} ALLEGRO_DISPLAY_D3D;


AL_FUNC(void, _al_d3d_set_blender, (ALLEGRO_DISPLAY_D3D *disp));

void _al_d3d_destroy_bitmap(ALLEGRO_BITMAP *bitmap);
void _al_d3d_update_render_state(ALLEGRO_DISPLAY *display);


#ifdef __cplusplus
}
#endif

#endif
