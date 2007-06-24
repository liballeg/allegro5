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

#include "allegro.h"
#include "system_new.h"
#include "internal/aintern.h"
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"
#include "internal/aintern_vector.h"
#include "allegro/platform/aintwin.h"
#include "allegro/internal/aintern_thread.h"

#include "wddraw.h"
#include "d3d.h"

static AL_DISPLAY_INTERFACE *vt = 0;

static LPDIRECT3D9 _al_d3d = 0;
LPDIRECT3DDEVICE9 _al_d3d_device = 0;

static D3DPRESENT_PARAMETERS d3d_pp;

static _AL_VECTOR d3d_created_displays = _AL_VECTOR_INITIALIZER(AL_DISPLAY_D3D *);

static HWND d3d_hidden_window;

static float d3d_ortho_w;
static float d3d_ortho_h;

static bool d3d_already_fullscreen = false;
static bool _al_d3d_device_lost = false;
static AL_DISPLAY_D3D *d3d_destroyed_display = NULL;
static LPDIRECT3DSURFACE9 d3d_current_texture_render_target = NULL;

static AL_DISPLAY *d3d_target_display_before_device_lost = NULL;
static AL_BITMAP *d3d_target_bitmap_before_device_lost = NULL;

/* Preserved vertex buffers for speed */
static IDirect3DVertexBuffer9 *d3d_line_vertex_buffer;

/* The window code needs this to set a thread handle */
AL_DISPLAY_D3D *_al_d3d_last_created_display = 0;

static AL_DISPLAY_D3D *d3d_fullscreen_display;

static _AL_MUTEX d3d_device_mutex;

/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
static int d3d_new_display_format = 0;
static int d3d_new_display_refresh_rate = 0;
static int d3d_new_display_flags = 0;
static int d3d_new_display_w;
static int d3d_new_display_h;
static bool d3d_waiting_for_display = true;

static bool d3d_display_initialized = false;
	
static bool d3d_bitmaps_prepared_for_reset = false;

/* Dummy back buffer bitmap */
AL_BITMAP_D3D d3d_backbuffer;

static int allegro_formats[] = {
	ALLEGRO_PIXEL_FORMAT_ANY,
	ALLEGRO_PIXEL_FORMAT_ARGB_8888,
	ALLEGRO_PIXEL_FORMAT_ARGB_4444,
	ALLEGRO_PIXEL_FORMAT_RGB_565,
	ALLEGRO_PIXEL_FORMAT_ARGB_1555,
	-1
};

/* Mapping of Allegro formats to D3D formats */
static int d3d_formats[] = {
	ALLEGRO_PIXEL_FORMAT_ANY,
	D3DFMT_A8R8G8B8,
	D3DFMT_A4R4G4B4,
	D3DFMT_R5G6B5,
	D3DFMT_A1R5G5B5,
	-1
};

/* Dummy graphics driver for compatibility */
GFX_DRIVER _al_d3d_dummy_gfx_driver = {
	0,
	"D3D Compatibility GFX driver",
	"D3D Compatibility GFX driver",
	"D3D Compatibility GFX driver",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	0, 0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	_al_win_directx_create_mouse_cursor,
	_al_win_directx_destroy_mouse_cursor,
	_al_win_directx_set_mouse_cursor,
	_al_win_directx_set_system_mouse_cursor,
	_al_win_directx_show_mouse_cursor,
	_al_win_directx_hide_mouse_cursor
};


void _al_d3d_lock_device()
{
	_al_mutex_lock(&d3d_device_mutex);
}

void _al_d3d_unlock_device()
{
	_al_mutex_unlock(&d3d_device_mutex);
}


int _al_format_to_d3d(int format)
{
	int i;
	D3DDISPLAYMODE d3d_dm;

	for (i = 1; allegro_formats[i] >= 0; i++) {
		if (allegro_formats[i] == format) {
			return d3d_formats[i];
		}
	}

	/* If not found or ALLEGRO_PIXEL_FORMAT_ANY, return desktop format */
	
	IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

	return d3d_dm.Format;
}

int _al_d3d_format_to_allegro(int d3d_fmt)
{
	int i;

	for (i = 1; d3d_formats[i] >= 0; i++) {
		if (d3d_formats[i] == d3d_fmt) {
			return allegro_formats[i];
		}
	}

	return -1;
}

static int d3d_al_color_to_d3d(int format, AL_COLOR *color)
{
	unsigned char r, g, b, a;
	_al_unmap_rgba(format, color, &r, &g, &b, &a);
	return D3DCOLOR_ARGB(a, r, g, b);
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

static bool d3d_create_vertex_buffers()
{
	if (IDirect3DDevice9_CreateVertexBuffer(_al_d3d_device,
			sizeof(D3D_COLORED_VERTEX)*2,
			D3DUSAGE_WRITEONLY, D3DFVF_COLORED_VERTEX,
			D3DPOOL_MANAGED, &d3d_line_vertex_buffer, NULL) != D3D_OK) {
		TRACE("Creating line vertex buffer failed.\n");
		return true;
	}

	return false;
}

static void d3d_destroy_vertex_buffers()
{
	IDirect3DVertexBuffer9_Release(d3d_line_vertex_buffer);
}

static void d3d_reset_state()
{
	if (_al_d3d_is_device_lost()) return;

	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ZENABLE, D3DZB_FALSE);
	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ZWRITEENABLE, FALSE);
	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_LIGHTING, FALSE);
	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
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

static void _al_d3d_set_ortho_projection(float w, float h)
{
	D3DMATRIX matOrtho;
	D3DMATRIX matIdentity;

	if (_al_d3d_is_device_lost()) return;

	d3d_ortho_w = w;
	d3d_ortho_h = h;

	d3d_get_identity_matrix(&matIdentity);
	d3d_get_ortho_matrix(w, h, &matOrtho);

	_al_d3d_lock_device();

	IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_PROJECTION, &matOrtho);
	IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_WORLD, &matIdentity);
	IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_VIEW, &matIdentity);

	_al_d3d_unlock_device();
}

static bool d3d_display_mode_matches(D3DDISPLAYMODE *dm, int w, int h, int format, int refresh_rate)
{
	if ((dm->Width == (unsigned int)w) &&
	    (dm->Height == (unsigned int)h) &&
	    ((!refresh_rate) || (dm->RefreshRate == (unsigned int)refresh_rate)) &&
	    (dm->Format == (unsigned int)_al_format_to_d3d(format))) {
	    	return true;
	}
	return false;
}

static int d3d_get_max_refresh_rate(int w, int h, int format)
{
	UINT num_modes = IDirect3D9_GetAdapterModeCount(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format));
	UINT i;
	D3DDISPLAYMODE display_mode;
	UINT max = 0;

	for (i = 0; i < num_modes; i++) {
		if (IDirect3D9_EnumAdapterModes(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
			TRACE("d3d_get_max_refresh_rate: EnumAdapterModes failed.\n");
			return -1;
		}
		if (d3d_display_mode_matches(&display_mode, w, h, format, 0)) {
			if (display_mode.RefreshRate > max) {
				max = display_mode.RefreshRate;
			}
		}
	}

	return max;
}

static bool d3d_check_mode(int w, int h, int format, int refresh_rate)
{
	UINT num_modes = IDirect3D9_GetAdapterModeCount(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format));
	UINT i;
	D3DDISPLAYMODE display_mode;

	for (i = 0; i < num_modes; i++) {
		if (IDirect3D9_EnumAdapterModes(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
			TRACE("d3d_check_mode: EnumAdapterModes failed.\n");
			return false;
		}
		if (d3d_display_mode_matches(&display_mode, w, h, format, refresh_rate)) {
			return true;
		}
	}

	return false;
}

/* FIXME: release swap chain too? */
static bool d3d_create_hidden_device()
{
	D3DDISPLAYMODE d3d_dm;
	LPDIRECT3DSURFACE9 render_target;
	int ret;

	d3d_hidden_window = _al_win_create_hidden_window();
	if (d3d_hidden_window == 0) {
		TRACE("Failed to create hidden window.\n");
		return 0;
	}

	IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.BackBufferFormat = d3d_dm.Format;
	d3d_pp.BackBufferWidth = 100;
	d3d_pp.BackBufferHeight = 100;
	d3d_pp.BackBufferCount = 1;
	d3d_pp.Windowed = 1;
	d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d_pp.hDeviceWindow = d3d_hidden_window;

	/* FIXME: try hardware vertex processing first? */
	if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d3d_hidden_window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3d_pp, &_al_d3d_device)) != D3D_OK) {
		if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL, d3d_hidden_window,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&d3d_pp, &_al_d3d_device)) != D3D_OK) {
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
				default:
					TRACE("Direct3D Device creation failed.\n");
					break;
			}
			return 0;
		}
	}

	/* We won't be using this surface, so release it immediately */

	IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target);
	IDirect3DSurface9_Release(render_target);

	if (d3d_create_vertex_buffers() != false) {
		TRACE("Failed to create vertex buffers.\n");
		IDirect3DDevice9_Release(_al_d3d_device);
		_al_d3d_device = 0;
		return 0;
	}

	TRACE("Direct3D device created.\n");

	return 1;
}

static bool d3d_create_fullscreen_device(AL_DISPLAY_D3D *d,
	int format, int refresh_rate, int flags)
{
	int ret;

	if (!d3d_check_mode(d->display.w, d->display.h, format, refresh_rate)) {
		TRACE("d3d_create_fullscreen_device: Mode not supported.\n");
		return 0;
	}

	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.BackBufferFormat = _al_format_to_d3d(format);
	d3d_pp.BackBufferWidth = d->display.w;
	d3d_pp.BackBufferHeight = d->display.h;
	d3d_pp.BackBufferCount = 1;
	d3d_pp.Windowed = 0;
	d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (flags & AL_SINGLEBUFFER) {
		d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
	}
	else {
		d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	}
	d3d_pp.hDeviceWindow = d->window;
	if (refresh_rate) {
		d3d_pp.FullScreen_RefreshRateInHz = refresh_rate;
	}
	else {
		d3d_pp.FullScreen_RefreshRateInHz =
			d3d_get_max_refresh_rate(d->display.w, d->display.h, format);
	}

	/* FIXME: try hardware vertex processing first? */
	if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d->window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3d_pp, &_al_d3d_device)) != D3D_OK) {
		if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
				D3DDEVTYPE_HAL, d->window,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&d3d_pp, &_al_d3d_device)) != D3D_OK) {
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
				default:
					TRACE("Direct3D Device creation failed.\n");
					break;
			}
			return 0;
		}
	}

	IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &d->render_target);
	IDirect3DDevice9_GetSwapChain(_al_d3d_device, 0, &d->swap_chain);

	if (d3d_create_vertex_buffers() != false) {
		TRACE("Failed to create vertex buffers.\n");
		IDirect3DDevice9_Release(_al_d3d_device);
		_al_d3d_device = 0;
		return 0;
	}

	d3d_reset_state();

	TRACE("Fullscreen Direct3D device created.\n");

	return 1;
}

static void d3d_destroy_device()
{
	IDirect3DDevice9_Release(_al_d3d_device);
	_al_d3d_device = NULL;
}

static void d3d_destroy_hidden_device()
{
	d3d_destroy_device();
	DestroyWindow(d3d_hidden_window);
	d3d_hidden_window = 0;
}

bool _al_d3d_init_display()
{
	if ((_al_d3d = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
		TRACE("Direct3DCreate9 failed.\n");
		return true;
	}

	D3DDISPLAYMODE d3d_dm;

	IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

	if (IDirect3D9_CheckDeviceFormat(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_RENDERTARGET,
			D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) != D3D_OK) {
		IDirect3D9_Release(_al_d3d);
		TRACE("Texture rendering not supported.\n");
		return true;
	}

	_al_mutex_init(&d3d_device_mutex);


	d3d_backbuffer.is_backbuffer = 1;

	return false;
}

static bool d3d_create_swap_chain(AL_DISPLAY_D3D *d,
	int format, int refresh_rate, int flags)
{
	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.BackBufferFormat = _al_format_to_d3d(format);
	d3d_pp.BackBufferWidth = d->display.w;
	d3d_pp.BackBufferHeight = d->display.h;
	d3d_pp.BackBufferCount = 1;
	d3d_pp.Windowed = 1;
	d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (flags & AL_SINGLEBUFFER) {
		d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
	}
	else {
		d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	}
	d3d_pp.hDeviceWindow = d->window;

	HRESULT hr;

	if ((hr = IDirect3DDevice9_CreateAdditionalSwapChain(_al_d3d_device, &d3d_pp, &d->swap_chain)) != D3D_OK) {
		TRACE("d3d_create_swap_chain: CreateAdditionalSwapChain failed.\n");
		return 0;
	}

	if (IDirect3DSwapChain9_GetBackBuffer(d->swap_chain, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target) != D3D_OK) {
		IDirect3DSwapChain9_Release(d->swap_chain);
		TRACE("d3d_create_swap_chain: GetBackBuffer failed.\n");
		return 0;
	}

	d3d_reset_state();

	return 1;
}

static void d3d_destroy_display(AL_DISPLAY *display)
{
	unsigned int i;
	AL_DISPLAY_D3D *d3d_display = (AL_DISPLAY_D3D *)display;
	AL_SYSTEM_WIN *system = (AL_SYSTEM_WIN *)al_system_driver();

	if (d3d_current_texture_render_target) {
		IDirect3DSurface9_Release(d3d_current_texture_render_target);
	}
	if (d3d_display->render_target)
		IDirect3DSurface9_Release(d3d_display->render_target);
	if (d3d_display->swap_chain)
		IDirect3DSwapChain9_Release(d3d_display->swap_chain);

	if (display->flags & AL_WINDOWED) {
		if (d3d_created_displays._size <= 1) {
			d3d_destroy_vertex_buffers();
			d3d_destroy_hidden_device();
		}
	}
	else {
		d3d_destroy_vertex_buffers();
		d3d_already_fullscreen = false;
	}

	d3d_destroyed_display = d3d_display;
	while (!d3d_display->destroyed)
		al_rest(1);
	d3d_destroyed_display = NULL;

	_al_win_ungrab_input();

	PostMessage(d3d_display->window, _al_win_msg_suicide, 0, 0);

	_al_event_source_free(&display->es);

	_al_win_delete_from_vector(&system->system.displays, display);

	_al_win_delete_from_vector(&d3d_created_displays, display);

	if (d3d_created_displays._size > 0) {
		AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, 0);
		AL_DISPLAY_D3D *d = *dptr;
		_al_win_wnd = d->window;
		win_grab_input();
	}

	_AL_FREE(display);
}

void _al_d3d_prepare_for_reset()
{
	unsigned int i;

	if (!d3d_bitmaps_prepared_for_reset) {
		_al_d3d_prepare_bitmaps_for_reset();
		d3d_bitmaps_prepared_for_reset = true;
	}
	_al_d3d_release_default_pool_textures();

	if (d3d_current_texture_render_target) {
		IDirect3DSurface9_Release(d3d_current_texture_render_target);
		d3d_current_texture_render_target = NULL;
	}

	for (i = 0; i < d3d_created_displays._size; i++) {
		AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
		AL_DISPLAY_D3D *disp = *dptr;

		IDirect3DSurface9_Release(disp->render_target);
		IDirect3DSwapChain9_Release(disp->swap_chain);
	}
}

static bool _al_d3d_reset_device()
{
	if (_al_d3d_device) {
		if (d3d_already_fullscreen) {
			unsigned int i;
			HRESULT hr;
		
			_al_d3d_lock_device();

			_al_d3d_prepare_for_reset();

			ZeroMemory(&d3d_pp, sizeof(d3d_pp));
			d3d_pp.BackBufferFormat = _al_format_to_d3d(d3d_fullscreen_display->display.format);
			d3d_pp.BackBufferWidth = d3d_fullscreen_display->display.w;
			d3d_pp.BackBufferHeight = d3d_fullscreen_display->display.h;
			d3d_pp.BackBufferCount = 1;
			d3d_pp.Windowed = 0;
			d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3d_pp.hDeviceWindow = d3d_fullscreen_display->window;
			d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
			d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
			if (d3d_fullscreen_display->display.flags & AL_SINGLEBUFFER) {
				d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
			}
			else {
				d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			}
			if (d3d_fullscreen_display->display.refresh_rate) {
				d3d_pp.FullScreen_RefreshRateInHz =
					d3d_fullscreen_display->display.refresh_rate;
			}
			else {
				d3d_pp.FullScreen_RefreshRateInHz =
					d3d_get_max_refresh_rate(d3d_fullscreen_display->display.w,
						d3d_fullscreen_display->display.h,
						d3d_fullscreen_display->display.format);
			}

			while ((hr = IDirect3DDevice9_Reset(_al_d3d_device, &d3d_pp)) != D3D_OK) {
				/* FIXME: Should we try forever? */
				TRACE("_al_d3d_reset_device: Reset failed. Trying again.\n");
				al_rest(10);
			}

			IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &d3d_fullscreen_display->render_target);
			IDirect3DDevice9_GetSwapChain(_al_d3d_device, 0, &d3d_fullscreen_display->swap_chain);

			_al_d3d_unlock_device();
		}
		else {
			unsigned int i;
			HRESULT hr;
			LPDIRECT3DSURFACE9 render_target;
			D3DDISPLAYMODE d3d_dm;
			
			_al_d3d_lock_device();

			_al_d3d_prepare_for_reset();

			IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

			ZeroMemory(&d3d_pp, sizeof(d3d_pp));
			d3d_pp.BackBufferFormat = d3d_dm.Format;
			d3d_pp.BackBufferWidth = 100;
			d3d_pp.BackBufferHeight = 100;
			d3d_pp.BackBufferCount = 1;
			d3d_pp.Windowed = 1;
			d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3d_pp.hDeviceWindow = d3d_hidden_window;
			d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

			/* Try to reset a few times */
			for (i = 0; i < 10; i++) {
				if (IDirect3DDevice9_Reset(_al_d3d_device, &d3d_pp) == D3D_OK) {
					break;
				}
				al_rest(100);
			}
			if (i == 10) {
				_al_d3d_unlock_device();
				return 0;
			}

			/* We don't need this render target */
			IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target);
			IDirect3DSurface9_Release(render_target);

			for (i = 0; i < d3d_created_displays._size; i++) {
				AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
				AL_DISPLAY_D3D *d = *dptr;
				AL_DISPLAY *al_display = (AL_DISPLAY *)d;
				int flags = 0;

				d3d_create_swap_chain(d, al_display->format, al_display->refresh_rate, al_display->flags);
			}
			
			_al_d3d_unlock_device();

		}
	}

	_al_d3d_refresh_texture_memory();

	al_set_current_display(d3d_target_display_before_device_lost);
	al_set_target_bitmap(d3d_target_bitmap_before_device_lost);

	_al_d3d_lock_device();

	d3d_reset_state();

	_al_d3d_unlock_device();
	
	d3d_bitmaps_prepared_for_reset = false;

	return 1;
}

static void d3d_do_resize(AL_DISPLAY *d)
{
	WINDOWINFO wi;
	D3DDISPLAYMODE d3d_dm;
	AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)d;
	
	if (_al_d3d_device) {
		wi.cbSize = sizeof(WINDOWINFO);
		GetWindowInfo(disp->window, &wi);
		d->w = wi.rcClient.right - wi.rcClient.left;
		d->h = wi.rcClient.bottom - wi.rcClient.top;

		if (gfx_driver) {
			gfx_driver->w = d->w;
			gfx_driver->h = d->h;
		}
	}

	_al_d3d_reset_device();
}

static void d3d_display_thread_proc(HANDLE unused)
{
	AL_DISPLAY_D3D *d;
	DWORD result;
	MSG msg;
	HRESULT hr;
	bool lost_event_generated = false;

	_al_d3d_last_created_display = NULL;

	/*
	 * Direct3D will only allow 1 fullscreen swap chain, and you can't
	 * combine fullscreen and windowed swap chains.
	 */
	if (d3d_already_fullscreen ||
		((d3d_created_displays._size > 0) && !(d3d_new_display_flags & AL_WINDOWED))) {
		d3d_waiting_for_display = false;
		return;
	}

	if (d3d_new_display_format == ALLEGRO_PIXEL_FORMAT_ANY) {
		/* Choose the desktop format for windowed modes */
		if (d3d_new_display_flags & AL_WINDOWED) {
			D3DDISPLAYMODE d3d_dm;
			IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);
			d3d_new_display_format = _al_d3d_format_to_allegro(d3d_dm.Format);
		}
		else {
			/*
			 * This format should be supported anywhere
			 * in fullscreen mode.
			 */
			d3d_new_display_format = ALLEGRO_PIXEL_FORMAT_RGB_565;
		}
	}

	if (!d3d_parameters_are_valid(d3d_new_display_format, d3d_new_display_refresh_rate, d3d_new_display_flags)) {
		TRACE("d3d_create_display: Invalid parameters.\n");
		d3d_waiting_for_display = false;
		return;
	}


	d = _AL_MALLOC(sizeof *d);
	memset(d, 0, sizeof *d);

	d->display.w = d3d_new_display_w;
	d->display.h = d3d_new_display_h;
	d->display.vt = vt;

	AL_SYSTEM_WIN *system = (AL_SYSTEM_WIN *)al_system_driver();

	/* Add ourself to the list of displays. */
	AL_DISPLAY_D3D **add = _al_vector_alloc_back(&system->system.displays);
	*add = d;

	/* Each display is an event source. */
	_al_event_source_init(&d->display.es);

	_al_d3d_last_created_display = d;

	d->window = _al_win_create_window((AL_DISPLAY *)d, d3d_new_display_w,
		d3d_new_display_h, d3d_new_display_flags);
	
	if (!d->window) {
		d3d_waiting_for_display = false;
		return;
	}

	d->display.format = d3d_new_display_format;
	d->display.refresh_rate = d3d_new_display_refresh_rate;
	d->display.flags = d3d_new_display_flags;

	if (d3d_new_display_flags & AL_WINDOWED) {
		if (!d3d_create_swap_chain(d, d3d_new_display_format, d3d_new_display_refresh_rate, d3d_new_display_flags)) {
			d3d_destroy_display((AL_DISPLAY *)d);
			_al_d3d_last_created_display = NULL;
			d3d_waiting_for_display = false;
			return;
		}
	}
	else {
		if (!d3d_create_fullscreen_device(d, d3d_new_display_format, d3d_new_display_refresh_rate, d3d_new_display_flags)) {
			d3d_destroy_display((AL_DISPLAY *)d);
			_al_d3d_last_created_display = NULL;
			d3d_waiting_for_display = false;
			return;
		}
	}

	d->destroyed = false;

	/* Keep track of the displays created */
	add = _al_vector_alloc_back(&d3d_created_displays);
	*add = d;

	d3d_waiting_for_display = false;

	for (;;) {
		if (d == d3d_destroyed_display) {
			break;
		}
		/* FIXME: How long should we wait? */
		result = MsgWaitForMultipleObjects(_win_input_events, _win_input_event_id, FALSE, 5/*INFINITE*/, QS_ALLINPUT);
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
			if (_al_d3d_device) {
				_al_d3d_lock_device();
				hr = IDirect3DDevice9_TestCooperativeLevel(_al_d3d_device);

				if (hr == D3D_OK) {
					_al_d3d_device_lost = false;
				}
				else if (hr == D3DERR_DEVICELOST) {
					/* device remains lost */
					if (!lost_event_generated) {
						_al_event_source_lock(&d->display.es);
						if (_al_event_source_needs_to_generate_event(&d->display.es)) {
							AL_EVENT *event = _al_event_source_get_unused_event(&d->display.es);
							if (event) {
								event->display.type = AL_EVENT_DISPLAY_LOST;
								event->display.timestamp = al_current_time();
								_al_event_source_emit_event(&d->display.es, event);
							}
						}
						_al_event_source_unlock(&d->display.es);
						lost_event_generated = true;
					}
				}
				else if (hr == D3DERR_DEVICENOTRESET) {
					if (_al_d3d_reset_device()) {
						_al_d3d_device_lost = false;
						_al_event_source_lock(&d->display.es);
						if (_al_event_source_needs_to_generate_event(&d->display.es)) {
							AL_EVENT *event = _al_event_source_get_unused_event(&d->display.es);
							if (event) {
								event->display.type = AL_EVENT_DISPLAY_FOUND;
								event->display.timestamp = al_current_time();
								_al_event_source_emit_event(&d->display.es, event);
							}
						}
						_al_event_source_unlock(&d->display.es);
						lost_event_generated = false;
					}
				}
				_al_d3d_unlock_device();
			}
		}
	}

End:
	if (!(d->display.flags & AL_WINDOWED)) {
		IDirect3DDevice9_Release(_al_d3d_device);
	}

	_al_win_delete_thread_handle(GetCurrentThreadId());

	d->destroyed = true;


	TRACE("d3d display thread exits\n");
}


/* Create a new X11 dummy display, which maps directly to a GLX window. */
static AL_DISPLAY *d3d_create_display(int w, int h)
{
	int format, refresh_rate, flags;

	_al_d3d_lock_device();

	if (!d3d_display_initialized) {
		if (_al_d3d_init_display() != false) {
			return NULL;
		}
		d3d_display_initialized = true;
	}
	
	format = al_get_new_display_format();
	refresh_rate = al_get_new_display_refresh_rate();
	flags = al_get_new_display_flags();

	if (d3d_created_displays._size == 0 && (flags & AL_WINDOWED)) {
		if (!d3d_create_hidden_device()) {
			_al_d3d_unlock_device();
			return NULL;
		}
	}

	d3d_new_display_w = w;
	d3d_new_display_h = h;

	d3d_new_display_format = al_get_new_display_format();
	d3d_new_display_refresh_rate = al_get_new_display_refresh_rate();
	d3d_new_display_flags = al_get_new_display_flags();

	_beginthread(d3d_display_thread_proc, 0, 0);

	while (d3d_waiting_for_display)
		al_rest(1);

	d3d_waiting_for_display = true;

	if (_al_d3d_last_created_display == NULL) {
		return NULL;
	}

	win_grab_input();

	if (!(flags & AL_WINDOWED)) {
		d3d_already_fullscreen = true;
		d3d_fullscreen_display = _al_d3d_last_created_display;
	}

	_al_d3d_unlock_device();

	return (AL_DISPLAY *)_al_d3d_last_created_display;
}

/* FIXME: this will have to return a success/failure */
static void d3d_set_current_display(AL_DISPLAY *d)
{
	/* Don't need to do anything */
}

/* Dummy implementation of clear. */
static void d3d_clear(AL_DISPLAY *d, AL_COLOR *color)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;

	if (_al_d3d_is_device_lost()) return;
	_al_d3d_lock_device();

	IDirect3DDevice9_Clear(_al_d3d_device, 0, NULL,
		D3DCLEAR_TARGET,
		d3d_al_color_to_d3d(d->format, color),
		1, 0);
	
	_al_d3d_unlock_device();
}

/* Dummy implementation of line. */
static void d3d_draw_line(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
   AL_COLOR *color)
{
	static D3D_COLORED_VERTEX points[2] = { { 0.0f, 0.0f, 0.0f, 0 }, };
	DWORD d3d_color = d3d_al_color_to_d3d(d->format, color);
	BYTE *vertex_buffer_data;
	
	if (_al_d3d_is_device_lost()) return;
	_al_d3d_lock_device();

	if (IDirect3DVertexBuffer9_Lock(d3d_line_vertex_buffer, 0, 0,
			(void **)&vertex_buffer_data, D3DLOCK_DISCARD) != D3D_OK) {
		_al_d3d_unlock_device();
		return;
	}

	points[0].x = fx;
	points[0].y = fy;
	points[0].color = d3d_color;

	points[1].x = tx;
	points[1].y = ty;
	points[1].color = d3d_color;

	memcpy(vertex_buffer_data, points, sizeof(points));

	IDirect3DVertexBuffer9_Unlock(d3d_line_vertex_buffer);

	if (IDirect3DDevice9_SetStreamSource(_al_d3d_device, 0,
			d3d_line_vertex_buffer, 0, sizeof(D3D_COLORED_VERTEX)) != D3D_OK) {
		_al_d3d_unlock_device();
		return;
	}

	IDirect3DDevice9_SetFVF(_al_d3d_device, D3DFVF_COLORED_VERTEX);

	IDirect3DDevice9_DrawPrimitive(_al_d3d_device, D3DPT_LINELIST, 0,
		1);
	
	_al_d3d_unlock_device();
}
 
/* Dummy implementation of a filled rectangle. */
static void d3d_draw_filled_rectangle(AL_DISPLAY *d, float tlx, float tly,
   float brx, float bry, AL_COLOR *color)
{
	D3DRECT rect;
	float w = brx - tlx;
	float h = bry - tly;

	if (_al_d3d_is_device_lost()) return;
	
	if (w < 1 || h < 1) {
		return;
	}
	
	rect.x1 = tlx;
	rect.y1 = tly;
	rect.x2 = brx;
	rect.y2 = bry;

	_al_d3d_lock_device();

	_al_d3d_draw_textured_quad(NULL,
		0.0f, 0.0f, w, h,
		tlx, tly, w, h,
		w/2, h/2, 0.0f,
		d3d_al_color_to_d3d(d->format, color), 0);

	_al_d3d_unlock_device();
}

/* Dummy implementation of flip. */
static void d3d_flip_display(AL_DISPLAY *d)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;
	HRESULT hr;
	
	if (_al_d3d_is_device_lost()) return;
	_al_d3d_lock_device();

	IDirect3DDevice9_EndScene(_al_d3d_device);

	hr = IDirect3DSwapChain9_Present(disp->swap_chain, 0, 0, disp->window, NULL, 0);

	if (hr == D3DERR_DEVICELOST) {
		_al_d3d_device_lost = true;
		_al_d3d_unlock_device();
		return;
	}

	IDirect3DDevice9_BeginScene(_al_d3d_device);
	
	_al_d3d_unlock_device();
}

static bool d3d_update_display_region(AL_DISPLAY *d,
	int x, int y,
	int width, int height)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;
	HRESULT hr;
	RGNDATA *rgndata;
	bool ret;
	
	if (_al_d3d_is_device_lost()) return false;
	_al_d3d_lock_device();

	if (d->flags & AL_SINGLEBUFFER) {
		AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;

		IDirect3DDevice9_EndScene(_al_d3d_device);

		RECT rect;
		rect.left = x;
		rect.right = x+width;
		rect.top = y;
		rect.bottom = y+height;

		rgndata = malloc(sizeof(RGNDATA)+sizeof(RECT)-1);
		rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
		rgndata->rdh.iType = RDH_RECTANGLES;
		rgndata->rdh.nCount = 1;
		rgndata->rdh.nRgnSize = sizeof(RECT);
		memcpy(&rgndata->rdh.rcBound, &rect, sizeof(RECT));
		memcpy(rgndata->Buffer, &rect, sizeof(RECT));

		hr = IDirect3DSwapChain9_Present(disp->swap_chain, &rect, &rect, disp->window, rgndata, 0);

		free(rgndata);

		if (hr == D3DERR_DEVICELOST) {
			_al_d3d_device_lost = true;
			_al_d3d_unlock_device();
			return true;
		}

		IDirect3DDevice9_BeginScene(_al_d3d_device);

		ret = true;
	}
	else {
		ret = false;
	}
	
	_al_d3d_unlock_device();

	return ret;
}

static void d3d_notify_resize(AL_DISPLAY *d)
{
	if (d->flags & AL_WINDOWED) {
		d3d_do_resize(d);
	}
}

/*
 * Returns false if the device is not in a usable state.
 */
bool _al_d3d_is_device_lost()
{
	HRESULT hr;

	return (_al_d3d_device_lost);
}

/*
 * Upload a rectangle of a compatibility bitmap.
 */
static void d3d_upload_compat_screen(BITMAP *bitmap, int x, int y, int w, int h)
{
	int cx;
	int cy;
	int pixel;
	LPDIRECT3DSURFACE9 render_target;
	RECT rect;
	D3DLOCKED_RECT locked_rect;

	if (_al_d3d_is_device_lost()) return;
	_al_d3d_lock_device();

	x += bitmap->x_ofs;
	y += bitmap->y_ofs;
	
	if (x < 0) {
		w -= -x;
		x = 0;
	}
	if (y < 0) {
		h -= -y;
		y = 0;
	}

	if (w <= 0 || h <= 0) {
		_al_d3d_unlock_device();
		return;
	}

	if (IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target) != D3D_OK) {
		TRACE("d3d_upload_compat_bitmap: GetRenderTarget failed.\n");
		_al_d3d_unlock_device();
		return;
	}

	rect.left = x;
	rect.top = y;
	rect.right = x + w;
	rect.bottom = y + h;

	if (IDirect3DSurface9_LockRect(render_target, &locked_rect, &rect, 0) != D3D_OK) {
		IDirect3DSurface9_Release(render_target);
		_al_d3d_unlock_device();
		return;
	}

	_al_convert_compat_bitmap(
		screen,
		locked_rect.pBits, _al_current_display->format, locked_rect.Pitch,
		x, y, 0, 0, w, h);

	IDirect3DSurface9_UnlockRect(render_target);
	IDirect3DSurface9_Release(render_target);

	_al_d3d_unlock_device();

	al_update_display_region(x, y, w, h);
}

AL_BITMAP *_al_d3d_create_bitmap(AL_DISPLAY *d,
	unsigned int w, unsigned int h)
{
   AL_BITMAP_D3D *bitmap = (AL_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);
   int format;
   int flags;

   format = al_get_new_bitmap_format();
   flags = al_get_new_bitmap_flags();

   if (format == ALLEGRO_PIXEL_FORMAT_ANY) {
   	format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
   }

   bitmap->bitmap.vt = _al_bitmap_d3d_driver();
   bitmap->bitmap.memory = NULL;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;

   bitmap->video_texture = 0;
   bitmap->system_texture = 0;
   bitmap->initialized = false;
   bitmap->is_backbuffer = false;
   bitmap->xo = 0;
   bitmap->yo = 0;

   return &bitmap->bitmap;
}

static void d3d_set_target_bitmap(AL_DISPLAY *display, AL_BITMAP *bitmap)
{
	AL_DISPLAY_D3D *d3d_display = (AL_DISPLAY_D3D *)bitmap->display;
	AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;

	if (_al_d3d_is_device_lost()) return;

	/* Release the previous render target */
	_al_d3d_lock_device();
	if (d3d_current_texture_render_target != NULL) {
		IDirect3DSurface9_Release(d3d_current_texture_render_target);
		d3d_current_texture_render_target = NULL;
	}
	_al_d3d_unlock_device();

	/* Set the render target */
	if (d3d_bmp->is_backbuffer) {
		if (IDirect3DDevice9_SetRenderTarget(_al_d3d_device, 0, d3d_display->render_target) != D3D_OK) {
			TRACE("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
			IDirect3DSurface9_Release(d3d_current_texture_render_target);
			return;
		}
		_al_d3d_unlock_device();
		_al_d3d_set_ortho_projection(display->w, display->h);
		_al_d3d_lock_device();
		IDirect3DDevice9_BeginScene(_al_d3d_device);
		d3d_target_display_before_device_lost = display;
		d3d_target_bitmap_before_device_lost = bitmap;
		_al_d3d_unlock_device();
	}
	else {
		_al_d3d_lock_device();
		IDirect3DDevice9_EndScene(_al_d3d_device);
		if (IDirect3DTexture9_GetSurfaceLevel(d3d_bmp->video_texture, 0, &d3d_current_texture_render_target) != D3D_OK) {
			TRACE("d3d_set_target_bitmap: Unable to get texture surface level.\n");
			return;
		}
		if (IDirect3DDevice9_SetRenderTarget(_al_d3d_device, 0, d3d_current_texture_render_target) != D3D_OK) {
			TRACE("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
			IDirect3DSurface9_Release(d3d_current_texture_render_target);
			return;
		}
		_al_d3d_unlock_device();
		_al_d3d_set_ortho_projection(d3d_bmp->texture_w, d3d_bmp->texture_h);
		_al_d3d_lock_device();
		IDirect3DDevice9_BeginScene(_al_d3d_device);
		d3d_target_display_before_device_lost = display;
		d3d_target_bitmap_before_device_lost = bitmap;
		_al_d3d_unlock_device();
	}

	_al_d3d_lock_device();

	d3d_reset_state();

	_al_d3d_unlock_device();
}

static AL_BITMAP *d3d_get_backbuffer()
{
	d3d_backbuffer.bitmap.display = _al_current_display;
	d3d_backbuffer.bitmap.format = _al_current_display->format;
	d3d_backbuffer.bitmap.flags = 0;
	d3d_backbuffer.bitmap.w = _al_current_display->w;
	d3d_backbuffer.bitmap.h = _al_current_display->h;
	return (AL_BITMAP *)&d3d_backbuffer;
}

static AL_BITMAP *d3d_get_frontbuffer()
{
	return NULL;
}

static bool d3d_is_compatible_bitmap(AL_DISPLAY *display, AL_BITMAP *bitmap)
{
	return true;
}

static void d3d_switch_out(void)
{
	_al_d3d_prepare_bitmaps_for_reset();
	d3d_bitmaps_prepared_for_reset = true;
}

/* Obtain a reference to this driver. */
AL_DISPLAY_INTERFACE *_al_display_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = d3d_create_display;
   vt->destroy_display = d3d_destroy_display;
   vt->set_current_display = d3d_set_current_display;
   vt->clear = d3d_clear;
   vt->draw_line = d3d_draw_line;
   vt->draw_filled_rectangle = d3d_draw_filled_rectangle;
   vt->flip_display = d3d_flip_display;
   vt->update_display_region = d3d_update_display_region;
   vt->notify_resize = d3d_notify_resize;
   vt->create_bitmap = _al_d3d_create_bitmap;
   vt->upload_compat_screen = d3d_upload_compat_screen;
   vt->set_target_bitmap = d3d_set_target_bitmap;
   vt->get_backbuffer = d3d_get_backbuffer;
   vt->get_frontbuffer = d3d_get_frontbuffer;
   vt->is_compatible_bitmap = d3d_is_compatible_bitmap;
   vt->switch_out = d3d_switch_out;
   vt->draw_memory_bitmap_region = NULL;

   d3d_backbuffer.bitmap.vt = (AL_BITMAP_INTERFACE *)_al_bitmap_d3d_driver();

   return vt;
}

