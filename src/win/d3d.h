#include "allegro5/allegro.h"
#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_direct3d.h"
#include "allegro5/platform/aintwin.h"

#include <windows.h>
#include <d3d9.h>

/* The MinGW copy of d3d9.h doesn't currently define this constant. */
#ifndef D3D9b_SDK_VERSION
   #ifdef D3D_DEBUG_INFO
      #define D3D9b_SDK_VERSION (31 | 0x80000000)
   #else
      #define D3D9b_SDK_VERSION 31
   #endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Flexible vertex formats */
#define D3DFVF_COLORED_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DFVF_TL_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)


typedef struct ALLEGRO_SYSTEM_D3D ALLEGRO_SYSTEM_D3D;

typedef struct ALLEGRO_BITMAP_D3D
{
   ALLEGRO_BITMAP bitmap; /* This must be the first member. */

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

   bool modified;
} ALLEGRO_BITMAP_D3D;

typedef struct ALLEGRO_DISPLAY_D3D
{
   ALLEGRO_DISPLAY_WIN win_display; /* This must be the first member. */
   bool es_inited;

   /* Driver specifics */
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DSURFACE9 render_target;
   LPDIRECT3DSURFACE9 depth_stencil;

   bool do_reset;
   bool reset_done;
   bool reset_success;

   ALLEGRO_BITMAP_D3D backbuffer_bmp;

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
} ALLEGRO_DISPLAY_D3D;


/* Colored vertex */
typedef struct D3D_COLORED_VERTEX
{
   float x;
   float y;
   float z;
   D3DCOLOR color;
} D3D_COLORED_VERTEX; 


/* Transformed & lit vertex */
typedef struct D3D_TL_VERTEX
{
   float x;		/* x position */
   float y;		/* y position */
   float z;		/* z position */
   D3DCOLOR diffuse;    /* diffuse color */
   float tu;		/* texture coordinate */
   float tv;		/* texture coordinate */
} D3D_TL_VERTEX;

extern LPDIRECT3D9 _al_d3d;

ALLEGRO_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void);

AL_FUNC(ALLEGRO_BITMAP *, _al_d3d_create_bitmap,
   (ALLEGRO_DISPLAY *d, int w, int h));
//bool _al_d3d_is_device_lost(void);
//void _al_d3d_lock_device();
//void _al_d3d_unlock_device();
int _al_pixel_format_to_d3d(int format);
int _al_d3d_format_to_allegro(int d3d_fmt);
bool _al_d3d_render_to_texture_supported(void);
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);

void _al_d3d_release_default_pool_textures(void);
void _al_d3d_prepare_bitmaps_for_reset(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_refresh_texture_memory(void);
bool _al_d3d_recreate_bitmap_textures(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);
bool _al_d3d_supports_separate_alpha_blend(ALLEGRO_DISPLAY *display);
void _al_d3d_bmp_init(void);
void _al_d3d_bmp_destroy(void);

void _al_d3d_generate_display_format_list(void);
int _al_d3d_num_display_formats(void);
void _al_d3d_get_nth_format(int i, int *allegro, D3DFORMAT *d3d);
void _al_d3d_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref);
void _al_d3d_destroy_display_format_list(void);
ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_d3d_get_display_settings(int i);
void _al_d3d_resort_display_settings(void);

extern ALLEGRO_MUTEX *_al_d3d_lost_device_mutex;

/* Helper to get smallest fitting power of two. */
AL_INLINE_STATIC(int, pot, (int x),
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
})

#ifdef __cplusplus
}
#endif
