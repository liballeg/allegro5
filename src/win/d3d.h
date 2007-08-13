#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_display.h"
#include "allegro/internal/aintern_bitmap.h"

#include <windows.h>
#include <d3d9.h>

#include "win_new.h"


/* Flexible vertex formats */
#define D3DFVF_COLORED_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
#define D3DFVF_TL_VERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)


typedef struct ALLEGRO_SYSTEM_D3D ALLEGRO_SYSTEM_D3D;
typedef struct ALLEGRO_BITMAP_D3D ALLEGRO_BITMAP_D3D;
typedef struct ALLEGRO_DISPLAY_D3D ALLEGRO_DISPLAY_D3D;

struct ALLEGRO_BITMAP_D3D
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
};

struct ALLEGRO_DISPLAY_D3D
{
   ALLEGRO_DISPLAY display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   LPDIRECT3DSWAPCHAIN9 swap_chain;
   LPDIRECT3DSURFACE9 render_target;

   /*
    * The display thread must communicate with the main thread
    * through these variables.
    */
   bool end_thread;    /* The display thread should end */
   bool initialized;   /* Fully initialized */
   bool init_failed;   /* Initialization failed */
   bool thread_ended;  /* The display thread has ended */

   ALLEGRO_BITMAP_D3D backbuffer_bmp;
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


ALLEGRO_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void);

AL_VAR(LPDIRECT3DDEVICE9, _al_d3d_device);

AL_FUNC(ALLEGRO_BITMAP *, _al_d3d_create_bitmap,
   (ALLEGRO_DISPLAY *d, int w, int h));
bool _al_d3d_is_device_lost(void);
void _al_d3d_lock_device();
void _al_d3d_unlock_device();
int _al_format_to_d3d(int format);
int _al_d3d_format_to_allegro(int d3d_fmt);

void _al_d3d_release_default_pool_textures();
void _al_d3d_prepare_bitmaps_for_reset();
void _al_d3d_refresh_texture_memory();
void _al_d3d_draw_textured_quad(ALLEGRO_BITMAP_D3D *bmp,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh,
   float cx, float cy, float angle,
   D3DCOLOR color, int flags);
void _al_d3d_release_bitmap_textures(void);
bool _al_d3d_recreate_bitmap_textures(void);
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);
void _al_d3d_sync_bitmap(ALLEGRO_BITMAP *dest);

/* Helper to get smallest fitting power of two. */
static inline int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}
