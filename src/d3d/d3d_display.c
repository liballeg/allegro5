/* This is only a dummy driver, not implementing most required things properly,
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

#include "d3d.h"

/* The window code needs this to set a thread handle */
AL_DISPLAY_D3D *_al_d3d_last_created_display = 0;

static AL_DISPLAY_INTERFACE *vt = 0;

static D3DPRESENT_PARAMETERS d3d_pp;

static _AL_VECTOR d3d_created_displays = _AL_VECTOR_INITIALIZER(AL_DISPLAY_D3D *);

static HWND d3d_hidden_window;

static float d3d_ortho_w;
static float d3d_ortho_h;

/* Preserved vertex buffers for speed */
static IDirect3DVertexBuffer8 *d3d_line_vertex_buffer;


static bool d3d_create_vertex_buffers()
{
	if (IDirect3DDevice8_CreateVertexBuffer(_al_d3d_device,
			sizeof(D3D_COLORED_VERTEX)*2,
			D3DUSAGE_WRITEONLY, D3DFVF_COLORED_VERTEX,
			D3DPOOL_MANAGED, &d3d_line_vertex_buffer) != D3D_OK) {
		TRACE("Creating line vertex buffer failed.\n");
		return true;
	}

	return false;
}

static void d3d_reset_state()
{
	IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ZENABLE, D3DZB_FALSE);
	IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ZWRITEENABLE, FALSE);
	IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_LIGHTING, FALSE);
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

	IDirect3DDevice8_SetTransform(_al_d3d_device, D3DTS_PROJECTION, &matOrtho);
	IDirect3DDevice8_SetTransform(_al_d3d_device, D3DTS_WORLD, &matIdentity);
	IDirect3DDevice8_SetTransform(_al_d3d_device, D3DTS_VIEW, &matIdentity);
}

static bool d3d_create_device()
{
	D3DDISPLAYMODE d3d_dm;
	LPDIRECT3DSURFACE8 render_target;
	int ret;

	d3d_hidden_window = _al_d3d_create_hidden_window();
	if (d3d_hidden_window == 0) {
		TRACE("Failed to create hidden window.\n");
		return 1;
	}

	IDirect3D8_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

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
	if ((ret = IDirect3D8_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d3d_hidden_window,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3d_pp, &_al_d3d_device)) != D3D_OK) {
		if ((ret = IDirect3D8_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
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

	IDirect3DDevice8_GetRenderTarget(_al_d3d_device, &render_target);
	IDirect3DSurface8_Release(render_target);

	d3d_reset_state();

	if (d3d_create_vertex_buffers() != false) {
		TRACE("Failed to create vertex buffers.\n");
		IDirect3DDevice8_Release(_al_d3d_device);
		_al_d3d_device = 0;
		return 1;
	}

	TRACE("Direct3D device created.\n");

	return 0;
}

bool _al_d3d_init_display()
{
	if ((_al_d3d = Direct3DCreate8(D3D_SDK_VERSION)) == NULL) {
		TRACE("Direct3DCreate8 failed.\n");
		return true;
	}

	D3DDISPLAYMODE d3d_dm;

	IDirect3D8_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

	if (IDirect3D8_CheckDeviceFormat(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_RENDERTARGET,
			D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) != D3D_OK) {
		IDirect3D8_Release(_al_d3d);
		TRACE("Texture rendering not supported.\n");
		return true;
	}

	if (IDirect3D8_CheckDeviceFormat(_al_d3d, D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE, D3DFMT_D24S8) != D3D_OK) {
		IDirect3D8_Release(_al_d3d);
		TRACE("Texture depth stencil not supported.\n");
		return true;
	}

	if (d3d_create_device() != 0) {
		return 1;
	}

	return false;
}

IDirect3DSurface8 *al_d3d_get_current_render_target(void)
{
	AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)_al_current_display;

	return disp->render_target;
}

static bool d3d_create_swap_chain(AL_DISPLAY_D3D *d)
{
	D3DDISPLAYMODE d3d_dm;

	IDirect3D8_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

	ZeroMemory(&d3d_pp, sizeof(d3d_pp));
	d3d_pp.BackBufferFormat = d3d_dm.Format;
	d3d_pp.BackBufferWidth = d->display.w;
	d3d_pp.BackBufferHeight = d->display.h;
	d3d_pp.BackBufferCount = 1;
	d3d_pp.Windowed = 1;
	d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3d_pp.hDeviceWindow = d->window;
	d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3d_pp.EnableAutoDepthStencil = true;
	d3d_pp.AutoDepthStencilFormat = D3DFMT_D24S8;

	if (IDirect3DDevice8_CreateAdditionalSwapChain(_al_d3d_device, &d3d_pp, &d->swap_chain) != D3D_OK) {
		return true;
	}

	if (IDirect3DSwapChain8_GetBackBuffer(d->swap_chain, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target) != D3D_OK) {
		IDirect3DSwapChain8_Release(d->swap_chain);
		return true;
	}

	if (IDirect3DDevice8_CreateDepthStencilSurface(_al_d3d_device, d->display.w, d->display.h,
			D3DFMT_D24S8, D3DMULTISAMPLE_NONE, &d->stencil_buffer) != D3D_OK) {
		IDirect3DSurface8_Release(d->render_target);
		IDirect3DSwapChain8_Release(d->swap_chain);
		return true;
	}

	return false;
}

/* Create a new X11 dummy display, which maps directly to a GLX window. */
static AL_DISPLAY *d3d_create_display(int w, int h, int flags)
{
	AL_DISPLAY_D3D *d = _AL_MALLOC(sizeof *d);
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

	printf("creating window\n");

	d->window = _al_d3d_create_window(w, h);

	printf("created window\n");

	if (d3d_create_swap_chain(d) != 0) {
		return 0;
	}

	printf("created swap chain\n");

	/* Keep track of the displays created */
	add = _al_vector_alloc_back(&d3d_created_displays);
	*add = d;

	_al_d3d_win_grab_input();

	printf("created display\n");

	return (AL_DISPLAY *)d;
}

static void d3d_make_display_current(AL_DISPLAY *d)
{
	if (_al_d3d_device) {
		AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)d;

		_al_d3d_set_ortho_projection(d->w, d->h);

		IDirect3DDevice8_SetRenderTarget(_al_d3d_device, disp->render_target, disp->stencil_buffer);

		d3d_reset_state();

		IDirect3DDevice8_BeginScene(_al_d3d_device);
	}

}

/* Dummy implementation of clear. */
static void d3d_clear(AL_DISPLAY *d, AL_COLOR color)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;
	IDirect3DDevice8_Clear(_al_d3d_device, 0, NULL,
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
	
	if (IDirect3DVertexBuffer8_Lock(d3d_line_vertex_buffer, 0, 0,
			&vertex_buffer_data, D3DLOCK_DISCARD) != D3D_OK) {
		return;
	}

	points[0].x = fx;
	points[0].y = fy;
	points[0].color = d3d_color;

	points[1].x = tx;
	points[1].y = ty;
	points[1].color = d3d_color;

	memcpy(vertex_buffer_data, points, sizeof(points));

	IDirect3DVertexBuffer8_Unlock(d3d_line_vertex_buffer);

	if (IDirect3DDevice8_SetStreamSource(_al_d3d_device, 0,
			d3d_line_vertex_buffer, sizeof(D3D_COLORED_VERTEX)) != D3D_OK) {
		return;
	}

	IDirect3DDevice8_SetVertexShader(_al_d3d_device, D3DFVF_COLORED_VERTEX);

	IDirect3DDevice8_DrawPrimitive(_al_d3d_device, D3DPT_LINELIST, 0,
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

	IDirect3DDevice8_Clear(_al_d3d_device,
		1, &rect, D3DCLEAR_TARGET, AL_COLOR_TO_D3D(color),
		1.0f, 0);
}

/* Dummy implementation of flip. */
static void d3d_flip(AL_DISPLAY *d)
{
	AL_DISPLAY_D3D* disp = (AL_DISPLAY_D3D*)d;

	IDirect3DDevice8_EndScene(_al_d3d_device);

	IDirect3DSwapChain8_Present(disp->swap_chain, 0, 0, disp->window, 0);

	IDirect3DDevice8_BeginScene(_al_d3d_device);
}

static void d3d_acknowledge_resize(AL_DISPLAY *d)
{
	AL_DISPLAY_D3D *disp = (AL_DISPLAY_D3D *)d;
	WINDOWINFO wi;
	D3DDISPLAYMODE d3d_dm;

	wi.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(disp->window, &wi);
	d->w = wi.rcClient.right - wi.rcClient.left;
	d->h = wi.rcClient.bottom - wi.rcClient.top;


	if (_al_d3d_device) {
		unsigned int i;
		HRESULT hr;
		LPDIRECT3DSURFACE8 render_target;
	
		_al_d3d_release_default_pool_textures();

		for (i = 0; i < d3d_created_displays._size; i++) {
			AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
			AL_DISPLAY_D3D *disp = *dptr;

			IDirect3DSurface8_Release(disp->stencil_buffer);
			IDirect3DSurface8_Release(disp->render_target);
			IDirect3DSwapChain8_Release(disp->swap_chain);
		}

		IDirect3D8_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

		ZeroMemory(&d3d_pp, sizeof(d3d_pp));
		d3d_pp.BackBufferFormat = d3d_dm.Format;
		d3d_pp.BackBufferWidth = 100;
		d3d_pp.BackBufferHeight = 100;
		d3d_pp.BackBufferCount = 1;
		d3d_pp.Windowed = 1;
		d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3d_pp.hDeviceWindow = d3d_hidden_window;
		d3d_pp.EnableAutoDepthStencil = true;
		d3d_pp.AutoDepthStencilFormat = D3DFMT_D24S8;
		d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

		hr = IDirect3DDevice8_Reset(_al_d3d_device, &d3d_pp);

		if (hr) {
			TRACE("d3d_acknowledge_resize: Reset failed.\n");
		}

		IDirect3DDevice8_GetRenderTarget(_al_d3d_device, &render_target);
		IDirect3DSurface8_Release(render_target);

		for (i = 0; i < d3d_created_displays._size; i++) {
			AL_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
			AL_DISPLAY_D3D *d = *dptr;

			d3d_create_swap_chain(d);
		}

		d3d_reset_state();

		_al_d3d_refresh_texture_memory();
	}
}

/* Dummy implementation. */
// TODO: rename to create_bitmap once the A4 function of same name is out of the
// way
// FIXME: think about moving this to xbitmap.c instead..
AL_BITMAP *_al_d3d_create_bitmap(AL_DISPLAY *d, int w, int h, int flags)
{
   AL_BITMAP_D3D *bitmap = (AL_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);
   bitmap->bitmap.vt = _al_bitmap_d3d_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.flags = flags;
   //bitmap->bitmap.display = d;
   bitmap->video_texture = 0;
   bitmap->system_texture = 0;
   bitmap->pot_bmp = 0;
   bitmap->created = false;
   return &bitmap->bitmap;
}

/* Obtain a reference to this driver. */
AL_DISPLAY_INTERFACE *_al_display_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = d3d_create_display;
   vt->make_display_current = d3d_make_display_current;
   vt->clear = d3d_clear;
   vt->line = d3d_draw_line;
   vt->filled_rectangle = d3d_filled_rectangle;
   vt->flip = d3d_flip;
   vt->acknowledge_resize = d3d_acknowledge_resize;
   vt->create_bitmap = _al_d3d_create_bitmap;

   return vt;
}
