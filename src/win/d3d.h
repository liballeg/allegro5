#include "allegro5/allegro.h"
#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_direct3d.h"
#include "allegro5/internal/aintern_shader.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/platform/alplatf.h"

#include <windows.h>

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
// Fixed pipeline vertex
#define D3DFVF_FIXED_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
// Programmable pipeline vertex
#define D3DFVF_ALLEGRO_VERTEX (D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE4(1))

// Vertex structure when using fixed pipeline (otherwise it's ALLEGRO_VERTEX)
struct D3D_FIXED_VERTEX {
   float x, y, z;
   D3DCOLOR color;
   float u, v;
};

typedef struct ALLEGRO_SYSTEM_D3D ALLEGRO_SYSTEM_D3D;

extern LPDIRECT3D9 _al_d3d;

ALLEGRO_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void);

int _al_pixel_format_to_d3d(int format);
int _al_d3d_format_to_allegro(int d3d_fmt);
bool _al_d3d_render_to_texture_supported(void);
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);

void _al_d3d_release_default_pool_textures(ALLEGRO_DISPLAY *display);
void _al_d3d_refresh_texture_memory(ALLEGRO_DISPLAY *display);
bool _al_d3d_recreate_bitmap_textures(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);
bool _al_d3d_supports_separate_alpha_blend(ALLEGRO_DISPLAY *display);
void _al_d3d_bmp_destroy(void);

void _al_d3d_generate_display_format_list(void);
int _al_d3d_num_display_formats(void);
void _al_d3d_get_nth_format(int i, int *allegro, D3DFORMAT *d3d);
void _al_d3d_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref);
void _al_d3d_destroy_display_format_list(void);
ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_d3d_get_display_settings(int i);
void _al_d3d_resort_display_settings(void);

#ifdef ALLEGRO_CFG_SHADER_HLSL
void _al_d3d_on_lost_shaders(ALLEGRO_DISPLAY *display);
void _al_d3d_on_reset_shaders(ALLEGRO_DISPLAY *display);
void _al_d3d_init_shaders(void);
void _al_d3d_shutdown_shaders(void);
#endif

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
