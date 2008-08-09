#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"

#include <windows.h>
#include <d3d9.h>

#include "win_new.h"


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
   bool is_target;

   D3DLOCKED_RECT locked_rect;

   struct ALLEGRO_DISPLAY_D3D *display;

   IDirect3DSurface9 *render_target;
} ALLEGRO_BITMAP_D3D;

typedef struct ALLEGRO_DISPLAY_D3D
{
   ALLEGRO_DISPLAY display; /* This must be the first member. */

   /* Driver specifics */
   HWND window;
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DSURFACE9 render_target;

   /*
    * The display thread must communicate with the main thread
    * through these variables.
    */
   bool end_thread;    /* The display thread should end */
   bool initialized;   /* Fully initialized */
   bool init_failed;   /* Initialization failed */
   bool thread_ended;  /* The display thread has ended */

   bool do_reset;
   bool reset_done;
   bool reset_success;

   ALLEGRO_BITMAP_D3D backbuffer_bmp;

   int mouse_range_x1;
   int mouse_range_y1;
   int mouse_range_x2;
   int mouse_range_y2;

   bool device_lost;
   ALLEGRO_BITMAP *target_bitmap_before_device_lost;

   bool ignore_ack; // al_resize_display doesn't need acknowledge_resize
   		    // (but you should do it anyway, for portability)

   bool bitmaps_prepared_for_reset;
   UINT adapter;
   bool faux_fullscreen;

   TCHAR device_name[32];
   _AL_MUTEX mutex;

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
   D3DCOLOR color;		/* color */
   float tu;		/* texture coordinate */
   float tv;		/* texture coordinate */
} D3D_TL_VERTEX;


ALLEGRO_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void);

AL_FUNC(ALLEGRO_BITMAP *, _al_d3d_create_bitmap,
   (ALLEGRO_DISPLAY *d, int w, int h));
//bool _al_d3d_is_device_lost(void);
//void _al_d3d_lock_device();
//void _al_d3d_unlock_device();
int _al_format_to_d3d(int format);
int _al_d3d_format_to_allegro(int d3d_fmt);
void _al_d3d_set_blender(ALLEGRO_DISPLAY_D3D *disp);
bool _al_d3d_render_to_texture_supported(void);
void d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);

void _al_d3d_release_default_pool_textures(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_prepare_bitmaps_for_reset(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_refresh_texture_memory(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_draw_textured_quad(ALLEGRO_DISPLAY_D3D *, ALLEGRO_BITMAP_D3D *bmp,
   float sx, float sy, float sw, float sh,
   float dx, float dy, float dw, float dh,
   float cx, float cy, float angle,
   D3DCOLOR color, int flags, bool pivot);
void _al_d3d_release_bitmap_textures(ALLEGRO_DISPLAY_D3D *disp);
bool _al_d3d_recreate_bitmap_textures(ALLEGRO_DISPLAY_D3D *disp);
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap);
void _al_d3d_sync_bitmap(ALLEGRO_BITMAP *dest);

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
