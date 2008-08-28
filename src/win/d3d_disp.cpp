/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Direct3D display driver
 *
 *      By Trent Gamblin.
 *
 */

#include <string.h>
#include <stdio.h>
#include <process.h>
#include <cmath>

#include "allegro5/allegro5.h"

extern "C" {
#include "allegro5/system_new.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/platform/aintwin.h"
#include "allegro5/internal/aintern_thread.h"
#include "wddraw.h"
}

#include "d3d.h"

extern "C" {

static ALLEGRO_DISPLAY_INTERFACE *vt = 0;

static LPDIRECT3D9 _al_d3d = 0;

static D3DPRESENT_PARAMETERS d3d_pp;

/* FIXME: This can probably go, it's kept in the system driver too */
static _AL_VECTOR d3d_created_displays = _AL_VECTOR_INITIALIZER(ALLEGRO_DISPLAY_D3D *);

static float d3d_ortho_w;
static float d3d_ortho_h;

static HWND fullscreen_focus_window;
static bool ffw_set = false;

#ifdef WANT_D3D9EX
// Stuff dynamically loaded from dlls
typedef HRESULT (WINAPI *_dyn_create_type)(UINT, IDirect3D9Ex **);
static _dyn_create_type _dyn_create;
#endif


static bool d3d_can_wait_for_vsync;

static bool render_to_texture_supported = true;
static bool is_vista = false;
static int num_faux_fullscreen_windows = 0;
static bool already_fullscreen = false; /* real fullscreen */

static DWORD d3d_min_filter = D3DTEXF_NONE;
static DWORD d3d_mag_filter = D3DTEXF_NONE;

/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
typedef struct new_display_parameters {
   ALLEGRO_DISPLAY_D3D *display;
   bool init_failed;
} new_display_parameters;


static int allegro_formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ARGB_8888,
   ALLEGRO_PIXEL_FORMAT_ARGB_4444,
   ALLEGRO_PIXEL_FORMAT_RGB_565,
   ALLEGRO_PIXEL_FORMAT_ARGB_1555,
   ALLEGRO_PIXEL_FORMAT_XRGB_8888,
   -1
};

/* Mapping of Allegro formats to D3D formats */
static int d3d_formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   D3DFMT_A8R8G8B8,
   D3DFMT_A4R4G4B4,
   D3DFMT_R5G6B5,
   D3DFMT_A1R5G5B5,
   D3DFMT_X8R8G8B8,
   -1
};


bool al_d3d_supports_non_pow2_textures(void)
{
   D3DCAPS9 caps;
   int adapter = al_get_current_video_adapter();
   if (adapter == -1)
         adapter = 0;

   /* This might have to change for multihead */
   if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps) != D3D_OK) {
   	return false;
   }

   if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0) {
      return true;
   }

   return false;
}


bool al_d3d_supports_non_square_textures(void)
{
   D3DCAPS9 caps;
   int adapter = al_get_current_video_adapter();
   if (adapter == -1)
      adapter = 0;

   /* This might have to change for multihead */
   if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps) != D3D_OK) {
   	return false;
   }

   if ((caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) == 0) {
      return true;
   }

   return false;
}




int _al_format_to_d3d(int format)
{
   int i;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      if (!_al_pixel_format_is_real(allegro_formats[i]))
         continue;
      if (allegro_formats[i] == format) {
         return d3d_formats[i];
      }
   }

   return D3DFMT_R5G6B5;
}

int _al_d3d_format_to_allegro(int d3d_fmt)
{
   int i;

   for (i = 0; d3d_formats[i] >= 0; i++) {
      if (!_al_pixel_format_is_real(allegro_formats[i]))
         continue;
      if (d3d_formats[i] == d3d_fmt) {
         return allegro_formats[i];
      }
   }

   return -1;
}

static int d3d_al_color_to_d3d(ALLEGRO_COLOR color)
{
   unsigned char r, g, b, a;
   int result;
   al_unmap_rgba(color, &r, &g, &b, &a);
   result = D3DCOLOR_ARGB(a, r, g, b);
   return result;
}

static bool d3d_format_is_valid(int format)
{
   int i;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      if (allegro_formats[i] == format)
         return true;
   }

   return false;
}


static bool d3d_parameters_are_valid(int format, int refresh_rate, int flags)
{
   if (!d3d_format_is_valid(format))
      return false;
   
   return true;
}


static DWORD d3d_get_filter(AL_CONST char *s)
{
   if (strstr(s, "linear"))
      return D3DTEXF_LINEAR;
   if (strstr(s, "anisotropic"))
      return D3DTEXF_ANISOTROPIC;
   return D3DTEXF_NONE;
}


static void d3d_reset_state(ALLEGRO_DISPLAY_D3D *disp)
{
   if (disp->device_lost) return;

   disp->device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
   disp->device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
   disp->device->SetRenderState(D3DRS_LIGHTING, FALSE);
   disp->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   disp->device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   
   /* Set up filtering */
   if (disp->device->SetSamplerState(0, D3DSAMP_MINFILTER, d3d_min_filter) != D3D_OK)
      TRACE("SetSamplerState failed\n");
   if (disp->device->SetSamplerState(0, D3DSAMP_MAGFILTER, d3d_mag_filter) != D3D_OK)
      TRACE("SetSamplerState failed\n");
   if (disp->device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE) != D3D_OK)
      TRACE("SetSamplerState failed\n");
}

void _al_d3d_get_current_ortho_projection_parameters(float *w, float *h)
{
   *w = d3d_ortho_w;
   *h = d3d_ortho_h;
}

static void d3d_get_ortho_matrix(float w, float h, D3DMATRIX *matrix)
{
   float left = 0.0f;
   float right = w;
   float top = 0.0f;
   float bottom = h;
   float neer = -1.0f;
   float farr = 1.0f;

   matrix->m[1][0] = 0.0f;
   matrix->m[2][0] = 0.0f;
   matrix->m[0][1] = 0.0f;
   matrix->m[2][1] = 0.0f;
   matrix->m[0][2] = 0.0f;
   matrix->m[1][2] = 0.0f;
   matrix->m[0][3] = 0.0f;
   matrix->m[1][3] = 0.0f;
   matrix->m[2][3] = 0.0f;

   matrix->m[0][0] = 2.0f / (right - left);
   matrix->m[1][1] = 2.0f / (top - bottom);
   matrix->m[2][2] = 2.0f / (farr - neer);

   matrix->m[3][0] = -((right+left)/(right-left));
   matrix->m[3][1] = -((top+bottom)/(top-bottom));
   matrix->m[3][2] = -((farr+neer)/(farr-neer));
   matrix->m[3][3] = 1.0f;
}

static void d3d_get_identity_matrix(D3DMATRIX *matrix)
{
   int i, j;
   int one = 0;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         if (j == one)
            matrix->m[j][i] = 1.0f;
         else
            matrix->m[j][i] = 0.0f;
      }
      one++;
   }
}

static void _al_d3d_set_ortho_projection(ALLEGRO_DISPLAY_D3D *disp, float w, float h)
{
   D3DMATRIX matOrtho;
   D3DMATRIX matIdentity;

   if (disp->device_lost) return;

   d3d_ortho_w = w;
   d3d_ortho_h = h;

   d3d_get_identity_matrix(&matIdentity);
   d3d_get_ortho_matrix(w, h, &matOrtho);

   disp->device->SetTransform(D3DTS_PROJECTION, &matOrtho);
   disp->device->SetTransform(D3DTS_WORLD, &matIdentity);
   disp->device->SetTransform(D3DTS_VIEW, &matIdentity);
}

static bool d3d_display_mode_matches(D3DDISPLAYMODE *dm, int w, int h, int format, int refresh_rate)
{
   if ((dm->Width == (unsigned int)w) &&
       (dm->Height == (unsigned int)h) &&
       ((!refresh_rate) || (dm->RefreshRate == (unsigned int)refresh_rate)) &&
       ((int)dm->Format == (int)_al_format_to_d3d(format))) {
          return true;
   }
   return false;
}

static bool d3d_check_mode(int w, int h, int format, int refresh_rate, UINT adapter)
{
   UINT num_modes;
   UINT i;
   D3DDISPLAYMODE display_mode;
   
   TRACE("in d3d_check_mode adapter=%d format=%d d3d_format=%d\n", adapter, format, _al_format_to_d3d(format));
   num_modes = _al_d3d->GetAdapterModeCount(adapter, (D3DFORMAT)_al_format_to_d3d(format));
   TRACE("num_modes=%d\n", num_modes);

   for (i = 0; i < num_modes; i++) {
	   TRACE("i=%d\n", i);
      if (_al_d3d->EnumAdapterModes(adapter, (D3DFORMAT)_al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
         TRACE("d3d_check_mode: EnumAdapterModes failed.\n");
         return false;
      }
      if (d3d_display_mode_matches(&display_mode, w, h, format, refresh_rate)) {
         return true;
      }
   }

   return false;
}

static int d3d_get_default_refresh_rate(UINT adapter)
{
   D3DDISPLAYMODE d3d_dm;
   _al_d3d->GetAdapterDisplayMode(adapter, &d3d_dm);
   return d3d_dm.RefreshRate;
}


static bool d3d_create_fullscreen_device(ALLEGRO_DISPLAY_D3D *d,
   int format, int refresh_rate, int flags)
{
   int ret;
   bool reset_all = false;
   ALLEGRO_DISPLAY_WIN *win_display = &d->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;

   if (!d3d_check_mode(al_display->w, al_display->h, format, refresh_rate, d->adapter)) {
      TRACE("d3d_create_fullscreen_device: Mode not supported.\n");
      return 0;
   }
   TRACE("after check_mode\n");

   ZeroMemory(&d3d_pp, sizeof(d3d_pp));
   d3d_pp.BackBufferFormat = (D3DFORMAT)_al_format_to_d3d(format);
   d3d_pp.BackBufferWidth = al_display->w;
   d3d_pp.BackBufferHeight = al_display->h;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 0;
   d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
   d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   if (flags & ALLEGRO_SINGLEBUFFER) {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
   }
   else {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   }
   d3d_pp.hDeviceWindow = win_display->window;
   TRACE("d3d_pp.hDeviceWindow=%p\n", d3d_pp.hDeviceWindow);

   TRACE("getting refresh rate\n");
   if (refresh_rate) {
      d3d_pp.FullScreen_RefreshRateInHz = refresh_rate;
   }
   else {
      d3d_pp.FullScreen_RefreshRateInHz = d3d_get_default_refresh_rate(d->adapter);
   }

   TRACE("HERE before if is vista\n");

   if (ffw_set == false) {
      fullscreen_focus_window = win_display->window;
      ffw_set = true;
   }
   else {
      reset_all = true;
   }

#ifdef WANT_D3D9EX
   if (is_vista) {
    D3DDISPLAYMODEEX mode;
   	IDirect3D9Ex *d3d = (IDirect3D9Ex *)_al_d3d;
	mode.Size = sizeof(D3DDISPLAYMODEEX);
	mode.Width = al_display->w;
	mode.Height = al_display->h;
	mode.RefreshRate = d3d_pp.FullScreen_RefreshRateInHz;
	mode.Format = d3d_pp.BackBufferFormat;
	mode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

	TRACE("calling CreateDeviceEx\n");
	if ((ret = d3d->CreateDeviceEx(d->adapter,
		 D3DDEVTYPE_HAL, fullscreen_focus_window,
		 D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
		 &d3d_pp, &mode, (IDirect3DDevice9Ex **)(&d->device))) != D3D_OK) {
	      if ((ret = d3d->CreateDeviceEx(d->adapter,
		    D3DDEVTYPE_HAL, fullscreen_focus_window,
		    D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
		    &d3d_pp, &mode, (IDirect3DDevice9Ex **)(&d->device))) != D3D_OK) {
		 switch (ret) {
		    case D3DERR_INVALIDCALL:
		       TRACE("D3DERR_INVALIDCALL in create_device.\n");
		       break;
		    case D3DERR_NOTAVAILABLE:
		       TRACE("D3DERR_NOTAVAILABLE in create_device.\n");
		       break;
		    case D3DERR_OUTOFVIDEOMEMORY:
		       TRACE("D3DERR_OUTOFVIDEOMEMORY in create_device.\n");
		       break;
		    case D3DERR_DEVICELOST:
		       TRACE("D3DERR_DEVICELOST in create_device.\n");
		       break;
		    default:
		       TRACE("Direct3D Device creation failed.\n");
		       break;
		 }
		 return 0;
	      }
	   }
   }
   else {
#endif   
	if ((ret = _al_d3d->CreateDevice(d->adapter,
		 D3DDEVTYPE_HAL, fullscreen_focus_window,
		 D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
		 &d3d_pp, &d->device)) != D3D_OK) {
	      if ((ret = _al_d3d->CreateDevice(D3DADAPTER_DEFAULT,
		    D3DDEVTYPE_HAL, fullscreen_focus_window,
		    D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
		    &d3d_pp, &d->device)) != D3D_OK) {
		 switch (ret) {
		    case D3DERR_INVALIDCALL:
		       TRACE("D3DERR_INVALIDCALL in create_device.\n");
		       break;
		    case D3DERR_NOTAVAILABLE:
		       TRACE("D3DERR_NOTAVAILABLE in create_device.\n");
		       break;
		    case D3DERR_OUTOFVIDEOMEMORY:
		       TRACE("D3DERR_OUTOFVIDEOMEMORY in create_device.\n");
		       break;
		    case D3DERR_DEVICELOST:
		       TRACE("D3DERR_DEVICELOST in create_device.\n");
		       break;
		    default:
		       TRACE("Direct3D Device creation failed.\n");
		       break;
		 }
		 return 0;
	      }
	   }
#ifdef WANT_D3D9EX	   
   }
#endif

   d->device->GetRenderTarget(0, &d->render_target);

   TRACE("Fullscreen Direct3D device created.\n");
   
   d->device->BeginScene();

   TRACE("Reset all=%d\n", reset_all);
   if (reset_all) {
      int i;
      for (i = 0; i < (int)d3d_created_displays._size; i++) {
         ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&d3d_created_displays, i);
         ALLEGRO_DISPLAY_D3D *disp = *dptr;
         if (disp != d) {
            if (disp != d) {
	            TRACE("Resetting\n");
	            disp->do_reset = true;
	            while (!disp->reset_done) {
		            al_rest(0.001);
	            }
	            disp->reset_done = false;
            }
         }
      }
   }

   return 1;
}

static void d3d_destroy_device(ALLEGRO_DISPLAY_D3D *disp)
{
   while (disp->device->Release() != 0) {
      TRACE("d3d_destroy_device: ref count not 0\n");
   }
   //disp->device->Release();
   disp->device = NULL;
}


bool _al_d3d_render_to_texture_supported(void)
{
   return render_to_texture_supported;
}



bool _al_d3d_init_display()
{
   D3DDISPLAYMODE d3d_dm;
   OSVERSIONINFO info;

   info.dwOSVersionInfoSize = sizeof(info);
   GetVersionEx(&info);
   is_vista = info.dwMajorVersion >= 6;

#ifdef WANT_D3D9EX
   if (is_vista) {
	_dyn_create = (_dyn_create_type)GetProcAddress(GetModuleHandle(TEXT("d3d9.dll")), "Direct3DCreate9Ex");
	if (_dyn_create != NULL) {
		if (_dyn_create(D3D_SDK_VERSION, (LPDIRECT3D9EX *)&_al_d3d) != D3D_OK) {
			TRACE("Direct3DCreate9Ex failed\n");
			return false;
		}
	}
	else {
		TRACE("Direct3DCreate9Ex not in d3d9.dll");
		is_vista = false;
	}
   }

   if (!is_vista) {
#endif   
	   if ((_al_d3d = Direct3DCreate9(D3D9b_SDK_VERSION)) == NULL) {
	      TRACE("Direct3DCreate9 failed.\n");
	      return false;
	   }
#ifdef WANT_D3D9EX	   
   }
#endif   

   _al_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3d_dm);

   if (_al_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
         D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_RENDERTARGET,
         D3DRTYPE_TEXTURE, d3d_dm.Format) != D3D_OK)
      render_to_texture_supported = false;
   else
      render_to_texture_supported = true;
   
   TRACE("Render-to-texture: %d\n", render_to_texture_supported);

   return true;
}

static bool d3d_create_device(ALLEGRO_DISPLAY_D3D *d,
   int format, int refresh_rate, int flags)
{
   HRESULT hr;
   ALLEGRO_DISPLAY_WIN *win_display = &d->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;

   TRACE("in d3d_create_device\n");

   ZeroMemory(&d3d_pp, sizeof(d3d_pp));
   d3d_pp.BackBufferFormat = (D3DFORMAT)_al_format_to_d3d(format);
   d3d_pp.BackBufferWidth = al_display->w;
   d3d_pp.BackBufferHeight = al_display->h;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 1;
   d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
   d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   if (flags & ALLEGRO_SINGLEBUFFER) {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
   }
   else {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   }
   d3d_pp.hDeviceWindow = win_display->window;

   if ((hr = _al_d3d->CreateDevice(D3DADAPTER_DEFAULT,
         D3DDEVTYPE_HAL, win_display->window,
         D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
         &d3d_pp, (LPDIRECT3DDEVICE9 *)&d->device)) != D3D_OK) {
      if (hr == D3DERR_NOTAVAILABLE) {
      	TRACE("CreateDevice failed: 1\n");
      }
      else if (hr == D3DERR_DEVICELOST) {
      	TRACE("CreateDevice failed: 2\n");
      }
      else if (hr == D3DERR_INVALIDCALL) {
      	TRACE("CreateDevice failed: 3\n");
      }
      else if (hr == D3DERR_OUTOFVIDEOMEMORY) {
      	TRACE("CreateDevice failed: 4\n");
      }
      else if (hr == E_OUTOFMEMORY) {
      	TRACE("CreateDevice failed: 5\n");
      }
      else {
      	TRACE("Unknown error %u\n", (unsigned)hr);
      }
      TRACE("d3d_create_device: CreateDevice failed.\n");
      return 0;
   }

   if (d->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target) != D3D_OK) {
      TRACE("d3d_create_device: GetBackBuffer failed.\n");
      return 0;
   }

   if (d->device->BeginScene() != D3D_OK) {
      TRACE("BeginScene failed in create_device\n");
   }
   else {
      TRACE("BeginScene succeeded in create_device\n");
   }

   return 1;
}

/* When a display is destroyed, its bitmaps get converted
 * to memory bitmaps
 */
static void d3d_release_bitmaps(ALLEGRO_DISPLAY *display)
{
   while (display->bitmaps._size > 0) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref_back(&display->bitmaps);
      ALLEGRO_BITMAP *b = *bptr;
      _al_convert_to_memory_bitmap(b);
   }
}


static void d3d_release_current_target(bool release_backbuffer)
{
   ALLEGRO_BITMAP *curr;
   ALLEGRO_BITMAP_D3D *curr_d3d;

   curr = al_get_target_bitmap();
   if (curr && !(curr->flags & ALLEGRO_MEMORY_BITMAP)) {
   	curr_d3d = (ALLEGRO_BITMAP_D3D *)curr;
      if (curr_d3d->render_target) {
         if (curr_d3d->is_backbuffer && release_backbuffer) {
            ALLEGRO_DISPLAY_D3D *dd = (ALLEGRO_DISPLAY_D3D *)curr->display;
            if (dd->render_target->Release() != 0) {
               TRACE("d3d_release_current_target: (bb) ref count not 0\n");
            }
         }
         else {
            if (curr_d3d->render_target->Release() != 0) {
               TRACE("d3d_release_current_target: (bmp) ref count not 0\n");
            }
	         curr_d3d->render_target = NULL;
         }
      }
	}
}


static void d3d_destroy_display_internals(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;

   d3d_display->device->EndScene();
   
   d3d_release_bitmaps((ALLEGRO_DISPLAY *)d3d_display);
   //_al_d3d_release_default_pool_textures();

   d3d_release_current_target(false);

   if (d3d_display->render_target->Release() != 0) {
      TRACE("d3d_destroy_d3d_display_internals: (bb) ref count not 0\n");
   }

   d3d_display->end_thread = true;
   while (!d3d_display->thread_ended)
      al_rest(0.001);
   
   _al_win_ungrab_input();
   
   SendMessage(win_display->window, _al_win_msg_suicide, 0, 0);
}

static void d3d_destroy_display(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;

   d3d_destroy_display_internals(d3d_display);

   _al_vector_find_and_delete(&system->system.displays, &display);

   _al_vector_find_and_delete(&d3d_created_displays, &display);

   if (d3d_created_displays._size > 0) {
      ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&d3d_created_displays, 0);
      ALLEGRO_DISPLAY_D3D *d = *dptr;
      win_grab_input();
   }
   else {
      ffw_set = false;
      already_fullscreen = false;
   }

   _al_event_source_free(&display->es);

   _al_vector_free(&display->bitmaps);
   _AL_FREE(display);
}

void _al_d3d_prepare_for_reset(ALLEGRO_DISPLAY_D3D *disp)
{
   _al_d3d_prepare_bitmaps_for_reset(disp);
   _al_d3d_release_default_pool_textures(disp);
   if (disp->render_target->Release() != 0) {
      TRACE("_al_d3d_prepare_for_reset: (bb) ref count not 0\n");
   }
}

static bool _al_d3d_reset_device(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   ALLEGRO_BITMAP *curr = al_get_target_bitmap();
   bool reset_target = false;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;


    _al_d3d_prepare_for_reset(d3d_display);

    if (al_display->flags & ALLEGRO_FULLSCREEN) {
        HRESULT hr;
      
        ZeroMemory(&d3d_pp, sizeof(d3d_pp));
        d3d_pp.BackBufferFormat = (D3DFORMAT)_al_format_to_d3d(al_display->format);
        d3d_pp.BackBufferWidth = al_display->w;
        d3d_pp.BackBufferHeight = al_display->h;
        d3d_pp.BackBufferCount = 1;
        d3d_pp.Windowed = 0;
        d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3d_pp.hDeviceWindow = win_display->window;
        d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        if (al_display->flags & ALLEGRO_SINGLEBUFFER) {
            d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
        }
        else {
           d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        }
        if (al_display->refresh_rate) {
           d3d_pp.FullScreen_RefreshRateInHz =
              al_display->refresh_rate;
        }
        else {
           d3d_pp.FullScreen_RefreshRateInHz = d3d_get_default_refresh_rate(d3d_display->adapter);
        }
#ifdef WANT_D3D9EX	
	 		if (is_vista) {
				D3DDISPLAYMODEEX mode;
				IDirect3DDevice9Ex *dev = (IDirect3DDevice9Ex *)d3d_display->device;
				mode.Size = sizeof(D3DDISPLAYMODEEX);
				mode.Width = d3d_pp.BackBufferWidth;
				mode.Height = d3d_pp.BackBufferHeight;
				mode.RefreshRate = d3d_pp.FullScreen_RefreshRateInHz;
				mode.Format = d3d_pp.BackBufferFormat;
				mode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
				hr = dev->ResetEx(&d3d_pp, &mode);
			}
			else {
#endif			
		         	hr = d3d_display->device->Reset(&d3d_pp);
#ifdef WANT_D3D9EX				
			}
#endif			
			if (hr != D3D_OK) {
				switch (hr) {
				case D3DERR_INVALIDCALL:
				TRACE("D3DERR_INVALIDCALL in reset.\n");
				break;
				case D3DERR_NOTAVAILABLE:
				TRACE("D3DERR_NOTAVAILABLE in reset.\n");
				break;
				case D3DERR_OUTOFVIDEOMEMORY:
				TRACE("D3DERR_OUTOFVIDEOMEMORY in reset.\n");
				break;
				case D3DERR_DEVICELOST:
				TRACE("D3DERR_DEVICELOST in reset.\n");
				break;
				default:
				TRACE("Direct3D Device reset failed (unknown reason).\n");
				break;
				}
				return 0;
			}
      }
      else {
         unsigned int i;
         
         ZeroMemory(&d3d_pp, sizeof(d3d_pp));
         d3d_pp.BackBufferFormat = (D3DFORMAT)_al_format_to_d3d(al_display->format);
         d3d_pp.BackBufferWidth = al_display->w;
         d3d_pp.BackBufferHeight = al_display->h;
         d3d_pp.BackBufferCount = 1;
         d3d_pp.Windowed = 1;
         d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
         d3d_pp.hDeviceWindow = win_display->window;
         d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

         /* Try to reset a few times */
         for (i = 0; i < 5; i++) {
            if (d3d_display->device->Reset(&d3d_pp) == D3D_OK) {
               break;
            }
            al_rest(0.100);
         }
         if (i == 5) {
   	    TRACE("Reset failed\n");
            return 0;
         }
	}

   d3d_display->device->GetRenderTarget(0, &d3d_display->render_target);

   _al_d3d_refresh_texture_memory(d3d_display);

   d3d_reset_state(d3d_display);

   return 1;
}

static int d3d_choose_display_format(int fake)
{
   /* Pick an appropriate format if the user is vague */
   switch (fake) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_RGB_565;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_ARGB_1555;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_RGB_565;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_ARGB_4444;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         fake = -1;
         break;
      default:
         break;
   }

   return fake;
}

static BOOL IsTextureFormatOk(D3DFORMAT TextureFormat, D3DFORMAT AdapterFormat) 
{
   HRESULT hr = _al_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL,
      AdapterFormat,
      0,
      D3DRTYPE_TEXTURE,
      TextureFormat);
    
   return SUCCEEDED(hr);
}

static int real_choose_bitmap_format(int bits, bool alpha)
{
   int i;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      int aformat = allegro_formats[i];
      D3DFORMAT dformat;
      D3DFORMAT adapter_format;
      int adapter_format_allegro;
      if (!_al_pixel_format_is_real(aformat))
         continue;
      if (bits && al_get_pixel_format_bits(aformat) != bits)
         continue;
      dformat = (D3DFORMAT)d3d_formats[i];
      adapter_format_allegro = al_get_new_display_format();
      if (!_al_pixel_format_is_real(adapter_format_allegro))
         adapter_format_allegro = d3d_choose_display_format(adapter_format_allegro);
      adapter_format = (D3DFORMAT)_al_format_to_d3d(adapter_format_allegro);
      if (IsTextureFormatOk(dformat, adapter_format))
         return aformat;
   }

   return -1;
}

static int d3d_choose_bitmap_format(int fake)
{

   TRACE("Choosing (%d)\n", fake);

   switch (fake) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
         fake = real_choose_bitmap_format(0, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
         fake = real_choose_bitmap_format(0, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         fake = real_choose_bitmap_format(32, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         fake = real_choose_bitmap_format(32, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         fake = real_choose_bitmap_format(24, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA:
         fake = real_choose_bitmap_format(24, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         fake = real_choose_bitmap_format(16, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         fake = real_choose_bitmap_format(16, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:
         fake = real_choose_bitmap_format(15, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA:
         fake = real_choose_bitmap_format(15, true);
         break;
      default:
         fake = -1;
   }

   TRACE("Chose: %d\n", fake);

   return fake;
}


static bool d3d_create_display_internals(ALLEGRO_DISPLAY_D3D *display);
static void d3d_destroy_display_internals(ALLEGRO_DISPLAY_D3D *display);

/*
 * The window and swap chain must be created in the same
 * thread that runs the message loop. It also must be
 * reset from the same thread.
 */
static void d3d_display_thread_proc(void *arg)
{
   ALLEGRO_DISPLAY_D3D *d3d_display;
   ALLEGRO_DISPLAY_WIN *win_display;
   ALLEGRO_DISPLAY *al_display;
   DWORD result;
   MSG msg;
   HRESULT hr;
   bool lost_event_generated = false;
   new_display_parameters *params = (new_display_parameters *)arg;
   D3DCAPS9 caps;
   int new_format;

   d3d_display = params->display;
   win_display = &d3d_display->win_display;
   al_display = &win_display->display;

   new_format = al_display->format;

   if (!_al_pixel_format_is_real(al_display->format)) {
      int f = d3d_choose_display_format(al_display->format);
      if (f < 0) {
         params->init_failed = true;
         return;
      }
      new_format = f;
   }

   if (!d3d_parameters_are_valid(al_display->format, al_display->refresh_rate, al_display->flags)) {
      TRACE("d3d_display_thread_proc: Invalid parameters.\n");
      params->init_failed = true;
      return;
   }

   al_display->format = new_format;

   if (d3d_display->faux_fullscreen) {
   	ALLEGRO_MONITOR_INFO mi;
	DEVMODE dm;
	bool found = true;
	int refresh_rate;
	DISPLAY_DEVICE dd;
	if (already_fullscreen) {
 	    int i;

 	    for (i = 0; i < (int)d3d_created_displays._size; i++) {
 		    ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&d3d_created_displays, i);
 		    ALLEGRO_DISPLAY_D3D *disp = *dptr;
		    if (disp != d3d_display) {
			   TRACE("Destroying internals\n");
			   d3d_destroy_display_internals(disp);
 			   disp->end_thread = false;
 			   disp->initialized = false;
			   disp->init_failed = false;
			   disp->thread_ended = false;
		    }
	    }
	}

	TRACE("Here d3d_display->adapter=%d\n", d3d_display->adapter);
	al_get_monitor_info(d3d_display->adapter, &mi);
	TRACE("mi.x1=%d mi.y1=%d mi.x2=%d mi.y2=%d\n", mi.x1, mi.y1, mi.x2, mi.y2);
	/* Yes this is an "infinite" loop (suggested by MS on msdn) */
	for (int i = 0; ; i++) {
		TRACE("i=%d\n", i);
		dd.cb = sizeof(dd);
		TRACE("Calling EnumDisplayDevices\n");
		if (!EnumDisplayDevices(NULL, i, &dd, 0)) {
			found = false;
			break;
		}
		TRACE("dd.DeviceName=%s\n", dd.DeviceName);
		if (!EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS,
				&dm)) {
			continue;
		}
		if (mi.x1 == dm.dmPosition.x && mi.y1 == dm.dmPosition.y) {
			break;
		}

	}
	if (!found) {
	      TRACE("d3d_display_thread_proc: Error setting faux fullscreen mode.\n");
	      params->init_failed = true;
	      return;
	}
	if (al_display->refresh_rate) {
	   refresh_rate = al_display->refresh_rate;
	}
	else {
	   refresh_rate = d3d_get_default_refresh_rate(d3d_display->adapter);
	}
        d3d_display->device_name = (TCHAR *)_AL_MALLOC(sizeof(TCHAR)*32);
	strcpy(d3d_display->device_name, dd.DeviceName);
	TRACE("going to call _al_win_create_faux_fullscreen_window\n");
   	win_display->window = _al_win_create_faux_fullscreen_window(dd.DeviceName, al_display,
		mi.x1, mi.y1, al_display->w, al_display->h,
		refresh_rate, al_display->flags);
	TRACE("Called _al_win_create_faux_fullscreen_window\n");

	if (already_fullscreen) {
 	    int i;
		already_fullscreen = false;
 	    for (i = 0; i < (int)d3d_created_displays._size; i++) {
 		    ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&d3d_created_displays, i);
 		    ALLEGRO_DISPLAY_D3D *disp = *dptr;
		    if (disp != d3d_display) {
			   TRACE("Creating internals\n");
			   disp->faux_fullscreen = true;
			   TRACE("Before create, disp=%p\n", disp);
			   d3d_create_display_internals(disp);
			   _al_d3d_recreate_bitmap_textures(disp);
			}
	    }
	}

   	num_faux_fullscreen_windows++;
   }
   else {
	   TRACE("Normal window\n");
	   win_display->window = _al_win_create_window(al_display, al_display->w,
	      al_display->h, al_display->flags);
   }	      
   
   if (!win_display->window) {
      params->init_failed = true;
      return;
   }

   if (!(al_display->flags & ALLEGRO_FULLSCREEN) || d3d_display->faux_fullscreen) {
      if (!d3d_create_device(d3d_display, al_display->format, al_display->refresh_rate, al_display->flags)) {
         d3d_display->thread_ended = true;
         d3d_destroy_display(al_display);
         params->init_failed = true;
         return;
      }
   }
   else {
	   TRACE("Creating real fullscreen device\n");
      if (!d3d_create_fullscreen_device(d3d_display, al_display->format, al_display->refresh_rate, al_display->flags)) {
         d3d_display->thread_ended = true;
         d3d_destroy_display(al_display);
         params->init_failed = true;
         return;
      }
	  TRACE("Real fullscreen device created\n");
   }


   d3d_display->device->GetDeviceCaps(&caps);
   d3d_can_wait_for_vsync = ((caps.Caps & D3DCAPS_READ_SCANLINE) != 0);

   d3d_display->thread_ended = false;

   d3d_display->initialized = true;

   for (;;) {
      if (d3d_display->end_thread) {
         break;
      }
      /* FIXME: How long should we wait? */
      result = MsgWaitForMultipleObjects(_win_input_events, _win_input_event_id, FALSE, 1/*INFINITE*/, QS_ALLINPUT);
      if (result < (DWORD) WAIT_OBJECT_0 + _win_input_events) {
         /* one of the registered events is in signaled state */
         (*_win_input_event_handler[result - WAIT_OBJECT_0])();
      }
      else if (result == (DWORD) WAIT_OBJECT_0 + _win_input_events) {
         /* messages are waiting in the queue */
         while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, NULL, 0, 0)) {
               DispatchMessage(&msg);
            }
            else {
               goto End;
            }
         }
            hr = d3d_display->device->TestCooperativeLevel();

            if (hr == D3D_OK) {
               d3d_display->device_lost = false;
            }
            else if (hr == D3DERR_DEVICELOST) {
               /* device remains lost */
               if (!lost_event_generated) {
                  _al_event_source_lock(&al_display->es);
                  if (_al_event_source_needs_to_generate_event(&al_display->es)) {
                     ALLEGRO_EVENT *event = _al_event_source_get_unused_event(&al_display->es);
                     if (event) {
                        event->display.type = ALLEGRO_EVENT_DISPLAY_LOST;
                        event->display.timestamp = al_current_time();
                        _al_event_source_emit_event(&al_display->es, event);
                     }
                  }
                  _al_event_source_unlock(&al_display->es);
                  lost_event_generated = true;
               }
            }
            else if (hr == D3DERR_DEVICENOTRESET) {
               if (_al_d3d_reset_device(d3d_display)) {
                  d3d_display->device_lost = false;
                  _al_event_source_lock(&al_display->es);
                  if (_al_event_source_needs_to_generate_event(&al_display->es)) {
                     ALLEGRO_EVENT *event = _al_event_source_get_unused_event(&al_display->es);
                     if (event) {
                        event->display.type = ALLEGRO_EVENT_DISPLAY_FOUND;
                        event->display.timestamp = al_current_time();
                        _al_event_source_emit_event(&al_display->es, event);
                     }
                  }
                  _al_event_source_unlock(&al_display->es);
                  lost_event_generated = false;
               }
            }
            if (d3d_display->do_reset) {
               d3d_display->reset_success = _al_d3d_reset_device(d3d_display);
               d3d_display->reset_done = true;
               d3d_display->do_reset = false;
            }
         }
   }

End:

   d3d_destroy_device(d3d_display);

   if (d3d_display->faux_fullscreen) {
	TRACE("Changing resolution back\n");
   	ChangeDisplaySettingsEx(d3d_display->device_name, NULL, NULL, 0, NULL);//CDS_FULLSCREEN
        _AL_FREE(d3d_display->device_name);
	num_faux_fullscreen_windows--;
       TRACE("Res changed back\n");
   }

   _al_win_delete_thread_handle(GetCurrentThreadId());

   d3d_display->thread_ended = true;

   TRACE("d3d display thread exits\n");
}

static bool d3d_create_display_internals(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   new_display_parameters params;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;
   static bool cfg_read = false;
   ALLEGRO_SYSTEM *sys;
   AL_CONST char *s;

   params.display = d3d_display;
   params.init_failed = false;

   _beginthread(d3d_display_thread_proc, 0, &params);

   while (!params.display->initialized && !params.init_failed)
      al_rest(0.001);


   if (params.init_failed) {
      return false;
   }
   
   if (!cfg_read) {
      cfg_read = true;

      sys = al_system_driver();
   
      if (sys->config) {
         s = al_config_get_value(sys->config, "gfx", "min_filter");
         if (s)
            d3d_min_filter = d3d_get_filter(s);
         s = al_config_get_value(sys->config, "gfx", "mag_filter");
         if (s)
            d3d_mag_filter = d3d_get_filter(s);
      }
   }

   d3d_reset_state(d3d_display);

   d3d_display->backbuffer_bmp.is_backbuffer = true;
   d3d_display->backbuffer_bmp.bitmap.display = al_display;
   d3d_display->backbuffer_bmp.bitmap.format = al_display->format;
   d3d_display->backbuffer_bmp.bitmap.flags = 0;
   d3d_display->backbuffer_bmp.bitmap.w = al_display->w;
   d3d_display->backbuffer_bmp.bitmap.h = al_display->h;
   d3d_display->backbuffer_bmp.bitmap.cl = 0;
   d3d_display->backbuffer_bmp.bitmap.ct = 0;
   d3d_display->backbuffer_bmp.bitmap.cr = al_display->w-1;
   d3d_display->backbuffer_bmp.bitmap.cb = al_display->h-1;
   d3d_display->backbuffer_bmp.bitmap.vt = (ALLEGRO_BITMAP_INTERFACE *)_al_bitmap_d3d_driver();
   d3d_display->backbuffer_bmp.display = d3d_display;

   /* Alpha blending is the default */
   d3d_display->device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   d3d_display->device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   d3d_display->device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

   return true;
}


static ALLEGRO_DISPLAY *d3d_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)_AL_MALLOC(sizeof(ALLEGRO_DISPLAY_D3D));
   ALLEGRO_DISPLAY_D3D **add;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;
   int adapter = al_get_current_video_adapter();
   if (adapter == -1)
      adapter = 0;

   memset(d3d_display, 0, sizeof *d3d_display);
 
   d3d_display->adapter = adapter;
   d3d_display->ignore_ack = false;
   al_display->w = w;
   al_display->h = h;
   al_display->format = al_get_new_display_format();
   al_display->refresh_rate = al_get_new_display_refresh_rate();
   al_display->flags = al_get_new_display_flags();
   al_display->vt = vt;
   
#ifdef WANT_D3D9EX
   if (!is_vista) {
#endif   
	   if (al_display->flags & ALLEGRO_FULLSCREEN) {
	      if (already_fullscreen || d3d_created_displays._size != 0) {
	   	      d3d_display->faux_fullscreen = true;
		  }
		  else {
			  already_fullscreen = true;
			  d3d_display->faux_fullscreen = false;
		  }
	   }
	   else {
	   	d3d_display->faux_fullscreen = false;
	   }
#ifdef WANT_D3D9EX
   }
   else {
	   d3d_display->faux_fullscreen = false;
   }
#endif

   TRACE("faux_fullscreen=%d\n", d3d_display->faux_fullscreen);

   if (!d3d_create_display_internals(d3d_display)) {
      TRACE("d3d_create_display failed.\n");
      _AL_FREE(d3d_display);
      return NULL;
   }

   /* Add ourself to the list of displays. */
   add = (ALLEGRO_DISPLAY_D3D **)_al_vector_alloc_back(&system->system.displays);
   *add = d3d_display;

   /* Each display is an event source. */
   _al_event_source_init(&al_display->es);

   /* Keep track of the displays created */
   add = (ALLEGRO_DISPLAY_D3D **)_al_vector_alloc_back(&d3d_created_displays);
   *add = d3d_display;

   _al_win_active_window = win_display->window;

   /* Setup the mouse */
 
   win_display->mouse_range_x1 = 0;
   win_display->mouse_range_y1 = 0;
   win_display->mouse_range_x2 = w;
   win_display->mouse_range_y2 = h;
   if (al_is_mouse_installed()) {
      al_set_mouse_xy(w/2, h/2);
      al_set_mouse_range(0, 0, w, h);
   }

   win_display->mouse_selected_hcursor = 0;
   win_display->mouse_cursor_shown = false;

   // Activate the window (grabs input)
   SendMessage(win_display->window, WM_ACTIVATE, WA_ACTIVE, 0);

   return al_display;
}

static bool d3d_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)d;

   if (d3d_display->do_reset)
	   return false;

   al_set_current_video_adapter(d3d_display->adapter);

   return true;
}


static int d3d_al_blender_to_d3d(int al_mode)
{
   int num_modes = 4;

   int allegro_modes[] = {
      ALLEGRO_ZERO,
      ALLEGRO_ONE,
      ALLEGRO_ALPHA,
      ALLEGRO_INVERSE_ALPHA
   };

   int d3d_modes[] = {
      D3DBLEND_ZERO,
      D3DBLEND_ONE,
      D3DBLEND_SRCALPHA,
      D3DBLEND_INVSRCALPHA
   };

   int i;

   for (i = 0; i < num_modes; i++) {
      if (al_mode == allegro_modes[i]) {
         return d3d_modes[i];
      }
   }

   TRACE("Unknown blend mode.");

   return D3DBLEND_ONE;
}

void _al_d3d_set_blender(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   int src, dst;
   ALLEGRO_COLOR color;

   al_get_blender(&src, &dst, &color);

   src = d3d_al_blender_to_d3d(src);
   dst = d3d_al_blender_to_d3d(dst);

   if (d3d_display->device->SetRenderState(D3DRS_SRCBLEND, src) != D3D_OK)
      TRACE("Failed to set source blender");
   if (d3d_display->device->SetRenderState(D3DRS_DESTBLEND, dst) != D3D_OK)
      TRACE("Failed to set dest blender");
   d3d_display->device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_BLENDDIFFUSEALPHA);
}



static DWORD d3d_blend_colors(
   ALLEGRO_COLOR *color,
   ALLEGRO_COLOR *bc)
{
   ALLEGRO_COLOR result;
   float r, g, b, a;

   al_unmap_rgba_f(*color, &r, &g, &b, &a);

   result = al_map_rgba_f(
      r*bc->r,
      g*bc->g,
      b*bc->b,
      a*bc->a);

   return d3d_al_color_to_d3d(result);
}

/* Dummy implementation of line. */
static void d3d_draw_line(ALLEGRO_DISPLAY *al_display, float fx, float fy, float tx, float ty,
   ALLEGRO_COLOR *color)
{
   static D3D_COLORED_VERTEX points[2] = { { 0.0f, 0.0f, 0.0f, 0 }, };
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_COLOR *bc = _al_get_blend_color();
   DWORD d3d_color;
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)al_display;
   
   if (d3d_display->device_lost) return;

   if (!_al_d3d_render_to_texture_supported()) {
      _al_draw_line_memory((int)fx, (int)fy, (int)tx, (int)ty, color);
      return;
   }

   d3d_set_bitmap_clip(target);

   d3d_color = d3d_blend_colors(color, bc);

   fx -= 0.5f;
   fy -= 0.5f;
   tx -= 0.5f;
   ty -= 0.5f;

   if (target->parent) {
      fx += target->xofs;
      tx += target->xofs;
      fy += target->yofs;
      ty += target->yofs;
   }

   points[0].x = fx;
   points[0].y = fy;
   points[0].color = d3d_color;

   points[1].x = tx;
   points[1].y = ty;
   points[1].color = d3d_color;

   _al_d3d_set_blender(d3d_display);

   d3d_display->device->SetFVF(D3DFVF_COLORED_VERTEX);

   if (d3d_display->device->DrawPrimitiveUP(D3DPT_LINELIST, 1,
         points, sizeof(D3D_COLORED_VERTEX)) != D3D_OK) {
      TRACE("DrawPrimitive failed in d3d_draw_line.\n");
   }
}
 
static void d3d_draw_rectangle(ALLEGRO_DISPLAY *al_display, float tlx, float tly,
   float brx, float bry, ALLEGRO_COLOR *color, int flags)
{
   D3DRECT rect;
   float w = brx - tlx;
   float h = bry - tly;
   ALLEGRO_BITMAP *target;
   ALLEGRO_COLOR *bc = _al_get_blend_color();
   DWORD d3d_color;
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)al_display;

   if (!(flags & ALLEGRO_FILLED)) {
      d3d_draw_line(al_display, tlx, tly, brx, tly, color);
      d3d_draw_line(al_display, tlx, bry, brx, bry, color);
      d3d_draw_line(al_display, tlx, tly, tlx, bry, color);
      d3d_draw_line(al_display, brx, tly, brx, bry, color);
      return;
   }

   tlx -= 0.5f;
   tly -= 0.5f;
   brx -= 0.5f;
   bry -= 0.5f;

   if (d3d_display->device_lost) return;
   
   if (!_al_d3d_render_to_texture_supported()) {
      _al_draw_rectangle_memory((int)tlx, (int)tly, (int)brx, (int)bry, color, flags);
      return;
   }

   target = al_get_target_bitmap();

   d3d_set_bitmap_clip(target);

   d3d_color = d3d_blend_colors(color, bc);
   
   if (w < 1 || h < 1) {
      return;
   }

   if (target->parent) {
      tlx += target->xofs;
      brx += target->xofs;
      tly += target->yofs;
      bry += target->yofs;
   }
   
   rect.x1 = (LONG)tlx;
   rect.y1 = (LONG)tly;
   rect.x2 = (LONG)brx;
   rect.y2 = (LONG)bry;

   _al_d3d_set_blender(d3d_display);

   _al_d3d_draw_textured_quad(d3d_display, NULL,
      0.0f, 0.0f, w, h,
      tlx, tly, w, h,
      w/2, h/2, 0.0f,
      d3d_color, 0, false);
}

static void d3d_clear(ALLEGRO_DISPLAY *al_display, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;
   D3DRECT rect;
   int src, dst;
   ALLEGRO_COLOR blend_color;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   rect.x1 = 0;
   rect.y1 = 0;
   rect.x2 = target->w;
   rect.y2 = target->h;

   /* Clip */
   if (rect.x1 < target->cl)
      rect.x1 = target->cl;
   if (rect.y1 < target->ct)
      rect.y1 = target->ct;
   if (rect.x2 > target->cr)
      rect.x2 = target->cr;
   if (rect.y2 > target->cb)
      rect.y2 = target->cb;

   if (target->parent) {
      rect.x1 += target->xofs;
      rect.y1 += target->yofs;
      rect.x2 += target->xofs;
      rect.y2 += target->yofs;
   }

   al_get_blender(&src, &dst, &blend_color);
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));

   d3d_draw_rectangle(al_display, rect.x1, rect.y1, rect.x2+1, rect.y2+1, color, ALLEGRO_FILLED);

   al_set_blender(src, dst, blend_color);
}



void d3d_draw_pixel(ALLEGRO_DISPLAY *al_display, float x, float y, ALLEGRO_COLOR *color)
{
   d3d_draw_rectangle(al_display, x, y, x+1, y+1, color, ALLEGRO_FILLED);
}



static void d3d_flip_display(ALLEGRO_DISPLAY *al_display)
{
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   HRESULT hr;
   
   if (d3d_display->device_lost) return;

   d3d_display->device->EndScene();

   hr = d3d_display->device->Present(NULL, NULL, win_display->window, NULL);

   d3d_display->device->BeginScene();
   
   if (hr == D3DERR_DEVICELOST) {
      d3d_display->device_lost = true;
      return;
   }
}

static bool d3d_update_display_region(ALLEGRO_DISPLAY *al_display,
   int x, int y,
   int width, int height)
{
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   HRESULT hr;
   RGNDATA *rgndata;
   bool ret;
   
   if (d3d_display->device_lost) return false;

   if (al_display->flags & ALLEGRO_SINGLEBUFFER) {
      RECT rect;
      ALLEGRO_DISPLAY_D3D* disp = (ALLEGRO_DISPLAY_D3D*)al_display;

      rect.left = x;
      rect.right = x+width;
      rect.top = y;
      rect.bottom = y+height;

      rgndata = (RGNDATA *)malloc(sizeof(RGNDATA)+sizeof(RECT)-1);
      rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
      rgndata->rdh.iType = RDH_RECTANGLES;
      rgndata->rdh.nCount = 1;
      rgndata->rdh.nRgnSize = sizeof(RECT);
      memcpy(&rgndata->rdh.rcBound, &rect, sizeof(RECT));
      memcpy(rgndata->Buffer, &rect, sizeof(RECT));

      d3d_display->device->EndScene();

      hr = d3d_display->device->Present(&rect, &rect, win_display->window, rgndata);

      d3d_display->device->BeginScene();

      free(rgndata);

      if (hr == D3DERR_DEVICELOST) {
         d3d_display->device_lost = true;
         return true;
      }

      ret = true;
   }
   else {
      ret = false;
   }
   
   return ret;
}

/*
 * Sets a clipping rectangle
 */
void d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY_D3D *disp = ((ALLEGRO_BITMAP_D3D *)bitmap)->display;
   float plane[4];
   int left, right, top, bottom;

   if (!disp)
      return;

   if (bitmap->parent) {
	  left = bitmap->xofs + bitmap->cl;
	  right = bitmap->xofs + bitmap->cr;
	  top = bitmap->yofs + bitmap->ct;
	  bottom = bitmap->yofs + bitmap->cb;
   }
   else {
	  left = bitmap->cl;
	  right = bitmap->cr;
	  top = bitmap->ct;
	  bottom = bitmap->cb;
	  if (left == 0 && top == 0 &&
			right == (bitmap->w-1) &&
			bottom == (bitmap->h-1)) {
		 disp->device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
		 return;
	  }
   }

   if (left == 0) left = 1;
   if (top == 0) top = 1;
   if (right == 0) right = 1;
   if (bottom == 0) bottom = 1;

   plane[0] = 1.0f / left;
   plane[1] = 0.0f;
   plane[2] = 0.0f;
   plane[3] = -1;
   disp->device->SetClipPlane(0, plane);

   plane[0] = -1.0f / right;
   plane[1] = 0.0f;
   plane[2] = 0.0f;
   plane[3] = 1;
   disp->device->SetClipPlane(1, plane);

   plane[0] = 0.0f;
   plane[1] = 1.0f / top;
   plane[2] = 0.0f;
   plane[3] = -1;
   disp->device->SetClipPlane(2, plane);

   plane[0] = 0.0f;
   plane[1] = -1.0f / bottom;
   plane[2] = 0.0f;
   plane[3] = 1;
   disp->device->SetClipPlane(3, plane);

   /* Enable the first four clipping planes */
   disp->device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0xF);
}

static bool d3d_resize_display(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)d;
   ALLEGRO_DISPLAY_WIN *win_display = &disp->win_display;
   bool ret;

   disp->ignore_ack = true;

   if (d->flags & ALLEGRO_FULLSCREEN) {
      d3d_destroy_display_internals(disp);
      d->w = width;
      d->h = height;
      disp->end_thread = false;
      disp->initialized = false;
      disp->init_failed = false;
      disp->thread_ended = false;
	  /* What's this? */
	  if (d3d_created_displays._size <= 1) {
		  ffw_set = false;
	  }
      if (!d3d_create_display_internals(disp)) {
         _AL_FREE(disp);
         return false;
      }
      al_set_current_display(d);
      al_set_target_bitmap(al_get_backbuffer());
      _al_d3d_recreate_bitmap_textures(disp);

      disp->backbuffer_bmp.bitmap.w = width;
      disp->backbuffer_bmp.bitmap.h = height;

	  ret = true;
   }
   else {
      RECT win_size;
      WINDOWINFO wi;

      win_size.left = 0;
      win_size.top = 0;
      win_size.right = width;
      win_size.bottom = height;

      wi.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(win_display->window, &wi);

      AdjustWindowRectEx(&win_size, wi.dwStyle, FALSE, wi.dwExStyle);

      ret = (SetWindowPos(win_display->window, HWND_TOP,
         0, 0,
         win_size.right-win_size.left,
         win_size.bottom-win_size.top,
         SWP_NOMOVE|SWP_NOZORDER)) != 0;

      /*
       * The clipping rectangle and bitmap size must be
       * changed to match the new size.
       */
      _al_push_target_bitmap();
      al_set_target_bitmap(&disp->backbuffer_bmp.bitmap);
      disp->backbuffer_bmp.bitmap.w = width;
      disp->backbuffer_bmp.bitmap.h = height;
      al_set_clipping_rectangle(0, 0, width-1, height-1);
      d3d_set_bitmap_clip(&disp->backbuffer_bmp.bitmap);
      _al_pop_target_bitmap();
#if 0

      TRACE("resizing windowed display!\n");
      d3d_destroy_display_internals(disp);
      d->w = width;
      d->h = height;
      disp->end_thread = false;
      disp->initialized = false;
      disp->init_failed = false;
      disp->thread_ended = false;
	  /* What's this? */
	  if (d3d_created_displays._size <= 1) {
		  ffw_set = false;
	  }
      if (!d3d_create_display_internals(disp)) {
         _AL_FREE(disp);
         return false;
      }
      al_set_current_display(d);
      al_set_target_bitmap(al_get_backbuffer());
      _al_d3d_recreate_bitmap_textures(disp);

      disp->backbuffer_bmp.bitmap.w = width;
      disp->backbuffer_bmp.bitmap.h = height;
#endif
      ret = true;
   }


   return ret;
}

static bool d3d_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
      WINDOWINFO wi;
      ALLEGRO_DISPLAY *old;
      ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)d;
      ALLEGRO_DISPLAY_WIN *win_display = &disp->win_display;

      if (disp->ignore_ack) {
	      disp->ignore_ack = false;
	      return true;
      }

      wi.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(win_display->window, &wi);
      d->w = wi.rcClient.right - wi.rcClient.left;
      d->h = wi.rcClient.bottom - wi.rcClient.top;

      disp->backbuffer_bmp.bitmap.w = d->w;
      disp->backbuffer_bmp.bitmap.h = d->h;

      old = al_get_current_display();
      al_set_current_display(d);
      al_set_clipping_rectangle(0, 0, d->w-1, d->h-1);
      al_set_current_display(old);

      disp->do_reset = true;
      while (!disp->reset_done) {
	      al_rest(0.001);
      }
      disp->reset_done = false;

      return disp->reset_success;
}

ALLEGRO_BITMAP *_al_d3d_create_bitmap(ALLEGRO_DISPLAY *d,
   int w, int h)
{
   ALLEGRO_BITMAP_D3D *bitmap = (ALLEGRO_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);
   int format;
   int flags;

   ASSERT(bitmap);

   format = al_get_new_bitmap_format();
   flags = al_get_new_bitmap_flags();

   if (!_al_pixel_format_is_real(format)) {
      format = d3d_choose_bitmap_format(format);
      if (format < 0) {
         return NULL;
      }
   }
   
   bitmap->bitmap.vt = _al_bitmap_d3d_driver();
   bitmap->bitmap.memory = NULL;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;
   bitmap->bitmap.pitch = w * al_get_pixel_size(format);

   bitmap->video_texture = 0;
   bitmap->system_texture = 0;
   bitmap->initialized = false;
   bitmap->is_backbuffer = false;
   bitmap->render_target = NULL;

   bitmap->display = (ALLEGRO_DISPLAY_D3D *)d;

   return &bitmap->bitmap;
}

ALLEGRO_BITMAP *d3d_create_sub_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *parent,
   int x, int y, int width, int height)
{
   ALLEGRO_BITMAP_D3D *bitmap = (ALLEGRO_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);

   bitmap->texture_w = 0;
   bitmap->texture_h = 0;
   bitmap->video_texture = NULL;
   bitmap->system_texture = NULL;
   bitmap->initialized = false;
   bitmap->is_backbuffer = ((ALLEGRO_BITMAP_D3D *)parent)->is_backbuffer;
   bitmap->display = (ALLEGRO_DISPLAY_D3D *)display;
   bitmap->render_target = NULL;

   bitmap->bitmap.vt = parent->vt;
   return (ALLEGRO_BITMAP *)bitmap;
}

static void d3d_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *target;
   ALLEGRO_BITMAP_D3D *d3d_target;
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;

   if (d3d_display->device_lost) return;

   //_al_mutex_lock(&d3d_display->mutex);

   if (bitmap->parent) {
      target = bitmap->parent;
   }
   else {
      target = bitmap;
   }
   d3d_target = (ALLEGRO_BITMAP_D3D *)target;

   /* Release the previous target bitmap if it was not the backbuffer */

   d3d_release_current_target(true);

   /* Set the render target */
   if (d3d_target->is_backbuffer) {
	   TRACE("d3d_display=%p d3d_target->display=%p\n", d3d_display, d3d_target->display);
	   d3d_display = d3d_target->display;
	   TRACE("d3d_display=%p, device=%p target=%p\n", d3d_display, d3d_display->device, d3d_display->render_target);
      if (d3d_display->device->SetRenderTarget(0, d3d_display->render_target) != D3D_OK) {
         TRACE("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
		 //_al_mutex_unlock(&d3d_display->mutex);
         return;
      }
      _al_d3d_set_ortho_projection(d3d_display, display->w, display->h);
   }
   else {
	   d3d_display = (ALLEGRO_DISPLAY_D3D *)display;
      if (_al_d3d_render_to_texture_supported()) {
         if (d3d_target->video_texture->GetSurfaceLevel(0, &d3d_target->render_target) != D3D_OK) {
            TRACE("d3d_set_target_bitmap: Unable to get texture surface level.\n");
			//_al_mutex_unlock(&d3d_display->mutex);
            return;
         }
         if (d3d_display->device->SetRenderTarget(0, d3d_target->render_target) != D3D_OK) {
            TRACE("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
			   d3d_target->render_target->Release();
			//_al_mutex_unlock(&d3d_display->mutex);
            return;
         }
         _al_d3d_set_ortho_projection(d3d_display, d3d_target->texture_w, d3d_target->texture_h);
	  }
   }

   d3d_reset_state(d3d_display);

   d3d_set_bitmap_clip(bitmap);

   //_al_mutex_unlock(&d3d_display->mutex);
}

static ALLEGRO_BITMAP *d3d_get_backbuffer(ALLEGRO_DISPLAY *display)
{
   return (ALLEGRO_BITMAP *)&(((ALLEGRO_DISPLAY_D3D *)display)->backbuffer_bmp);
}

static ALLEGRO_BITMAP *d3d_get_frontbuffer(ALLEGRO_DISPLAY *display)
{
   return NULL;
}

static bool d3d_is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_D3D *d3d_bitmap = (ALLEGRO_BITMAP_D3D *)bitmap;
   //return (ALLEGRO_DISPLAY_D3D *)display == d3d_bitmap->display;
   /* Not entirely sure on this yet, but it seems one dsiplays bitmaps
    * can be displayed on other displays.
    */
   return true;
}

static void d3d_switch_out(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)display;
   _al_d3d_prepare_bitmaps_for_reset(disp);
}

static void  d3d_switch_in(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;

   if (al_is_mouse_installed())
      al_set_mouse_range(win_display->mouse_range_x1, win_display->mouse_range_y1,
         win_display->mouse_range_x2, win_display->mouse_range_y2);
}

static bool d3d_wait_for_vsync(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *d3d_display;
   D3DRASTER_STATUS status;

   if (!d3d_can_wait_for_vsync)
      return false;

   d3d_display = (ALLEGRO_DISPLAY_D3D *)display;

   do {
      d3d_display->device->GetRasterStatus(0, &status);
   } while (!status.InVBlank);

   return true;
}



static bool d3d_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id);

ALLEGRO_MOUSE_CURSOR *d3d_create_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *sprite, int xfocus, int yfocus)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   HWND wnd = win_display->window;
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor;

   win_cursor = _al_win_create_mouse_cursor(wnd, sprite, xfocus, yfocus);
   return (ALLEGRO_MOUSE_CURSOR *) win_cursor;
}

static void d3d_destroy_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor = (ALLEGRO_MOUSE_CURSOR_WIN *) cursor;

   ASSERT(win_cursor);

   if (win_cursor->hcursor == win_display->mouse_selected_hcursor) {
      d3d_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
   }

   _al_win_destroy_mouse_cursor(win_cursor);
}

static bool d3d_set_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   ALLEGRO_MOUSE_CURSOR_WIN *win_cursor = (ALLEGRO_MOUSE_CURSOR_WIN *) cursor;

   ASSERT(win_cursor);
   ASSERT(win_cursor->hcursor);

   win_display->mouse_selected_hcursor = win_cursor->hcursor;

   if (win_display->mouse_cursor_shown) {
      _al_win_set_mouse_hcursor(win_cursor->hcursor);
   }

   return true;
}

static bool d3d_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;
   HCURSOR wc;

   wc = _al_win_system_cursor_to_hcursor(cursor_id);
   if (!wc) {
      return false;
   }

   win_display->mouse_selected_hcursor = wc;
   if (win_display->mouse_cursor_shown) {
      /*
      MySetCursor(wc);
      PostMessage(wgl_display->window, WM_MOUSEMOVE, 0, 0);
      */
      _al_win_set_mouse_hcursor(wc);
   }
   return true;
}

static bool d3d_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;

   /* XXX do we need this? */
   if (!win_display->mouse_selected_hcursor) {
      d3d_set_system_mouse_cursor(display, ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
   }

   _al_win_set_mouse_hcursor(win_display->mouse_selected_hcursor);
   win_display->mouse_cursor_shown = true;

   return true;
}

static bool d3d_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *) display;

   _al_win_set_mouse_hcursor(NULL);
   win_display->mouse_cursor_shown = false;

   PostMessage(win_display->window, WM_SETCURSOR, 0, 0);

   return true;
}



/* Exposed stuff */

LPDIRECT3DDEVICE9 al_d3d_get_device(ALLEGRO_DISPLAY *display)
{
	ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;
	return d3d_display->device;
}


HWND al_d3d_get_hwnd(ALLEGRO_DISPLAY *display)
{
	return ((ALLEGRO_DISPLAY_WIN *)display)->window;
}


LPDIRECT3DTEXTURE9 al_d3d_get_system_texture(ALLEGRO_BITMAP *bitmap)
{
	return ((ALLEGRO_BITMAP_D3D *)bitmap)->system_texture;
}


LPDIRECT3DTEXTURE9 al_d3d_get_video_texture(ALLEGRO_BITMAP *bitmap)
{
	return ((ALLEGRO_BITMAP_D3D *)bitmap)->video_texture;
}

static void d3d_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   _al_win_set_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
}

static void d3d_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   if (display->flags & ALLEGRO_FULLSCREEN) {
      ALLEGRO_MONITOR_INFO info;
      ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;
      al_get_monitor_info(d3d_display->adapter, &info);
      *x = info.x1;
      *y = info.y1;
   }
   else {
      _al_win_get_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
   }
}

static void d3d_toggle_frame(ALLEGRO_DISPLAY *display, bool onoff)
{
   _al_win_toggle_window_frame(
      display,
      ((ALLEGRO_DISPLAY_WIN *)display)->window,
      display->w, display->h, onoff);
}

/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_d3d_driver(void)
{
   if (vt) return vt;

   vt = (ALLEGRO_DISPLAY_INTERFACE *)_AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = d3d_create_display;
   vt->destroy_display = d3d_destroy_display;
   vt->set_current_display = d3d_set_current_display;
   vt->clear = d3d_clear;
   vt->draw_line = d3d_draw_line;
   vt->draw_rectangle = d3d_draw_rectangle;
   vt->draw_pixel = d3d_draw_pixel;
   vt->flip_display = d3d_flip_display;
   vt->update_display_region = d3d_update_display_region;
   vt->acknowledge_resize = d3d_acknowledge_resize;
   vt->resize_display = d3d_resize_display;
   vt->create_bitmap = _al_d3d_create_bitmap;
   vt->set_target_bitmap = d3d_set_target_bitmap;
   vt->get_backbuffer = d3d_get_backbuffer;
   vt->get_frontbuffer = d3d_get_frontbuffer;
   vt->is_compatible_bitmap = d3d_is_compatible_bitmap;
   vt->switch_out = d3d_switch_out;
   vt->switch_in = d3d_switch_in;
   vt->draw_memory_bitmap_region = NULL;
   vt->create_sub_bitmap = d3d_create_sub_bitmap;
   vt->wait_for_vsync = d3d_wait_for_vsync;

   vt->create_mouse_cursor = d3d_create_mouse_cursor;
   vt->destroy_mouse_cursor = d3d_destroy_mouse_cursor;
   vt->set_mouse_cursor = d3d_set_mouse_cursor;
   vt->set_system_mouse_cursor = d3d_set_system_mouse_cursor;
   vt->show_mouse_cursor = d3d_show_mouse_cursor;
   vt->hide_mouse_cursor = d3d_hide_mouse_cursor;

   vt->set_icon = _al_win_set_display_icon;
   vt->set_window_position = d3d_set_window_position;
   vt->get_window_position = d3d_get_window_position;
   vt->toggle_frame = d3d_toggle_frame;
   vt->set_window_title = _al_win_set_window_title;

   return vt;
}

int _al_d3d_get_num_display_modes(int format, int refresh_rate, int flags)
{
   UINT num_modes;
   UINT i, j;
   D3DDISPLAYMODE display_mode;
   int matches = 0;

   /* If any, go through all formats */
   if (!_al_pixel_format_is_real(format)) {
      j = 0;
   }
   /* Else find the matching format */
   else {
      for (j = 0; allegro_formats[j] != -1; j++) {
         if (allegro_formats[j] == format)
	    break;
      }
      if (allegro_formats[j] == -1)
         return 0;
   }

   for (; allegro_formats[j] != -1; j++) {
      int adapter = al_get_current_video_adapter();
      if (adapter == -1)
         adapter = 0;

      if (!_al_pixel_format_is_real(allegro_formats[j]))
         continue;

      num_modes = _al_d3d->GetAdapterModeCount(adapter, (D3DFORMAT)d3d_formats[j]);
   
      for (i = 0; i < num_modes; i++) {
         if (_al_d3d->EnumAdapterModes(adapter, (D3DFORMAT)_al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
            return matches;
         }
         if (refresh_rate && display_mode.RefreshRate != (unsigned)refresh_rate)
            continue;
         matches++;
      }
   
      if (_al_pixel_format_is_real(format))
         break;
   }

   return matches;
}

ALLEGRO_DISPLAY_MODE *_al_d3d_get_display_mode(int index, int format,
   int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode)
{
   UINT num_modes;
   UINT i, j;
   D3DDISPLAYMODE display_mode;
   int matches = 0;

   /* If any, go through all formats */
   if (!_al_pixel_format_is_real(format)) {
      j = 0;
   }
   /* Else find the matching format */
   else {
      for (j = 0; allegro_formats[j] != -1; j++) {
         if (allegro_formats[j] == format)
	    break;
      }
      if (allegro_formats[j] == -1)
         return NULL;
   }

   for (; allegro_formats[j] != -1; j++) {
      int adapter = al_get_current_video_adapter();
      if (adapter == -1)
         adapter = 0;

      if (!_al_pixel_format_is_real(allegro_formats[j]))
         continue;

      num_modes = _al_d3d->GetAdapterModeCount(adapter, (D3DFORMAT)d3d_formats[j]);
   
      for (i = 0; i < num_modes; i++) {
         if (_al_d3d->EnumAdapterModes(adapter, (D3DFORMAT)_al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
            return NULL;
         }
         if (refresh_rate && display_mode.RefreshRate != (unsigned)refresh_rate)
            continue;
         if (matches == index) {
            mode->width = display_mode.Width;
            mode->height = display_mode.Height;
            mode->format = allegro_formats[j];
            mode->refresh_rate = display_mode.RefreshRate;
   	    return mode;
         }
         matches++;
      }
   
      if (_al_pixel_format_is_real(format))
         break;
   }

   return mode;
}


int _al_d3d_get_num_video_adapters(void)
{
	return _al_d3d->GetAdapterCount();
}

void _al_d3d_get_monitor_info(int adapter, ALLEGRO_MONITOR_INFO *info)
{
	HMONITOR mon = _al_d3d->GetAdapterMonitor(adapter);
	MONITORINFO mi;

	TRACE("adapter=%d\n", adapter);

	if (!mon) {
		info->x1 = info->y1 = info->x2 = info->y2 = -1;
	}
	else {
		mi.cbSize = sizeof(mi);
		GetMonitorInfo(mon, &mi);
		info->x1 = mi.rcMonitor.left;
		info->y1 = mi.rcMonitor.top;
		info->x2 = mi.rcMonitor.right;
		info->y2 = mi.rcMonitor.bottom;
	}
	

}


} // end extern "C"
