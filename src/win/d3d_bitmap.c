/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * bitmap driver.
 */

/* FIXME: do we need AL_BITMAP->memory for display textures? */

#include <string.h>
#include <stdio.h>
#include <math.h>

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
	/* FIXME: set display */
	AL_BITMAP *ret = (AL_BITMAP *)_al_d3d_create_bitmap(0, dw, dh);
	AL_BITMAP *read_bmp;
	BITMAP *temp_src;
	BITMAP *temp_dest;
	bool scaled = false;
	int x;
	int y;

	/* Hack so we don't have to rewrite stretch_blit for AL_BITMAP */
	
	if (sw != dw || sh != dh) {
		int end_sh = (int)sh;
		int end_dh = (int)dh;
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
		/*
		if (flag & AL_BLEND) {
			alpha_mask = 0;
			pixel_mask = 0xFFFFFFFF;
		}
		else {
			alpha_mask = 0xFF000000;
			pixel_mask = 0xFFFFFF;
		}
		*/
		alpha_mask = 0;
		pixel_mask = 0xFFFFFFFF;
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
#if 0
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
#endif

	return ret;
}


#if 0
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
#endif


#if 0
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
#endif

static void d3d_allegro_matrix_to_d3d(MATRIX_f *alleg_matrix,
	D3DMATRIX *d3d_matrix)
{
	d3d_matrix->m[0][0] = alleg_matrix->v[0][0];
	d3d_matrix->m[1][0] = alleg_matrix->v[1][0];
	d3d_matrix->m[2][0] = alleg_matrix->v[2][0];
	d3d_matrix->m[0][1] = alleg_matrix->v[0][1];
	d3d_matrix->m[1][1] = alleg_matrix->v[1][1];
	d3d_matrix->m[2][1] = alleg_matrix->v[2][1];
	d3d_matrix->m[0][2] = alleg_matrix->v[0][2];
	d3d_matrix->m[1][2] = alleg_matrix->v[1][2];
	d3d_matrix->m[2][2] = alleg_matrix->v[2][2];

	d3d_matrix->m[3][0] = alleg_matrix->t[0];
	d3d_matrix->m[3][1] = alleg_matrix->t[1];
	d3d_matrix->m[3][2] = alleg_matrix->t[2];

	d3d_matrix->m[0][3] = 0.0f;
	d3d_matrix->m[1][3] = 0.0f;
	d3d_matrix->m[2][3] = 0.0f;
	d3d_matrix->m[3][3] = 1.0f;
}

/*
 * Do a transformation for a quad drawing.
 */
static void d3d_transform(D3D_TL_VERTEX vertices[],
	float cx, float cy, float dx, float dy, 
	float angle)
{
	MATRIX_f center_matrix;
	MATRIX_f rotation_matrix;
	MATRIX_f dest_matrix;
	MATRIX_f verts_matrix;
	MATRIX_f tmp1, tmp2;
	MATRIX_f result;
	//D3DMATRIX d3d_matrix;
	int i;

	get_translation_matrix_f(&center_matrix, -cx, -cy, 0.0f);
	get_z_rotate_matrix_f(&rotation_matrix, angle*256.0f/(M_PI*2));
	get_translation_matrix_f(&dest_matrix, dx, dy, 0.0f);

	for (i = 0; i < 3; i++)	{
		verts_matrix.v[i][0] = vertices[i].x;
		verts_matrix.v[i][1] = vertices[i].y;
		verts_matrix.v[i][2] = vertices[i].z;
	}
	verts_matrix.t[0] = vertices[i].x;
	verts_matrix.t[1] = vertices[i].y;
	verts_matrix.t[2] = vertices[i].z;

	matrix_mul_f(&center_matrix, &rotation_matrix, &tmp1);
	matrix_mul_f(&tmp1, &dest_matrix, &tmp2);
	matrix_mul_f(&tmp2, &verts_matrix, &result);

	for (i = 0; i < 3; i++) {
		vertices[i].x = result.v[i][0];
		vertices[i].y = result.v[i][1];
		vertices[i].z = result.v[i][2];
	}
	vertices[i].x = result.t[0];
	vertices[i].y = result.t[1];
	vertices[i].z = result.t[2];

	//d3d_allegro_matrix_to_d3d(&result3, &d3d_matrix);

	//IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_VIEW, &d3d_matrix);
}

/*
 * Draw a textured quad (or filled rectangle)
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
void _al_d3d_draw_textured_quad(AL_BITMAP_D3D *bmp,
	float sx, float sy, float sw, float sh,
	float dx, float dy, float dw, float dh,
	float cx, float cy, float angle,
	D3DCOLOR color, int flags)
{
	float right;
	float bottom;
	float tu_start;
	float tv_start;
	float tu_end;
	float tv_end;
	int texture_w;
	int texture_h;
	float dest_x;
	float dest_y;

	const float z = 0.0f;

	right  = dx + dw;
	bottom = dy + dh;

	if (bmp) {
		texture_w = bmp->texture_w;
		texture_h = bmp->texture_h;
	}
	else {
		texture_w = 0;
		texture_h = 0;
	}

	tu_start = (sx+0.5f) / texture_w;
	tv_start = (sy+0.5f) / texture_h;
	tu_end = sw / texture_w + tu_start;
	tv_end = sh / texture_h + tv_start;

	if (flags & AL_FLIP_HORIZONTAL) {
		float temp = tu_start;
		tu_start = tu_end;
		tu_end = temp;
		/* Weird hack -- not sure why this is needed */
		tu_start -= 1.0f / texture_w;
		tu_end -= 1.0f / texture_w;
	}
	if (flags & AL_FLIP_VERTICAL) {
		float temp = tv_start;
		tv_start = tv_end;
		tv_end = temp;
		/* Weird hack -- not sure why this is needed */
		tv_start -= 1.0f / texture_h;
		tv_end -= 1.0f / texture_h;
	}

	D3D_TL_VERTEX vertices[4] = {
		/* x,    y,      z, color, tu,        tv     */
		{ dx,    dy,     z, color, tu_start,  tv_start },
		{ right, dy,     z, color, tu_end,    tv_start },
		{ right, bottom, z, color, tu_end,    tv_end   },
		{ dx,    bottom, z, color, tu_start,  tv_end   }
	};

	if (angle == 0.0f) {
		cx = dx + cx;
		cy = dy + cy;
	}
	else {
		cx = dx + cx * (dw / sw);
		cy = dy + cy * (dh / sh);
	}

	if (angle == 0.0f) {
		dest_x = cx;
		dest_y = cy;
	}
	else {
		dest_x = dx;
		dest_y = dy;
	}

	d3d_transform(vertices, cx, cy, dest_x, dest_y, angle);
	//d3d_transform_vertices(vertices, center, &dest, angle);

	if (bmp) {
		if (IDirect3DDevice9_SetTexture(_al_d3d_device, 0,
				(IDirect3DBaseTexture9 *)bmp->video_texture) != D3D_OK) {
			TRACE("_al_d3d_draw_textured_quad: SetTexture failed.\n");
			return;
		}
	}
	else {
		IDirect3DDevice9_SetTexture(_al_d3d_device, 0, NULL);
	}

/*
	IDirect3DDevice9_SetTextureStageState(_al_d3d_device,
		0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	
	IDirect3DDevice9_SetTextureStageState(_al_d3d_device,
		0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		*/
	
	if (IDirect3DDevice9_DrawPrimitiveUP(_al_d3d_device, D3DPT_TRIANGLEFAN, 2,
			vertices, sizeof(D3D_TL_VERTEX)) != D3D_OK) {
		TRACE("_al_d3d_draw_textured_quad: DrawPrimitive failed.\n");
		return;
	}


	IDirect3DDevice9_SetTexture(_al_d3d_device, 0, NULL);
}

static void d3d_sync_bitmap_memory(AL_BITMAP *bitmap)
{
   D3DLOCKED_RECT locked_rect;
   AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;

   if (IDirect3DTexture9_LockRect(d3d_bmp->system_texture, 0, &locked_rect, NULL, 0) == D3D_OK) {
	_al_convert_bitmap_data(locked_rect.pBits, bitmap->format, locked_rect.Pitch,
		bitmap->memory, bitmap->format, _al_pixel_size(bitmap->format)*bitmap->w,
		0, 0, 0, 0, bitmap->w, bitmap->h);
	IDirect3DTexture9_UnlockRect(d3d_bmp->system_texture, 0);
   }
   else {
   	TRACE("d3d_sync_bitmap_memory: Couldn't lock texture.\n");
   }
}

static void d3d_sync_bitmap_texture(AL_BITMAP *bitmap,
	unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
   D3DLOCKED_RECT locked_rect;
   RECT rect;
   AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;

   rect.left = x;
   rect.top = y;
   rect.right = x + width;
   rect.bottom = y + height;

   if (rect.right != pot(x + width))
   	rect.right++;
   if (rect.bottom != pot(y + height))
   	rect.bottom++;

   if (IDirect3DTexture9_LockRect(d3d_bmp->system_texture, 0, &locked_rect, &rect, 0) == D3D_OK) {
	_al_convert_bitmap_data(bitmap->memory, bitmap->format, bitmap->w*_al_pixel_size(bitmap->format),
		locked_rect.pBits, bitmap->format, locked_rect.Pitch,
		x, y, 0, 0, width, height);
	/* Copy an extra row and column so the texture ends nicely */
	if (rect.bottom > bitmap->h) {
		_al_convert_bitmap_data(
			bitmap->memory,
			bitmap->format, bitmap->w*_al_pixel_size(bitmap->format),
			locked_rect.pBits,
			bitmap->format, locked_rect.Pitch,
			0, bitmap->h-1,
			0, height,
			width, 1);
	}
	if (rect.right > bitmap->w) {
		_al_convert_bitmap_data(
			bitmap->memory,
			bitmap->format, bitmap->w*_al_pixel_size(bitmap->format),
			locked_rect.pBits,
			bitmap->format, locked_rect.Pitch,
			bitmap->w-1, 0,
			width, 0,
			1, height);
	}
	if (rect.bottom > bitmap->h && rect.right > bitmap->w) {
		_al_convert_bitmap_data(
			bitmap->memory,
			bitmap->format, bitmap->w*_al_pixel_size(bitmap->format),
			locked_rect.pBits,
			bitmap->format, locked_rect.Pitch,
			bitmap->w-1, bitmap->h-1,
			width, height,
			1, 1);
	}
	IDirect3DTexture9_UnlockRect(d3d_bmp->system_texture, 0);
   }
   else {
   	TRACE("d3d_sync_bitmap_texture: Couldn't lock texture to upload.\n");
   }
}

static void d3d_do_upload(AL_BITMAP_D3D *d3d_bmp, int x, int y, int width,
	int height, bool sync_from_memory)
{
	if (sync_from_memory) {
		d3d_sync_bitmap_texture((AL_BITMAP *)d3d_bmp,
			x, y, width, height);
	}
		
	if (IDirect3DDevice9_UpdateTexture(_al_d3d_device,
			(IDirect3DBaseTexture9 *)d3d_bmp->system_texture,
			(IDirect3DBaseTexture9 *)d3d_bmp->video_texture) != D3D_OK) {
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
		IDirect3DTexture9_Release(bmp->video_texture);
	}
}

static bool d3d_create_textures(int w, int h,
	LPDIRECT3DTEXTURE9 *video_texture, LPDIRECT3DTEXTURE9 *system_texture,
	int format)
{
	if (video_texture) {
		if (IDirect3DDevice9_CreateTexture(_al_d3d_device, w, h, 1,
				D3DUSAGE_RENDERTARGET, _al_pixel_format_to_d3d(format), D3DPOOL_DEFAULT,
				video_texture, NULL) != D3D_OK) {
			TRACE("d3d_create_textures: Unable to create video texture.\n");
			return 1;
		}
	}

	if (system_texture) {
		if (IDirect3DDevice9_CreateTexture(_al_d3d_device, w, h, 1,
				0,/*D3DUSAGE_DYNAMIC,*/ _al_pixel_format_to_d3d(format), D3DPOOL_SYSTEMMEM,
				system_texture, NULL) != D3D_OK) {
			TRACE("d3d_create_textures: Unable to create system texture.\n");
			if (video_texture) {
				IDirect3DTexture9_Release(*video_texture);
			}
			return 1;
		}
	}

	return 0;
}

/*
 * Must be called before the D3D device is reset (e.g., when
 * resizing a window). All non-synced display bitmaps must be
 * synced to memory.
 */
void _al_d3d_prepare_bitmaps_for_reset()
{
	unsigned int i;

	for (i = 0; i < created_bitmaps._size; i++) {
		AL_BITMAP_D3D **bptr = _al_vector_ref(&created_bitmaps, i);
		AL_BITMAP_D3D *bmp = *bptr;
		AL_BITMAP *al_bmp = (AL_BITMAP *)bmp;
		if (!(al_bmp->flags & AL_SYNC_MEMORY_COPY) || !(al_bmp->flags & AL_MEMORY_BITMAP)) {
			d3d_sync_bitmap_memory(al_bmp);
		}
	}
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
			&bmp->video_texture, 0, al_bmp->format);
		if (al_bmp->flags & AL_SYNC_MEMORY_COPY) {
			d3d_sync_bitmap_texture(al_bmp,
				0, 0, al_bmp->w, al_bmp->h);
			IDirect3DDevice9_UpdateTexture(_al_d3d_device,
				(IDirect3DBaseTexture9 *)bmp->system_texture,
				(IDirect3DBaseTexture9 *)bmp->video_texture);
		}
	}
}

// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
/* FIXME: maybe this should return success/failure */
static void d3d_upload_bitmap(AL_BITMAP *bitmap, int x, int y,
	int width, int height)
{
	AL_BITMAP_D3D *d3d_bmp = (void *)bitmap;
	int w = bitmap->w;
	int h = bitmap->h;

	if (_al_d3d_is_device_lost()) return;
	_al_d3d_lock_device();

	if (d3d_bmp->created != true) {
		d3d_bmp->texture_w = pot(w);
		d3d_bmp->texture_h = pot(h);
		if (d3d_bmp->video_texture == 0)
			d3d_create_textures(d3d_bmp->texture_w,
			d3d_bmp->texture_h,
			&d3d_bmp->video_texture,
			&d3d_bmp->system_texture,
			bitmap->format);

		/*
		 * Keep track of created bitmaps, in case the display is lost
		 * or resized.
		 */
		*(AL_BITMAP_D3D **)_al_vector_alloc_back(&created_bitmaps) = d3d_bmp;

		d3d_bmp->created = true;
	}

	_al_d3d_unlock_device();

	d3d_do_upload(d3d_bmp, x, y, width, height, true);
}

static AL_BITMAP *d3d_create_bitmap_from_surface(LPDIRECT3DSURFACE9 surface)
{
	D3DSURFACE_DESC desc;
	D3DLOCKED_RECT locked_rect;
	AL_BITMAP *bmp;
	unsigned int y;
	
	if (IDirect3DSurface9_GetDesc(surface, &desc) != D3D_OK) {
		TRACE("d3d_create_bitmap_from_surface: GetDesc failed.\n");
		return NULL;
	}

	if (IDirect3DSurface9_LockRect(surface, &locked_rect, 0, D3DLOCK_READONLY) != D3D_OK) {
		TRACE("d3d_create_bitmap_from_surface: LockRect failed.\n");
		return NULL;
	}

	bmp = _al_d3d_create_bitmap(0, desc.Width, desc.Height);
	// FIXME support other pixel formats

	for (y = 0; y < desc.Height; y++) {
		memcpy(
			bmp->memory+(desc.Width*y*4),
			locked_rect.pBits+(locked_rect.Pitch*y),
			desc.Width*4
		);
	}

	IDirect3DSurface9_UnlockRect(surface);

	return bmp;
}

/* Draw the bitmap at the specified position. */
static void d3d_blit_real(AL_BITMAP *src,
	float sx, float sy, float sw, float sh,
	float source_center_x, float source_center_y,
	float dx, float dy, float dw, float dh,
	float angle, int flags)
{
	AL_BITMAP_D3D *d3d_src = (AL_BITMAP_D3D *)src;
	AL_BITMAP *dest = al_get_target_bitmap();
	AL_BITMAP_D3D *d3d_dest = (AL_BITMAP_D3D *)al_get_target_bitmap();
	float old_ortho_w;
	float old_ortho_h;
	DWORD light_color = 0xFFFFFF;
	static bool alpha_test = false; /* Used for masking */

	if (_al_d3d_is_device_lost()) return;

	IDirect3DDevice9_SetFVF(_al_d3d_device, D3DFVF_TL_VERTEX);

	angle = -angle;


	if (flags & AL_MASK_SOURCE) {
		AL_BITMAP *munged_bmp = d3d_munge_bitmap(flags, src, src->w, src->h, 0, 0, 0, src->w, src->h, 0);

		d3d_upload_bitmap(munged_bmp, 0, 0, src->w, src->h);

		flags = flags & ~(AL_MASK_SOURCE);

		alpha_test = true;
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, TRUE);
		if (flags & AL_MASK_SOURCE) {
			IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHAFUNC, D3DCMP_EQUAL);
		}
		else {
			IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
		}
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHAREF, 0);

		memcpy(&munged_bmp->light_color, &src->light_color, sizeof(AL_COLOR));

		d3d_blit_real(munged_bmp, sx, sy, sw, sh,
			source_center_x, source_center_y,
			dx, dy, dw, dh,
			angle, flags);

		al_destroy_bitmap(munged_bmp);

		return;
	}

	_al_d3d_lock_device();

	/*
	if (flag & AL_BLEND) {
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else if (flag & AL_LIT) {
		IDirect3DDevice9_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLOROP, D3DTOP_BLENDDIFFUSEALPHA);
		IDirect3DDevice9_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		IDirect3DDevice9_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		light_color = AL_COLOR_TO_D3D(src->light_color);
	}
	*/

	_al_d3d_draw_textured_quad(d3d_src,
		sx, sy, sw, sh,
		dx, dy, dw, dh,
		source_center_x, source_center_y,
		angle, light_color,
		flags);

	/*
	if (flag & AL_BLEND) {
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, FALSE);
	}
	else if (flag & AL_LIT) {
		IDirect3DDevice9_SetTextureStageState(_al_d3d_device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	}
	*/

#if 0
	if (((flag & AL_MASK_DEST) || (flag & AL_MASK_INV_DEST)) && angle != 0.0f) {
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_STENCILENABLE, FALSE);
	}
#endif

	if (alpha_test == true) {
		alpha_test = false;
		IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHATESTENABLE, FALSE);
	}

	_al_d3d_unlock_device();

	/* 
	 * If we're rendering to a texture, and the bitmap has the
	 * AL_SYNC_MEMORY_COPY flag, sync the texture to system memory.
	 */
	if (!d3d_dest->is_frontbuffer && !d3d_dest->is_backbuffer &&
			(dest->flags & AL_SYNC_MEMORY_COPY)) {
		LPDIRECT3DSURFACE9 system_texture_surface;
		LPDIRECT3DSURFACE9 video_texture_surface;
		RECT rect;
		POINT point;
	
		_al_d3d_lock_device();

		IDirect3DDevice9_EndScene(_al_d3d_device);

		if (IDirect3DTexture9_GetSurfaceLevel(d3d_dest->system_texture,
				0, &system_texture_surface) != D3D_OK) {
			TRACE("d3d_blit_real: GetSurfaceLevel failed while updating video texture.\n");
			_al_d3d_unlock_device();
			return;
		}

		if (IDirect3DTexture9_GetSurfaceLevel(d3d_dest->video_texture,
				0, &video_texture_surface) != D3D_OK) {
			TRACE("d3d_blit_real: GetSurfaceLevel failed while updating video texture.\n");
			_al_d3d_unlock_device();
			return;
		}

		rect.left = 0;
		rect.top = 0;
		rect.right = d3d_dest->bitmap.w;
		rect.bottom = d3d_dest->bitmap.h;

		point.x = 0;
		point.y = 0;

		if (IDirect3DDevice9_GetRenderTargetData(_al_d3d_device,
				video_texture_surface,
				system_texture_surface) != D3D_OK) {
			TRACE("d3d_blit_real: GetRenderTargetData failed.\n");
			_al_d3d_unlock_device();
			return;
		}

		d3d_sync_bitmap_memory(dest);

		/*
		d3d_copy_surface_to_texture(system_texture_surface,
			d3d_dest->system_texture, dest->w,
			dest->h);
			*/

		/* FIXME: the device can be lost here */
		/*
		if (IDirect3DDevice9_UpdateSurface(_al_d3d_device,
				video_texture_surface,
				&rect,
				system_texture_surface,
				&point) != D3D_OK) {
			TRACE("d3d_blit_real: UpdateSurface failed while updating video texture.\n");
			IDirect3DSurface9_Release(system_texture_surface);
			IDirect3DSurface9_Release(video_texture_surface);
			return;
		}
		*/

		IDirect3DSurface9_Release(system_texture_surface);
		IDirect3DSurface9_Release(video_texture_surface);
		
		IDirect3DDevice9_BeginScene(_al_d3d_device);

		d3d_sync_bitmap_memory(dest);

		_al_d3d_unlock_device();
	}
}

static void d3d_draw_bitmap(AL_BITMAP *bitmap, float x, float y, int flags)
{
	d3d_blit_real(bitmap, 0.0f, 0.0f, bitmap->w, bitmap->h,
		bitmap->w/2, bitmap->h/2,
		x, y, bitmap->w, bitmap->h,
		0.0f, flags);
}

#if 0
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
#endif

static void d3d_destroy_bitmap(AL_BITMAP *bitmap)
{
   AL_BITMAP_D3D *d3d_bmp = (void *)bitmap;

   if (d3d_bmp->video_texture) {
   	IDirect3DTexture9_Release(d3d_bmp->video_texture);
   }
   if (d3d_bmp->system_texture) {
   	IDirect3DTexture9_Release(d3d_bmp->system_texture);
   }

/*
   if (d3d_bmp->is_screen && !d3d_bmp->is_sub_bitmap) {
   	al_destroy_display(_al_current_display);
	_al_current_display = NULL;
   }
   */

   _al_d3d_delete_from_vector(&created_bitmaps, d3d_bmp);
}

static AL_LOCKED_RECTANGLE *d3d_lock_region(AL_BITMAP *bitmap,
	int x, int y, int w, int h, AL_LOCKED_RECTANGLE *locked_rectangle,
	int flags)
{
	AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;
	RECT rect;

	rect.left = x;
	rect.right = x + w;   /* FIXME: add 1 ? */
	rect.top = y;
	rect.bottom = y + h;  /* FIXME: add 1 ? */

	if (IDirect3DTexture9_LockRect(d3d_bmp->system_texture, 0, &d3d_bmp->locked_rect, &rect, 0) != D3D_OK) {
		return NULL;
	}

	locked_rectangle->data = d3d_bmp->locked_rect.pBits;
	locked_rectangle->format = bitmap->format;
	locked_rectangle->pitch = d3d_bmp->locked_rect.Pitch;

	return locked_rectangle;
}

static void d3d_unlock_region(AL_BITMAP *bitmap)
{
	AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;
	bool sync;

	IDirect3DTexture9_UnlockRect(d3d_bmp->system_texture, 0);

	if (bitmap->lock_flags & AL_LOCK_READONLY)
		return;

	if (bitmap->flags & AL_SYNC_MEMORY_COPY)
		sync = true;
	else
		sync = false;

	d3d_do_upload(d3d_bmp, bitmap->lock_x, bitmap->lock_y,
		bitmap->lock_width, bitmap->lock_height, sync);
}

/*
static void d3d_make_compat_screen(AL_BITMAP *bitmap)
{
	AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;
	d3d_bmp->is_screen = true;
	gfx_driver = &d3d_dummy_gfx_driver;
	gfx_driver->w = bitmap->w;
	gfx_driver->h = bitmap->h;
}
*/

/* Obtain a reference to this driver. */
AL_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->draw_bitmap = d3d_draw_bitmap;
/*
   vt->blit = d3d_blit;
   vt->blit_region = d3d_blit_region;
   vt->blit_scaled = d3d_blit_scaled;
   vt->rotate_bitmap = d3d_rotate_bitmap;
   vt->rotate_scaled = d3d_rotate_scaled;
   vt->blit_region_3 = d3d_blit_region_3;
   */
   vt->upload_bitmap = d3d_upload_bitmap;
   //vt->download_bitmap = d3d_download_bitmap;
   vt->destroy_bitmap = d3d_destroy_bitmap;
   vt->lock_region = d3d_lock_region;
   vt->unlock_region = d3d_unlock_region;
   //vt->upload_compat_bitmap = d3d_upload_compat_bitmap;
   //vt->make_compat_screen = d3d_make_compat_screen;

   return vt;
}
