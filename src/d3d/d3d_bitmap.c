/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * bitmap driver.
 */

#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "system_new.h"
#include "internal/aintern.h"
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"

#include "d3d.h"


static AL_BITMAP_INTERFACE *vt;
static _AL_VECTOR created_bitmaps = _AL_VECTOR_INITIALIZER(AL_BITMAP_D3D *);


static AL_BITMAP *d3d_munge_bitmap(int flag,
	AL_BITMAP *src, unsigned int sw, unsigned int sh,
	void *dest_surface, int dx, int dy,
	unsigned int dw, unsigned int dh, unsigned int dest_pitch)
{
	AL_BITMAP *ret = (AL_BITMAP *)_al_d3d_create_bitmap(0, dw, dh, 0);
	AL_BITMAP *read_bmp;
	BITMAP *temp_src;
	BITMAP *temp_dest;
	bool scaled = false;
	unsigned int x;
	unsigned int y;

	/* Hack so we don't have to rewrite stretch_blit for AL_BITMAP */
	
	if (sw != dw || sh != dh) {
		unsigned int end_sh = (int)sh;
		unsigned int end_dh = (int)dh;
		int src_size = sizeof(BITMAP) + sizeof(char *) * end_sh;
		int dest_size = sizeof(BITMAP) + sizeof(char *) * end_dh;
	
		temp_src = create_bitmap_ex(32, sw, sh);
		temp_dest = create_bitmap_ex(32, dw, dh);

		for (y = 0; y < end_sh; y++) {
			temp_src->line[y] = src->memory + (src->w*y*4);
		}

		for (y = 0; y < end_dh; y++) {
			temp_dest->line[y] = ret->memory + (ret->w*y*4);
		}

		stretch_blit(temp_src, temp_dest, 0, 0, src->w, src->h, 0, 0, dw, dh);

		destroy_bitmap(temp_src);
		destroy_bitmap(temp_dest);

		scaled = true;

		read_bmp = ret;
	}
	else {
		read_bmp = src;
	}

	if (flag & AL_MASK_SOURCE) {
		int alpha_mask;
		int pixel_mask;
		if (flag & AL_BLEND) {
			alpha_mask = 0;
			pixel_mask = 0xFFFFFFFF;
		}
		else {
			alpha_mask = 0xFF000000;
			pixel_mask = 0xFFFFFF;
		}
		for (y = 0; y < read_bmp->h; y++) {
			for (x = 0; x < read_bmp->w; x++) {
				/* FIXME: use proper getpixel/putpixel */
				int pixel = *(uint32_t *)(read_bmp->memory+(y*read_bmp->w+x)*4);
				/* FIXME: use the mask color */
				if ((pixel & 0xFFFFFF) == 0xFF00FF) {
					pixel = 0x00FF00FF;
				}
				else {
					pixel = (pixel & pixel_mask) | alpha_mask;
				}
				*(uint32_t *)(ret->memory+(y*ret->w+x)*4) = pixel;
			}
		}
	}
	else if (flag & AL_MASK_INV_SOURCE) {
		for (y = 0; y < read_bmp->h; y++) {
			for (x = 0; x < read_bmp->w; x++) {
				/* FIXME: use proper getpixel/putpixel */
				int pixel = *(uint32_t *)(read_bmp->memory+(y*read_bmp->w+x)*4);
				/* FIXME: use the mask color */
				if ((pixel & 0xFFFFFF) != 0xFF00FF) {
					pixel = pixel & 0xFFFFFF;
				}
				else {
					/* Do nothing */
				}
				*(uint32_t *)(ret->memory+(y*ret->w+x)*4) = pixel;
			}
		}
	}
	else if (flag & AL_MASK_DEST) {
		int alpha_mask;
		int pixel_mask;
		/* Destination clipping */
		unsigned int start_x = MAX(0, -dx);
		unsigned int start_y = MAX(0, -dy);
		unsigned int end_x = MIN(read_bmp->w, dw-dx);
		unsigned int end_y = MIN(read_bmp->h, dh-dy);
		if (flag & AL_BLEND) {
			alpha_mask = 0;
			pixel_mask = 0xFFFFFFFF;
		}
		else {
			alpha_mask = 0xFF000000;
			pixel_mask = 0xFFFFFF;
		}
		for (y = start_y; y < end_y; y++) {
			for (x = start_x; x < end_x; x++) {
				/* FIXME: use proper getpixel/putpixel */
				int src_pix = *(uint32_t *)(read_bmp->memory+(y*read_bmp->w+x)*4);
				int dst_pix = *(uint32_t *)(dest_surface+((dy+y)*dest_pitch+(dx+x)*4));
				int pixel;
				/* FIXME: use the mask color */
				if ((dst_pix & 0xFFFFFF) == 0xFF00FF) {
					pixel = (src_pix & pixel_mask) | alpha_mask;
				}
				else {
					pixel = src_pix & 0xFFFFFF;
				}
				*(uint32_t *)(ret->memory+(y*ret->w+x)*4) = pixel;
			}
		}
	}
	else if (flag & AL_MASK_INV_DEST) {
		int alpha_mask;
		int pixel_mask;
		/* Destination clipping */
		unsigned int start_x = MAX(0, -dx);
		unsigned int start_y = MAX(0, -dy);
		unsigned int end_x = MIN(read_bmp->w, dw-dx);
		unsigned int end_y = MIN(read_bmp->h, dh-dy);
		if (flag & AL_BLEND) {
			alpha_mask = 0;
			pixel_mask = 0xFFFFFFFF;
		}
		else {
			alpha_mask = 0xFF000000;
			pixel_mask = 0xFFFFFF;
		}
		for (y = start_y; y < end_y; y++) {
			for (x = start_x; x < end_x; x++) {
				/* FIXME: use proper getpixel/putpixel */
				int src_pix = *(uint32_t *)(read_bmp->memory+(y*read_bmp->w+x)*4);
				int dst_pix = *(uint32_t *)(dest_surface+((dy+y)*dest_pitch+(dx+x)*4));
				int pixel;
				/* FIXME: use the mask color */
				if ((dst_pix & 0xFFFFFF) != 0xFF00FF) {
					pixel = (src_pix & pixel_mask) | alpha_mask;
				}
				else {
					pixel = src_pix & 0xFFFFFF;
				}
				*(uint32_t *)(ret->memory+(y*ret->w+x)*4) = pixel;
			}
		}
	}

	return ret;
}


/*
 * This is used for destination masking. The masked pixels need to be
 * replaced with alpha or the other way around.
 */
static void d3d_remove_mask_alpha(int flag, AL_BITMAP *bmp)
{
	unsigned int x;
	unsigned int y;

	for (y = 0; y < bmp->h; y++) {
		for (x = 0; x < bmp->w; x++) {
			int pixel = *(uint32_t *)(bmp->memory+((y*bmp->w+x)*4));
			if ((pixel & 0xFFFFFF) == 0xFF00FF) {
				pixel = 0xFF00FF;
			}
			else {
				pixel = (pixel & 0xFFFFFF) | 0xFF000000;
			}
			*(uint32_t *)(bmp->memory+((y*bmp->w+x)*4)) = pixel;
		}
	}
}


/*
 * Build a transformation matrix from the arguments.
 * 
 * dest - The output matrix
 * center - The center of scaling and rotation
 * dest - Drawing destination
 * angle - The angle of rotation in radians
 */
static D3DMATRIX *d3d_build_matrix(D3DMATRIX* out, D3DXVECTOR2* centre, 
	D3DXVECTOR2 *dest, float angle)
{
	D3DXMATRIX matrices[3];

	D3DXMatrixTranslation(&matrices[0], -centre->x, -centre->y, 0);
	D3DXMatrixIdentity(&matrices[1]);
	D3DXMatrixIdentity(&matrices[2]);
	D3DXMatrixRotationZ(&matrices[1], angle);
	D3DXMatrixTranslation(&matrices[2], dest->x, dest->y, 0);

	D3DXMatrixMultiply(out, &matrices[0], &matrices[1]);
	D3DXMatrixMultiply(out, out, &matrices[2]);

	return out;
}

/*
 * Transform 4 points
 *
 * center - The center of scaling and rotation
 * dest - Drawing destination
 * angle - Angle of rotation in radians
 */
static void d3d_transform_vertices(D3D_TL_VERTEX vertices[], D3DXVECTOR2* center, 
	D3DXVECTOR2 *dest, float angle)
{
	D3DXMATRIX transform;
	D3DXMATRIX verts;
	D3DXMATRIX result;
	int i;

	/* Build the transformation matrix */
	d3d_build_matrix(&transform, center, dest, angle);

	/* Put the vertices in a matrix */
	for (i = 0; i < 4; i++)	{
		verts.m[i][0] = vertices[i].x;
		verts.m[i][1] = vertices[i].y;
		verts.m[i][2] = vertices[i].z;
		verts.m[i][3] = 1.0f;  /* the scaling factor, w */
	}

	/* Transform the vertices */
	D3DXMatrixMultiply(&result, &verts, &transform);

	/* Put the result back into vertices */
	for (i = 0; i < 4; i++) {
		vertices[i].x = result.m[i][0];
		vertices[i].y = result.m[i][1];
		vertices[i].z = result.m[i][2];
	}
}

/*
 * Draw a textured quad
 *
 * texture - The source texture
 * x - x coordinate to draw the texture at
 * y - y coordinate to draw the texture at
 * w - The width of the actual texture image. This can differ from the size
 *     of the texture because texture sizes are sometimes made bigger so
 *     they will be a power of two.
 * h - The width of the actual texture image.
 * center - Center of scaling and rotation relative to x,y
 * angle - Angle of rotation in radians
 * color - RGB and alpha channels are modulated by this value.
 *         Use 0xFFFFFFFF for an unmodified blit.
 */
void d3d_draw_textured_quad(int flag, AL_BITMAP_D3D *bmp,
	float sx, float sy, float sw, float sh,
	float dx, float dy, float dw, float dh,
	D3DXVECTOR2* center, float angle,
	D3DCOLOR color)
{
	float right;
	float bottom;
	float tu_start;
	float tv_start;
	float tu_end;
	float tv_end;
	D3DXVECTOR2 default_center;
	D3DXVECTOR2 dest;

	const float z = 0.0f;

	right  = dx + dw;
	bottom = dy + dh;

	tu_start = (sx+0.5f) / bmp->texture_w;
	tv_start = (sy+0.5f) / bmp->texture_h;
	tu_end = sw / bmp->texture_w + tu_start;
	tv_end = sh / bmp->texture_h + tv_start;

	if (flag & AL_FLIP_HORIZONTAL) {
		float temp = tu_start;
		tu_start = tu_end;
		tu_end = temp;
		/* Weird hack -- not sure why this is needed */
		tu_start -= 1.0f / bmp->texture_w;
		tu_end -= 1.0f / bmp->texture_w;
	}
	if (flag & AL_FLIP_VERTICAL) {
		float temp = tv_start;
		tv_start = tv_end;
		tv_end = temp;
		/* Weird hack -- not sure why this is needed */
		tv_start -= 1.0f / bmp->texture_h;
		tv_end -= 1.0f / bmp->texture_h;
	}

	D3D_TL_VERTEX vertices[4] = {
		/* x,    y,      z, color, tu,        tv     */
		{ dx,    dy,     z, color, tu_start,  tv_start },
		{ right, dy,     z, color, tu_end,    tv_start },
		{ right, bottom, z, color, tu_end,    tv_end   },
		{ dx,    bottom, z, color, tu_start,  tv_end   }
	};

	D3DXVECTOR2 v_center;

	if (angle == 0.0f) {
		v_center.x = dx + center->x;
		v_center.y = dy + center->y;
	}
	else {
		v_center.x = dx + center->x * (dw / sw);
		v_center.y = dy + center->y * (dh / sh);
	}

	/* Don't modify the argument passed in */
	center = &v_center;

	if (angle == 0.0f) {
		dest.x = center->x;
		dest.y = center->y;
	}
	else {
		dest.x = dx;
		dest.y = dy;
	}

	d3d_transform_vertices(vertices, center, &dest, angle);

	// Draw the sprite
	if (IDirect3DDevice8_SetTexture(_al_d3d_device, 0,
			(IDirect3DBaseTexture8 *)bmp->video_texture) != D3D_OK) {
		TRACE("d3d_draw_textured_quad: SetTexture failed.\n");
		return;
	}

	IDirect3DDevice8_SetTextureStageState(_al_d3d_device,
		0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	
	IDirect3DDevice8_SetTextureStageState(_al_d3d_device,
		0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	
	if (IDirect3DDevice8_DrawPrimitiveUP(_al_d3d_device, D3DPT_TRIANGLEFAN, 2,
			vertices, sizeof(D3D_TL_VERTEX)) != D3D_OK) {
		TRACE("d3d_draw_textured_quad: DrawPrimitive failed.\n");
		return;
	}


	IDirect3DDevice8_SetTexture(_al_d3d_device, 0, 0);
}

static void d3d_copy_texture_data(LPDIRECT3DTEXTURE8 texture, AL_BITMAP *bmp)
{
   D3DLOCKED_RECT locked_rect;

   if (IDirect3DTexture8_LockRect(texture, 0, &locked_rect, NULL, 0) == D3D_OK) {
	unsigned int x;
	unsigned int y;
	// FIXME: use proper getpixel
	for (y = 0; y < bmp->h; y++) {
		for (x = 0; x < bmp->w; x++) {
			int p = *(uint32_t *)(locked_rect.pBits+locked_rect.Pitch*y+x*4);
			*(uint32_t *)(bmp->memory+(x+y*bmp->w)*4) = p;
		}
	}
	IDirect3DTexture8_UnlockRect(texture, 0);
   }
   else {
   	TRACE("d3d_copy_texture_data: Couldn't lock texture.\n");
   }
}

static void d3d_copy_bitmap_data(AL_BITMAP *bmp, LPDIRECT3DTEXTURE8 texture,
	unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
   D3DLOCKED_RECT locked_rect;
   RECT rect;

   rect.left = x;
   rect.top = y;
   rect.right = x + width;
   rect.bottom = y + height;

   if (rect.right != pot(x + width))
   	rect.right++;
   if (rect.bottom != pot(y + height))
   	rect.bottom++;

   if (IDirect3DTexture8_LockRect(texture, 0, &locked_rect, &rect, 0) == D3D_OK) {
	unsigned int cx;
	unsigned int cy;
	int p;
	int d3d_pixel;
	int p;
	int d3d_pixel;
	// FIXME: use proper getpixel
	for (cy = y; cy < y+height; cy++) {
		for (cx = x; cx < x+width; cx++) {
			p = *(uint32_t *)(bmp->memory+(cx+cy*bmp->w)*4);
			d3d_pixel = (geta32(p) << 24) | (getr32(p) << 16) |
			*(uint32_t *)
				(locked_rect.pBits+(locked_rect.Pitch*cy)+(cx*4)) = d3d_pixel;
		}
	}
	/* Copy an extra row so the texture ends nicely */
	if (rect.bottom >= bmp->h) {
		cy = bmp->h;
		for (cx = x; cx <= x+width; cx++) {
			p = *(uint32_t *)(bmp->memory+(MIN(bmp->w-1, cx)+(MIN(bmp->h-1, cy))*bmp->w)*4);
			d3d_pixel = (geta32(p) << 24) | (getr32(p) << 16) |
				(getg32(p) << 8) | getb32(p);
			*(uint32_t *)
				(locked_rect.pBits+(locked_rect.Pitch*cy)+(cx*4)) = d3d_pixel;
		}
	}

	/* Copy an extra column so the texture ends nicely */
	if (rect.right >= bmp->w) {
		cx = bmp->w;
		for (cy = y; cy <= y+height; cy++) {
			p = *(uint32_t *)(bmp->memory+(MIN(bmp->w-1, cx)+(MIN(bmp->h-1, cy))*bmp->w)*4);
			d3d_pixel = (geta32(p) << 24) | (getr32(p) << 16) |
				(getg32(p) << 8) | getb32(p);
			*(uint32_t *)
				(locked_rect.pBits+(locked_rect.Pitch*cy)+(cx*4)) = d3d_pixel;
		}
	}
	IDirect3DTexture8_UnlockRect(texture, 0);
   }
   else {
   	TRACE("d3d_copy_bitmap_data: Couldn't lock texture to upload.\n");
   }
}

static void d3d_do_upload(AL_BITMAP_D3D *d3d_bmp, int x, int y, int width,
	int height)
{
	d3d_copy_bitmap_data((AL_BITMAP *)d3d_bmp, d3d_bmp->system_texture,
		x, y, width, height);
		
	if (IDirect3DDevice8_UpdateTexture(_al_d3d_device,
			(IDirect3DBaseTexture8 *)d3d_bmp->system_texture,
			(IDirect3DBaseTexture8 *)d3d_bmp->video_texture) != D3D_OK) {
		TRACE("d3d_do_upload: Couldn't update texture.\n");
		return;
	}
}

/*
 * Release all default pool textures. This must be done before
 * resetting the device.
 */
void _al_d3d_release_default_pool_textures()
{
	unsigned int i;

	for (i = 0; i < created_bitmaps._size; i++) {
		AL_BITMAP_D3D **bptr = _al_vector_ref(&created_bitmaps, i);
		AL_BITMAP_D3D *bmp = *bptr;
		IDirect3DTexture8_Release(bmp->video_texture);
	}
}

static bool d3d_create_textures(int w, int h,
	LPDIRECT3DTEXTURE8 *video_texture, LPDIRECT3DTEXTURE8 *system_texture)
{
	if (video_texture) {
		if (IDirect3DDevice8_CreateTexture(_al_d3d_device, w, h, 1,
				D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
				video_texture) != D3D_OK) {
			TRACE("d3d_create_textures: Unable to create video texture.\n");
			return 1;
		}
	}

	if (system_texture) {
		if (IDirect3DDevice8_CreateTexture(_al_d3d_device, w, h, 1,
				0,/*D3DUSAGE_DYNAMIC,*/ D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
				system_texture) != D3D_OK) {
			TRACE("d3d_create_textures: Unable to create system texture.\n");
			if (video_texture) {
				IDirect3DTexture8_Release(*video_texture);
			}
			return 1;
		}
	}

	return 0;
}

/*
 * Refresh the texture memory. This must be done after a device is
 * lost or after it is reset.
 */
void _al_d3d_refresh_texture_memory()
{
	unsigned int i;

	for (i = 0; i < created_bitmaps._size; i++) {
		AL_BITMAP_D3D **bptr = _al_vector_ref(&created_bitmaps, i);
		AL_BITMAP_D3D *bmp = *bptr;
		AL_BITMAP *al_bmp = (AL_BITMAP *)bmp;
		d3d_create_textures(bmp->texture_w, bmp->texture_h,
			&bmp->video_texture, 0);
		d3d_copy_bitmap_data(al_bmp, bmp->system_texture,
			0, 0, al_bmp->w, al_bmp->h);
		IDirect3DDevice8_UpdateTexture(_al_d3d_device,
			(IDirect3DBaseTexture8 *)bmp->system_texture,
			(IDirect3DBaseTexture8 *)bmp->video_texture);
	}
}

// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
static void d3d_upload_bitmap(AL_BITMAP *bitmap, int x, int y,
	int width, int height)
{
	AL_BITMAP_D3D *d3d_bmp = (void *)bitmap;
	int w = bitmap->w;
	int h = bitmap->h;

	if (d3d_bmp->created != true) {
		d3d_bmp->texture_w = pot(w);
		d3d_bmp->texture_h = pot(h);
		if (d3d_bmp->video_texture == 0)
			d3d_create_textures(d3d_bmp->texture_w,
			d3d_bmp->texture_h,
			&d3d_bmp->video_texture,
			&d3d_bmp->system_texture);

		/*
		 * Keep track of created bitmaps, in case the display is lost
		 * or resized.
		 */
		*(AL_BITMAP_D3D **)_al_vector_alloc_back(&created_bitmaps) = d3d_bmp;

		d3d_bmp->created = true;
	}

	d3d_do_upload(d3d_bmp, x, y, width, height);
}

static AL_BITMAP *d3d_create_bitmap_from_surface(LPDIRECT3DSURFACE8 surface)
{
	D3DSURFACE_DESC desc;
	D3DLOCKED_RECT locked_rect;
	AL_BITMAP *bmp;
	unsigned int y;
	
	if (IDirect3DSurface8_GetDesc(surface, &desc) != D3D_OK) {
		TRACE("d3d_create_bitmap_from_surface: GetDesc failed.\n");
		return NULL;
	}

	if (IDirect3DSurface8_LockRect(surface, &locked_rect, 0, D3DLOCK_READONLY) != D3D_OK) {
		TRACE("d3d_create_bitmap_from_surface: LockRect failed.\n");
		return NULL;
	}

	bmp = _al_d3d_create_bitmap(0, desc.Width, desc.Height, 0);
	// FIXME support other pixel formats

	for (y = 0; y < desc.Height; y++) {
		memcpy(
			bmp->memory+(desc.Width*y*4),
			locked_rect.pBits+(locked_rect.Pitch*y),
			desc.Width*4
		);
	}

	IDirect3DSurface8_UnlockRect(surface);

	return bmp;
}

/* Draw the bitmap at the specified position. */
static void d3d_blit_real(int flag, AL_BITMAP *src,
	float sx, float sy, float sw, float sh,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest,
	float dx, float dy, float dw, float dh,
	float angle)
{
	AL_BITMAP_D3D *d3d_src = (AL_BITMAP_D3D *)src;
	AL_BITMAP_D3D *d3d_dest = (AL_BITMAP_D3D *)dest;
	D3DXVECTOR2 center;
	LPDIRECT3DSURFACE8 old_render_target;
	LPDIRECT3DSURFACE8 old_stencil_buffer;
	LPDIRECT3DSURFACE8 texture_render_target;
	float old_ortho_w;
	float old_ortho_h;
	DWORD light_color = 0xFFFFFF;
	static bool alpha_test = false; /* Used for masking */


	IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
	IDirect3DDevice8_SetVertexShader(_al_d3d_device, D3DFVF_TL_VERTEX);

	center.x = source_center_x;// * (dw/sw);
	center.y = source_center_y;// * (dh/sh);

	angle = -angle;


	if (flag & AL_MASK_SOURCE || flag & AL_MASK_INV_SOURCE) {
		AL_BITMAP *munged_bmp = d3d_munge_bitmap(flag, src, src->w, src->h, 0, 0, 0, src->w, src->h, 0);

		d3d_upload_bitmap(munged_bmp, 0, 0, src->w, src->h);

		flag = flag & ~(AL_MASK_SOURCE | AL_MASK_INV_SOURCE);

		alpha_test = true;
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, TRUE);
		if (flag & AL_MASK_SOURCE) {
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHAFUNC, D3DCMP_EQUAL);
		}
		else {
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
		}
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHAREF, 0);

		memcpy(&munged_bmp->light_color, &src->light_color, sizeof(AL_COLOR));

		d3d_blit_real(flag, munged_bmp, sx, sy, sw, sh,
			source_center_x, source_center_y,
			dest, dx, dy, dw, dh,
			angle);

		al_destroy_bitmap(munged_bmp);

		return;
	}
	else if (flag & AL_MASK_DEST || flag & AL_MASK_INV_DEST) {
		if (angle == 0.0f) {
			LPDIRECT3DSURFACE8 read_surface;
			D3DLOCKED_RECT locked_rect;
			void *pBits;
			D3DSURFACE_DESC desc;
			int rw;	// read buffer width
			int rh; // read buffer height
			float start_x, start_y;
			if (dest == 0) {
				LPDIRECT3DSURFACE8 render_target;
				if (IDirect3DDevice8_GetRenderTarget(_al_d3d_device, &render_target) != D3D_OK) {
					TRACE("d3d_blit_real: GetRenderTarget failed (MASK_DEST, no rotation).\n");
					return;
				}
				read_surface = render_target;
			}
			else {
				IDirect3DTexture8_GetSurfaceLevel(d3d_dest->system_texture, 0, &read_surface);
			}

			if (IDirect3DSurface8_GetDesc(read_surface, &desc) != D3D_OK) {
				TRACE("d3d_blit_real: GetDesc failed (MASK_DEST, no rotation).\n");
				IDirect3DSurface8_Release(read_surface);
				return;
			}

			rw = desc.Width;
			rh = desc.Height;

			if (IDirect3DSurface8_LockRect(read_surface, &locked_rect, 0, D3DLOCK_READONLY) != D3D_OK) {
				TRACE("d3d_blit_real: LockRect failed (MASK_DEST, no rotation).\n");
				IDirect3DSurface8_Release(read_surface);
				return;
			}
			if (alpha_test == true) {
				flag |= AL_BLEND;
			}
			AL_BITMAP *munged_bmp = d3d_munge_bitmap(flag, src, src->w, src->h, locked_rect.pBits, dx, dy, dw, dh, locked_rect.Pitch);
			IDirect3DSurface8_UnlockRect(read_surface);
			IDirect3DSurface8_Release(read_surface);
			d3d_upload_bitmap(munged_bmp, 0, 0, dw, dh);
			flag = flag & ~(AL_MASK_DEST | AL_MASK_INV_DEST);
			start_x = sx * (dw / sw);
			start_y = sy * (dh / sh);
			d3d_blit_real(flag|AL_BLEND,
				munged_bmp, start_x, start_y, dw, dh,
				source_center_x, source_center_y,
				dest, dx, dy, dw, dh,
				angle);
			al_destroy_bitmap(munged_bmp);
			if (dest != 0) {
				IDirect3DDevice8_UpdateTexture(_al_d3d_device,
					(IDirect3DBaseTexture8 *)d3d_dest->system_texture,
					(IDirect3DBaseTexture8 *)d3d_dest->video_texture);
			}
			return;
		}
		else {  /* Since we're rotating, use the stencil buffer */
			LPDIRECT3DSURFACE8 dest_surface;
			AL_BITMAP *dest_as_bmp;
			D3DXVECTOR2 mask_center;
			DWORD old_alphatest_value;

			if (dest == NULL) {
				if (IDirect3DDevice8_GetRenderTarget(_al_d3d_device, &dest_surface) != D3D_OK) {
					TRACE("d3d_blit_real: GetRenderTarget failed (MASK_DEST, rotated).\n");
					return;
				}

				dest_as_bmp = d3d_create_bitmap_from_surface(dest_surface);

				if (!dest_as_bmp) {
					IDirect3DSurface8_Release(dest_surface);
					return;
				}

				d3d_remove_mask_alpha(flag, dest_as_bmp);

				d3d_upload_bitmap(dest_as_bmp, 0, 0, dest_as_bmp->w, dest_as_bmp->h);
			}
			else {
				if (IDirect3DTexture8_GetSurfaceLevel(d3d_dest->system_texture, 0, &dest_surface) != D3D_OK) {
					TRACE("d3d_blit_real: GetSurfaceLevel failed (MASK_DEST, rotated).\n");
					return;
				}
				if (IDirect3DDevice8_GetRenderTarget(_al_d3d_device, &old_render_target) != D3D_OK) {
					TRACE("d3d_blit_real: GetRenderTarget failed (MASK_DEST, rotated).\n");
					IDirect3DSurface8_Release(dest_surface);
					return;
				}
				if (IDirect3DDevice8_GetDepthStencilSurface(_al_d3d_device, &old_stencil_buffer) != D3D_OK) {
					TRACE("d3d_blit_real: Unable to get current stencil buffer (MASK_DEST, rotated).\n");
					IDirect3DSurface8_Release(dest_surface);
					IDirect3DSurface8_Release(old_render_target);
					return;
				}
				if (IDirect3DDevice8_SetRenderTarget(_al_d3d_device, dest_surface, old_stencil_buffer) != D3D_OK) {
					TRACE("d3d_blit_real: Unable to set render target to texture surface (MASK_DEST, rotated).\n");
					IDirect3DSurface8_Release(dest_surface);
					IDirect3DSurface8_Release(old_render_target);
					IDirect3DSurface8_Release(old_stencil_buffer);
					return;
				}

				dest_as_bmp = d3d_create_bitmap_from_surface(dest_surface);

				if (!dest_as_bmp) {
					TRACE("d3d_blit_real: d3d_create_bitmap_from_surface failed (MASK_DEST, rotated).\n");
					IDirect3DSurface8_Release(dest_surface);
					IDirect3DSurface8_Release(old_render_target);
					IDirect3DSurface8_Release(old_stencil_buffer);
					return;
				}

				d3d_remove_mask_alpha(flag, dest_as_bmp);

				d3d_upload_bitmap(dest_as_bmp, 0, 0, dest_as_bmp->w, dest_as_bmp->h);

				_al_d3d_get_current_ortho_projection_parameters(&old_ortho_w, &old_ortho_h);
				_al_d3d_set_ortho_projection(
					((AL_BITMAP_D3D *)dest_as_bmp)->texture_w,
					((AL_BITMAP_D3D *)dest_as_bmp)->texture_h);
				IDirect3DDevice8_BeginScene(_al_d3d_device);
			}

			IDirect3DSurface8_Release(dest_surface);

			/* Clear the stencil buffer */
			IDirect3DDevice8_Clear(_al_d3d_device, 1, 0, D3DCLEAR_STENCIL, 0, 0.0f, 0);

			/* Set these states to write 1's to the stencil buffer */
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILENABLE, TRUE);
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILREF, 1);

			/* Save the old ALPHATEST state */
			IDirect3DDevice8_GetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, &old_alphatest_value);

			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, TRUE);

			/* Turn off drawing to the color buffer */
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, TRUE);
			if (flag & AL_MASK_DEST) {
				IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHAFUNC, D3DCMP_EQUAL);
			}
			else {
				IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
			}
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHAREF, 0);

			mask_center.x = dest_as_bmp->w / 2;
			mask_center.y = dest_as_bmp->h / 2;

			d3d_draw_textured_quad(0, (AL_BITMAP_D3D *)dest_as_bmp,
				0, 0, dest_as_bmp->w, dest_as_bmp->h,
				0, 0, dest_as_bmp->w, dest_as_bmp->h,
				&mask_center, 0.0f, 0xFFFFFF);

			/*
			 * Turn off blending, and set the stencil states
			 * to only accept pixels where there is a 1 in the
			 * stencil buffer. The stencil buffer will be disabled
			 * after drawing the image below.
			 */
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, old_alphatest_value);
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILENABLE, TRUE);
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILFUNC, D3DCMP_EQUAL);
			IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILREF, 1);
			
			al_destroy_bitmap(dest_as_bmp);

			if (dest != NULL) {
				IDirect3DDevice8_EndScene(_al_d3d_device);
				IDirect3DDevice8_SetRenderTarget(_al_d3d_device, old_render_target, old_stencil_buffer);
				IDirect3DSurface8_Release(old_render_target);
				IDirect3DSurface8_Release(old_stencil_buffer);
				IDirect3DDevice8_BeginScene(_al_d3d_device);
				_al_d3d_set_ortho_projection(old_ortho_w, old_ortho_h);
			}
		}
	}

	/* Drawing to something other than the display */
	if (dest != NULL) {
		IDirect3DDevice8_EndScene(_al_d3d_device);
		if (IDirect3DDevice8_GetRenderTarget(_al_d3d_device, &old_render_target) != D3D_OK) {
			TRACE("d3d_blit_real: Unable to get current render target.\n");
			return;
		}
		if (IDirect3DTexture8_GetSurfaceLevel(d3d_dest->video_texture, 0, &texture_render_target) != D3D_OK) {
			TRACE("d3d_blit_real: Unable to get texture surface level.\n");
			IDirect3DSurface8_Release(old_render_target);
			return;
		}
		if (IDirect3DDevice8_SetRenderTarget(_al_d3d_device, texture_render_target, 0) != D3D_OK) {
			TRACE("d3d_blit_real: Unable to set render target to texture surface.\n");
			IDirect3DSurface8_Release(old_render_target);
			IDirect3DSurface8_Release(texture_render_target);
			return;
		}
		_al_d3d_get_current_ortho_projection_parameters(&old_ortho_w, &old_ortho_h);
		_al_d3d_set_ortho_projection(d3d_dest->texture_w, d3d_dest->texture_h);
		IDirect3DDevice8_BeginScene(_al_d3d_device);
	}
	
	if (flag & AL_BLEND) {
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else if (flag & AL_LIT) {
		IDirect3DDevice8_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
		IDirect3DDevice8_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		IDirect3DDevice8_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		light_color = AL_COLOR_TO_D3D(src->light_color);
	}

	d3d_draw_textured_quad(flag, d3d_src,
		sx, sy, sw, sh,
		dx, dy, dw, dh,
		&center, angle, light_color);

	if (flag & AL_BLEND) {
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, FALSE);
	}
	else if (flag & AL_LIT) {
		IDirect3DDevice8_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	}

	if (((flag & AL_MASK_DEST) || (flag & AL_MASK_INV_DEST)) && angle != 0.0f) {
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_STENCILENABLE, FALSE);
	}

	if (alpha_test == true) {
		alpha_test = false;
		IDirect3DDevice8_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, FALSE);
	}

	/* Were drawing to something other than the display */
	if (dest != NULL) {
		LPDIRECT3DSURFACE8 system_texture_surface;
		RECT rect;
		POINT point;

		IDirect3DDevice8_EndScene(_al_d3d_device);

		if (IDirect3DTexture8_GetSurfaceLevel(d3d_dest->system_texture,
				0, &system_texture_surface) != D3D_OK) {
			TRACE("d3d_blit_real: GetSurfaceLevel failed while updating video texture.\n");
			return;
		}

		rect.left = 0;
		rect.top = 0;
		rect.right = d3d_dest->bitmap.w;
		rect.bottom = d3d_dest->bitmap.h;

		point.x = 0;
		point.y = 0;

		/* FIXME: the device can be lost here */
		if (IDirect3DDevice8_CopyRects(_al_d3d_device,
				texture_render_target,
				&rect, 1,
				system_texture_surface,
				&point) != D3D_OK) {
			TRACE("d3d_blit_real: CopyRects failed while updating video texture.\n");
			IDirect3DSurface8_Release(system_texture_surface);
		}

		IDirect3DTexture8_Release(system_texture_surface);
		IDirect3DTexture8_Release(texture_render_target);

		d3d_copy_texture_data(d3d_dest->system_texture, (AL_BITMAP *)d3d_dest);

		IDirect3DDevice8_SetRenderTarget(_al_d3d_device, old_render_target, 0);
		IDirect3DSurface8_Release(old_render_target);
		IDirect3DDevice8_BeginScene(_al_d3d_device);
		_al_d3d_set_ortho_projection(old_ortho_w, old_ortho_h);
	}
}

static void d3d_blit(int flag, AL_BITMAP *src,
	AL_BITMAP *dest, float dx, float dy)
{
	d3d_blit_real(flag,
		src, 0.0f, 0.0f, src->w, src->h,
		src->w/2, src->h/2,
		dest, dx, dy, src->w, src->h,
		0.0f);
}

static void d3d_blit_region(int flag, AL_BITMAP *src,
	float sx, float sy,
	AL_BITMAP *dest,
	float dx, float dy,
	float w, float h)
{
	d3d_blit_real(flag,
		src, sx, sy, w, h,
		0.0f, 0.0f,
		dest, dx, dy, w, h,
		0.0f);
}

static void d3d_blit_scaled(int flag,
	AL_BITMAP *src,	float sx, float sy, float sw, float sh,
	AL_BITMAP *dest, float dx, float dy, float dw, float dh)
{
	d3d_blit_real(flag,
		src, sx, sy, sw, sh, (sw-sx)/2, (sh-sy)/2,
		dest, dx, dy, dw, dh, 0.0f);
}

static void d3d_rotate_bitmap(int flag,
	AL_BITMAP *src,
	float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dx, float dy,
	float angle)
{
	d3d_blit_real(flag,
		src, 0.0f, 0.0f, src->w, src->h,
		source_center_x, source_center_y,
		dest, dx, dy, src->w, src->h,
		angle);
}

static void d3d_rotate_scaled(int flag,
	AL_BITMAP *src, float source_center_x, float source_center_y,
	AL_BITMAP *dest, float dest_x, float dest_y,
	float xscale, float yscale, float angle)
{
	d3d_blit_real(flag,
		src, 0.0f, 0.0f, src->w, src->h,
		source_center_x, source_center_y,
		dest, dest_x, dest_y, src->w*xscale, src->h*yscale,
		angle);
}

static void d3d_blit_region_3(int flag,
	AL_BITMAP *source1, int source1_x, int source1_y,
	AL_BITMAP *source2, int source2_x, int source2_y,
	AL_BITMAP *dest, int dest_x, int dest_y,
	int dest_w, int dest_h)
{
	d3d_blit_region(flag, source1, source1_x, source1_y,
		dest, dest_x, dest_y, dest_w, dest_h);
}

static void d3d_put_pixel(int flag, AL_BITMAP *bitmap, int x, int y,
	AL_COLOR *color)
{
	*(uint32_t *)(bitmap->memory+(y*bitmap->w+x)*4) = makeacol32(
		color->r*255, color->g*255, color->b*255, color->a*255);

	if (bitmap->locked == false) {
		d3d_do_upload((AL_BITMAP_D3D *)bitmap, x, y, 1, 1);
	}
}

static void d3d_download_bitmap(AL_BITMAP *bitmap)
{
}

static void d3d_destroy_bitmap(AL_BITMAP *bitmap)
{
   AL_BITMAP_D3D *d3d_bmp = (void *)bitmap;
   unsigned int i;

   if (d3d_bmp->video_texture) {
   	IDirect3DTexture8_Release(d3d_bmp->video_texture);
   }
   if (d3d_bmp->system_texture) {
   	IDirect3DTexture8_Release(d3d_bmp->system_texture);
   }
   for (i = 0; i < created_bitmaps._size; i++) {
   	AL_BITMAP_D3D **bmp = _al_vector_ref(&created_bitmaps, i);
	if (*bmp == d3d_bmp) {
		_al_vector_delete_at(&created_bitmaps, i);
		break;
	}
   }
}

/*
 * Upload a rectangle of a compatibility bitmap.
 */
static void d3d_upload_compat_bitmap(BITMAP *bitmap, int x, int y, int w, int h)
{
	AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap->al_bitmap;
	int cx;
	int cy;
	int pixel;
	AL_COLOR d3d_pixel;

	al_lock_bitmap((AL_BITMAP *)d3d_bmp, x, y, w, h);

	for (cy = y; cy < y+h; cy++) {
		if (bitmap_color_depth(bitmap) == 32) {
			for (cx = x; cx < x+w; cx++) {
				pixel = _getpixel32(bitmap, cx, cy);
				AL_PIXEL32_TO_COLOR(pixel, d3d_pixel);
				al_put_pixel(0, (AL_BITMAP *)d3d_bmp, cx, cy, &d3d_pixel);
			}
		}
		if (bitmap_color_depth(bitmap) == 24) {
			for (cx = x; cx < x+w; cx++) {
				pixel = _getpixel24(bitmap, cx, cy);
				AL_PIXEL24_TO_COLOR(pixel, d3d_pixel);
				al_put_pixel(0, (AL_BITMAP *)d3d_bmp, cx, cy, &d3d_pixel);
			}
		}
		if (bitmap_color_depth(bitmap) == 16) {
			for (cx = x; cx < x+w; cx++) {
				pixel = _getpixel16(bitmap, cx, cy);
				AL_PIXEL16_TO_COLOR(pixel, d3d_pixel);
				al_put_pixel(0, (AL_BITMAP *)d3d_bmp, cx, cy, &d3d_pixel);
			}
		}
		if (bitmap_color_depth(bitmap) == 15) {
			for (cx = x; cx < x+w; cx++) {
				pixel = _getpixel15(bitmap, cx, cy);
				AL_PIXEL15_TO_COLOR(pixel, d3d_pixel);
				al_put_pixel(0, (AL_BITMAP *)d3d_bmp, cx, cy, &d3d_pixel);
			}
		}
		if (bitmap_color_depth(bitmap) == 8) {
			for (cx = x; cx < x+w; cx++) {
				pixel = _getpixel(bitmap, cx, cy);
				AL_PIXEL8_TO_COLOR(pixel, d3d_pixel);
				al_put_pixel(0, (AL_BITMAP *)d3d_bmp, cx, cy, &d3d_pixel);
			}
		}
	}

	al_unlock_bitmap((AL_BITMAP *)d3d_bmp);

	if (d3d_bmp->is_screen) {
		al_blit(0, (AL_BITMAP *)bitmap->al_bitmap, 0, 0, 0);

		al_flip();
	}
}

static void d3d_make_compat_screen(AL_BITMAP *bitmap)
{
	AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;
	d3d_bmp->is_screen = true;
}

/* Obtain a reference to this driver. */
AL_BITMAP_INTERFACE *_al_bitmap_d3d_driver(int flag)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->blit = d3d_blit;
   vt->blit_region = d3d_blit_region;
   vt->blit_scaled = d3d_blit_scaled;
   vt->rotate_bitmap = d3d_rotate_bitmap;
   vt->rotate_scaled = d3d_rotate_scaled;
   vt->blit_region_3 = d3d_blit_region_3;
   vt->put_pixel = d3d_put_pixel;
   vt->upload_bitmap = d3d_upload_bitmap;
   vt->download_bitmap = d3d_download_bitmap;
   vt->destroy_bitmap = d3d_destroy_bitmap;
   vt->upload_compat_bitmap = d3d_upload_compat_bitmap;
   vt->make_compat_screen = d3d_make_compat_screen;

   return vt;
}
