#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"

#include <windows.h>
#include <d3d9.h>

#include "win_new.h"


/* Flexible vertex formats */
#define D3DFVF_COLORED_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DFVF_TL_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)


typedef struct AL_SYSTEM_D3D AL_SYSTEM_D3D;
typedef struct AL_BITMAP_D3D AL_BITMAP_D3D;
typedef struct AL_DISPLAY_D3D AL_DISPLAY_D3D;

struct AL_DISPLAY_D3D
{
   AL_DISPLAY display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   LPDIRECT3DSWAPCHAIN9 swap_chain;
   LPDIRECT3DSURFACE9 render_target;
   //bool keyboard_initialized;
   //LPDIRECT3DSURFACE8 stencil_buffer;
};

struct AL_BITMAP_D3D
{
	AL_BITMAP bitmap; /* This must be the first member. */

	/* Driver specifics. */

	unsigned int texture_w;
	unsigned int texture_h;

	LPDIRECT3DTEXTURE9 video_texture;
	LPDIRECT3DTEXTURE9 system_texture;

	bool initialized;
	bool is_backbuffer;

	unsigned int xo;	/* offsets for sub bitmaps */
	unsigned int yo;

	D3DLOCKED_RECT locked_rect;
};


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
	D3DCOLOR color;		/* color */
	float tu;		/* texture coordinate */
	float tv;		/* texture coordinate */
} D3D_TL_VERTEX;


AL_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void);
AL_SYSTEM_INTERFACE *_al_system_d3d_driver(void);

AL_VAR(LPDIRECT3D9, _al_d3d);
AL_VAR(LPDIRECT3DDEVICE9, _al_d3d_device);

AL_VAR(AL_DISPLAY_D3D *, _al_d3d_last_created_display);

AL_VAR(bool, _al_d3d_keyboard_initialized);

bool _al_d3d_init_display();
AL_BITMAP *_al_d3d_create_bitmap(AL_DISPLAY *d,
	unsigned int w, unsigned int h);
void _al_d3d_get_current_ortho_projection_parameters(float *w, float *h);
void _al_d3d_set_ortho_projection(float w, float h);
bool _al_d3d_is_device_lost(void);
void _al_d3d_lock_device();
void _al_d3d_unlock_device();
int _al_pixel_format_to_d3d(int format);
int _al_d3d_format_to_allegro_format(int d3d_fmt);

bool _al_d3d_init_keyboard();
//void _al_d3d_set_kb_cooperative_level(HWND window);

void _al_d3d_release_default_pool_textures();
void _al_d3d_prepare_bitmaps_for_reset();
void _al_d3d_refresh_texture_memory();
void _al_d3d_draw_textured_quad(AL_BITMAP_D3D *bmp,
	float sx, float sy, float sw, float sh,
	float dx, float dy, float dw, float dh,
	float cx, float cy, float angle,
	D3DCOLOR color, int flags);

/* Helper to get smallest fitting power of two. */
static inline int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}
