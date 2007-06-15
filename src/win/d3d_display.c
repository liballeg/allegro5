/*
 * This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * display driver.
 */

#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "system_new.h"
#include "internal/aintern.h"
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"
#include "internal/aintern_vector.h"
#include "allegro/platform/aintwin.h"

#include "wddraw.h"
#include "d3d.h"

static AL_DISPLAY_INTERFACE *vt = 0;

static D3DPRESENT_PARAMETERS d3d_pp;

static _AL_VECTOR d3d_created_displays = _AL_VECTOR_INITIALIZER(AL_DISPLAY_D3D *);

static HWND d3d_hidden_window;

static float d3d_ortho_w;
static float d3d_ortho_h;

/* Preserved vertex buffers for speed */
static IDirect3DVertexBuffer9 *d3d_line_vertex_buffer;

/* The window code needs this to set a thread handle */
AL_DISPLAY_D3D *_al_d3d_last_created_display = 0;

/* Dummy graphics driver for compatibility */
GFX_DRIVER _al_d3d_dummy_gfx_driver = {
	0,
	"Dummy Compatibility GFX driver",
	"Dummy Compatibility GFX driver",
	"Dummy Compatibility GFX driver",
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

static int d3d_valid_formats[] = {
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

static int al_format_to_d3d(int format)
{
	int i;
	D3DDISPLAYMODE d3d_dm;

	for (i = 1; d3d_valid_formats[i] >= 0; i++) {
		if (d3d_valid_formats[i] == format) {
			return d3d_formats[i];
		}
	}

	/* If not found or ALLEGRO_PIXEL_FORMAT_ANY, return desktop format */
	
	IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

	return d3d_dm.Format;
}

static bool d3d_format_is_valid(int format)
{
	int i;

	for (i = 0; d3d_valid_formats[i] >= 0; i++) {
		if (d3d_valid_formats[i] == format)
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

static void d3d_reset_state()
{
	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ZENABLE, D3DZB_FALSE);
	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ZWRITEENABLE, FALSE);
	IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_LIGHTING, FALSE);
}

void _al_d3d_get_current_ortho_projection_parameters(float *w, float *h)
{
	*w = d3d_ortho_w;
	*h = d3d_ortho_h;
}

void _al_d3d_set_ortho_projection(float w, float h)
{
	D3DXMATRIX matOrtho;
	D3DXMATRIX matIdentity;

	d3d_ortho_w = w;
	d3d_ortho_h = h;

	D3DXMatrixOrthoOffCenterLH(&matOrtho, 0.0f, d3d_ortho_w, d3d_ortho_h, 0.0f, -1.0f, 1.0f);

	D3DXMatrixIdentity(&matIdentity);

	IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_PROJECTION, &matOrtho);
	IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_WORLD, &matIdentity);
	IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_VIEW, &matIdentity);
}

static bool d3d_create_device()
{
	D3DDISPLAYMODE d3d_dm;
	LPDIRECT3DSURFACE9 render_target;
	int ret;

	d3d_hidden_window = _al_d3d_create_hidden_window();
	if (d3d_hidden_window == 0) {
		TRACE("Failed to create hidden window.\n");
		return 1;
	}

	_al_d3d_set_kb_cooperative_level(d3d_hidden_window);

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
			return 1;
		}
	}

	/* We won't be using this surface, so release it immediately */

	IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target);
	IDirect3DSurface9_Release(render_target);

	d3d_reset_state();

	if (d3d_create_vertex_buffers() != false) {
		TRACE("Failed to create vertex buffers.\n");
		IDirect3DDevice9_Release(_al_d3d_device);
		_al_d3d_device = 0;
		return 1;
	}

	TRACE("Direct3D device created.\n");

	return 0;
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

/*
	if (IDirect3D9_CheckDeviceFormat(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE, D3DFMT_D24S8) != D3D_OK) {
		IDirect3D8_Release(_al_d3d);
		TRACE("Texture depth stencil not supported.\n");
		return true;
	}
	*/

	if (d3d_create_device() != 0) {
		return 1;
	}

	return false;
}

IDirect3DSurface9 *al_d3d_get_current_render_target(void)
{
	AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)_al_current_display;

	return disp->render_target;
}

static bool d3d_create_swap_chain(AL_DISPLAY_D3D *d,
	int format, int refresh_rate, int flags)
{
	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.BackBufferFormat = al_format_to_d3d(format);
	d3d_pp.BackBufferWidth = d->display.w;
	d3d_pp.BackBufferHeight = d->display.h;
	d3d_pp.BackBufferCount = 1;
	d3d_pp.Windowed = 1;
	d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (flags & AL_UPDATE_IMMEDIATE) {
		d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
	}
	else {
		d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	}
	d3d_pp.hDeviceWindow = d->window;
	d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	//d3d_pp.EnableAutoDepthStencil = true;
	//d3d_pp.AutoDepthStencilFormat = D3DFMT_D24S8;

	if (IDirect3DDevice9_CreateAdditionalSwapChain(_al_d3d_device, &d3d_pp, &d->swap_chain) != D3D_OK) {
		return true;
	}

	if (IDirect3DSwapChain9_GetBackBuffer(d->swap_chain, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target) != D3D_OK) {
		IDirect3DSwapChain9_Release(d->swap_chain);
		return true;
	}

/*
	if (IDirect3DDevice8_CreateDepthStencilSurface(_al_d3d_device, d->display.w, d->display.h,
			D3DFMT_D24S8, D3DMULTISAMPLE_NONE, &d->stencil_buffer) != D3D_OK) {
		IDirect3DSurface8_Release(d->render_target);
		IDirect3DSwapChain8_Release(d->swap_chain);
		return true;
	}
	*/

	return false;
}

/* Create a new X11 dummy display, which maps directly to a GLX window. */
static AL_DISPLAY *d3d_create_display(int w, int h)
{
	int format;
	int refresh_rate;
	int flags;
	AL_DISPLAY_D3D *d;

	al_get_display_parameters(&format, &refresh_rate, &flags);

	if (format == ALLEGRO_PIXEL_FORMAT_ANY) {
		/* FIXME: Use desktop format? */
		format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
	}

	if (!d3d_parameters_are_valid(format, refresh_rate, flags)) {
		TRACE("d3d_create_display: Invalid parameters.\n");
		return NULL;
	}

	d = _AL_MALLOC(sizeof *d);
	memset(d, 0, sizeof *d);

	d->display.w = w;
	d->display.h = h;
	d->display.vt = vt;

	AL_SYSTEM_D3D *system = (AL_SYSTEM_D3D *)al_system_driver();

	/* Add ourself to the list of displays. */
	AL_DISPLAY_D3D **add = _al_vector_alloc_back(&system->system.displays);
	*add = d;

	/* Each display is an event source. */
	_al_event_source_init(&d->display.es);

	_al_d3d_last_created_display = d;

	d->window = _al_d3d_create_window(w, h);

	d->display.format = format;
	d->display.refresh_rate = refresh_rate;
	d->display.flags = flags;

	if (d3d_create_swap_chain(d, format, refresh_rate, flags) != 0) {
		return 0;
	}

	/* Keep track of the displays created */
	add = _al_vector_alloc_back(&d3d_created_displays);
	*add = d;

	win_grab_input();

	return (AL_DISPLAY *)d;
}

static void d3d_destroy_display(AL_DISPLAY *display)
{
	unsigned int i;
	AL_DISPLAY_D3D *d3d_display = (AL_DISPLAY_D3D *)display;
	AL_SYSTEM_D3D *system = (AL_SYSTEM_D3D *)al_system_driver();

	IDirect3DSurface9_Release(d3d_display->render_target);
	//IDirect3DSurface9_Release(d3d_display->stencil_buffer);
	IDirect3DSwapChain9_Release(d3d_display->swap_chain);

	_al_d3d_win_ungrab_input();

	SendMessage(d3d_display->window, _al_win_msg_suicide, 0, 0);

	_al_event_source_free(&display->es);

	//_al_vector_find_and_delete(&system->system.displays, display);
	_al_d3d_delete_from_vector(&system->system.displays, display);

	//_al_vector_find_and_delete(&d3d_created_displays, display);
	_al_d3d_delete_from_vector(&d3d_created_displays, display);

	if (d3d_created_displays._size > 0) {
		AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, 0);
		AL_DISPLAY_D3D *d = *dptr;
		_al_win_wnd = d->window;
		win_grab_input();
	}

	_AL_FREE(display);
}

static void d3d_change_current_display(AL_DISPLAY *d)
{
	if (_al_d3d_device) {
		AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)d;

		_al_d3d_set_ortho_projection(d->w, d->h);

		IDirect3DDevice9_SetRenderTarget(_al_d3d_device, 0, disp->render_target);

		d3d_reset_state();

		IDirect3DDevice9_BeginScene(_al_d3d_device);
	}
}

/* Dummy implementation of clear. */
static void d3d_clear(AL_DISPLAY *d, AL_COLOR color)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;
	IDirect3DDevice9_Clear(_al_d3d_device, 0, NULL,
		D3DCLEAR_TARGET,
		AL_COLOR_TO_D3D(color),
		1, 0);
}

/* Dummy implementation of line. */
static void d3d_draw_line(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
   AL_COLOR color)
{
	static D3D_COLORED_VERTEX points[2] = { { 0.0f, 0.0f, 0.0f, 0 }, };
	DWORD d3d_color = AL_COLOR_TO_D3D(color);
	BYTE *vertex_buffer_data;
	
	if (IDirect3DVertexBuffer9_Lock(d3d_line_vertex_buffer, 0, 0,
			(void **)&vertex_buffer_data, D3DLOCK_DISCARD) != D3D_OK) {
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
		return;
	}

	IDirect3DDevice9_SetFVF(_al_d3d_device, D3DFVF_COLORED_VERTEX);

	IDirect3DDevice9_DrawPrimitive(_al_d3d_device, D3DPT_LINELIST, 0,
		1);
}
 
/* Dummy implementation of a filled rectangle. */
static void d3d_filled_rectangle(AL_DISPLAY *d, float tlx, float tly,
   float brx, float bry, AL_COLOR color)
{
	D3DRECT rect;

	if ((brx-tlx) < 1 || (bry-tly) < 1)
		return;

	rect.x1 = tlx;
	rect.y1 = tly;
	rect.x2 = brx;
	rect.y2 = bry;

	IDirect3DDevice9_Clear(_al_d3d_device,
		1, &rect, D3DCLEAR_TARGET, AL_COLOR_TO_D3D(color),
		1.0f, 0);
}

/* Dummy implementation of flip. */
static void d3d_flip_display(AL_DISPLAY *d)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;

	IDirect3DDevice9_EndScene(_al_d3d_device);

	IDirect3DSwapChain9_Present(disp->swap_chain, 0, 0, disp->window, NULL, 0);

	IDirect3DDevice9_BeginScene(_al_d3d_device);
}

/* Dummy implementation of flip. */
static void d3d_flip_display_region(AL_DISPLAY *d,
	int x, int y,
	int width, int height)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;

	if (d->flags & AL_UPDATE_IMMEDIATE) {
		AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;

		IDirect3DDevice9_EndScene(_al_d3d_device);

		RECT rect;
		rect.left = x;
		rect.right = x+width;
		rect.top = y;
		rect.bottom = y+height;

		long start = al_current_time();

		IDirect3DSwapChain9_Present(disp->swap_chain, &rect, &rect, disp->window, NULL, 0);

		IDirect3DDevice9_BeginScene(_al_d3d_device);
	}
}

static void d3d_notify_resize(AL_DISPLAY *d)
{
	AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)d;
	WINDOWINFO wi;
	D3DDISPLAYMODE d3d_dm;

	wi.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(disp->window, &wi);
	d->w = wi.rcClient.right - wi.rcClient.left;
	d->h = wi.rcClient.bottom - wi.rcClient.top;

	if (gfx_driver) {
		gfx_driver->w = d->w;
		gfx_driver->h = d->h;
	}

	if (_al_d3d_device) {
		unsigned int i;
		HRESULT hr;
		LPDIRECT3DSURFACE9 render_target;
	
		_al_d3d_prepare_bitmaps_for_reset();
		_al_d3d_release_default_pool_textures();

		for (i = 0; i < d3d_created_displays._size; i++) {
			AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
			AL_DISPLAY_D3D *disp = *dptr;

			//IDirect3DSurface9_Release(disp->stencil_buffer);
			IDirect3DSurface9_Release(disp->render_target);
			IDirect3DSwapChain9_Release(disp->swap_chain);
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
		//d3d_pp.EnableAutoDepthStencil = true;
		//d3d_pp.AutoDepthStencilFormat = D3DFMT_D24S8;
		d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		hr = IDirect3DDevice9_Reset(_al_d3d_device, &d3d_pp);

		if (hr) {
			TRACE("d3d_notify_resize: Reset failed.\n");
		}

		IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target);
		IDirect3DSurface9_Release(render_target);

		for (i = 0; i < d3d_created_displays._size; i++) {
			AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
			AL_DISPLAY_D3D *d = *dptr;
			AL_DISPLAY *al_display = (AL_DISPLAY *)d;
			int flags = 0;

			d3d_create_swap_chain(d, al_display->format, al_display->refresh_rate, al_display->flags);
		}

		d3d_reset_state();

		_al_d3d_refresh_texture_memory();
	}
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

	if (w <= 0 || h <= 0)
		return;

	if (IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target) != D3D_OK) {
		TRACE("d3d_upload_compat_bitmap: GetRenderTarget failed.\n");
		return;
	}

	rect.left = x;
	rect.top = y;
	rect.right = x + w;
	rect.bottom = y + h;

	if (IDirect3DSurface9_LockRect(render_target, &locked_rect, &rect, 0) != D3D_OK) {
		IDirect3DSurface9_Release(render_target);
		return;
	}

	_al_convert_compat_bitmap(
		screen,
		locked_rect.pBits, _al_current_display->format, locked_rect.Pitch,
		x, y, 0, 0, w, h);

#if 0
	if (bitmap_color_depth(bitmap) == 8 || bitmap_color_depth(bitmap) == 12) {
		_al_convert_bitmap_data(bitmap->dat, ALLEGRO_PIXEL_FORMAT_PALETTE_8, bitmap->w,
			locked_rect.pBits, _al_current_display->format, locked_rect.Pitch,
			x, y, w, h);
	}
	if (bitmap_color_depth(bitmap) == 15) {
		for (cx = x; cx < x+w; cx++) {
			int pixel = *(uint16_t *)(bitmap->line[cy]+cx*2);
			*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr15(pixel), getg15(pixel), getb15(pixel));
		}
	}
	if (bitmap_color_depth(bitmap) == 16) {
		for (cx = x; cx < x+w; cx++) {
			int pixel = *(uint16_t *)(bitmap->line[cy]+cx*2);
			*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr16(pixel), getg16(pixel), getb16(pixel));
		}
	}
	if (bitmap_color_depth(bitmap) == 24) {
		for (cx = x; cx < x+w; cx++) {
			int pixel = READ3BYTES(bitmap->line[cy]+cx*3);
			*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr24(pixel), getg24(pixel), getb24(pixel));
		}
	}
	if (bitmap_color_depth(bitmap) == 32) {
		for (cx = x; cx < x+w; cx++) {
			int pixel = *(uint32_t *)(bitmap->line[cy]+cx*4);
			*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr32(pixel), getg32(pixel), getb32(pixel));
		}
	}
/*
	for (cy = y; cy < y+h; cy++) {
		if (bitmap_color_depth(bitmap) == 8 || bitmap_color_depth(bitmap) == 12) {
			for (cx = x; cx < x+w; cx++) {
				int pixel = *(unsigned char *)(bitmap->line[cy]+cx);
				*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr8(pixel), getg8(pixel), getb8(pixel));
			}
		}
		if (bitmap_color_depth(bitmap) == 15) {
			for (cx = x; cx < x+w; cx++) {
				int pixel = *(uint16_t *)(bitmap->line[cy]+cx*2);
				*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr15(pixel), getg15(pixel), getb15(pixel));
			}
		}
		if (bitmap_color_depth(bitmap) == 16) {
			for (cx = x; cx < x+w; cx++) {
				int pixel = *(uint16_t *)(bitmap->line[cy]+cx*2);
				*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr16(pixel), getg16(pixel), getb16(pixel));
			}
		}
		if (bitmap_color_depth(bitmap) == 24) {
			for (cx = x; cx < x+w; cx++) {
				int pixel = READ3BYTES(bitmap->line[cy]+cx*3);
				*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr24(pixel), getg24(pixel), getb24(pixel));
			}
		}
		if (bitmap_color_depth(bitmap) == 32) {
			for (cx = x; cx < x+w; cx++) {
				int pixel = *(uint32_t *)(bitmap->line[cy]+cx*4);
				*(uint32_t *)(locked_rect.pBits+(cy-y)*locked_rect.Pitch+(cx-x)*4) = D3DCOLOR_ARGB(0, getr32(pixel), getg32(pixel), getb32(pixel));
			}
		}
	}
*/
#endif

	IDirect3DSurface9_UnlockRect(render_target);
	IDirect3DSurface9_Release(render_target);


	al_flip_display_region(x, y, w, h);
}

AL_BITMAP *_al_d3d_create_bitmap(AL_DISPLAY *d,
	unsigned int w, unsigned int h)
{
   AL_BITMAP_D3D *bitmap = (AL_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);
   int format;
   int flags;

   al_get_bitmap_parameters(&format, &flags);

   if (format == ALLEGRO_PIXEL_FORMAT_ANY) {
   	/* FIXME: use display format */
   	format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
   }

   bitmap->bitmap.memory = _AL_MALLOC(w * h * _al_pixel_size(format));
   bitmap->bitmap.vt = _al_bitmap_d3d_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;
   bitmap->bitmap.display = d;

   bitmap->video_texture = 0;
   bitmap->system_texture = 0;
   bitmap->created = false;
   bitmap->xo = 0;
   bitmap->yo = 0;

   return &bitmap->bitmap;
}

AL_BITMAP *_al_d3d_create_sub_bitmap(AL_DISPLAY *d, AL_BITMAP *parent,
	unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
   AL_BITMAP_D3D *d3d_parent = (AL_BITMAP_D3D *)parent;
   AL_BITMAP_D3D *bitmap = (AL_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);

   bitmap->bitmap.memory = parent->memory;
   bitmap->bitmap.vt = _al_bitmap_d3d_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.format = parent->format;
   bitmap->bitmap.flags = parent->flags;
   bitmap->bitmap.display = d;

   bitmap->video_texture = d3d_parent->video_texture;
   bitmap->system_texture = d3d_parent->system_texture;
   bitmap->created = true;
   bitmap->xo = x;
   bitmap->yo = y;

   return &bitmap->bitmap;
}

/* Obtain a reference to this driver. */
AL_DISPLAY_INTERFACE *_al_display_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = d3d_create_display;
   vt->destroy_display = d3d_destroy_display;
   vt->change_current_display = d3d_change_current_display;
   vt->clear = d3d_clear;
   vt->line = d3d_draw_line;
   vt->filled_rectangle = d3d_filled_rectangle;
   vt->flip_display = d3d_flip_display;
   vt->flip_display_region = d3d_flip_display_region;
   vt->notify_resize = d3d_notify_resize;
   vt->create_bitmap = _al_d3d_create_bitmap;
   vt->create_sub_bitmap = _al_d3d_create_sub_bitmap;
   vt->upload_compat_screen = d3d_upload_compat_screen;

   return vt;
}
