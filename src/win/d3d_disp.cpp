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

#include <windows.h>

#include <string.h>
#include <stdio.h>
#include <process.h>
#include <math.h>

#include "allegro5/allegro.h"

#include "allegro5/system.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_tri_soft.h" // For ALLEGRO_VERTEX
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_wclipboard.h"
#include "allegro5/platform/aintwin.h"

#include "d3d.h"
#include "prim_directx_shader.inc"

// Based on the code in <VersionHelpers.h> but that header is not
// available on Windows 7 SDK and earlier.
static BOOL is_windows_vista_or_greater() {
   OSVERSIONINFOEX info;
   ULONGLONG mask = 0;
   VER_SET_CONDITION(mask, VER_MAJORVERSION, VER_GREATER_EQUAL);
   VER_SET_CONDITION(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
   VER_SET_CONDITION(mask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
   memset(&info, 0, sizeof(info));
   info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
   info.dwMajorVersion = 6;

   return VerifyVersionInfo(&info, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, mask) != FALSE;
}

static void d3d_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap);
static void d3d_update_transformation(ALLEGRO_DISPLAY* disp, ALLEGRO_BITMAP *target);
static bool d3d_init_display();

// C++ needs to cast void pointers
#define get_extra(b) ((ALLEGRO_BITMAP_EXTRA_D3D *)\
   (b->parent ? b->parent->extra : b->extra))

static const char* _al_d3d_module_name = "d3d9.dll";

ALLEGRO_DEBUG_CHANNEL("d3d")

static ALLEGRO_DISPLAY_INTERFACE *vt = 0;

static HMODULE _al_d3d_module = 0;

typedef LPDIRECT3D9 (WINAPI *DIRECT3DCREATE9PROC)(UINT);

static DIRECT3DCREATE9PROC _al_d3d_create = (DIRECT3DCREATE9PROC)NULL;

#ifdef ALLEGRO_CFG_D3D9EX
typedef HRESULT (WINAPI *DIRECT3DCREATE9EXPROC)(UINT, LPDIRECT3D9EX*);

static DIRECT3DCREATE9EXPROC _al_d3d_create_ex = (DIRECT3DCREATE9EXPROC)NULL;
#endif

LPDIRECT3D9 _al_d3d = 0;

static D3DPRESENT_PARAMETERS d3d_pp;

static HWND fullscreen_focus_window;
static bool ffw_set = false;

static bool d3d_can_wait_for_vsync;

static bool render_to_texture_supported = true;
static bool is_vista = false;
static int num_faux_fullscreen_windows = 0;
static bool already_fullscreen = false; /* real fullscreen */

static ALLEGRO_MUTEX *present_mutex;
ALLEGRO_MUTEX *_al_d3d_lost_device_mutex;

/*
 * In the context of this file, legacy cards pretty much refer to older Intel cards.
 * They are distinguished by three misfeatures:
 * 1. They don't support shaders
 * 2. They don't support custom vertices
 * 3. DrawIndexedPrimitiveUP is broken
 *
 * Since shaders are used 100% of the time, this means that for these cards
 * the incoming vertices are first converted into the vertex type that these cards
 * can handle.
 */
static bool use_fixed_pipeline = false;
static bool know_card_type = false;

static bool check_use_fixed_pipeline(LPDIRECT3DDEVICE9 device)
{
   if (!know_card_type) {
      ALLEGRO_CONFIG* sys_cfg = al_get_system_config();
      const char* detection_setting = al_get_config_value(sys_cfg, "graphics", "prim_d3d_legacy_detection");
      detection_setting = detection_setting ? detection_setting : "default";
      if (strcmp(detection_setting, "default") == 0) {
         D3DCAPS9 caps;
         device->GetDeviceCaps(&caps);
         if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
            use_fixed_pipeline = true;
      } else if(strcmp(detection_setting, "force_legacy") == 0) {
         use_fixed_pipeline = true;
      } else if(strcmp(detection_setting, "force_modern") == 0) {
          use_fixed_pipeline = false;
      } else {
         ALLEGRO_WARN("Invalid setting for prim_d3d_legacy_detection.\n");
         use_fixed_pipeline = false;
      }
      if (use_fixed_pipeline) {
         ALLEGRO_WARN("Your GPU is considered legacy! Some of the features of the primitives addon will be slower/disabled.\n");
      }
      know_card_type = true;
   }
   return use_fixed_pipeline;
}

static ALLEGRO_MUTEX *primitives_mutex;
static char *fixed_buffer = NULL;
static size_t fixed_buffer_size = 0;

#define FOURCC(c0, c1, c2, c3) ((int)(c0) | ((int)(c1) << 8) | ((int)(c2) << 16) | ((int)(c3) << 24))

/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
struct D3D_DISPLAY_PARAMETERS {
   ALLEGRO_DISPLAY_D3D *display;
   volatile bool init_failed;
   HANDLE AckEvent;
   int window_x, window_y;
   /* Not owned. */
   const char *window_title;
};


static int allegro_formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY,
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_XRGB_8888,
   ALLEGRO_PIXEL_FORMAT_ARGB_8888,
   //ALLEGRO_PIXEL_FORMAT_ARGB_4444,  this format seems not to be allowed
   ALLEGRO_PIXEL_FORMAT_RGB_565,
   //ALLEGRO_PIXEL_FORMAT_ARGB_1555,  this format seems not to be allowed
   ALLEGRO_PIXEL_FORMAT_ABGR_F32,
   ALLEGRO_PIXEL_FORMAT_SINGLE_CHANNEL_8,
   ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1,
   ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3,
   ALLEGRO_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5,
   -1
};

/* Mapping of Allegro formats to D3D formats */
static int d3d_formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY,
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   D3DFMT_X8R8G8B8,
   D3DFMT_A8R8G8B8,
   //D3DFMT_A4R4G4B4,
   D3DFMT_R5G6B5,
   //D3DFMT_A1R5G5B5,
   D3DFMT_A32B32G32R32F,
   D3DFMT_L8,
   FOURCC('D', 'X', 'T', '1'),
   FOURCC('D', 'X', 'T', '3'),
   FOURCC('D', 'X', 'T', '5'),
   -1
};

static void (*d3d_release_callback)(ALLEGRO_DISPLAY *display) = NULL;
static void (*d3d_restore_callback)(ALLEGRO_DISPLAY *display) = NULL;

#define ERR(e) case e: return #e;
static char const *_al_d3d_error_string(HRESULT e)
{
   switch (e) {
      ERR(D3D_OK)
      ERR(D3DERR_NOTAVAILABLE)
      ERR(D3DERR_DEVICELOST)
      ERR(D3DERR_INVALIDCALL)
      ERR(D3DERR_OUTOFVIDEOMEMORY)
      ERR(E_OUTOFMEMORY)
   }
   return "UNKNOWN";
}
#undef ERR

static D3DFORMAT d3d_get_depth_stencil_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *settings)
{
   struct DEPTH_STENCIL_DESC {
      int d;
      int s;
      D3DFORMAT format;
   };

   DEPTH_STENCIL_DESC formats[] = {
      {  0, 0, (D3DFORMAT)0 },
      { 32, 0, D3DFMT_D32 },
      { 15, 1, D3DFMT_D15S1 },
      { 24, 8, D3DFMT_D24S8 },
      { 24, 0, D3DFMT_D24X8 },
      { 24, 4, D3DFMT_D24X4S4 },
      { 16, 0, D3DFMT_D16 },
      { -1, -1, (D3DFORMAT)0 }
   };


   for (int i = 0; formats[i].d >= 0; i++) {
      if (settings->settings[ALLEGRO_DEPTH_SIZE] == formats[i].d &&
            settings->settings[ALLEGRO_STENCIL_SIZE] == formats[i].s)
         return formats[i].format;
   }

   return (D3DFORMAT)0;
}

static void d3d_call_callbacks(_AL_VECTOR *v, ALLEGRO_DISPLAY *display)
{
   int i;
   for (i = 0; i < (int)v->_size; i++) {
      void (**callback)(ALLEGRO_DISPLAY *) = (void (**)(ALLEGRO_DISPLAY *))_al_vector_ref(v, i);
      (*callback)(display);
   }
}

/* Function: al_set_d3d_device_release_callback
 */
void al_set_d3d_device_release_callback(
   void (*callback)(ALLEGRO_DISPLAY *display))
{
   d3d_release_callback = callback;
}

/* Function: al_set_d3d_device_restore_callback
 */
void al_set_d3d_device_restore_callback(
   void (*callback)(ALLEGRO_DISPLAY *display))
{
   d3d_restore_callback = callback;
}

bool _al_d3d_supports_separate_alpha_blend(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)display;
   return d3d_disp->supports_separate_alpha_blend;
}

/* Function: al_have_d3d_non_pow2_texture_support
 */
bool al_have_d3d_non_pow2_texture_support(void)
{
   D3DCAPS9 caps;
   int adapter = al_get_new_display_adapter();

   if (!_al_d3d && !d3d_init_display())
      return false;
   if (adapter < 0)
      adapter = 0;

   /* This might have to change for multihead */
   if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps) != D3D_OK) {
      if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_REF, &caps) != D3D_OK) {
         return false;
      }
   }

   if ((caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0) {
      return true;
   }

   return false;
}

static int d3d_get_max_texture_size(int adapter)
{
   D3DCAPS9 caps;

   if (!_al_d3d && !d3d_init_display())
      return -1;

   if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps) != D3D_OK) {
      if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_REF, &caps) != D3D_OK) {
         return -1;
      }
   }

   return _ALLEGRO_MIN(caps.MaxTextureWidth, caps.MaxTextureHeight);
}

/* Function: al_have_d3d_non_square_texture_support
 */
bool al_have_d3d_non_square_texture_support(void)
{
   D3DCAPS9 caps;
   int adapter = al_get_new_display_adapter();
   if (!_al_d3d && !d3d_init_display())
      return false;
   if (adapter < 0)
      adapter = 0;

   /* This might have to change for multihead */
   if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps) != D3D_OK) {
      if (_al_d3d->GetDeviceCaps(adapter, D3DDEVTYPE_REF, &caps) != D3D_OK) {
         return false;
      }
   }

   if ((caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) == 0) {
      return true;
   }

   return false;
}


int _al_pixel_format_to_d3d(int format)
{
   int i;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      if (!_al_pixel_format_is_real(allegro_formats[i]))
         continue;
      if (allegro_formats[i] == format) {
         return d3d_formats[i];
      }
   }

   //return D3DFMT_R5G6B5;
   return -1;
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

int _al_d3d_num_display_formats(void)
{
   return sizeof(d3d_formats) / sizeof(d3d_formats[0]);
}

void _al_d3d_get_nth_format(int i, int *allegro, D3DFORMAT *d3d)
{
   *allegro = allegro_formats[i];
   *d3d = (D3DFORMAT)d3d_formats[i];
}

static void d3d_reset_state(ALLEGRO_DISPLAY_D3D *disp)
{
   if (disp->device_lost)
      return;

   disp->blender_state_op          = -1;
   disp->blender_state_src         = -1;
   disp->blender_state_dst         = -1;
   disp->blender_state_alpha_op    = -1;
   disp->blender_state_alpha_src   = -1;
   disp->blender_state_alpha_dst   = -1;

   disp->scissor_state.bottom = -1;
   disp->scissor_state.top    = -1;
   disp->scissor_state.left   = -1;
   disp->scissor_state.right  = -1;

   disp->device->SetRenderState(D3DRS_LIGHTING, FALSE);
   disp->device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
   disp->device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

   _al_d3d_update_render_state((ALLEGRO_DISPLAY *)disp);
}

static bool d3d_display_mode_matches(D3DDISPLAYMODE *dm, int w, int h, int format, int refresh_rate)
{
   if ((dm->Width == (unsigned int)w) &&
       (dm->Height == (unsigned int)h) &&
       ((!refresh_rate) || (dm->RefreshRate == (unsigned int)refresh_rate)) &&
       ((int)dm->Format == (int)_al_pixel_format_to_d3d(format))) {
          return true;
   }
   return false;
}

static bool d3d_check_mode(int w, int h, int format, int refresh_rate, UINT adapter)
{
   UINT num_modes;
   UINT i;
   D3DDISPLAYMODE display_mode;

   if (!_al_d3d && !d3d_init_display())
      return false;

   num_modes = _al_d3d->GetAdapterModeCount(adapter, (D3DFORMAT)_al_pixel_format_to_d3d(format));

   for (i = 0; i < num_modes; i++) {
      if (_al_d3d->EnumAdapterModes(adapter, (D3DFORMAT)_al_pixel_format_to_d3d(format), i, &display_mode) != D3D_OK) {
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
   if (!_al_d3d && !d3d_init_display())
      return 0;
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

   (void)flags;

   if (!d3d_check_mode(al_display->w, al_display->h, format, refresh_rate, win_display->adapter)) {
      ALLEGRO_ERROR("d3d_create_fullscreen_device: Mode not supported.\n");
      return 0;
   }

   ZeroMemory(&d3d_pp, sizeof(d3d_pp));
   d3d_pp.BackBufferFormat = (D3DFORMAT)_al_pixel_format_to_d3d(format);
   d3d_pp.BackBufferWidth = al_display->w;
   d3d_pp.BackBufferHeight = al_display->h;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 0;
   if (d->vsync) {
      d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
   }
   else {
      d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   }

   if (d->depth_stencil_format) {
      d3d_pp.EnableAutoDepthStencil = true;
      d3d_pp.AutoDepthStencilFormat = d->depth_stencil_format;
   }
   if (d->samples) {
      d3d_pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
      d3d_pp.MultiSampleQuality = d->samples;
   }
   else
      d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

   if (d->single_buffer) {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
   }
   else {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   }
   d3d_pp.hDeviceWindow = win_display->window;

   if (refresh_rate) {
      d3d_pp.FullScreen_RefreshRateInHz = refresh_rate;
   }
   else {
      d3d_pp.FullScreen_RefreshRateInHz = d3d_get_default_refresh_rate(win_display->adapter);
      al_display->refresh_rate = d3d_pp.FullScreen_RefreshRateInHz;
   }

   if (ffw_set == false) {
      fullscreen_focus_window = win_display->window;
      ffw_set = true;
   }
   else {
      reset_all = true;
   }

#ifdef ALLEGRO_CFG_D3D9EX
   if (is_vista) {
      D3DDISPLAYMODEEX mode;
      IDirect3D9Ex *d3d = (IDirect3D9Ex *)_al_d3d;
      mode.Size = sizeof(D3DDISPLAYMODEEX);
      mode.Width = al_display->w;
      mode.Height = al_display->h;
      mode.RefreshRate = d3d_pp.FullScreen_RefreshRateInHz;
      mode.Format = d3d_pp.BackBufferFormat;
      mode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;

      if ((ret = d3d->CreateDeviceEx(win_display->adapter,
               D3DDEVTYPE_HAL, fullscreen_focus_window,
               D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED|D3DCREATE_SCREENSAVER,
               &d3d_pp, &mode, (IDirect3DDevice9Ex **)(&d->device))) != D3D_OK) {
         if ((ret = d3d->CreateDeviceEx(win_display->adapter,
                  D3DDEVTYPE_HAL, fullscreen_focus_window,
                  D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED|D3DCREATE_SCREENSAVER,
                  &d3d_pp, &mode, (IDirect3DDevice9Ex **)(&d->device))) != D3D_OK) {
            if ((ret = d3d->CreateDeviceEx(win_display->adapter,
                     D3DDEVTYPE_REF, fullscreen_focus_window,
                     D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED|D3DCREATE_SCREENSAVER,
                     &d3d_pp, &mode, (IDirect3DDevice9Ex **)(&d->device))) != D3D_OK) {
               if ((ret = d3d->CreateDeviceEx(win_display->adapter,
                        D3DDEVTYPE_REF, fullscreen_focus_window,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED|D3DCREATE_SCREENSAVER,
                        &d3d_pp, &mode, (IDirect3DDevice9Ex **)(&d->device))) != D3D_OK) {
                  switch (ret) {
                     case D3DERR_INVALIDCALL:
                        ALLEGRO_ERROR("D3DERR_INVALIDCALL in create_device.\n");
                        break;
                     case D3DERR_NOTAVAILABLE:
                        ALLEGRO_ERROR("D3DERR_NOTAVAILABLE in create_device.\n");
                        break;
                     case D3DERR_OUTOFVIDEOMEMORY:
                        ALLEGRO_ERROR("D3DERR_OUTOFVIDEOMEMORY in create_device.\n");
                        break;
                     case D3DERR_DEVICELOST:
                        ALLEGRO_ERROR("D3DERR_DEVICELOST in create_device.\n");
                        break;
                     default:
                        ALLEGRO_ERROR("Direct3D Device creation failed.\n");
                        break;
                  }
                  return 0;
            }  }
         }
      }
   }
   else
#endif
   {
      if ((ret = _al_d3d->CreateDevice(win_display->adapter,
               D3DDEVTYPE_HAL, fullscreen_focus_window,
               D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
               &d3d_pp, &d->device)) != D3D_OK) {
         if ((ret = _al_d3d->CreateDevice(win_display->adapter,
                  D3DDEVTYPE_HAL, fullscreen_focus_window,
                  D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
                  &d3d_pp, &d->device)) != D3D_OK) {
            if ((ret = _al_d3d->CreateDevice(win_display->adapter,
                     D3DDEVTYPE_REF, fullscreen_focus_window,
                     D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
                     &d3d_pp, &d->device)) != D3D_OK) {
               if ((ret = _al_d3d->CreateDevice(win_display->adapter,
                        D3DDEVTYPE_REF, fullscreen_focus_window,
                        D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
                        &d3d_pp, &d->device)) != D3D_OK) {
                  switch (ret) {
                     case D3DERR_INVALIDCALL:
                        ALLEGRO_ERROR("D3DERR_INVALIDCALL in create_device.\n");
                        break;
                     case D3DERR_NOTAVAILABLE:
                        ALLEGRO_ERROR("D3DERR_NOTAVAILABLE in create_device.\n");
                        break;
                     case D3DERR_OUTOFVIDEOMEMORY:
                        ALLEGRO_ERROR("D3DERR_OUTOFVIDEOMEMORY in create_device.\n");
                        break;
                     case D3DERR_DEVICELOST:
                        ALLEGRO_ERROR("D3DERR_DEVICELOST in create_device.\n");
                        break;
                     default:
                        ALLEGRO_ERROR("Direct3D Device creation failed.\n");
                        break;
                  }
                  return 0;
               }
            }
         }
      }
   }

   d->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target);

   ALLEGRO_INFO("Fullscreen Direct3D device created.\n");

   d->device->BeginScene();

   ALLEGRO_SYSTEM *system = (ALLEGRO_SYSTEM *)al_get_system_driver();

   if (reset_all) {
      int i;
      for (i = 0; i < (int)system->displays._size; i++) {
         ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&system->displays, i);
         ALLEGRO_DISPLAY_D3D *disp = *dptr;
         if (disp != d) {
            if (disp != d && (disp->win_display.display.flags & ALLEGRO_FULLSCREEN)) {
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
      ALLEGRO_WARN("d3d_destroy_device: ref count not 0\n");
   }
   disp->device = NULL;
}


bool _al_d3d_render_to_texture_supported(void)
{
   return render_to_texture_supported;
}



static bool d3d_init_display()
{
   D3DDISPLAYMODE d3d_dm;

   _al_d3d_module = _al_win_safe_load_library(_al_d3d_module_name);
   if (_al_d3d_module == NULL) {
      ALLEGRO_ERROR("Failed to open '%s' library\n", _al_d3d_module_name);
      return false;
   }

   #ifdef ALLEGRO_CFG_D3DX9
      _al_load_d3dx9_module();
   #endif

   is_vista = is_windows_vista_or_greater();

#ifdef ALLEGRO_CFG_D3D9EX
   if (is_vista) {
      _al_d3d_create_ex = (DIRECT3DCREATE9EXPROC)GetProcAddress(_al_d3d_module, "Direct3DCreate9Ex");
      if (_al_d3d_create_ex != NULL) {
         if (_al_d3d_create_ex(D3D_SDK_VERSION, (LPDIRECT3D9EX *)&_al_d3d) != D3D_OK) {
            ALLEGRO_ERROR("Direct3DCreate9Ex failed\n");
            FreeLibrary(_al_d3d_module);
            return false;
         }
      }
      else {
         ALLEGRO_INFO("Direct3DCreate9Ex not in %s\n", _al_d3d_module_name);
         is_vista = false;
      }
   }

   if (!is_vista)
#endif
   {
      _al_d3d_create = (DIRECT3DCREATE9PROC)GetProcAddress(_al_d3d_module, "Direct3DCreate9");
      if (_al_d3d_create != NULL) {
         if ((_al_d3d = _al_d3d_create(D3D9b_SDK_VERSION)) == NULL) {
            ALLEGRO_ERROR("Direct3DCreate9 failed.\n");
               FreeLibrary(_al_d3d_module);
            return false;
         }
      }
      else {
         ALLEGRO_ERROR("Direct3DCreate9 not in %s\n", _al_d3d_module_name);
         FreeLibrary(_al_d3d_module);
         return false;
      }
   }

   _al_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3d_dm);

   if (_al_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
         D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_RENDERTARGET,
         D3DRTYPE_TEXTURE, d3d_dm.Format) != D3D_OK) {
      if (_al_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
            D3DDEVTYPE_REF, d3d_dm.Format, D3DUSAGE_RENDERTARGET,
            D3DRTYPE_TEXTURE, d3d_dm.Format) != D3D_OK) {
               render_to_texture_supported = false;
      }
      else
         render_to_texture_supported = true;
   }
   else
      render_to_texture_supported = true;


   ALLEGRO_INFO("Render-to-texture: %d\n", render_to_texture_supported);

   present_mutex = al_create_mutex();
   _al_d3d_lost_device_mutex = al_create_mutex();
   primitives_mutex = al_create_mutex();

#ifdef ALLEGRO_CFG_SHADER_HLSL
   _al_d3d_init_shaders();
#endif

   return true;
}


static ALLEGRO_DISPLAY_D3D *d3d_create_display_internals(ALLEGRO_DISPLAY_D3D *d3d_display, bool free_on_error);
static void d3d_destroy_display_internals(ALLEGRO_DISPLAY_D3D *display);


static void d3d_make_faux_fullscreen_stage_one(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   ALLEGRO_SYSTEM *system = al_get_system_driver();

   if (already_fullscreen || num_faux_fullscreen_windows) {
      int i;
      for (i = 0; i < (int)system->displays._size; i++) {
      ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&system->displays, i);
      ALLEGRO_DISPLAY_D3D *disp = *dptr;
         if (disp != d3d_display) {// && (disp->win_display.display.flags & ALLEGRO_FULLSCREEN)) {
            d3d_destroy_display_internals(disp);
            disp->win_display.end_thread = false;
            disp->win_display.thread_ended = false;
         }
      }
   }
}


static void d3d_make_faux_fullscreen_stage_two(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   ALLEGRO_SYSTEM *system = al_get_system_driver();

   if (already_fullscreen || num_faux_fullscreen_windows) {
      int i;
      already_fullscreen = false;
      for (i = 0; i < (int)system->displays._size; i++) {
         ALLEGRO_DISPLAY_D3D **dptr = (ALLEGRO_DISPLAY_D3D **)_al_vector_ref(&system->displays, i);
         ALLEGRO_DISPLAY_D3D *disp = *dptr;
         if (disp != d3d_display) {// && (disp->win_display.display.flags & ALLEGRO_FULLSCREEN)) {
            if (disp->win_display.display.flags & ALLEGRO_FULLSCREEN)
               disp->faux_fullscreen = true;
            disp = d3d_create_display_internals(disp, true);
            if (!disp) {
               ALLEGRO_ERROR("d3d_create_display_internals failed.\n");
               /* XXX we don't try to recover from this yet */
               abort();
            }
            ASSERT(disp);
            _al_d3d_recreate_bitmap_textures(disp);
         }
      }
   }
}

static bool d3d_create_device(ALLEGRO_DISPLAY_D3D *d,
   int format, int refresh_rate, int flags, bool convert_to_faux)
{
   HRESULT hr;
   ALLEGRO_DISPLAY_WIN *win_display = &d->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;
   int adapter = win_display->adapter;

   (void)refresh_rate;
   (void)flags;

   /* Ideally if you're targetting vanilla Direct3D 9 you should create
    * your windowed displays before any fullscreen ones. If you don't,
    * your fullscreen displays will be turned into "faux-fullscreen"
    * displays, basically screen-filling windows set out in front of
    * everything else.
    */
#ifdef ALLEGRO_CFG_D3D9EX
   if (convert_to_faux)
      d3d_make_faux_fullscreen_stage_one(d);
#else
   (void)convert_to_faux;
#endif

   ZeroMemory(&d3d_pp, sizeof(d3d_pp));

   d3d_pp.BackBufferFormat = (D3DFORMAT)_al_pixel_format_to_d3d(format);

   d3d_pp.BackBufferWidth = al_display->w;
   d3d_pp.BackBufferHeight = al_display->h;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 1;
   if (d->vsync) {
      d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
   }
   else {
      d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   }

   if (d->depth_stencil_format) {
      d3d_pp.EnableAutoDepthStencil = true;
      d3d_pp.AutoDepthStencilFormat = d->depth_stencil_format;
      ALLEGRO_INFO("Chose depth stencil format %d\n", d->depth_stencil_format);
   }
   else {
      ALLEGRO_INFO("Using no depth stencil buffer\n");
   }

   if (d->samples) {
      d3d_pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
      d3d_pp.MultiSampleQuality = d->samples;
   }
   else
      d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

   if (d->single_buffer) {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
   }
   else {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   }
   d3d_pp.hDeviceWindow = win_display->window;

   if (!refresh_rate) {
      al_display->refresh_rate = d3d_get_default_refresh_rate(win_display->adapter);
   }

   if (adapter < 0)
      adapter = 0;

   if ((hr = _al_d3d->CreateDevice(adapter,
         D3DDEVTYPE_HAL, win_display->window,
         D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
         &d3d_pp, (LPDIRECT3DDEVICE9 *)&d->device)) != D3D_OK) {
      ALLEGRO_DEBUG("trying D3DCREATE_SOFTWARE_VERTEXPROCESSING\n");
      if ((hr = _al_d3d->CreateDevice(adapter,
            D3DDEVTYPE_HAL, win_display->window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
            &d3d_pp, (LPDIRECT3DDEVICE9 *)&d->device)) != D3D_OK) {
         ALLEGRO_DEBUG("trying D3DDEVTYPE_REF\n");
         if ((hr = _al_d3d->CreateDevice(adapter,
               D3DDEVTYPE_REF, win_display->window,
               D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
               &d3d_pp, (LPDIRECT3DDEVICE9 *)&d->device)) != D3D_OK) {
            ALLEGRO_DEBUG("trying D3DDEVTYPE_REF|D3DCREATE_SOFTWARE_VERTEXPROCESSING\n");
            if ((hr = _al_d3d->CreateDevice(adapter,
                  D3DDEVTYPE_REF, win_display->window,
                  D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE|D3DCREATE_MULTITHREADED,
                  &d3d_pp, (LPDIRECT3DDEVICE9 *)&d->device)) != D3D_OK) {

               ALLEGRO_ERROR("CreateDevice failed: %s\n", _al_d3d_error_string(hr));
               return 0;
            }
         }
      }
   }

   if (d->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target) != D3D_OK) {
   //if (d->device->GetRenderTarget(0, &d->render_target) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_device: GetBackBuffer failed.\n");
      return 0;
   }

   if (d->device->BeginScene() != D3D_OK) {
      ALLEGRO_ERROR("BeginScene failed in create_device\n");
   }
   else {
      ALLEGRO_DEBUG("BeginScene succeeded in create_device\n");
   }

#ifdef ALLEGRO_CFG_D3D9EX
   if (convert_to_faux)
      d3d_make_faux_fullscreen_stage_two(d);
#endif

   ALLEGRO_DEBUG("Success\n");

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


static void d3d_destroy_display_internals(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   ALLEGRO_DISPLAY *al_display = (ALLEGRO_DISPLAY *)d3d_display;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;

   if (d3d_display->device) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
      _al_remove_display_invalidated_callback(al_display, _al_d3d_on_lost_shaders);
      _al_remove_display_validated_callback(al_display, _al_d3d_on_reset_shaders);
#endif
      d3d_call_callbacks(&al_display->display_invalidated_callbacks, al_display);
#ifdef ALLEGRO_CFG_SHADER_HLSL
      _al_add_display_invalidated_callback(al_display, _al_d3d_on_lost_shaders);
      _al_add_display_validated_callback(al_display, _al_d3d_on_reset_shaders);
#endif
      d3d_display->device->EndScene();
   }

   d3d_release_bitmaps((ALLEGRO_DISPLAY *)d3d_display);
   if (d3d_display->loop_index_buffer)
      d3d_display->loop_index_buffer->Release();

   ALLEGRO_DEBUG("waiting for display %p's thread to end\n", d3d_display);
   if (win_display->window) {
      SendMessage(win_display->window, _al_win_msg_suicide, 0, 0);
      while (!win_display->thread_ended)
         al_rest(0.001);
   }

   ASSERT(al_display->vt);
}

static void d3d_destroy_display(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_get_system_driver();
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;
   ALLEGRO_DISPLAY *old_disp = al_get_current_display();

   ALLEGRO_INFO("destroying display %p (current %p)\n", display, old_disp);

   if (old_disp != display)
      _al_set_current_display_only(display);

   if (system->mouse_grab_display == display)
      al_ungrab_mouse();

   _al_win_destroy_display_icons(display);

   d3d_destroy_display_internals(d3d_display);

   _al_vector_free(&display->display_invalidated_callbacks);
   _al_vector_free(&display->display_validated_callbacks);

   _al_vector_find_and_delete(&system->system.displays, &display);

   if (system->system.displays._size <= 0) {
      ffw_set = false;
      already_fullscreen = false;
   }

   if (d3d_display->es_inited) {
      _al_event_source_free(&display->es);
      d3d_display->es_inited = false;
   }

   _al_vector_free(&display->bitmaps);
   _al_vector_free(&((ALLEGRO_DISPLAY_WIN*) display)->msg_callbacks);

   if (old_disp != display)
      _al_set_current_display_only(old_disp);

   al_free(display->vertex_cache);
   al_free(display);
}

void _al_d3d_prepare_for_reset(ALLEGRO_DISPLAY_D3D *disp)
{
   ALLEGRO_DISPLAY *al_display = (ALLEGRO_DISPLAY *)disp;

   if (d3d_release_callback) {
      (*d3d_release_callback)(al_display);
   }

   d3d_call_callbacks(&al_display->display_invalidated_callbacks, al_display);

   _al_d3d_release_default_pool_textures((ALLEGRO_DISPLAY *)disp);
   while (disp->render_target && disp->render_target->Release() != 0) {
      ALLEGRO_WARN("_al_d3d_prepare_for_reset: (bb) ref count not 0\n");
   }
   disp->render_target = NULL;
   get_extra(al_get_backbuffer(al_display))->render_target = NULL;
}

static bool _al_d3d_reset_device(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;

   al_lock_mutex(_al_d3d_lost_device_mutex);

    _al_d3d_prepare_for_reset(d3d_display);

    if (al_display->flags & ALLEGRO_FULLSCREEN) {
       HRESULT hr;

       ZeroMemory(&d3d_pp, sizeof(d3d_pp));
       d3d_pp.BackBufferFormat = (D3DFORMAT)_al_pixel_format_to_d3d(_al_deduce_color_format(&al_display->extra_settings));
       d3d_pp.BackBufferWidth = al_display->w;
       d3d_pp.BackBufferHeight = al_display->h;
       d3d_pp.BackBufferCount = 1;
       d3d_pp.Windowed = 0;
       d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
       d3d_pp.hDeviceWindow = win_display->window;
       if (d3d_display->vsync) {
          d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
       }
       else {
          d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
       }

       if (d3d_display->depth_stencil_format) {
          d3d_pp.EnableAutoDepthStencil = true;
          d3d_pp.AutoDepthStencilFormat = d3d_display->depth_stencil_format;
       }
       if (d3d_display->samples) {
          d3d_pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
          d3d_pp.MultiSampleQuality = d3d_display->samples;
       }
       else
          d3d_pp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

       if (d3d_display->single_buffer) {
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
          d3d_pp.FullScreen_RefreshRateInHz = d3d_get_default_refresh_rate(win_display->adapter);
       }
#ifdef ALLEGRO_CFG_D3D9EX
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
       else
#endif
       {
          hr = d3d_display->device->Reset(&d3d_pp);
       }
       if (hr != D3D_OK) {
          switch (hr) {
             case D3DERR_INVALIDCALL:
                ALLEGRO_ERROR("D3DERR_INVALIDCALL in reset.\n");
                break;
             case D3DERR_NOTAVAILABLE:
                ALLEGRO_ERROR("D3DERR_NOTAVAILABLE in reset.\n");
                break;
             case D3DERR_OUTOFVIDEOMEMORY:
                ALLEGRO_ERROR("D3DERR_OUTOFVIDEOMEMORY in reset.\n");
                break;
             case D3DERR_DEVICELOST:
                ALLEGRO_ERROR("D3DERR_DEVICELOST in reset.\n");
                d3d_display->device_lost = true;
                break;
             default:
                ALLEGRO_ERROR("Direct3D Device reset failed (unknown reason).\n");
                break;
          }
          al_unlock_mutex(_al_d3d_lost_device_mutex);
          return 0;
       }
    }
    else {
       ZeroMemory(&d3d_pp, sizeof(d3d_pp));
       d3d_pp.BackBufferFormat = (D3DFORMAT)_al_pixel_format_to_d3d(_al_deduce_color_format(&al_display->extra_settings));
       d3d_pp.BackBufferWidth = al_display->w;
       d3d_pp.BackBufferHeight = al_display->h;
       d3d_pp.BackBufferCount = 1;
       d3d_pp.Windowed = 1;
       d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
       d3d_pp.hDeviceWindow = win_display->window;
       if (d3d_display->vsync) {
          d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
       }
       else {
          d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
       }

       if (d3d_display->depth_stencil_format) {
          d3d_pp.EnableAutoDepthStencil = true;
          d3d_pp.AutoDepthStencilFormat = d3d_display->depth_stencil_format;
       }
       if (d3d_display->samples) {
          d3d_pp.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
          d3d_pp.MultiSampleQuality = d3d_display->samples;
       }
       else
          d3d_pp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

       if (d3d_display->single_buffer) {
          d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
       }
       else {
          d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
       }

       /* Must be 0 for windowed modes */
       d3d_pp.FullScreen_RefreshRateInHz = 0;

       HRESULT hr = d3d_display->device->Reset(&d3d_pp);
       if (hr != D3D_OK) {
          switch (hr) {
             case D3DERR_INVALIDCALL:
                ALLEGRO_ERROR("D3DERR_INVALIDCALL in reset.\n");
                break;
             case D3DERR_NOTAVAILABLE:
                ALLEGRO_ERROR("D3DERR_NOTAVAILABLE in reset.\n");
                break;
             case D3DERR_OUTOFVIDEOMEMORY:
                ALLEGRO_ERROR("D3DERR_OUTOFVIDEOMEMORY in reset.\n");
                break;
             case D3DERR_DEVICELOST:
                ALLEGRO_ERROR("D3DERR_DEVICELOST in reset.\n");
                d3d_display->device_lost = true;
                break;
             default:
                ALLEGRO_ERROR("Direct3D Device reset failed (unknown reason).\n");
                break;
          }
          al_unlock_mutex(_al_d3d_lost_device_mutex);
          return 0;
       }
    }

   d3d_display->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d3d_display->render_target);

   _al_d3d_refresh_texture_memory(al_display);

   d3d_display->device->BeginScene();

   d3d_reset_state(d3d_display);

   /* Restore the target bitmap. */
   if (d3d_display->target_bitmap) {
      ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY*)d3d_display;
      d3d_set_target_bitmap(display, d3d_display->target_bitmap);
      d3d_update_transformation(display, d3d_display->target_bitmap);
   }

   al_unlock_mutex(_al_d3d_lost_device_mutex);

   return 1;
}

static int d3d_choose_display_format(int fake)
{
   /* Pick an appropriate format if the user is vague */
   switch (fake) {
      case ALLEGRO_PIXEL_FORMAT_ANY:
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_RGB_565;
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

   if (hr != D3D_OK) {
      hr = _al_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
         D3DDEVTYPE_REF,
         AdapterFormat,
         0,
         D3DRTYPE_TEXTURE,
         TextureFormat);
   }

   return SUCCEEDED(hr);
}

/* Same as above, but using Allegro's formats */
static bool is_texture_format_ok(ALLEGRO_DISPLAY *display, int texture_format)
{
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D*)display;
   return IsTextureFormatOk((D3DFORMAT)_al_pixel_format_to_d3d(texture_format),
      (D3DFORMAT)_al_pixel_format_to_d3d(d3d_display->format));
}

static int real_choose_bitmap_format(ALLEGRO_DISPLAY_D3D *d3d_display,
   int bits, bool alpha)
{
   int i;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      int aformat = allegro_formats[i];
      D3DFORMAT dformat;
      D3DFORMAT adapter_format;
      int adapter_format_allegro;
      if (!_al_pixel_format_is_real(aformat)) {
         ALLEGRO_DEBUG("Fake format\n");
         continue;
      }
      if (bits && al_get_pixel_format_bits(aformat) != bits) {
         ALLEGRO_DEBUG("#Bits don't match\n");
         continue;
      }
      if (alpha && !_al_pixel_format_has_alpha(aformat)) {
         ALLEGRO_DEBUG("Alpha doesn't match\n");
         continue;
      }
      dformat = (D3DFORMAT)d3d_formats[i];
      adapter_format_allegro = d3d_display->format;
      if (!_al_pixel_format_is_real(adapter_format_allegro))
         adapter_format_allegro = d3d_choose_display_format(adapter_format_allegro);
      ALLEGRO_DEBUG("Adapter format is %d\n", adapter_format_allegro);
      adapter_format = (D3DFORMAT)_al_pixel_format_to_d3d(adapter_format_allegro);
      if (IsTextureFormatOk(dformat, adapter_format)) {
         ALLEGRO_DEBUG("Found a format\n");
         return aformat;
      }
      ALLEGRO_DEBUG("Texture format not OK\n");
   }

   ALLEGRO_WARN("Failed to find format\n");

   return -1;
}

static int d3d_choose_bitmap_format(ALLEGRO_DISPLAY_D3D *d3d_display, int fake)
{
   switch (fake) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 0, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY:
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 0, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 32, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 32, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 24, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 16, false);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 16, true);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:
         fake = real_choose_bitmap_format(d3d_display, 15, false);
         break;
      default:
         fake = -1;
   }

   return fake;
}

struct CREATE_WINDOW_INFO {
   ALLEGRO_DISPLAY *display;
   DISPLAY_DEVICE dd;
   ALLEGRO_MONITOR_INFO mi;
   int w;
   int h;
   int refresh_rate;
   int flags;
   bool inited;
   bool quit;
};

static void d3d_create_window_proc(CREATE_WINDOW_INFO *info)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)info->display;

   win_display->window = _al_win_create_window(
      info->display,
      info->w,
      info->h,
      info->flags
   );
}

static void *d3d_create_faux_fullscreen_window_proc(void *arg)
{
   CREATE_WINDOW_INFO *info = (CREATE_WINDOW_INFO *)arg;
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)info->display;

   win_display->window =
      _al_win_create_faux_fullscreen_window(
         info->dd.DeviceName,
         info->display,
         info->mi.x1,
         info->mi.y1,
         info->w,
         info->h,
         info->refresh_rate,
         info->flags
      );

   return NULL;
}

/*
 * Display must be created in same thread that resets it
 */
static void *d3d_display_thread_proc(void *arg)
{
   ALLEGRO_DISPLAY_D3D *d3d_display;
   ALLEGRO_DISPLAY_WIN *win_display;
   ALLEGRO_DISPLAY *al_display;
   D3D_DISPLAY_PARAMETERS *params = (D3D_DISPLAY_PARAMETERS *)arg;
   int new_format;
   bool convert_to_faux;
   CREATE_WINDOW_INFO *info = (CREATE_WINDOW_INFO *)al_calloc(1, sizeof(*info));
   HRESULT hr;
   bool lost_event_generated = false;
   D3DCAPS9 caps;
   MSG msg;

   d3d_display = params->display;
   win_display = &d3d_display->win_display;
   al_display = &win_display->display;

   if (al_display->flags & ALLEGRO_FULLSCREEN) {
      convert_to_faux = true;
   }
   else {
      convert_to_faux = false;
   }

   /* So that we can call the functions using TLS from this thread. */
   al_set_new_display_flags(al_display->flags);
   al_set_new_window_position(params->window_x, params->window_y);
   al_set_new_window_title(params->window_title);

   new_format = _al_deduce_color_format(&al_display->extra_settings);

   /* This should never happen, I think */
   if (!_al_pixel_format_is_real(_al_deduce_color_format(&al_display->extra_settings))) {
      int f = d3d_choose_display_format(_al_deduce_color_format(&al_display->extra_settings));
      if (f < 0) {
         d3d_destroy_display(al_display);
         params->init_failed = true;
         SetEvent(params->AckEvent);
         return NULL;
      }
      new_format = f;
      _al_set_color_components(new_format, &al_display->extra_settings, ALLEGRO_REQUIRE);
   }

   ALLEGRO_INFO("Chose a display format: %d\n", new_format);
   d3d_display->format = new_format;

   if (d3d_display->faux_fullscreen) {
      DEVMODE dm;
      bool found = true;
      int refresh_rate;

      num_faux_fullscreen_windows++;

      d3d_make_faux_fullscreen_stage_one(d3d_display);

      al_get_monitor_info(win_display->adapter, &info->mi);
      /* Yes this is an "infinite" loop (suggested by MS on msdn) */
      for (int i = 0; ; i++) {
         info->dd.cb = sizeof(info->dd);
         if (!EnumDisplayDevices(NULL, i, &info->dd, 0)) {
            found = false;
            break;
         }
         if (!EnumDisplaySettings(info->dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
            continue;
         }
         if (info->mi.x1 == dm.dmPosition.x && info->mi.y1 == dm.dmPosition.y) {
            break;
         }

      }
      if (!found) {
         ALLEGRO_ERROR("d3d_display_thread_proc: Error setting faux fullscreen mode.\n");
         num_faux_fullscreen_windows--;
         d3d_destroy_display(al_display);
         params->init_failed = true;
         SetEvent(params->AckEvent);
         return NULL;
      }
      if (al_display->refresh_rate) {
         refresh_rate = al_display->refresh_rate;
      }
      else {
         refresh_rate = d3d_get_default_refresh_rate(win_display->adapter);
      }
      d3d_display->device_name = (TCHAR *)al_malloc(sizeof(TCHAR)*32);
      strcpy((char*)d3d_display->device_name, (char*)info->dd.DeviceName);
      ALLEGRO_DEBUG("going to call _al_win_create_faux_fullscreen_window\n");

      info->display = al_display;
      info->w = al_display->w;
      info->h = al_display->h;
      info->refresh_rate = refresh_rate;
      info->flags = al_display->flags;

      d3d_create_faux_fullscreen_window_proc(info);

      if (!win_display->window) {
         ALLEGRO_DEBUG("Failed to create window (faux)fullscreen.\n");
         d3d_destroy_display(al_display);
         params->init_failed = true;
         SetEvent(params->AckEvent);
         return NULL;
      }

      ALLEGRO_DEBUG("Called _al_win_create_faux_fullscreen_window.\n");

      d3d_make_faux_fullscreen_stage_two(d3d_display);

      convert_to_faux = false;
   }
   else {
      ALLEGRO_INFO("Normal window.\n");

      info->display = al_display;
      info->w = al_display->w;
      info->h = al_display->h;
      info->flags = al_display->flags;

      d3d_create_window_proc(info);
   }

   if (!win_display->window) {
         ALLEGRO_DEBUG("Failed to create regular window.\n");
      d3d_destroy_display(al_display);
      params->init_failed = true;
      SetEvent(params->AckEvent);
      return NULL;
   }

   if (!(al_display->flags & ALLEGRO_FULLSCREEN) || d3d_display->faux_fullscreen) {
      if (!d3d_create_device(d3d_display, _al_deduce_color_format(&al_display->extra_settings),
            al_display->refresh_rate, al_display->flags, convert_to_faux)) {
         d3d_destroy_display(al_display);
         params->init_failed = true;
         SetEvent(params->AckEvent);
         return NULL;
      }
   }
   else {
      ALLEGRO_DEBUG("Creating real fullscreen device\n");
      if (!d3d_create_fullscreen_device(d3d_display, _al_deduce_color_format(&al_display->extra_settings),
            al_display->refresh_rate, al_display->flags)) {
         d3d_destroy_display(al_display);
         params->init_failed = true;
         SetEvent(params->AckEvent);
         return NULL;
      }
      ALLEGRO_INFO("Real fullscreen device created\n");
   }

   al_display->backbuffer_format = _al_deduce_color_format(&al_display->extra_settings);


   d3d_display->device->GetDeviceCaps(&caps);
   d3d_can_wait_for_vsync = ((caps.Caps & D3DCAPS_READ_SCANLINE) != 0);

   params->init_failed = false;
   win_display->thread_ended = false;
   win_display->end_thread = false;
   SetEvent(params->AckEvent);

   while (!win_display->end_thread) {
      al_rest(0.001);

      if (PeekMessage(&msg, NULL, 0, 0, FALSE)) {
         if (GetMessage(&msg, NULL, 0, 0) != 0)
            DispatchMessage(&msg);
         else
            break;                  /* WM_QUIT received or error (GetMessage returned -1)  */
      }

      if (!d3d_display->device) {
         continue;
      }

      if (d3d_display->device_lost) {
         hr = d3d_display->device->TestCooperativeLevel();

         if (hr == D3D_OK) {
            d3d_display->device_lost = false;
         }
         else if (hr == D3DERR_DEVICELOST) {
            /* device remains lost */
            if (!lost_event_generated) {
               ALLEGRO_DEBUG("D3DERR_DEVICELOST: d3d_display=%p\n", d3d_display);
               lost_event_generated = true;
               if (d3d_display->suppress_lost_events) {
                  ALLEGRO_DEBUG("DISPLAY_LOST event suppressed\n");
               }
               else {
                  _al_event_source_lock(&al_display->es);
                  if (_al_event_source_needs_to_generate_event(&al_display->es)) {
                     ALLEGRO_EVENT event;
                     memset(&event, 0, sizeof(event));
                     event.display.type = ALLEGRO_EVENT_DISPLAY_LOST;
                     event.display.timestamp = al_get_time();
                     _al_event_source_emit_event(&al_display->es, &event);
                  }
                  _al_event_source_unlock(&al_display->es);
                  al_rest(0.5); // give user time to respond
               }
            }
         }
         else if (hr == D3DERR_DEVICENOTRESET) {
            if (_al_d3d_reset_device(d3d_display)) {
               d3d_display->device_lost = false;
               d3d_reset_state(d3d_display);
               _al_event_source_lock(&al_display->es);
               if (_al_event_source_needs_to_generate_event(&al_display->es)) {
                  ALLEGRO_EVENT event;
                  memset(&event, 0, sizeof(event));
                  event.display.type = ALLEGRO_EVENT_DISPLAY_FOUND;
                  event.display.timestamp = al_get_time();
                  _al_event_source_emit_event(&al_display->es, &event);
               }
               _al_event_source_unlock(&al_display->es);
               lost_event_generated = false;
               d3d_call_callbacks(&al_display->display_validated_callbacks, al_display);
               if (d3d_restore_callback) {
                  (*d3d_restore_callback)(al_display);
               }
            }
         }
      }
      if (d3d_display->do_reset) {
         d3d_display->reset_success = _al_d3d_reset_device(d3d_display);
         if (d3d_restore_callback) {
            (*d3d_restore_callback)(al_display);
         }
         d3d_display->do_reset = false;
         d3d_display->reset_done = true;
      }
   }

   d3d_destroy_device(d3d_display);

   if (d3d_display->faux_fullscreen) {
      ChangeDisplaySettingsEx(d3d_display->device_name, NULL, NULL, 0, NULL);
      al_free(d3d_display->device_name);
      num_faux_fullscreen_windows--;
   }

   win_display->thread_ended = true;

   al_free(info);

   ALLEGRO_INFO("d3d display thread exits\n");

   return NULL;
}


static ALLEGRO_DISPLAY_D3D *d3d_create_display_helper(int w, int h)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_get_system_driver();
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)al_malloc(sizeof(ALLEGRO_DISPLAY_D3D));
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;

   memset(d3d_display, 0, sizeof *d3d_display);

   win_display->adapter = _al_win_determine_adapter();

   /* w/h may be reset below if ALLEGRO_FULLSCREEN_WINDOW is set */
   al_display->w = w;
   al_display->h = h;
   al_display->refresh_rate = al_get_new_display_refresh_rate();
   al_display->flags = al_get_new_display_flags();
   al_display->vt = vt;
   ASSERT(al_display->vt);

#ifdef ALLEGRO_CFG_D3D9EX
   if (!is_vista)
#endif
   {
      if (al_display->flags & ALLEGRO_FULLSCREEN) {
         if (already_fullscreen || system->system.displays._size != 0) {
            d3d_display->faux_fullscreen = true;
         }
         else {
            already_fullscreen = true;
            d3d_display->faux_fullscreen = false;
         }
      }
   }
#ifdef ALLEGRO_CFG_D3D9EX
   else {
      d3d_display->faux_fullscreen = false;
   }
#endif

   if (!(al_display->flags & ALLEGRO_FULLSCREEN)) {
      if (al_display->flags & ALLEGRO_FULLSCREEN_WINDOW) {
         ALLEGRO_MONITOR_INFO mi;
         al_get_monitor_info(win_display->adapter, &mi);
         al_display->w = mi.x2 - mi.x1;
         al_display->h = mi.y2 - mi.y1;
         d3d_display->faux_fullscreen = true;
      }
      else {
         d3d_display->faux_fullscreen = false;
      }
      win_display->toggle_w = w;
      win_display->toggle_h = h;
   }

   return d3d_display;
}

/* This function may return the original d3d_display argument,
 * or a new one, or NULL on error.
 */
static ALLEGRO_DISPLAY_D3D *d3d_create_display_internals(
   ALLEGRO_DISPLAY_D3D *d3d_display, bool free_on_error)
{
   D3D_DISPLAY_PARAMETERS params;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref =  _al_get_new_display_settings();
   int num_modes;
   int i;
   int window_x, window_y;
   /* save width and height in case new fullscreen-mode
    * fails inside d3d_display_thread_proc and destroys al_display */
   int pre_destroy_w = al_display->w;
   int pre_destroy_h = al_display->h;

   params.display = d3d_display;

   /* The window is created in a separate thread so we need to pass this
    * TLS on
    */
   al_get_new_window_position(&window_x, &window_y);
   params.window_x = window_x;
   params.window_y = window_y;
   params.window_title = al_get_new_window_title();

   _al_d3d_generate_display_format_list();

   _al_d3d_score_display_settings(ref);

   /* Checking each mode is slow, so do a resolution check first */
   if (al_display->flags & ALLEGRO_FULLSCREEN) {
      num_modes = al_get_num_display_modes();
      while (num_modes >= 0) {
         ALLEGRO_DISPLAY_MODE mode;
         al_get_display_mode(num_modes, &mode);
         if (mode.width == al_display->w && mode.height == al_display->h) {
            break;
         }
         num_modes--;
      }
      if (num_modes < 0) {
         // Failing resolution test is like failing to create a window
         // This helps determining if the window message thread needs
         // to be destroyed.
         win_display->window = NULL;
         if (free_on_error) {
            al_free(d3d_display);
         }
         return NULL;
      }
   }

   ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds = NULL;
   for (i = 0; (eds = _al_d3d_get_display_settings(i)); i++) {
      ALLEGRO_DEBUG("Trying format %d.\n", eds->index);

      d3d_display->depth_stencil_format = d3d_get_depth_stencil_format(eds);
      if (eds->settings[ALLEGRO_SAMPLES] > 0)
         d3d_display->samples = eds->settings[ALLEGRO_SAMPLES] - 1;
      else
         d3d_display->samples = 0;
      d3d_display->single_buffer = eds->settings[ALLEGRO_SINGLE_BUFFER] ? true : false;
      d3d_display->vsync = eds->settings[ALLEGRO_VSYNC] == 1;

      memcpy(&al_display->extra_settings, eds, sizeof al_display->extra_settings);

      params.init_failed = true;
      win_display->thread_ended = true;
      params.AckEvent = CreateEvent(NULL, false, false, NULL);

      al_run_detached_thread(d3d_display_thread_proc, &params);
      /* Wait some _finite_ time (10 secs or so) for display thread to init, and
       * give up if something horrible happened to it, unless we're in debug mode
       * and we may have intentionally stopped the execution to analyze the code.
       */
#ifdef DEBUGMODE
      WaitForSingleObject(params.AckEvent, INFINITE);
#else
      WaitForSingleObject(params.AckEvent, 10*1000);
#endif
      ALLEGRO_DEBUG("Resumed after wait.\n");

      CloseHandle(params.AckEvent);

      if (!params.init_failed) {
         break;
      }

      ALLEGRO_INFO("Format %d failed.\n", i);

      // Display has been destroyed in d3d_display_thread_proc, create empty template again
      d3d_display = d3d_create_display_helper(pre_destroy_w, pre_destroy_h);
      win_display = &d3d_display->win_display;
      al_display = &win_display->display;
      params.display = d3d_display;

      ALLEGRO_DEBUG("d3d_display = %p\n", d3d_display);
      ALLEGRO_DEBUG("win_display = %p\n", win_display);
      ALLEGRO_DEBUG("al_display  = %p\n", al_display);
      ASSERT(al_display->vt);
   }

   // Re-sort the display format list for use later
   _al_d3d_resort_display_settings();

   if (!eds) {
      ALLEGRO_WARN("All %d formats failed.\n", i);
      if (free_on_error) {
         al_free(d3d_display);
      }
      return NULL;
   }

   ALLEGRO_INFO("Format %d succeeded.\n", eds->index);

   d3d_reset_state(d3d_display);

   d3d_display->backbuffer_bmp.extra = &d3d_display->backbuffer_bmp_extra;
   d3d_display->backbuffer_bmp_extra.is_backbuffer = true;
   d3d_display->backbuffer_bmp._display = al_display;
   d3d_display->backbuffer_bmp._format = _al_deduce_color_format(&al_display->extra_settings);
   d3d_display->backbuffer_bmp._memory_format = d3d_display->backbuffer_bmp._format;
   d3d_display->backbuffer_bmp_extra.system_format = d3d_display->backbuffer_bmp._format;
   d3d_display->backbuffer_bmp._flags = ALLEGRO_VIDEO_BITMAP;
   d3d_display->backbuffer_bmp.w = al_display->w;
   d3d_display->backbuffer_bmp.h = al_display->h;
   d3d_display->backbuffer_bmp_extra.texture_w = al_display->w;
   d3d_display->backbuffer_bmp_extra.texture_h = al_display->h;
   d3d_display->backbuffer_bmp.cl = 0;
   d3d_display->backbuffer_bmp.ct = 0;
   d3d_display->backbuffer_bmp.cr_excl = al_display->w;
   d3d_display->backbuffer_bmp.cb_excl = al_display->h;
   d3d_display->backbuffer_bmp.vt = (ALLEGRO_BITMAP_INTERFACE *)_al_bitmap_d3d_driver();
   d3d_display->backbuffer_bmp_extra.display = d3d_display;
   d3d_display->target_bitmap = NULL;
   al_identity_transform(&d3d_display->backbuffer_bmp.transform);
   al_identity_transform(&d3d_display->backbuffer_bmp.proj_transform);
   al_orthographic_transform(&d3d_display->backbuffer_bmp.proj_transform, 0, 0, -1.0, al_display->w, al_display->h, 1.0);

   /* Alpha blending is the default */
   d3d_display->device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
   d3d_display->device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   d3d_display->device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

   if (!check_use_fixed_pipeline(d3d_display->device)) {
      HRESULT res;

      res = d3d_display->device->CreateIndexBuffer(2 * sizeof(int), 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &d3d_display->loop_index_buffer, 0);
      if (res != D3D_OK) {
         ALLEGRO_WARN("CreateIndexBuffer failed: %ld.\n", res);
         if (free_on_error) {
            win_display->end_thread = true;
            while (!win_display->thread_ended)
               al_rest(0.001);
            d3d_destroy_display_internals(d3d_display);
            al_free(d3d_display);
         }
         return NULL;
      }

      res = d3d_display->device->CreateVertexShader((const DWORD*)prim_shader_vs_bin, &d3d_display->primitives_shader);

      if (res != D3D_OK) {
         ALLEGRO_WARN("Creating the primitives HLSL shader failed: %ld.\n", res);
         if (free_on_error) {
            win_display->end_thread = true;
            while (!win_display->thread_ended)
               al_rest(0.001);
            d3d_display->loop_index_buffer->Release();
            al_free(d3d_display);
         }
         return NULL;
      }
   }

   ALLEGRO_DEBUG("Returning d3d_display: %p\n", d3d_display);
   return d3d_display;
}

static ALLEGRO_DISPLAY *d3d_create_display_locked(int w, int h)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_get_system_driver();
   ALLEGRO_DISPLAY_D3D *d3d_display = d3d_create_display_helper(w, h);
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   ALLEGRO_DISPLAY *al_display = &win_display->display;
   ALLEGRO_DISPLAY_D3D **add;
   D3DCAPS9 caps;

   ALLEGRO_INFO("faux_fullscreen=%d\n", d3d_display->faux_fullscreen);
   ALLEGRO_DEBUG("al_display=%p\n", al_display);
   ALLEGRO_DEBUG("al_display->vt=%p\n", al_display->vt);
   ASSERT(al_display->vt);

   d3d_display = d3d_create_display_internals(d3d_display, true);
   if (!d3d_display) {
      ALLEGRO_ERROR("d3d_create_display failed.\n");
      return NULL;
   }
   win_display = &d3d_display->win_display;
   al_display = &win_display->display;
   ALLEGRO_DEBUG("al_display=%p\n", al_display);
   ALLEGRO_DEBUG("al_display->vt=%p\n", al_display->vt);
   ASSERT(al_display->vt);

   /* Add ourself to the list of displays. */
   add = (ALLEGRO_DISPLAY_D3D **)_al_vector_alloc_back(&system->system.displays);
   *add = d3d_display;

   /* Each display is an event source. */
   _al_event_source_init(&al_display->es);
   d3d_display->es_inited = true;

#if 0
   /* Setup the mouse */
   if (al_display->flags & ALLEGRO_FULLSCREEN && al_is_mouse_installed()) {
      RAWINPUTDEVICE rid[1];
      rid[0].usUsagePage = 0x01;
      rid[0].usUsage = 0x02;
      rid[0].dwFlags = RIDEV_NOLEGACY;
      rid[0].hwndTarget = 0;
      if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE) {
          ALLEGRO_WARN("Failed to init mouse.\n");
      }
   }
#endif

   win_display->mouse_selected_hcursor = 0;
   win_display->mouse_cursor_shown = false;
   win_display->hide_mouse_on_move = false;

   SetForegroundWindow(win_display->window);
   _al_win_grab_input(win_display);

   _al_win_show_mouse_cursor(al_display);

   if (_al_d3d->GetDeviceCaps(win_display->adapter, D3DDEVTYPE_HAL, &caps) != D3D_OK
         && _al_d3d->GetDeviceCaps(win_display->adapter, D3DDEVTYPE_REF, &caps) != D3D_OK) {
      d3d_display->supports_separate_alpha_blend = false;
   }
   else {
      d3d_display->supports_separate_alpha_blend =
         ((caps.PrimitiveMiscCaps & D3DPMISCCAPS_SEPARATEALPHABLEND) != 0);
   }

   ASSERT(al_display->vt);
   return al_display;
}

static ALLEGRO_DISPLAY *d3d_create_display(int w, int h)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_DISPLAY_WIN *win_display;
   int *s;

   al_lock_mutex(present_mutex);
   display = d3d_create_display_locked(w, h);
   win_display = (ALLEGRO_DISPLAY_WIN *)display;
   al_unlock_mutex(present_mutex);

   if (!display)
      return NULL;

   ASSERT(display->vt);

   s = display->extra_settings.settings;
   s[ALLEGRO_MAX_BITMAP_SIZE] = d3d_get_max_texture_size(win_display->adapter);
   s[ALLEGRO_SUPPORT_SEPARATE_ALPHA] = _al_d3d_supports_separate_alpha_blend(display);
   s[ALLEGRO_SUPPORT_NPOT_BITMAP] = al_have_d3d_non_pow2_texture_support();
   s[ALLEGRO_CAN_DRAW_INTO_BITMAP] = render_to_texture_supported;

   _al_win_post_create_window(display);

   return display;
}

static bool d3d_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)d;

   if (d3d_display->do_reset)
      return false;

   _al_d3d_update_render_state(d);

   return true;
}


static int d3d_al_blender_to_d3d(int al_mode)
{
   const int d3d_modes[ALLEGRO_NUM_BLEND_MODES] = {
      D3DBLEND_ZERO,
      D3DBLEND_ONE,
      D3DBLEND_SRCALPHA,
      D3DBLEND_INVSRCALPHA,
      D3DBLEND_SRCCOLOR,
      D3DBLEND_DESTCOLOR,
      D3DBLEND_INVSRCCOLOR,
      D3DBLEND_INVDESTCOLOR,
      D3DBLEND_BLENDFACTOR,
      D3DBLEND_INVBLENDFACTOR
   };

   return d3d_modes[al_mode];
}

void _al_d3d_set_blender(ALLEGRO_DISPLAY_D3D *d3d_display)
{
   bool blender_changed;
   int op, src, dst, alpha_op, alpha_src, alpha_dst;
   ALLEGRO_COLOR color;
   unsigned char r, g, b, a;
   DWORD allegro_to_d3d_blendop[ALLEGRO_NUM_BLEND_OPERATIONS] = {
      D3DBLENDOP_ADD,
      D3DBLENDOP_SUBTRACT,
      D3DBLENDOP_REVSUBTRACT
   };

   blender_changed = false;

   al_get_separate_bitmap_blender(&op, &src, &dst,
      &alpha_op, &alpha_src, &alpha_dst);
   color = al_get_bitmap_blend_color();
   al_unmap_rgba(color, &r, &g, &b, &a);

   if (d3d_display->blender_state_op != op) {
      /* These may not be supported but they will always fall back to ADD
       * in that case.
       */
      d3d_display->device->SetRenderState(D3DRS_BLENDOP, allegro_to_d3d_blendop[op]);
      d3d_display->blender_state_op = op;
      blender_changed = true;
   }

   if (d3d_display->blender_state_alpha_op != alpha_op) {
      /* These may not be supported but they will always fall back to ADD
       * in that case.
       */
      d3d_display->device->SetRenderState(D3DRS_BLENDOPALPHA, allegro_to_d3d_blendop[alpha_op]);
      d3d_display->blender_state_alpha_op = alpha_op;
      blender_changed = true;
   }

   if (d3d_display->blender_state_src != src) {
      if (d3d_display->device->SetRenderState(D3DRS_SRCBLEND, d3d_al_blender_to_d3d(src)) != D3D_OK)
         ALLEGRO_ERROR("Failed to set source blender\n");
      d3d_display->blender_state_src = src;
      blender_changed = true;
   }

   if (d3d_display->blender_state_dst != dst) {
      if (d3d_display->device->SetRenderState(D3DRS_DESTBLEND, d3d_al_blender_to_d3d(dst)) != D3D_OK)
         ALLEGRO_ERROR("Failed to set dest blender\n");
      d3d_display->blender_state_dst = dst;
      blender_changed = true;
   }

   if (d3d_display->blender_state_alpha_src != alpha_src) {
      if (d3d_display->device->SetRenderState(D3DRS_SRCBLENDALPHA, d3d_al_blender_to_d3d(alpha_src)) != D3D_OK)
         ALLEGRO_ERROR("Failed to set source alpha blender\n");
      d3d_display->blender_state_alpha_src = alpha_src;
      blender_changed = true;
   }

   if (d3d_display->blender_state_alpha_dst != alpha_dst) {
      if (d3d_display->device->SetRenderState(D3DRS_DESTBLENDALPHA, d3d_al_blender_to_d3d(alpha_dst)) != D3D_OK)
         ALLEGRO_ERROR("Failed to set dest alpha blender\n");
      d3d_display->blender_state_alpha_dst = alpha_dst;
      blender_changed = true;
   }

   if (blender_changed) {
      bool enable_separate_blender = (op != alpha_op) || (src != alpha_src) || (dst != alpha_dst);
      d3d_display->device->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_RGBA(r, g, b, a));
      if (enable_separate_blender) {
         if (d3d_display->device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true) != D3D_OK)
            ALLEGRO_ERROR("D3DRS_SEPARATEALPHABLENDENABLE failed\n");
      }

      /* thedmd: Why is this function called anyway? */
      d3d_display->device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
   }
}

static void d3d_clear(ALLEGRO_DISPLAY *al_display, ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;

   if (target->parent) target = target->parent;

   if (d3d_display->device_lost)
      return;
   if (d3d_display->device->Clear(0, NULL, D3DCLEAR_TARGET,
         D3DCOLOR_ARGB((int)(color->a*255), (int)(color->r*255),
         (int)(color->g*255), (int)(color->b*255)), 0, 0) != D3D_OK) {
      ALLEGRO_ERROR("Clear failed\n");
   }
}

static void d3d_clear_depth_buffer(ALLEGRO_DISPLAY *al_display, float z)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;

   if (target->parent) target = target->parent;

   if (d3d_display->device_lost)
      return;
   if (d3d_display->device->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, z, 0)
         != D3D_OK) {
      ALLEGRO_ERROR("Clear zbuffer failed\n");
   }
}



// FIXME: does this need a programmable pipeline path?
static void d3d_draw_pixel(ALLEGRO_DISPLAY *disp, float x, float y, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;

   _al_d3d_set_blender(d3d_disp);

#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      UINT required_passes;
      ALLEGRO_VERTEX vertices[1];
      vertices[0].x = x;
      vertices[0].y = y;
      vertices[0].z = 0;
      vertices[0].color = *color;

      d3d_disp->device->SetFVF(D3DFVF_ALLEGRO_VERTEX);
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, false);
      d3d_disp->effect->Begin(&required_passes, 0);
      for (unsigned int i = 0; i < required_passes; i++) {
         d3d_disp->effect->BeginPass(i);
         if (d3d_disp->device->DrawPrimitiveUP(D3DPT_POINTLIST, 1,
               vertices, sizeof(ALLEGRO_VERTEX)) != D3D_OK) {
            ALLEGRO_ERROR("d3d_draw_pixel: DrawPrimitive failed.\n");
            return;
         }
         d3d_disp->effect->EndPass();
      }
      d3d_disp->effect->End();
   }
   else
#endif
   {
      D3D_FIXED_VERTEX vertices[1];
      vertices[0].x = x;
      vertices[0].y = y;
      vertices[0].z = 0;
      vertices[0].color = D3DCOLOR_COLORVALUE(color->r, color->g, color->b, color->a);

      d3d_disp->device->SetFVF(D3DFVF_FIXED_VERTEX);
      d3d_disp->device->SetTexture(0, NULL);
      if (d3d_disp->device->DrawPrimitiveUP(D3DPT_POINTLIST, 1,
            vertices, sizeof(D3D_FIXED_VERTEX)) != D3D_OK) {
         ALLEGRO_ERROR("d3d_draw_pixel: DrawPrimitive failed.\n");
         return;
      }
   }
}

static void d3d_flip_display(ALLEGRO_DISPLAY *al_display)
{
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   HRESULT hr;

   if (d3d_display->device_lost)
      return;

   al_lock_mutex(present_mutex);

   d3d_display->device->EndScene();

   hr = d3d_display->device->Present(NULL, NULL, win_display->window, NULL);

   d3d_display->device->BeginScene();

   al_unlock_mutex(present_mutex);

   if (hr == D3DERR_DEVICELOST) {
      d3d_display->device_lost = true;
      return;
   }
   else {
      al_backup_dirty_bitmaps(al_display);
   }
}

static void d3d_update_display_region(ALLEGRO_DISPLAY *al_display,
   int x, int y,
   int width, int height)
{
   ALLEGRO_DISPLAY_D3D* d3d_display = (ALLEGRO_DISPLAY_D3D*)al_display;
   ALLEGRO_DISPLAY_WIN *win_display = &d3d_display->win_display;
   HRESULT hr;
   RGNDATA *rgndata;

   if (d3d_display->device_lost)
      return;

   if (d3d_display->single_buffer) {
      RECT rect;

      rect.left = x;
      rect.right = x+width;
      rect.top = y;
      rect.bottom = y+height;

      rgndata = (RGNDATA *)al_malloc(sizeof(RGNDATA)+sizeof(RECT)-1);
      rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
      rgndata->rdh.iType = RDH_RECTANGLES;
      rgndata->rdh.nCount = 1;
      rgndata->rdh.nRgnSize = sizeof(RECT);
      memcpy(&rgndata->rdh.rcBound, &rect, sizeof(RECT));
      memcpy(rgndata->Buffer, &rect, sizeof(RECT));

      d3d_display->device->EndScene();

      hr = d3d_display->device->Present(&rect, &rect, win_display->window, rgndata);

      d3d_display->device->BeginScene();

      al_free(rgndata);

      if (hr == D3DERR_DEVICELOST) {
         d3d_display->device_lost = true;
         return;
      }
   }
   else {
      d3d_flip_display(al_display);
   }
}

/*
 * Sets a clipping rectangle
 */
void _al_d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   ALLEGRO_DISPLAY_D3D *disp = d3d_bmp->display;
   RECT rect;

   if (!disp)
      return;

   if (bitmap->parent) {
      rect.left = bitmap->xofs + bitmap->cl;
      rect.right = bitmap->xofs + bitmap->cr_excl;
      rect.top = bitmap->yofs + bitmap->ct;
      rect.bottom = bitmap->yofs + bitmap->cb_excl;
   }
   else {
      rect.left = bitmap->cl;
      rect.right = bitmap->cr_excl;
      rect.top = bitmap->ct;
      rect.bottom = bitmap->cb_excl;
   }

   if (memcmp(&disp->scissor_state, &rect, sizeof(RECT)) != 0) {

      if (rect.left == 0 && rect.top == 0 && rect.right == disp->win_display.display.w && rect.left == disp->win_display.display.h) {
         disp->device->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
         return;
      }

      disp->device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
      disp->device->SetScissorRect(&rect);

      disp->scissor_state = rect;
   }
}

static bool d3d_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   WINDOWINFO wi;
   ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)d;
   ALLEGRO_DISPLAY_WIN *win_display = &disp->win_display;
   int w, h;
   ALLEGRO_STATE state;

   wi.cbSize = sizeof(WINDOWINFO);
   GetWindowInfo(win_display->window, &wi);
   w = wi.rcClient.right - wi.rcClient.left;
   h = wi.rcClient.bottom - wi.rcClient.top;

   if (w > 0 && h > 0) {
      d->w = w;
      d->h = h;
   }

   disp->backbuffer_bmp.w = d->w;
   disp->backbuffer_bmp.h = d->h;
   disp->backbuffer_bmp.cl = 0;
   disp->backbuffer_bmp.ct = 0;
   disp->backbuffer_bmp.cr_excl = w;
   disp->backbuffer_bmp.cb_excl = h;
   al_identity_transform(&disp->backbuffer_bmp.proj_transform);
   al_orthographic_transform(&disp->backbuffer_bmp.proj_transform, 0, 0, -1.0, w, h, 1.0);

   disp->do_reset = true;
   while (!disp->reset_done) {
      al_rest(0.001);
   }
   disp->reset_done = false;

   /* XXX: This is not very efficient, it'd probably be better to call
    * the necessary functions directly. */
   al_store_state(&state, ALLEGRO_STATE_DISPLAY | ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(al_get_backbuffer(d));
   al_set_clipping_rectangle(0, 0, d->w, d->h);
   al_restore_state(&state);

   return disp->reset_success;
}

static bool d3d_resize_helper(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)d;
   ALLEGRO_DISPLAY_WIN *win_display = &disp->win_display;
   ALLEGRO_DISPLAY_D3D *new_disp;
   int full_w, full_h;
   ALLEGRO_MONITOR_INFO mi;
   int adapter = win_display->adapter;

   ALLEGRO_STATE backup;

   al_get_monitor_info(adapter, &mi);
   full_w = mi.x2 - mi.x1;
   full_h = mi.y2 - mi.y1;

   if ((d->flags & ALLEGRO_FULLSCREEN_WINDOW) &&
         (full_w != width || full_h != height)) {
      win_display->toggle_w = width;
      win_display->toggle_h = height;
      return true;
   }

   if (d->flags & ALLEGRO_FULLSCREEN) {
      /* Don't generate ALLEGRO_EVENT_DISPLAY_LOST events when destroying a
       * display for resizing.
       */
      disp->suppress_lost_events = true;
      d3d_destroy_display_internals(disp);

      d->w = width;
      d->h = height;
      /* reset refresh rate (let new mode choose one) */
      d->refresh_rate = 0;
      win_display->end_thread = false;
      win_display->thread_ended = false;
      /* What's this? */
      ALLEGRO_SYSTEM *system = al_get_system_driver();
      if (system->displays._size <= 1) {
         ffw_set = false;
      }
      /* The original display needs to remain intact so we can
       * recover if resizing a display fails.
       */
      new_disp = d3d_create_display_internals(disp, false);
      if (!new_disp) {
         ALLEGRO_ERROR("d3d_create_display_internals failed.\n");
         ASSERT(d->vt);
         return false;
      }
      ASSERT(new_disp == disp);
      ASSERT(d->vt);

      disp->suppress_lost_events = false;

      _al_d3d_recreate_bitmap_textures(disp);

      disp->backbuffer_bmp.w = width;
      disp->backbuffer_bmp.h = height;

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

      AdjustWindowRectEx(&win_size, wi.dwStyle, GetMenu(win_display->window) ? TRUE : FALSE, wi.dwExStyle);

      // FIXME: Handle failure (for example if window constraints are active?)
      SetWindowPos(win_display->window, HWND_TOP,
         0, 0,
         win_size.right-win_size.left,
         win_size.bottom-win_size.top,
         SWP_NOMOVE|SWP_NOZORDER);

      if (!(d->flags & ALLEGRO_FULLSCREEN_WINDOW)) {
         win_display->toggle_w = width;
         win_display->toggle_h = height;
      }

      /*
       * The clipping rectangle and bitmap size must be
       * changed to match the new size.
       */
      al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
      al_set_target_bitmap(&disp->backbuffer_bmp);
      disp->backbuffer_bmp.w = width;
      disp->backbuffer_bmp.h = height;
      al_set_clipping_rectangle(0, 0, width, height);
      _al_d3d_set_bitmap_clip(&disp->backbuffer_bmp);
      al_restore_state(&backup);

   }

   return true;
}

static bool d3d_resize_display(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)d;
   int orig_w = d->w;
   int orig_h = d->h;
   bool ret;

   al_backup_dirty_bitmaps(d);

   win_display->ignore_resize = true;

   if (!d3d_resize_helper(d, width, height)) {
      ALLEGRO_WARN("trying to restore original size: %d, %d\n",
         orig_w, orig_h);
      if (!d3d_resize_helper(d, orig_w, orig_h)) {
         ALLEGRO_ERROR("failed to restore original size: %d, %d\n",
            orig_w, orig_h);
      }
      ret = false;
   } else {
      ret = true;
      d3d_acknowledge_resize(d);
   }

   win_display->ignore_resize = false;

   return ret;
}

static ALLEGRO_BITMAP *d3d_create_bitmap(ALLEGRO_DISPLAY *d,
   int w, int h, int format, int flags)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP_EXTRA_D3D *extra;

   if (!_al_pixel_format_is_real(format)) {
      format = d3d_choose_bitmap_format((ALLEGRO_DISPLAY_D3D *)d, format);
      if (format < 0) {
         return NULL;
      }
   }

   if (_al_pixel_format_to_d3d(format) < 0) {
      ALLEGRO_ERROR("Requested bitmap format not supported (%s).\n",
         _al_pixel_format_name((ALLEGRO_PIXEL_FORMAT)format));
      return NULL;
   }

   if (!is_texture_format_ok(d, format)) {
      ALLEGRO_ERROR("Requested bitmap format not supported (%s).\n",
         _al_pixel_format_name((ALLEGRO_PIXEL_FORMAT)format));
      return NULL;
   }

   bool compressed = _al_pixel_format_is_compressed(format);
   if (compressed) {
      if (!_al_d3d_render_to_texture_supported()) {
         /* Not implemented. XXX: Why not? */
         return NULL;
      }
   }
   int block_width = al_get_pixel_block_width(format);
   int block_height = al_get_pixel_block_height(format);
   int block_size = al_get_pixel_block_size(format);

   ALLEGRO_INFO("Chose bitmap format %d\n", format);

   bitmap = (ALLEGRO_BITMAP *)al_malloc(sizeof *bitmap);
   ASSERT(bitmap);
   memset(bitmap, 0, sizeof(*bitmap));

   bitmap->vt = _al_bitmap_d3d_driver();
   bitmap->_format = format;
   bitmap->_flags = flags;
   al_identity_transform(&bitmap->transform);

   bitmap->pitch =
      _al_get_least_multiple(w, block_width) / block_width * block_size;
   bitmap->memory = (unsigned char *)al_malloc(
      bitmap->pitch * _al_get_least_multiple(h, block_height) / block_height);

   extra = (ALLEGRO_BITMAP_EXTRA_D3D *)al_calloc(1, sizeof *extra);
   bitmap->extra = extra;
   extra->video_texture = 0;
   extra->system_texture = 0;
   extra->initialized = false;
   extra->is_backbuffer = false;
   extra->render_target = NULL;
   extra->system_format = compressed ? ALLEGRO_PIXEL_FORMAT_ARGB_8888 : format;

   extra->display = (ALLEGRO_DISPLAY_D3D *)d;

   return bitmap;
}

void _al_d3d_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ASSERT(!al_is_sub_bitmap(bitmap));
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D*)_al_get_bitmap_display(bitmap);

   if (bitmap == d3d_display->target_bitmap) {
      d3d_display->target_bitmap = NULL;
   }

   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);

   if (d3d_bmp->video_texture) {
      if (d3d_bmp->video_texture->Release() != 0) {
         ALLEGRO_WARN("d3d_destroy_bitmap: Release video texture failed.\n");
      }
   }
   if (d3d_bmp->system_texture) {
      if (d3d_bmp->system_texture->Release() != 0) {
         ALLEGRO_WARN("d3d_destroy_bitmap: Release system texture failed.\n");
      }
   }

   if (d3d_bmp->render_target) {
      if (d3d_bmp->render_target->Release() != 0) {
         ALLEGRO_WARN("d3d_destroy_bitmap: Release render target failed.\n");
      }
   }

   al_free(bitmap->extra);
}

static void d3d_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *target;
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_target;
   ALLEGRO_BITMAP_EXTRA_D3D *old_target = NULL;
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;

   if (d3d_display->device_lost)
      return;

   if (bitmap->parent) {
      target = bitmap->parent;
   }
   else {
      target = bitmap;
   }

   d3d_target = get_extra(target);
   if (d3d_display->target_bitmap)
      old_target = get_extra(d3d_display->target_bitmap);

   /* Release the previous target bitmap if it was not the backbuffer */
   if (old_target && !old_target->is_backbuffer) {
      ALLEGRO_BITMAP *parent;
      if (d3d_display->target_bitmap->parent)
         parent = d3d_display->target_bitmap->parent;
      else
         parent = d3d_display->target_bitmap;
      ALLEGRO_BITMAP_EXTRA_D3D *e = get_extra(parent);
      if (e && e->render_target) {
         e->render_target->Release();
         e->render_target = NULL;
      }
   }
   d3d_display->target_bitmap = NULL;

   /* Set the render target */
   if (d3d_target->is_backbuffer) {
      d3d_display = d3d_target->display;
      if (d3d_display->device->SetRenderTarget(0, d3d_display->render_target) != D3D_OK) {
         ALLEGRO_ERROR("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
         return;
      }
      d3d_target->render_target = d3d_display->render_target;
      d3d_display->target_bitmap = bitmap;
   }
   else if (_al_pixel_format_is_compressed(al_get_bitmap_format(target))) {
      /* Do nothing, as it is impossible to directly draw to compressed textures via D3D.
       * Instead, everything will be handled by the memory routines. */
   }
   else {
      d3d_display = (ALLEGRO_DISPLAY_D3D *)display;
      if (_al_d3d_render_to_texture_supported()) {
         d3d_display->target_bitmap = bitmap;
         if (!d3d_target->video_texture) {
            /* This can happen if the user tries to set the target bitmap as
             * the device is lost, before the DISPLAY_LOST event is received.
             */
            ALLEGRO_WARN("d3d_set_target_bitmap: No video texture.\n");
            return;
         }
         if (d3d_target->video_texture->GetSurfaceLevel(0, &d3d_target->render_target) != D3D_OK) {
            ALLEGRO_ERROR("d3d_set_target_bitmap: Unable to get texture surface level.\n");
            return;
         }
         if (d3d_display->device->SetRenderTarget(0, d3d_target->render_target) != D3D_OK) {
            ALLEGRO_ERROR("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
            d3d_target->render_target->Release();
            return;
         }
      }
      if (d3d_display->samples) {
         d3d_display->device->SetDepthStencilSurface(NULL);
      }
   }

   d3d_reset_state(d3d_display);

   _al_d3d_set_bitmap_clip(bitmap);
}

static ALLEGRO_BITMAP *d3d_get_backbuffer(ALLEGRO_DISPLAY *display)
{
   return (ALLEGRO_BITMAP *)&(((ALLEGRO_DISPLAY_D3D *)display)->backbuffer_bmp);
}

static bool d3d_is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   return display == _al_get_bitmap_display(bitmap);
}

static void d3d_switch_out(ALLEGRO_DISPLAY *display)
{
   (void)display;
}

static void d3d_switch_in(ALLEGRO_DISPLAY *display)
{
   (void)display;
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


/* Exposed stuff */

/* Function: al_get_d3d_device
 */
LPDIRECT3DDEVICE9 al_get_d3d_device(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;
   return d3d_display->device;
}

/* Function: al_get_d3d_system_texture
 */
LPDIRECT3DTEXTURE9 al_get_d3d_system_texture(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_D3D *e = get_extra(bitmap);
   return e->system_texture;
}

/* Function: al_get_d3d_video_texture
 */
LPDIRECT3DTEXTURE9 al_get_d3d_video_texture(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_D3D *e = get_extra(bitmap);
   return e->video_texture;
}

/* Function: al_get_d3d_texture_position
 */
void al_get_d3d_texture_position(ALLEGRO_BITMAP *bitmap, int *u, int *v)
{
   ASSERT(bitmap);
   ASSERT(u);
   ASSERT(v);

   *u = bitmap->xofs;
   *v = bitmap->yofs;
}

/* Function: al_is_d3d_device_lost
 */
bool al_is_d3d_device_lost(ALLEGRO_DISPLAY *display)
{
   return ((ALLEGRO_DISPLAY_D3D *)display)->device_lost;
}

static void d3d_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   _al_win_set_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
}

static void d3d_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   if (display->flags & ALLEGRO_FULLSCREEN) {
      ALLEGRO_MONITOR_INFO info;
      ALLEGRO_DISPLAY_WIN *win_display = (ALLEGRO_DISPLAY_WIN *)display;
      al_get_monitor_info(win_display->adapter, &info);
      *x = info.x1;
      *y = info.y1;
   }
   else {
      _al_win_get_window_position(((ALLEGRO_DISPLAY_WIN *)display)->window, x, y);
   }
}


void _al_d3d_shutdown_display(void)
{
   if (!vt)
      return;

   _al_d3d_destroy_display_format_list();

   if (_al_d3d)
      _al_d3d->Release();
   al_destroy_mutex(present_mutex);
   al_destroy_mutex(_al_d3d_lost_device_mutex);
   al_destroy_mutex(primitives_mutex);

   _al_d3d_bmp_destroy();

#ifdef ALLEGRO_CFG_SHADER_HLSL
   _al_d3d_shutdown_shaders();
#endif

   FreeLibrary(_al_d3d_module);
   _al_d3d_module = NULL;

#ifdef ALLEGRO_CFG_D3DX9
   _al_unload_d3dx9_module();
#endif

   al_free(vt);
   vt = NULL;
}

static void* d3d_prepare_vertex_cache(ALLEGRO_DISPLAY* disp,
                                      int num_new_vertices)
{
   int size;

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      size = sizeof(ALLEGRO_VERTEX);
   }
   else {
      size = sizeof(D3D_FIXED_VERTEX);
   }

   disp->num_cache_vertices += num_new_vertices;
   if (!disp->vertex_cache) {
      disp->vertex_cache = al_malloc(num_new_vertices * size);

      disp->vertex_cache_size = num_new_vertices;
   } else if (disp->num_cache_vertices > disp->vertex_cache_size) {
      disp->vertex_cache = al_realloc(disp->vertex_cache,
                              2 * disp->num_cache_vertices * size);

      disp->vertex_cache_size = 2 * disp->num_cache_vertices;
   }
   return (unsigned char *)disp->vertex_cache +
         (disp->num_cache_vertices - num_new_vertices) * size;
}

static D3DTEXTUREADDRESS d3d_bitmap_wrap(ALLEGRO_BITMAP_WRAP wrap)
{
   switch (wrap) {
      default:
      case ALLEGRO_BITMAP_WRAP_DEFAULT:
         return D3DTADDRESS_CLAMP;
      case ALLEGRO_BITMAP_WRAP_REPEAT:
         return D3DTADDRESS_WRAP;
      case ALLEGRO_BITMAP_WRAP_CLAMP:
         return D3DTADDRESS_CLAMP;
      case ALLEGRO_BITMAP_WRAP_MIRROR:
         return D3DTADDRESS_MIRROR;
   }
}

void _al_set_d3d_sampler_state(IDirect3DDevice9* device, int sampler,
   ALLEGRO_BITMAP* bitmap, bool prim_default)
{
   int bitmap_flags = al_get_bitmap_flags(bitmap);
    ALLEGRO_BITMAP_WRAP wrap_u, wrap_v;
   _al_get_bitmap_wrap(bitmap, &wrap_u, &wrap_v);

   if (prim_default && wrap_u == ALLEGRO_BITMAP_WRAP_DEFAULT) {
      device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
   }
   else {
      device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, d3d_bitmap_wrap(wrap_u));
   }

   if (prim_default && wrap_v == ALLEGRO_BITMAP_WRAP_DEFAULT) {
      device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
   }
   else {
      device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, d3d_bitmap_wrap(wrap_v));
   }

   if (bitmap_flags & ALLEGRO_MIN_LINEAR) {
      device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
   }
   else {
      device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   }
   if (bitmap_flags & ALLEGRO_MAG_LINEAR) {
      device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
   }
   else {
      device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
   }
   if (bitmap_flags & ALLEGRO_MIPMAP) {
      device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
   }
   else {
      device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   }
}

static void d3d_flush_vertex_cache(ALLEGRO_DISPLAY* disp)
{
   if (!disp->vertex_cache)
      return;
   if (disp->num_cache_vertices == 0)
      return;

   ALLEGRO_DISPLAY_D3D* d3d_disp = (ALLEGRO_DISPLAY_D3D*)disp;
   ALLEGRO_BITMAP* cache_bmp = (ALLEGRO_BITMAP*)disp->cache_texture;
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(cache_bmp);

   if (d3d_disp->device_lost)
      return;

   _al_set_d3d_sampler_state(d3d_disp->device, 0, cache_bmp, false);

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->device->SetFVF(D3DFVF_ALLEGRO_VERTEX);
   }

#ifdef ALLEGRO_CFG_SHADER_HLSL
   UINT required_passes;
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, true);
      d3d_disp->effect->SetTexture(ALLEGRO_SHADER_VAR_TEX, d3d_bmp->video_texture);
      d3d_disp->effect->Begin(&required_passes, 0);
      ASSERT(required_passes > 0);
   }
#endif

   if (d3d_disp->device->SetTexture(0, d3d_bmp->video_texture) != D3D_OK) {
      ALLEGRO_ERROR("d3d_flush_vertex_cache: SetTexture failed.\n");
      return;
   }

   int size;

#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      size = sizeof(ALLEGRO_VERTEX);
      for (unsigned int i = 0; i < required_passes; i++) {
         d3d_disp->effect->BeginPass(i);
         if (d3d_disp->device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, disp->num_cache_vertices / 3,
            (void *)disp->vertex_cache, size) != D3D_OK) {
            ALLEGRO_ERROR("d3d_flush_vertex_cache: DrawPrimitive failed.\n");
            return;
         }
         d3d_disp->effect->EndPass();
      }
   }
   else
#endif
   {
      d3d_disp->device->SetFVF(D3DFVF_FIXED_VERTEX);
      size = sizeof(D3D_FIXED_VERTEX);
      if (d3d_disp->device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, disp->num_cache_vertices / 3,
         (void *)disp->vertex_cache, size) != D3D_OK) {
         ALLEGRO_ERROR("d3d_flush_vertex_cache: DrawPrimitive failed.\n");
         return;
      }
   }

   disp->num_cache_vertices = 0;
#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->effect->End();
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, false);
      d3d_disp->effect->SetTexture(ALLEGRO_SHADER_VAR_TEX, NULL);
   }
#endif

   d3d_disp->device->SetTexture(0, NULL);
}

static void d3d_update_transformation(ALLEGRO_DISPLAY* disp, ALLEGRO_BITMAP *target)
{
   ALLEGRO_DISPLAY_D3D* d3d_disp = (ALLEGRO_DISPLAY_D3D*)disp;
   ALLEGRO_TRANSFORM proj;

   al_copy_transform(&proj, &target->proj_transform);
   /* Direct3D uses different clipping in projection space than OpenGL.
    * In OpenGL the final clip space is [-1..1] x [-1..1] x [-1..1].
    *
    * In D3D the clip space is [-1..1] x [-1..1] x [0..1].
    *
    * So we need to scale and translate the final z component from [-1..1]
    * to [0..1]. We do that by scaling with 0.5 then translating by 0.5
    * below.
    *
    * The effect can be seen for example ex_projection - it is broken
    * without this.
    */
   ALLEGRO_TRANSFORM fix_d3d;
   al_identity_transform(&fix_d3d);
   al_scale_transform_3d(&fix_d3d, 1, 1, 0.5);
   al_translate_transform_3d(&fix_d3d, 0.0, 0.0, 0.5);
   /*
    * Shift by half a pixel to make the output match the OpenGL output.
    * Don't shift the actual proj_transform because if the user grabs it via
    * al_get_current_projection_transform() and then sends it to
    * al_use_projection_transform() the shift will be applied twice.
    */
   al_translate_transform(&fix_d3d, -1.0 / al_get_bitmap_width(target),
                          1.0 / al_get_bitmap_height(target));

   al_compose_transform(&proj, &fix_d3d);

   ALLEGRO_TRANSFORM projview;
   al_copy_transform(&projview, &target->transform);
   al_compose_transform(&projview, &proj);
   al_copy_transform(&disp->projview_transform, &projview);
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
      if (d3d_disp->effect)
         _al_hlsl_set_projview_matrix(d3d_disp->effect, &projview);
#endif
   }
   else {
      d3d_disp->device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX *)proj.m);
      d3d_disp->device->SetTransform(D3DTS_VIEW, (D3DMATRIX *)target->transform.m);
   }

   D3DVIEWPORT9 viewport;
   viewport.MinZ = 0;
   viewport.MaxZ = 1;
   viewport.Width = al_get_bitmap_width(target);
   viewport.Height = al_get_bitmap_height(target);
   if (target->parent) {
      viewport.X = target->xofs;
      viewport.Y = target->yofs;
   }
   else {
      viewport.X = 0;
      viewport.Y = 0;
   }
   d3d_disp->device->SetViewport(&viewport);
}

static void* convert_to_fixed_vertices(const void* vtxs, int num_vertices, const int* indices, bool loop, bool pp)
{
   const ALLEGRO_VERTEX* vtx = (const ALLEGRO_VERTEX *)vtxs;
   int ii;
   int num_needed_vertices = num_vertices;
   size_t needed_size;

   if (pp && !indices && !loop) {
      return (void *)vtxs;
   }

   if(loop)
      num_needed_vertices++;

   needed_size = num_needed_vertices * (pp ? sizeof(ALLEGRO_VERTEX) : sizeof(D3D_FIXED_VERTEX));

   if(fixed_buffer == 0) {
      fixed_buffer = (char *)al_malloc(needed_size);
      fixed_buffer_size = needed_size;
   } else if (needed_size > fixed_buffer_size) {
      size_t new_size = needed_size * 1.5;
      fixed_buffer = (char *)al_realloc(fixed_buffer, new_size);
      fixed_buffer_size = new_size;
   }

   if (pp) {
      ALLEGRO_VERTEX *buffer = (ALLEGRO_VERTEX*)fixed_buffer;
      for(ii = 0; ii < num_vertices; ii++) {
         if(indices)
            buffer[ii] = vtx[indices[ii]];
         else
            buffer[ii] = vtx[ii];
      }
      if(loop)
         buffer[num_vertices] = buffer[0];
   }
   else {
      D3D_FIXED_VERTEX *buffer = (D3D_FIXED_VERTEX*)fixed_buffer;
      for(ii = 0; ii < num_vertices; ii++) {
         ALLEGRO_VERTEX vertex;

         if(indices)
            vertex = vtx[indices[ii]];
         else
            vertex = vtx[ii];

         buffer[ii].x = vertex.x;
         buffer[ii].y = vertex.y;
         buffer[ii].z = vertex.z;

         buffer[ii].u = vertex.u;
         buffer[ii].v = vertex.v;

         buffer[ii].color = D3DCOLOR_COLORVALUE(vertex.color.r, vertex.color.g, vertex.color.b, vertex.color.a);
      }
      if(loop)
         buffer[num_vertices] = buffer[0];
   }

   return fixed_buffer;
}

struct D3D_STATE
{
   DWORD old_wrap_state[2];
   DWORD old_ttf_state;
   IDirect3DVertexShader9* old_vtx_shader;
};

static D3D_STATE setup_state(LPDIRECT3DDEVICE9 device, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture, ALLEGRO_DISPLAY* disp)
{
   D3D_STATE state = {};
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;
   bool use_programmable_pipeline = disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE;
   bool use_default_shader = false;

   if (use_programmable_pipeline || use_fixed_pipeline) {
      state.old_vtx_shader = NULL;
      _al_d3d_set_blender(d3d_disp);
   }
   else {
      /* Manually set the shader without D3DX. */
      IDirect3DPixelShader9* old_pix_shader;
      device->GetVertexShader(&state.old_vtx_shader);
      device->GetPixelShader(&old_pix_shader);
      use_default_shader = state.old_vtx_shader == NULL;

      if (!old_pix_shader) {
         _al_d3d_set_blender(d3d_disp);
      }

      if (use_default_shader) {
         device->SetVertexShader(d3d_disp->primitives_shader);
         /* See prim_directx_shader.inc */
         device->SetVertexShaderConstantF(0, (float*)&disp->projview_transform, 4);
      }
   }

   /* Set the vertex declarations */
   if (use_fixed_pipeline) {
      device->SetFVF(D3DFVF_FIXED_VERTEX);
   }
   else {
      if(decl) {
         device->SetVertexDeclaration((IDirect3DVertexDeclaration9*)decl->d3d_decl);
      }
      else {
         device->SetFVF(D3DFVF_ALLEGRO_VERTEX);
      }
   }

   /* Set up the texture */
   if (texture && (use_fixed_pipeline || use_default_shader || use_programmable_pipeline)) {
      LPDIRECT3DTEXTURE9 d3d_texture;
      int tex_x, tex_y;
      D3DSURFACE_DESC desc;
      float mat[4][4] = {
         {1, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 0, 1, 0},
         {0, 0, 0, 1}
      };

      d3d_texture = al_get_d3d_video_texture(texture);

      d3d_texture->GetLevelDesc(0, &desc);
      al_get_d3d_texture_position(texture, &tex_x, &tex_y);

      if(decl) {
         if(decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL].attribute) {
            mat[0][0] = 1.0f / desc.Width;
            mat[1][1] = 1.0f / desc.Height;
         } else {
            mat[0][0] = (float)al_get_bitmap_width(texture) / desc.Width;
            mat[1][1] = (float)al_get_bitmap_height(texture) / desc.Height;
         }
      } else {
         mat[0][0] = 1.0f / desc.Width;
         mat[1][1] = 1.0f / desc.Height;
      }
      mat[2][0] = (float)tex_x / desc.Width;
      mat[2][1] = (float)tex_y / desc.Height;

      if (use_fixed_pipeline) {
         device->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &state.old_ttf_state);
         device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
         device->SetTransform(D3DTS_TEXTURE0, (D3DMATRIX *)mat);
      }
      else if (use_programmable_pipeline) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
         d3d_disp->effect->SetMatrix(ALLEGRO_SHADER_VAR_TEX_MATRIX, (D3DXMATRIX *)mat);
         d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX_MATRIX, true);
         d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, true);
         d3d_disp->effect->SetTexture(ALLEGRO_SHADER_VAR_TEX, d3d_texture);
#endif
      }
      else {
         /* See prim_directx_shader.inc */
         float use_tex_matrix[4] = {1.0, 0., 0., 0.};
         device->SetVertexShaderConstantF(4, (float*)mat, 3);
         device->SetVertexShaderConstantF(8, use_tex_matrix, 1);
      }

      device->SetTexture(0, d3d_texture);
      device->GetSamplerState(0, D3DSAMP_ADDRESSU, &state.old_wrap_state[0]);
      device->GetSamplerState(0, D3DSAMP_ADDRESSV, &state.old_wrap_state[1]);
      _al_set_d3d_sampler_state(device, 0, texture, true);
   }
   else {
      /* Don't unbind the texture here if shaders are used, since the user may
       * have set the 0'th texture unit manually via the shader API. */
      if (!use_programmable_pipeline) {
         device->SetTexture(0, NULL);
      }
   }

   return state;
}

static void revert_state(D3D_STATE state, LPDIRECT3DDEVICE9 device, ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture)
{
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;
   (void)d3d_disp;
   bool use_programmable_pipeline = disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE;
   bool use_default_shader = state.old_vtx_shader == NULL;

   if (use_programmable_pipeline) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
      d3d_disp->effect->End();
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX_MATRIX, false);
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, false);
#endif
   }

   if (texture && (use_fixed_pipeline || use_default_shader || use_programmable_pipeline)) {
      device->SetSamplerState(0, D3DSAMP_ADDRESSU, state.old_wrap_state[0]);
      device->SetSamplerState(0, D3DSAMP_ADDRESSV, state.old_wrap_state[1]);
      if (use_fixed_pipeline) {
         device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, state.old_ttf_state);
      }
   }

   if (!use_programmable_pipeline && use_default_shader) {
      device->SetVertexShader(0);
   }
}

static int draw_prim_common(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
   const void* vtx, const ALLEGRO_VERTEX_DECL* decl,
   const int* indices, int num_vtx, int type)
{
   int stride;
   int num_primitives = 0;
   LPDIRECT3DDEVICE9 device;
   int min_idx = 0, max_idx = num_vtx - 1;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;
   UINT required_passes = 1;
   unsigned int i;
   D3D_STATE state;

   if (al_is_d3d_device_lost(disp)) {
      return 0;
   }

   if (use_fixed_pipeline) {
      stride = (int)sizeof(D3D_FIXED_VERTEX);
   }
   else {
      stride = (decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX));
   }

   if((use_fixed_pipeline && decl) || (decl && decl->d3d_decl == 0)) {
      if(!indices)
         return _al_draw_prim_soft(texture, vtx, decl, 0, num_vtx, type);
      else
         return _al_draw_prim_indexed_soft(texture, vtx, decl, indices, num_vtx, type);
   }

   int num_idx = num_vtx;
   if(indices)
   {
      int ii;
      for(ii = 0; ii < num_vtx; ii++)
      {
         int idx = indices[ii];
         if(ii == 0) {
            min_idx = idx;
            max_idx = idx;
         } else if (idx < min_idx) {
            min_idx = idx;
         } else if (idx > max_idx) {
            max_idx = idx;
         }
      }
      num_idx = max_idx + 1 - min_idx;
   }

   device = al_get_d3d_device(disp);

   state = setup_state(device, decl, texture, disp);

   /* Convert vertices for legacy cards */
   if(use_fixed_pipeline) {
      al_lock_mutex(primitives_mutex);
      vtx = convert_to_fixed_vertices(vtx, num_vtx, indices, type == ALLEGRO_PRIM_LINE_LOOP,
         disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE);
   }

#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (!use_fixed_pipeline && disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->effect->Begin(&required_passes, 0);
   }
#endif

   for (i = 0; i < required_passes; i++) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
      if (!use_fixed_pipeline && disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
         d3d_disp->effect->BeginPass(i);
      }
#endif

      if (!indices || use_fixed_pipeline)
      {
         switch (type) {
            case ALLEGRO_PRIM_LINE_LIST: {
               num_primitives = num_vtx / 2;
               device->DrawPrimitiveUP(D3DPT_LINELIST, num_primitives, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_LINE_STRIP: {
               num_primitives = num_vtx - 1;
               device->DrawPrimitiveUP(D3DPT_LINESTRIP, num_primitives, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_LINE_LOOP: {
               num_primitives = num_vtx - 1;
               device->DrawPrimitiveUP(D3DPT_LINESTRIP, num_primitives, vtx, stride);
               if(!use_fixed_pipeline) {
                  int in[2];
                  in[0] = 0;
                  in[1] = num_vtx-1;

                  device->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, num_vtx, 1, in, D3DFMT_INDEX32, vtx, stride);
               } else {
                  device->DrawPrimitiveUP(D3DPT_LINELIST, 1, (char*)vtx + stride * (num_vtx - 1), stride);
               }
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_LIST: {
               num_primitives = num_vtx / 3;
               device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, num_primitives, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_STRIP: {
               num_primitives = num_vtx - 2;
               device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, num_primitives, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_FAN: {
               num_primitives = num_vtx - 2;
               device->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, num_primitives, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_POINT_LIST: {
               num_primitives = num_vtx;
               device->DrawPrimitiveUP(D3DPT_POINTLIST, num_primitives, vtx, stride);
               break;
            };
         }
      } else {
         switch (type) {
            case ALLEGRO_PRIM_LINE_LIST: {
               num_primitives = num_vtx / 2;
               device->DrawIndexedPrimitiveUP(D3DPT_LINELIST, min_idx, num_idx, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_LINE_STRIP: {
               num_primitives = num_vtx - 1;
               device->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, min_idx, num_idx, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_LINE_LOOP: {
               int in[2];
               num_primitives = num_vtx - 1;
               device->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, min_idx, num_idx, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);

               in[0] = indices[0];
               in[1] = indices[num_vtx-1];
               min_idx = in[0] > in[1] ? in[1] : in[0];
               max_idx = in[0] > in[1] ? in[0] : in[1];
               num_idx = max_idx - min_idx + 1;
               device->DrawIndexedPrimitiveUP(D3DPT_LINELIST, min_idx, num_idx, 1, in, D3DFMT_INDEX32, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_LIST: {
               num_primitives = num_vtx / 3;
               device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, min_idx, num_idx, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_STRIP: {
               num_primitives = num_vtx - 2;
               device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLESTRIP, min_idx, num_idx, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_FAN: {
               num_primitives = num_vtx - 2;
               device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLEFAN, min_idx, num_idx, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
               break;
            };
            case ALLEGRO_PRIM_POINT_LIST: {
               /*
                * D3D does not support point lists in indexed mode, so we draw them using the non-indexed mode. To gain at least a semblance
                * of speed, we detect consecutive runs of vertices and draw them using a single DrawPrimitiveUP call
                */
               int ii = 0;
               int start_idx = indices[0];
               int run_length = 0;
               for(ii = 0; ii < num_vtx; ii++)
               {
                  run_length++;
                  if(indices[ii] + 1 != indices[ii + 1] || ii == num_vtx - 1) {
                     device->DrawPrimitiveUP(D3DPT_POINTLIST, run_length, (const char*)vtx + start_idx * stride, stride);
                     if(ii != num_vtx - 1)
                        start_idx = indices[ii + 1];
                     run_length = 0;
                  }
               }
               break;
            };
         }
      }
#ifdef ALLEGRO_CFG_SHADER_HLSL
      if (!use_fixed_pipeline && disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
         d3d_disp->effect->EndPass();
      }
#endif
   }

   if(use_fixed_pipeline)
      al_unlock_mutex(primitives_mutex);

   revert_state(state, device, target, texture);

   return num_primitives;
}

static int d3d_draw_prim(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   return draw_prim_common(target, texture, (const char*)vtxs + start * stride, decl, 0, end - start, type);
}

static int d3d_draw_prim_indexed(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{
   return draw_prim_common(target, texture, vtxs, decl, indices, num_vtx, type);
}

static int draw_buffer_common(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
   int num_primitives = 0;
   int num_vtx = end - start;
   LPDIRECT3DDEVICE9 device;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;
   UINT required_passes = 1;
   unsigned int i;
   D3D_STATE state;

   if (al_is_d3d_device_lost(disp)) {
      return 0;
   }

   if (vertex_buffer->decl && vertex_buffer->decl->d3d_decl == 0) {
      return _al_draw_buffer_common_soft(vertex_buffer, texture, index_buffer, start, end, type);
   }

   device = al_get_d3d_device(disp);

   state = setup_state(device, vertex_buffer->decl, texture, disp);

   device->SetStreamSource(0, (IDirect3DVertexBuffer9*)vertex_buffer->common.handle, 0, vertex_buffer->decl ? vertex_buffer->decl->stride : (int)sizeof(ALLEGRO_VERTEX));

   if (index_buffer) {
      device->SetIndices((IDirect3DIndexBuffer9*)index_buffer->common.handle);
   }

#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (!use_fixed_pipeline && disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->effect->Begin(&required_passes, 0);
   }
#endif

   for (i = 0; i < required_passes; i++) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
      if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
         d3d_disp->effect->BeginPass(i);
      }
#endif

      if (!index_buffer) {
         switch (type) {
            case ALLEGRO_PRIM_LINE_LIST: {
               num_primitives = num_vtx / 2;
               device->DrawPrimitive(D3DPT_LINELIST, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_LINE_STRIP: {
               num_primitives = num_vtx - 1;
               device->DrawPrimitive(D3DPT_LINESTRIP, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_LINE_LOOP: {
               num_primitives = num_vtx - 1;
               device->DrawPrimitive(D3DPT_LINESTRIP, start, num_primitives);

               if (d3d_disp->loop_index_buffer) {
                  int *indices;
                  al_lock_mutex(primitives_mutex);
                  d3d_disp->loop_index_buffer->Lock(0, 0, (void**)&indices, 0);
                  ASSERT(indices);
                  indices[0] = start;
                  indices[1] = start + num_vtx - 1;
                  d3d_disp->loop_index_buffer->Unlock();
                  device->SetIndices(d3d_disp->loop_index_buffer);
                  device->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, _al_get_vertex_buffer_size(vertex_buffer), 0, 1);
                  al_unlock_mutex(primitives_mutex);
               }
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_LIST: {
               num_primitives = num_vtx / 3;
               device->DrawPrimitive(D3DPT_TRIANGLELIST, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_STRIP: {
               num_primitives = num_vtx - 2;
               device->DrawPrimitive(D3DPT_TRIANGLESTRIP, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_FAN: {
               num_primitives = num_vtx - 2;
               device->DrawPrimitive(D3DPT_TRIANGLEFAN, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_POINT_LIST: {
               num_primitives = num_vtx;
               device->DrawPrimitive(D3DPT_POINTLIST, start, num_primitives);
               break;
            };
         }
      }
      else {
         int vbuff_size = _al_get_vertex_buffer_size(vertex_buffer);
         switch (type) {
            case ALLEGRO_PRIM_LINE_LIST: {
               num_primitives = num_vtx / 2;
               device->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, vbuff_size, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_LINE_STRIP: {
               num_primitives = num_vtx - 1;
               device->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, vbuff_size, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_LINE_LOOP: {
               /* Unimplemented, too hard to do in a consistent fashion. */
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_LIST: {
               num_primitives = num_vtx / 3;
               device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vbuff_size, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_STRIP: {
               num_primitives = num_vtx - 2;
               device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, vbuff_size, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_TRIANGLE_FAN: {
               num_primitives = num_vtx - 2;
               device->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN, 0, 0, vbuff_size, start, num_primitives);
               break;
            };
            case ALLEGRO_PRIM_POINT_LIST: {
               /* Unimplemented, too hard to do in a consistent fashion. */
               break;
            };
         }
      }

#ifdef ALLEGRO_CFG_SHADER_HLSL
      if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
         d3d_disp->effect->EndPass();
      }
#endif
   }

   if (use_fixed_pipeline)
      al_unlock_mutex(primitives_mutex);

   revert_state(state, device, target, texture);

   return num_primitives;
}

static int d3d_draw_vertex_buffer(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type)
{
   return draw_buffer_common(target, texture, vertex_buffer, NULL, start, end, type);
}

static int d3d_draw_indexed_buffer(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
   return draw_buffer_common(target, texture, vertex_buffer, index_buffer, start, end, type);
}

static int convert_storage(int storage)
{
   switch(storage) {
      case ALLEGRO_PRIM_FLOAT_2:
         return D3DDECLTYPE_FLOAT2;
         break;
      case ALLEGRO_PRIM_FLOAT_3:
         return D3DDECLTYPE_FLOAT3;
         break;
      case ALLEGRO_PRIM_SHORT_2:
         return D3DDECLTYPE_SHORT2;
         break;
      case ALLEGRO_PRIM_FLOAT_1:
         return D3DDECLTYPE_FLOAT1;
         break;
      case ALLEGRO_PRIM_FLOAT_4:
         return D3DDECLTYPE_FLOAT4;
         break;
      case ALLEGRO_PRIM_UBYTE_4:
         return D3DDECLTYPE_UBYTE4;
         break;
      case ALLEGRO_PRIM_SHORT_4:
         return D3DDECLTYPE_SHORT4;
         break;
      case ALLEGRO_PRIM_NORMALIZED_UBYTE_4:
         return D3DDECLTYPE_UBYTE4N;
         break;
      case ALLEGRO_PRIM_NORMALIZED_SHORT_2:
         return D3DDECLTYPE_SHORT2N;
         break;
      case ALLEGRO_PRIM_NORMALIZED_SHORT_4:
         return D3DDECLTYPE_SHORT4N;
         break;
      case ALLEGRO_PRIM_NORMALIZED_USHORT_2:
         return D3DDECLTYPE_USHORT2N;
         break;
      case ALLEGRO_PRIM_NORMALIZED_USHORT_4:
         return D3DDECLTYPE_USHORT4N;
         break;
      case ALLEGRO_PRIM_HALF_FLOAT_2:
         return D3DDECLTYPE_FLOAT16_2;
         break;
      case ALLEGRO_PRIM_HALF_FLOAT_4:
         return D3DDECLTYPE_FLOAT16_4;
         break;
      default:
         ASSERT(0);
         return D3DDECLTYPE_UNUSED;
   }
}

static bool d3d_create_vertex_decl(ALLEGRO_DISPLAY* display, ALLEGRO_VERTEX_DECL* decl)
{
  LPDIRECT3DDEVICE9 device;
  D3DVERTEXELEMENT9 d3delements[ALLEGRO_PRIM_ATTR_NUM + 1];
  int idx = 0;
  ALLEGRO_VERTEX_ELEMENT* e;
  D3DCAPS9 caps;

  device = al_get_d3d_device(display);

  device->GetDeviceCaps(&caps);
  if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
    decl->d3d_decl = 0;
  } else {
    int i;
    e = &decl->elements[ALLEGRO_PRIM_POSITION];
    if(e->attribute) {
      d3delements[idx].Stream = 0;
      d3delements[idx].Offset = e->offset;
      d3delements[idx].Type = convert_storage(e->storage);
      d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
      d3delements[idx].Usage = D3DDECLUSAGE_POSITION;
      d3delements[idx].UsageIndex = 0;
      idx++;
    }

    e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
    if(!e->attribute)
      e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
    if(e->attribute) {
      d3delements[idx].Stream = 0;
      d3delements[idx].Offset = e->offset;
      d3delements[idx].Type = convert_storage(e->storage);
      d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
      d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
      d3delements[idx].UsageIndex = 0;
      idx++;
    }

    e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
    if(e->attribute) {
      d3delements[idx].Stream = 0;
      d3delements[idx].Offset = e->offset;
      d3delements[idx].Type = D3DDECLTYPE_FLOAT4;
      d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
      d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
      d3delements[idx].UsageIndex = 1;
      idx++;
    }

    for (i = 0; i < ALLEGRO_PRIM_MAX_USER_ATTR; i++) {
      e = &decl->elements[ALLEGRO_PRIM_USER_ATTR + i];
      if (e->attribute) {
         d3delements[idx].Stream = 0;
         d3delements[idx].Offset = e->offset;
         d3delements[idx].Type = convert_storage(e->storage);
         d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
         d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
         d3delements[idx].UsageIndex = 2 + i;
         idx++;
      }
    }

    d3delements[idx].Stream = 0xFF;
    d3delements[idx].Offset = 0;
    d3delements[idx].Type = D3DDECLTYPE_UNUSED;
    d3delements[idx].Method = 0;
    d3delements[idx].Usage = 0;
    d3delements[idx].UsageIndex = 0;

    device->CreateVertexDeclaration(d3delements, (IDirect3DVertexDeclaration9**)&decl->d3d_decl);
  }
  return true;
}

static bool d3d_create_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int flags)
{
   LPDIRECT3DDEVICE9 device;
   IDirect3DVertexBuffer9* d3d_vbuff;
   int stride = buf->decl ? buf->decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   int fvf = D3DFVF_ALLEGRO_VERTEX;
   HRESULT res;
   void* locked_memory;

   /* There's just no point */
   if (use_fixed_pipeline) {
      ALLEGRO_WARN("Cannot create vertex buffer for a legacy card.\n");
      return false;
   }

   device = al_get_d3d_device(al_get_current_display());

   if (buf->decl) {
      device->SetVertexDeclaration((IDirect3DVertexDeclaration9*)buf->decl->d3d_decl);
      fvf = 0;
   }

   res = device->CreateVertexBuffer(stride * num_vertices, !(flags & ALLEGRO_PRIM_BUFFER_READWRITE) ? D3DUSAGE_WRITEONLY : 0,
                                    fvf, D3DPOOL_MANAGED, &d3d_vbuff, 0);
   if (res != D3D_OK) {
      ALLEGRO_WARN("CreateVertexBuffer failed: %ld.\n", res);
      return false;
   }

   if (initial_data != NULL) {
      d3d_vbuff->Lock(0, 0, &locked_memory, 0);
      memcpy(locked_memory, initial_data, stride * num_vertices);
      d3d_vbuff->Unlock();
   }

   buf->common.handle = (uintptr_t)d3d_vbuff;

   return true;
}

static bool d3d_create_index_buffer(ALLEGRO_INDEX_BUFFER* buf, const void* initial_data, size_t num_indices, int flags)
{
   LPDIRECT3DDEVICE9 device;
   IDirect3DIndexBuffer9* d3d_ibuff;
   HRESULT res;
   void* locked_memory;

   /* There's just no point */
   if (use_fixed_pipeline) {
      ALLEGRO_WARN("Cannot create index buffer for a legacy card.\n");
      return false;
   }

   device = al_get_d3d_device(al_get_current_display());

   res = device->CreateIndexBuffer(num_indices * buf->index_size, !(flags & ALLEGRO_PRIM_BUFFER_READWRITE) ? D3DUSAGE_WRITEONLY : 0,
                                   buf->index_size == 4 ? D3DFMT_INDEX32 : D3DFMT_INDEX16, D3DPOOL_MANAGED, &d3d_ibuff, 0);
   if (res != D3D_OK) {
      ALLEGRO_WARN("CreateIndexBuffer failed: %ld.\n", res);
      return false;
   }

   if (initial_data != NULL) {
      d3d_ibuff->Lock(0, 0, &locked_memory, 0);
      memcpy(locked_memory, initial_data, num_indices * buf->index_size);
      d3d_ibuff->Unlock();
   }

   buf->common.handle = (uintptr_t)d3d_ibuff;

   return true;
}

static void d3d_destroy_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf)
{
   ((IDirect3DVertexBuffer9*)buf->common.handle)->Release();
}

static void d3d_destroy_index_buffer(ALLEGRO_INDEX_BUFFER* buf)
{
   ((IDirect3DIndexBuffer9*)buf->common.handle)->Release();
}

static void* d3d_lock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf)
{
   DWORD flags = buf->common.lock_flags == ALLEGRO_LOCK_READONLY ? D3DLOCK_READONLY : 0;
   HRESULT res;

   res = ((IDirect3DVertexBuffer9*)buf->common.handle)->Lock((UINT)buf->common.lock_offset, (UINT)buf->common.lock_length, &buf->common.locked_memory, flags);
   if (res != D3D_OK) {
      ALLEGRO_WARN("Locking vertex buffer failed: %ld.\n", res);
      return 0;
   }

   return buf->common.locked_memory;
}

static void* d3d_lock_index_buffer(ALLEGRO_INDEX_BUFFER* buf)
{
   DWORD flags = buf->common.lock_flags == ALLEGRO_LOCK_READONLY ? D3DLOCK_READONLY : 0;
   HRESULT res;

   res = ((IDirect3DIndexBuffer9*)buf->common.handle)->Lock((UINT)buf->common.lock_offset, (UINT)buf->common.lock_length, &buf->common.locked_memory, flags);

   if (res != D3D_OK) {
      ALLEGRO_WARN("Locking index buffer failed: %ld.\n", res);
      return 0;
   }

   return buf->common.locked_memory;
}

static void d3d_unlock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf)
{
   ((IDirect3DVertexBuffer9*)buf->common.handle)->Unlock();
}

static void d3d_unlock_index_buffer(ALLEGRO_INDEX_BUFFER* buf)
{
   ((IDirect3DIndexBuffer9*)buf->common.handle)->Unlock();
}

/* Initialize and obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_d3d_driver(void)
{
   if (vt)
      return vt;

   if (!d3d_init_display())
      return NULL;

   vt = (ALLEGRO_DISPLAY_INTERFACE *)al_malloc(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = d3d_create_display;
   vt->destroy_display = d3d_destroy_display;
   vt->set_current_display = d3d_set_current_display;
   vt->clear = d3d_clear;
   vt->clear_depth_buffer = d3d_clear_depth_buffer;
   vt->draw_pixel = d3d_draw_pixel;
   vt->flip_display = d3d_flip_display;
   vt->update_display_region = d3d_update_display_region;
   vt->acknowledge_resize = d3d_acknowledge_resize;
   vt->resize_display = d3d_resize_display;
   vt->create_bitmap = d3d_create_bitmap;
   vt->set_target_bitmap = d3d_set_target_bitmap;
   vt->get_backbuffer = d3d_get_backbuffer;
   vt->is_compatible_bitmap = d3d_is_compatible_bitmap;
   vt->switch_out = d3d_switch_out;
   vt->switch_in = d3d_switch_in;
   vt->draw_memory_bitmap_region = NULL;
   vt->wait_for_vsync = d3d_wait_for_vsync;

   vt->set_mouse_cursor = _al_win_set_mouse_cursor;
   vt->set_system_mouse_cursor = _al_win_set_system_mouse_cursor;
   vt->show_mouse_cursor = _al_win_show_mouse_cursor;
   vt->hide_mouse_cursor = _al_win_hide_mouse_cursor;

   vt->set_icons = _al_win_set_display_icons;
   vt->set_window_position = d3d_set_window_position;
   vt->get_window_position = d3d_get_window_position;
   vt->get_window_borders = _al_win_get_window_borders;
   vt->set_window_constraints = _al_win_set_window_constraints;
   vt->get_window_constraints = _al_win_get_window_constraints;
   vt->apply_window_constraints = _al_win_apply_window_constraints;
   vt->set_display_flag = _al_win_set_display_flag;
   vt->set_window_title = _al_win_set_window_title;

   vt->flush_vertex_cache = d3d_flush_vertex_cache;
   vt->prepare_vertex_cache = d3d_prepare_vertex_cache;

   vt->update_transformation = d3d_update_transformation;

   vt->update_render_state = _al_d3d_update_render_state;

   vt->draw_prim = d3d_draw_prim;
   vt->draw_prim_indexed = d3d_draw_prim_indexed;

   vt->create_vertex_decl = d3d_create_vertex_decl;

   vt->create_vertex_buffer = d3d_create_vertex_buffer;
   vt->destroy_vertex_buffer = d3d_destroy_vertex_buffer;
   vt->lock_vertex_buffer = d3d_lock_vertex_buffer;
   vt->unlock_vertex_buffer = d3d_unlock_vertex_buffer;

   vt->create_index_buffer = d3d_create_index_buffer;
   vt->destroy_index_buffer = d3d_destroy_index_buffer;
   vt->lock_index_buffer = d3d_lock_index_buffer;
   vt->unlock_index_buffer = d3d_unlock_index_buffer;

   vt->draw_vertex_buffer = d3d_draw_vertex_buffer;
   vt->draw_indexed_buffer = d3d_draw_indexed_buffer;

   _al_win_add_clipboard_functions(vt);

   return vt;
}

int _al_d3d_get_num_display_modes(int format, int refresh_rate, int flags)
{
   UINT num_modes;
   UINT i, j;
   D3DDISPLAYMODE display_mode;
   int matches = 0;

   if (!_al_d3d && !d3d_init_display())
      return 0;

   (void)flags;

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
      int adapter = al_get_new_display_adapter();
      if (adapter < 0)
         adapter = 0;

      if (!_al_pixel_format_is_real(allegro_formats[j]) || _al_pixel_format_has_alpha(allegro_formats[j]))
         continue;

      num_modes = _al_d3d->GetAdapterModeCount(adapter, (D3DFORMAT)d3d_formats[j]);

      for (i = 0; i < num_modes; i++) {
         if (_al_d3d->EnumAdapterModes(adapter, (D3DFORMAT)d3d_formats[j], i, &display_mode) != D3D_OK) {
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

   if (!_al_d3d && !d3d_init_display())
      return NULL;

   (void)flags;

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
      int adapter = al_get_new_display_adapter();
      if (adapter < 0)
         adapter = 0;

      if (!_al_pixel_format_is_real(allegro_formats[j]) || _al_pixel_format_has_alpha(allegro_formats[j]))
         continue;

      num_modes = _al_d3d->GetAdapterModeCount(adapter, (D3DFORMAT)d3d_formats[j]);

      for (i = 0; i < num_modes; i++) {
         if (_al_d3d->EnumAdapterModes(adapter, (D3DFORMAT)d3d_formats[j], i, &display_mode) != D3D_OK) {
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

/* vim: set sts=3 sw=3 et: */
