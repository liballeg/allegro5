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
 *      Direct3D bitmap driver
 *
 *      By Trent Gamblin.
 *
 */

#include <string.h>
#include <stdio.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/system.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memblit.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/platform/aintwin.h"

#include "d3d.h"

extern "C" {

ALLEGRO_DEBUG_CHANNEL("d3d")


static ALLEGRO_BITMAP_INTERFACE *vt;
static _AL_VECTOR created_bitmaps;


void _al_d3d_bmp_init(void)
{
   _al_vector_init(&created_bitmaps, sizeof(ALLEGRO_BITMAP_D3D *));
}


void _al_d3d_bmp_destroy(void)
{
   while (!_al_vector_is_empty(&created_bitmaps))
      _al_vector_delete_at(&created_bitmaps, _al_vector_size(&created_bitmaps)-1);
   _al_vector_free(&created_bitmaps);
   _al_vector_init(&created_bitmaps, sizeof(ALLEGRO_SYSTEM_INTERFACE *));

   al_free(vt);
   vt = NULL;
}

static INLINE void transform_vertex(float* x, float* y)
{
   al_transform_coordinates(al_get_current_transform(), x, y);
}

/*
 * Draw a textured quad
 */
static void d3d_draw_textured_quad(
   ALLEGRO_DISPLAY_D3D *disp, ALLEGRO_BITMAP_D3D *bmp, ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, int flags)
{
   float right;
   float bottom;
   float tu_start;
   float tv_start;
   float tu_end;
   float tv_end;
   int texture_w;
   int texture_h;
   DWORD d3d_color;

   const float z = 0.0f;
   
   ALLEGRO_DISPLAY* aldisp = (ALLEGRO_DISPLAY*)disp;

   if (aldisp->num_cache_vertices != 0 && (uintptr_t)bmp != aldisp->cache_texture) {
      aldisp->vt->flush_vertex_cache(aldisp);
   }
   aldisp->cache_texture = (uintptr_t)bmp;

   D3D_TL_VERTEX* vertices = (D3D_TL_VERTEX*)aldisp->vt->prepare_vertex_cache(aldisp, 6);

   right  = sw;
   bottom = sh;

   texture_w = bmp->texture_w;
   texture_h = bmp->texture_h;
   tu_start = sx / texture_w;
   tv_start = sy / texture_h;
   tu_end = sw / texture_w + tu_start;
   tv_end = sh / texture_h + tv_start;

   if (flags & ALLEGRO_FLIP_HORIZONTAL) {
      float temp = tu_start;
      tu_start = tu_end;
      tu_end = temp;
   }
   if (flags & ALLEGRO_FLIP_VERTICAL) {
      float temp = tv_start;
      tv_start = tv_end;
      tv_end = temp;
   }

   d3d_color = D3DCOLOR_COLORVALUE(tint.r, tint.g, tint.b, tint.a);

   vertices[0].x = 0;
   vertices[0].y = 0;
   vertices[0].z = z;
   vertices[0].diffuse = d3d_color;
   vertices[0].tu = tu_start;
   vertices[0].tv = tv_start;

   vertices[1].x = right;
   vertices[1].y = 0;
   vertices[1].z = z;
   vertices[1].diffuse = d3d_color;
   vertices[1].tu = tu_end;
   vertices[1].tv = tv_start;

   vertices[2].x = right;
   vertices[2].y = bottom;
   vertices[2].z = z;
   vertices[2].diffuse = d3d_color;
   vertices[2].tu = tu_end;
   vertices[2].tv = tv_end;

   vertices[5].x = 0;
   vertices[5].y = bottom;
   vertices[5].z = z;
   vertices[5].diffuse = d3d_color;
   vertices[5].tu = tu_start;
   vertices[5].tv = tv_end;

   if (aldisp->cache_enabled) {
      transform_vertex(&vertices[0].x, &vertices[0].y);
      transform_vertex(&vertices[1].x, &vertices[1].y);
      transform_vertex(&vertices[2].x, &vertices[2].y);
      transform_vertex(&vertices[5].x, &vertices[5].y);
   }
   
   vertices[3] = vertices[0];
   vertices[4] = vertices[2];

   if(!aldisp->cache_enabled)
      aldisp->vt->flush_vertex_cache(aldisp);
}

/* Copy texture memory to bitmap->memory */
static void d3d_sync_bitmap_memory(ALLEGRO_BITMAP *bitmap)
{
   D3DLOCKED_RECT locked_rect;
   ALLEGRO_BITMAP_D3D *d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;
   LPDIRECT3DTEXTURE9 texture;

   if (_al_d3d_render_to_texture_supported())
      texture = d3d_bmp->system_texture;
   else
      texture = d3d_bmp->video_texture;

   if (texture->LockRect(0, &locked_rect, NULL, 0) == D3D_OK) {
      _al_convert_bitmap_data(locked_rect.pBits, bitmap->format, locked_rect.Pitch,
         bitmap->memory, bitmap->format, al_get_pixel_size(bitmap->format)*bitmap->w,
         0, 0, 0, 0, bitmap->w, bitmap->h);
      texture->UnlockRect(0);
   }
   else {
      ALLEGRO_ERROR("d3d_sync_bitmap_memory: Couldn't lock texture.\n");
   }
}

/* Copy bitmap->memory to texture memory */
static void d3d_sync_bitmap_texture(ALLEGRO_BITMAP *bitmap,
   int x, int y, int width, int height)
{
   D3DLOCKED_RECT locked_rect;
   RECT rect;
   ALLEGRO_BITMAP_D3D *d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;
   LPDIRECT3DTEXTURE9 texture;

   rect.left = x;
   rect.top = y;
   rect.right = x + width;
   rect.bottom = y + height;

   if (_al_d3d_render_to_texture_supported())
      texture = d3d_bmp->system_texture;
   else
      texture = d3d_bmp->video_texture;

   if (texture->LockRect(0, &locked_rect, &rect, 0) == D3D_OK) {
	   _al_convert_bitmap_data(bitmap->memory, bitmap->format, bitmap->w*al_get_pixel_size(bitmap->format),
		  locked_rect.pBits, bitmap->format, locked_rect.Pitch,
		  x, y, 0, 0, width, height);
	   /* Copy an extra row and column so the texture ends nicely */
	   if (rect.bottom > bitmap->h) {
		  _al_convert_bitmap_data(
			 bitmap->memory,
			 bitmap->format, bitmap->w*al_get_pixel_size(bitmap->format),
			 locked_rect.pBits,
			 bitmap->format, locked_rect.Pitch,
			 0, bitmap->h-1,
			 0, height,
			 width, 1);
	   }
	   if (rect.right > bitmap->w) {
		  _al_convert_bitmap_data(
			 bitmap->memory,
			 bitmap->format, bitmap->w*al_get_pixel_size(bitmap->format),
			 locked_rect.pBits,
			 bitmap->format, locked_rect.Pitch,
			 bitmap->w-1, 0,
			 width, 0,
			 1, height);
	   }
	   if (rect.bottom > bitmap->h && rect.right > bitmap->w) {
		  _al_convert_bitmap_data(
			 bitmap->memory,
			 bitmap->format, bitmap->w*al_get_pixel_size(bitmap->format),
			 locked_rect.pBits,
			 bitmap->format, locked_rect.Pitch,
			 bitmap->w-1, bitmap->h-1,
			 width, height,
			 1, 1);
	   }
	   texture->UnlockRect(0);
   }
   else {
      ALLEGRO_ERROR("d3d_sync_bitmap_texture: Couldn't lock texture to upload.\n");
   }
}

static void d3d_do_upload(ALLEGRO_BITMAP_D3D *d3d_bmp, int x, int y, int width,
   int height, bool sync_from_memory)
{
   ALLEGRO_BITMAP *bmp = (ALLEGRO_BITMAP *)d3d_bmp;

   if (sync_from_memory) {
      d3d_sync_bitmap_texture(bmp, x, y, width, height);
   }

   if (_al_d3d_render_to_texture_supported()) {
      if (d3d_bmp->display->device->UpdateTexture(
            (IDirect3DBaseTexture9 *)d3d_bmp->system_texture,
            (IDirect3DBaseTexture9 *)d3d_bmp->video_texture) != D3D_OK) {
         ALLEGRO_ERROR("d3d_do_upload: Couldn't update texture.\n");
         return;
      }
   }
}

/*
 * Release all default pool textures. This must be done before
 * resetting the device.
 */
void _al_d3d_release_default_pool_textures(void)
{
   unsigned int i;

   for (i = 0; i < created_bitmaps._size; i++) {
   	ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&created_bitmaps, i);
	ALLEGRO_BITMAP *albmp = *bptr;
	ALLEGRO_BITMAP_D3D *d3d_bmp;
	if ((albmp->flags & ALLEGRO_MEMORY_BITMAP) || (albmp->parent))
	   continue;
	d3d_bmp = (ALLEGRO_BITMAP_D3D *)albmp;
	if (!d3d_bmp->is_backbuffer && d3d_bmp->render_target) {
		d3d_bmp->render_target->Release();
		d3d_bmp->render_target = NULL;
	}
	if (d3d_bmp->video_texture) {
	   d3d_bmp->video_texture->Release();
	   d3d_bmp->video_texture = NULL;
	}
   }
}

static bool d3d_create_textures(ALLEGRO_DISPLAY_D3D *disp, int w, int h,
   int flags,
   LPDIRECT3DTEXTURE9 *video_texture, LPDIRECT3DTEXTURE9 *system_texture,
   int format)
{
   int levels;
   int autogenmipmap;
   int err;

   if (flags & ALLEGRO_MIPMAP) {
      /* "0" for all possible levels, required for auto mipmap generation. */
      levels = 0;
      autogenmipmap = D3DUSAGE_AUTOGENMIPMAP;
   }
   else {
      levels = 1;
      autogenmipmap = 0;
   }

   if (_al_d3d_render_to_texture_supported()) {
      if (video_texture) {
         err = disp->device->CreateTexture(w, h, levels,
            D3DUSAGE_RENDERTARGET | autogenmipmap,
            (D3DFORMAT)_al_pixel_format_to_d3d(format), D3DPOOL_DEFAULT,
            video_texture, NULL);
         if (err != D3D_OK && err != D3DOK_NOAUTOGEN) {
            ALLEGRO_ERROR("d3d_create_textures: Unable to create video texture.\n");
            return false;
         }
      }

      if (system_texture) {
         err = disp->device->CreateTexture(w, h, 1,
            0, (D3DFORMAT)_al_pixel_format_to_d3d(format), D3DPOOL_SYSTEMMEM,
            system_texture, NULL);
         if (err != D3D_OK) {
            ALLEGRO_ERROR("d3d_create_textures: Unable to create system texture.\n");
            if (video_texture && (*video_texture)) {
               (*video_texture)->Release();
               *video_texture = NULL;
            }
            return false;
         }
      }

      return true;
   }
   else {
      if (video_texture) {
         err = disp->device->CreateTexture(w, h, 1,
            0, (D3DFORMAT)_al_pixel_format_to_d3d(format), D3DPOOL_MANAGED,
            video_texture, NULL);
         if (err != D3D_OK) {
            ALLEGRO_ERROR("d3d_create_textures: Unable to create video texture (no render-to-texture).\n");
            return false;
         }
      }

      return true;
   }
}

static ALLEGRO_BITMAP
   *d3d_create_bitmap_from_surface(LPDIRECT3DSURFACE9 surface,
   int format, int flags)
{
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_BITMAP_D3D *d3d_bmp;
   D3DSURFACE_DESC desc;
   D3DLOCKED_RECT surf_locked_rect;
   D3DLOCKED_RECT sys_locked_rect;
   ALLEGRO_STATE backup;
   unsigned int y;

   if (surface->GetDesc(&desc) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_bitmap_from_surface: GetDesc failed.\n");
      return NULL;
   }

   if (surface->LockRect(&surf_locked_rect, 0, D3DLOCK_READONLY) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_bitmap_from_surface: LockRect failed.\n");
      return NULL;
   }

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

   al_set_new_bitmap_format(format);
   al_set_new_bitmap_flags(flags);

   bitmap = al_create_bitmap(desc.Width, desc.Height);
   d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;

   al_restore_state(&backup);

   if (!bitmap) {
      surface->UnlockRect();
      return NULL;
   }

   if (d3d_bmp->system_texture->LockRect(0, &sys_locked_rect, 0, 0) != D3D_OK) {
      surface->UnlockRect();
      al_destroy_bitmap(bitmap);
      ALLEGRO_ERROR("d3d_create_bitmap_from_surface: Lock system texture failed.\n");
      return NULL;
   }

   for (y = 0; y < desc.Height; y++) {
      memcpy(
         ((char*)sys_locked_rect.pBits)+(sys_locked_rect.Pitch*y),
         ((char*)surf_locked_rect.pBits)+(surf_locked_rect.Pitch*y),
         desc.Width*al_get_pixel_size(format)
      );
   }

   surface->UnlockRect();
   d3d_bmp->system_texture->UnlockRect(0);

   if (d3d_bmp->display->device->UpdateTexture(
         (IDirect3DBaseTexture9 *)d3d_bmp->system_texture,
         (IDirect3DBaseTexture9 *)d3d_bmp->video_texture) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_bitmap_from_texture: Couldn't update texture.\n");
   }

   return bitmap;
}

/* Copies video texture to system texture and bitmap->memory */
static bool _al_d3d_sync_bitmap(ALLEGRO_BITMAP *dest)
{
   ALLEGRO_BITMAP_D3D *d3d_dest;
   LPDIRECT3DSURFACE9 system_texture_surface;
   LPDIRECT3DSURFACE9 video_texture_surface;
   bool ok;
   UINT i;

   if (!_al_d3d_render_to_texture_supported()) {
      ALLEGRO_ERROR("Render-to-texture not supported.\n");
      return false;
   }

   if (dest->locked) {
      ALLEGRO_ERROR("Already locked.\n");
      return false;
   }

   d3d_dest = (ALLEGRO_BITMAP_D3D *)dest;

   if (d3d_dest->system_texture == NULL || d3d_dest->video_texture == NULL) {
      ALLEGRO_ERROR("A texture is null.\n");
      return false;
   }

   if (dest->parent) {
      dest = dest->parent;
   }

   ok = true;
   system_texture_surface = NULL;
   video_texture_surface = NULL;

   if (d3d_dest->system_texture->GetSurfaceLevel(
         0, &system_texture_surface) != D3D_OK) {
      ALLEGRO_ERROR("_al_d3d_sync_bitmap: GetSurfaceLevel failed while updating video texture.\n");
      ok = false;
   }

   if (ok && d3d_dest->video_texture->GetSurfaceLevel(
         0, &video_texture_surface) != D3D_OK) {
      ALLEGRO_ERROR("_al_d3d_sync_bitmap: GetSurfaceLevel failed while updating video texture.\n");
      ok = false;
   }

   if (ok && d3d_dest->display->device->GetRenderTargetData(
         video_texture_surface,
         system_texture_surface) != D3D_OK) {
      ALLEGRO_ERROR("_al_d3d_sync_bitmap: GetRenderTargetData failed.\n");
      ok = false;
   }

   if (system_texture_surface) {
       if ((i = system_texture_surface->Release()) != 0) {
	  ALLEGRO_DEBUG("_al_d3d_sync_bitmap (system) ref count == %d\n", i);
       }
   }
   if (video_texture_surface) {
       if ((i = video_texture_surface->Release()) != 0) {
	  // This can be non-zero
	  ALLEGRO_DEBUG("_al_d3d_sync_bitmap (video) ref count == %d\n", i);
       }
   }

   if (ok) {
      d3d_sync_bitmap_memory(dest);
   }

   return ok;
}

/*
 * Must be called before the D3D device is reset (e.g., when
 * resizing a window). All non-synced display bitmaps must be
 * synced to memory.
 */
void _al_d3d_prepare_bitmaps_for_reset(ALLEGRO_DISPLAY_D3D *disp)
{
   unsigned int i;

   if (disp->device_lost)
      return;

   if (!_al_d3d_render_to_texture_supported())
      return;

   al_lock_mutex(_al_d3d_lost_device_mutex);

   for (i = 0; i < created_bitmaps._size; i++) {
      ALLEGRO_BITMAP_D3D **bptr = (ALLEGRO_BITMAP_D3D **)_al_vector_ref(&created_bitmaps, i);
      ALLEGRO_BITMAP_D3D *bmp = *bptr;
      ALLEGRO_BITMAP *al_bmp = (ALLEGRO_BITMAP *)bmp;
      if (bmp->display == disp) {
         //d3d_sync_bitmap_memory(al_bmp);
         if (!al_bmp->preserve_texture) {
            bmp->modified = false;
         }
         else if (!bmp->is_backbuffer && bmp->modified && !(al_bmp->flags & ALLEGRO_MEMORY_BITMAP)) {
            _al_d3d_sync_bitmap(al_bmp);
            bmp->modified = false;
         }
      }
   }

   al_unlock_mutex(_al_d3d_lost_device_mutex);
}

/*
 * Called after the resize is done.
 */
bool _al_d3d_recreate_bitmap_textures(ALLEGRO_DISPLAY_D3D *disp)
{
   unsigned int i;

   for (i = 0; i < created_bitmaps._size; i++) {
      ALLEGRO_BITMAP_D3D **bptr = (ALLEGRO_BITMAP_D3D **)_al_vector_ref(&created_bitmaps, i);
      ALLEGRO_BITMAP_D3D *bmp = *bptr;
      ALLEGRO_BITMAP *al_bmp = (ALLEGRO_BITMAP *)bmp;

      if (bmp->display == disp) {
	      if (!d3d_create_textures(disp, bmp->texture_w,
		    bmp->texture_h,
		    al_bmp->flags,
		    &bmp->video_texture,
		    &bmp->system_texture,
		    al_bmp->format))
		 return false;
	      d3d_do_upload(bmp, 0, 0, al_bmp->w, al_bmp->h, true);
      }
   }

   return true;
}

/*
 * Refresh the texture memory. This must be done after a device is
 * lost or after it is reset.
 */
void _al_d3d_refresh_texture_memory(void)
{
   unsigned int i;

   for (i = 0; i < created_bitmaps._size; i++) {
      ALLEGRO_BITMAP_D3D **bptr = (ALLEGRO_BITMAP_D3D **)_al_vector_ref(&created_bitmaps, i);
      ALLEGRO_BITMAP_D3D *bmp = *bptr;
      ALLEGRO_BITMAP *al_bmp = (ALLEGRO_BITMAP *)bmp;
      ALLEGRO_DISPLAY_D3D *bmps_display = (ALLEGRO_DISPLAY_D3D *)al_bmp->display;

      if ((al_bmp->flags & ALLEGRO_MEMORY_BITMAP) || (al_bmp->parent)) {
         continue;
      }

      d3d_create_textures(bmps_display, bmp->texture_w, bmp->texture_h,
         al_bmp->flags,
	 &bmp->video_texture, /*&bmp->system_texture*/0, al_bmp->format);
      d3d_sync_bitmap_texture(al_bmp,
	 0, 0, al_bmp->w, al_bmp->h);
      if (_al_d3d_render_to_texture_supported()) {
	 bmps_display->device->UpdateTexture(
	    (IDirect3DBaseTexture9 *)bmp->system_texture,
	    (IDirect3DBaseTexture9 *)bmp->video_texture);
      }
   }
}

static bool d3d_upload_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_D3D *d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;
   int w = bitmap->w;
   int h = bitmap->h;

   if (d3d_bmp->display->device_lost)
      return false;

   if (d3d_bmp->initialized != true) {
      bool non_pow2 = al_have_d3d_non_pow2_texture_support();
      bool non_square = al_have_d3d_non_square_texture_support();

      if (non_pow2 && non_square) {
         // Any shape and size
         d3d_bmp->texture_w = w;
	 d3d_bmp->texture_h = h;
      }
      else if (non_pow2) {
         // Must be sqaure
         int max = _ALLEGRO_MAX(w,  h);
         d3d_bmp->texture_w = max;
         d3d_bmp->texture_h = max;
      }
      else {
         // Must be POW2
         d3d_bmp->texture_w = pot(w);
         d3d_bmp->texture_h = pot(h);
      }

      // Some cards/drivers don't like small textures
      if (d3d_bmp->texture_w < 16) d3d_bmp->texture_w = 16;
      if (d3d_bmp->texture_h < 16) d3d_bmp->texture_h = 16;

      if (d3d_bmp->video_texture == 0)
         if (!d3d_create_textures(d3d_bmp->display, d3d_bmp->texture_w,
               d3d_bmp->texture_h,
               d3d_bmp->bitmap.flags,
               &d3d_bmp->video_texture,
               &d3d_bmp->system_texture,
               bitmap->format)) {
            return false;
         }

      /*
       * Keep track of created bitmaps, in case the display is lost
       * or resized.
       */
      *(ALLEGRO_BITMAP_D3D **)_al_vector_alloc_back(&created_bitmaps) = d3d_bmp;

      d3d_bmp->initialized = true;
   }

   d3d_do_upload(d3d_bmp, 0, 0, w, h, false);

   return true;
}

static void d3d_draw_bitmap_region(
   ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_BITMAP_D3D *d3d_dest = (ALLEGRO_BITMAP_D3D *)dest;
   ALLEGRO_BITMAP_D3D *d3d_src = (ALLEGRO_BITMAP_D3D *)src;

   if (!_al_d3d_render_to_texture_supported()) {
      _al_draw_bitmap_region_memory(src, tint,
         (int)sx, (int)sy, (int)sw, (int)sh, 0, 0,
         (int)flags);
      return;
   }
   
   if (d3d_dest->display->device_lost)
      return;

   _al_d3d_set_bitmap_clip(dest);

   /* For sub-bitmaps */
   if (src->parent) {
      sx += src->xofs;
      sy += src->yofs;
      src = src->parent;
      d3d_src = (ALLEGRO_BITMAP_D3D *)src;
   }
   if (dest->parent) {
      dest = dest->parent;
      d3d_dest = (ALLEGRO_BITMAP_D3D *)dest;
   }

   if (d3d_src->is_backbuffer) {
      IDirect3DSurface9 *surface;
      D3DSURFACE_DESC desc;
      if (d3d_src->display->render_target->GetDesc(&desc) != D3D_OK) {
         ALLEGRO_ERROR("d3d_draw_bitmap_region: GetDesc failed.\n");
         return;
      }
      if (desc.MultiSampleType == D3DMULTISAMPLE_NONE) {
         surface = d3d_src->display->render_target;
      }
      else {
         RECT r;
         if (d3d_src->display->device->CreateRenderTarget(
		desc.Width,
		desc.Height,
		desc.Format,
		D3DMULTISAMPLE_NONE,
		0,
		TRUE,
		&surface,
		NULL
	) != D3D_OK) {
            ALLEGRO_ERROR(
	    	"d3d_draw_bitmap_region: CreateRenderTarget failed.\n");
            return;
         }
	 r.top = 0;
	 r.left = 0;
	 r.right = desc.Width;
	 r.bottom = desc.Height;
	 if (d3d_src->display->device->StretchRect(
	 	d3d_src->display->render_target,
		&r,
		surface,
		&r,
		D3DTEXF_NONE
	 ) != D3D_OK) {
	    ALLEGRO_ERROR("d3d_draw_bitmap_region: StretchRect failed.\n");
	    surface->Release();
	    return;
	 }
      }
      ALLEGRO_BITMAP *tmp_bmp = d3d_create_bitmap_from_surface(
         surface,
	 src->format,
         src->flags
      );
      if (tmp_bmp) {
         d3d_draw_bitmap_region((ALLEGRO_BITMAP *)tmp_bmp, tint,
            sx, sy, sw, sh, flags);
         al_destroy_bitmap(tmp_bmp);
	 if (desc.MultiSampleType != D3DMULTISAMPLE_NONE) {
	    surface->Release();
	 }
      }
      return;
   }

   _al_d3d_set_blender(d3d_dest->display);

   d3d_draw_textured_quad(
      d3d_dest->display, d3d_src, tint,
      sx, sy, sw, sh, flags);

   d3d_dest->modified = true;
}

static void d3d_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_D3D *d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;

   if (!al_is_sub_bitmap(bitmap)) {
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
   }

   _al_vector_find_and_delete(&created_bitmaps, &d3d_bmp);
}

static ALLEGRO_LOCKED_REGION *d3d_lock_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, int format,
   int flags)
{
   ALLEGRO_BITMAP_D3D *d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;

   if (d3d_bmp->display->device_lost)
      return NULL;

   RECT rect;
   DWORD Flags = flags & ALLEGRO_LOCK_READONLY ? D3DLOCK_READONLY : 0;
   int f = _al_get_real_pixel_format(al_get_current_display(), format);
   if (f < 0) {
      return NULL;
   }

   rect.left = x;
   rect.right = x + w;
   rect.top = y;
   rect.bottom = y + h;

   if (d3d_bmp->is_backbuffer) {
      ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)bitmap->display;
      if (d3d_disp->render_target->LockRect(&d3d_bmp->locked_rect, &rect, Flags) != D3D_OK) {
         ALLEGRO_ERROR("LockRect failed in d3d_lock_region.\n");
         return NULL;
      }
   }
   else {
      LPDIRECT3DTEXTURE9 texture;
      if (_al_d3d_render_to_texture_supported()) {
         /* 
	  * Sync bitmap->memory with texture
          */
	 bitmap->locked = false;
         if (!_al_d3d_sync_bitmap(bitmap)) {
            return NULL;
         }
	 bitmap->locked = true;
         texture = d3d_bmp->system_texture;
      }
      else {
         texture = d3d_bmp->video_texture;
      }
      if (texture->LockRect(0, &d3d_bmp->locked_rect, &rect, Flags) != D3D_OK) {
         ALLEGRO_ERROR("LockRect failed in d3d_lock_region.\n");
         return NULL;
      }
   }

   if (format == ALLEGRO_PIXEL_FORMAT_ANY || bitmap->format == format || f == bitmap->format) {
      bitmap->locked_region.data = d3d_bmp->locked_rect.pBits;
      bitmap->locked_region.format = bitmap->format;
      bitmap->locked_region.pitch = d3d_bmp->locked_rect.Pitch;
      bitmap->locked_region.pixel_size = al_get_pixel_size(bitmap->format);
   }
   else {
      bitmap->locked_region.pitch = al_get_pixel_size(f) * w;
      bitmap->locked_region.data = al_malloc(bitmap->locked_region.pitch*h);
      bitmap->locked_region.format = f;
      bitmap->locked_region.pixel_size = al_get_pixel_size(bitmap->format);
      if (!(bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY)) {
         _al_convert_bitmap_data(
            d3d_bmp->locked_rect.pBits, bitmap->format, d3d_bmp->locked_rect.Pitch,
            bitmap->locked_region.data, f, bitmap->locked_region.pitch,
            0, 0, 0, 0, w, h);
      }
   }

   return &bitmap->locked_region;
}

static void d3d_unlock_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_D3D *d3d_bmp = (ALLEGRO_BITMAP_D3D *)bitmap;

   d3d_bmp->modified = true;

   if (bitmap->locked_region.format != 0 && bitmap->locked_region.format != bitmap->format) {
      if (!(bitmap->lock_flags & ALLEGRO_LOCK_READONLY)) {
         _al_convert_bitmap_data(
            bitmap->locked_region.data, bitmap->locked_region.format, bitmap->locked_region.pitch,
            d3d_bmp->locked_rect.pBits, bitmap->format, d3d_bmp->locked_rect.Pitch,
            0, 0, 0, 0, bitmap->lock_w, bitmap->lock_h);
      }
      al_free(bitmap->locked_region.data);
   }

   if (d3d_bmp->is_backbuffer) {
      ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)bitmap->display;
      d3d_disp->render_target->UnlockRect();
   }
   else {
      LPDIRECT3DTEXTURE9 texture;
      if (_al_d3d_render_to_texture_supported())
         texture = d3d_bmp->system_texture;
      else
         texture = d3d_bmp->video_texture;
      texture->UnlockRect(0);
      if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY)
         return;
      d3d_do_upload(d3d_bmp, bitmap->lock_x, bitmap->lock_y,
         bitmap->lock_w, bitmap->lock_h, false);
   }
}


static void d3d_update_clipping_rectangle(ALLEGRO_BITMAP *bitmap)
{
   _al_d3d_set_bitmap_clip(bitmap);
}


/* Obtain a reference to this driver. */
ALLEGRO_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void)
{
   if (vt)
      return vt;

   vt = (ALLEGRO_BITMAP_INTERFACE *)al_malloc(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->draw_bitmap_region = d3d_draw_bitmap_region;
   vt->upload_bitmap = d3d_upload_bitmap;
   vt->update_clipping_rectangle = NULL;
   vt->destroy_bitmap = d3d_destroy_bitmap;
   vt->lock_region = d3d_lock_region;
   vt->unlock_region = d3d_unlock_region;
   vt->update_clipping_rectangle = d3d_update_clipping_rectangle;

   return vt;
}

} // end extern "C"
