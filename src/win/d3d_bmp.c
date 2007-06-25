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

#include "allegro.h"
#include "system_new.h"
#include "internal/aintern.h"
#include "internal/aintern_system.h"
#include "internal/aintern_display.h"
#include "internal/aintern_bitmap.h"

#include "d3d.h"


static AL_BITMAP_INTERFACE *vt;
static _AL_VECTOR created_bitmaps = _AL_VECTOR_INITIALIZER(AL_BITMAP_D3D *);


static AL_BITMAP *_al_d3d_create_masked_bitmap(AL_BITMAP *original)
{
   AL_BITMAP *masked_bmp;
   AL_COLOR mask_color;
   AL_LOCKED_REGION lr;
   int x, y;
   AL_COLOR pixel;
   AL_COLOR alpha_pixel;
   unsigned char r, g, b, a;

   _al_push_bitmap_parameters();
   al_set_new_bitmap_format(original->format);
   al_set_new_bitmap_flags(AL_USE_ALPHA|AL_SYNC_MEMORY_COPY);
   masked_bmp = al_create_bitmap(original->w, original->h);
   _al_pop_bitmap_parameters();

   _al_push_target_bitmap();
   al_set_target_bitmap(masked_bmp);
   al_draw_bitmap(original, 0, 0, 0);
   _al_pop_target_bitmap();


   if (!al_lock_bitmap(masked_bmp, &lr, 0)) {
      TRACE("_al_d3d_create_masked_bitmap: Couldn't lock new bitmap.\n");
      al_destroy_bitmap(masked_bmp);
      return NULL;
   }

   al_map_rgba(masked_bmp, &alpha_pixel, 0, 0, 0, 0);
   al_get_mask_color(&mask_color);

   _al_push_target_bitmap();
   al_set_target_bitmap(masked_bmp);

   for (y = 0; y < masked_bmp->h; y++) {
      for (x = 0; x < masked_bmp->w; x++) {
         al_get_pixel(masked_bmp, x, y, &pixel);
         if (memcmp(&pixel, &mask_color, sizeof(AL_COLOR)) == 0) {
            al_put_pixel(x, y, &alpha_pixel);
         }
         else if (!(original->flags & AL_USE_ALPHA)) {
            al_unmap_rgba(masked_bmp, &pixel, &r, &g, &b, &a);
            if (a < 255) {
               a = 255;
               al_map_rgba(masked_bmp, &pixel, r, g, b, a);
               al_put_pixel(x, y, &pixel);
            }
         }
      }
   }

   _al_pop_target_bitmap();

   al_unlock_bitmap(masked_bmp);

   return masked_bmp;
}

static void d3d_set_matrix(float *src, D3DMATRIX *dest)
{
   int x, y;
   int i = 0;

   for (y = 0; y < 4; y++) {
      for (x = 0; x < 4; x++) {
         dest->m[x][y] = src[i++];
      }
   }
}

static void d3d_get_identity_matrix(D3DMATRIX *result)
{
   float identity[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
   };

   d3d_set_matrix(identity, result);
}

static void d3d_get_translation_matrix(float tx, float ty, float tz,
   D3DMATRIX *result)
{
   float translation[16] = {
      1.0f, 0.0f, 0.0f,   tx,
      0.0f, 1.0f, 0.0f,   ty,
      0.0f, 0.0f, 1.0f,   tz,
      0.0f, 0.0f, 0.0f, 1.0f
   };

   d3d_set_matrix(translation, result);
}

static void d3d_get_z_rotation_matrix(float a, D3DMATRIX *result)
{
   float rotation[16] = {
        cos(a), -sin(a), 0.0f, 0.0f,
       sin(a),  cos(a), 0.0f, 0.0f,
         0.0f,    0.0f, 1.0f, 0.0f,
         0.0f,    0.0f, 0.0f, 1.0f
   }; 

   d3d_set_matrix(rotation, result);
}


/*
 * Do a transformation for a quad drawing.
 */
static void d3d_transform(D3D_TL_VERTEX vertices[],
   float cx, float cy, float dx, float dy, 
   float angle)
{
   D3DMATRIX center_matrix;
   D3DMATRIX rotation_matrix;
   D3DMATRIX dest_matrix;
   
   d3d_get_translation_matrix(-cx, -cy, 0.0f, &center_matrix);
   d3d_get_z_rotation_matrix(angle, &rotation_matrix);
   d3d_get_translation_matrix(dx, dy, 0.0f, &dest_matrix);

   IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_VIEW, &dest_matrix);
   IDirect3DDevice9_MultiplyTransform(_al_d3d_device, D3DTS_VIEW, &rotation_matrix);
   IDirect3DDevice9_MultiplyTransform(_al_d3d_device, D3DTS_VIEW, &center_matrix);
}

/*
 * Draw a textured quad (or filled rectangle)
 *
 * bmp - bitmap with texture to draw
 * sx, sy, sw, sh - source x, y, width, height
 * dx, dy, dw, dh - destination x, y, width, height
 * cx, cy - center of rotation and scaling
 * angle - rotation angle in radians (counter clockwise)
 * color - tint color?
 * flags - flipping flags
 *
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

   IDirect3DDevice9_SetFVF(_al_d3d_device, D3DFVF_TL_VERTEX);
   
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
      bitmap->memory, bitmap->format, _al_get_pixel_size(bitmap->format)*bitmap->w,
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
   _al_convert_bitmap_data(bitmap->memory, bitmap->format, bitmap->w*_al_get_pixel_size(bitmap->format),
      locked_rect.pBits, bitmap->format, locked_rect.Pitch,
      x, y, 0, 0, width, height);
   /* Copy an extra row and column so the texture ends nicely */
   if (rect.bottom > bitmap->h) {
      _al_convert_bitmap_data(
         bitmap->memory,
         bitmap->format, bitmap->w*_al_get_pixel_size(bitmap->format),
         locked_rect.pBits,
         bitmap->format, locked_rect.Pitch,
         0, bitmap->h-1,
         0, height,
         width, 1);
   }
   if (rect.right > bitmap->w) {
      _al_convert_bitmap_data(
         bitmap->memory,
         bitmap->format, bitmap->w*_al_get_pixel_size(bitmap->format),
         locked_rect.pBits,
         bitmap->format, locked_rect.Pitch,
         bitmap->w-1, 0,
         width, 0,
         1, height);
   }
   if (rect.bottom > bitmap->h && rect.right > bitmap->w) {
      _al_convert_bitmap_data(
         bitmap->memory,
         bitmap->format, bitmap->w*_al_get_pixel_size(bitmap->format),
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
            D3DUSAGE_RENDERTARGET, _al_format_to_d3d(format), D3DPOOL_DEFAULT,
            video_texture, NULL) != D3D_OK) {
         TRACE("d3d_create_textures: Unable to create video texture.\n");
         return false;
      }
   }

   if (system_texture) {
      if (IDirect3DDevice9_CreateTexture(_al_d3d_device, w, h, 1,
            0, _al_format_to_d3d(format), D3DPOOL_SYSTEMMEM,
            system_texture, NULL) != D3D_OK) {
         TRACE("d3d_create_textures: Unable to create system texture.\n");
         if (video_texture) {
            IDirect3DTexture9_Release(*video_texture);
         }
         return false;
      }
   }

   return true;
}

static AL_BITMAP *d3d_create_bitmap_from_surface(LPDIRECT3DSURFACE9 surface,
   int flags)
{
   AL_BITMAP *bitmap;
   AL_BITMAP_D3D *d3d_bmp;
   D3DSURFACE_DESC desc;
   D3DLOCKED_RECT surf_locked_rect;
   D3DLOCKED_RECT sys_locked_rect;
   int format;
   unsigned int y;

   if (IDirect3DSurface9_GetDesc(surface, &desc) != D3D_OK) {
      TRACE("d3d_create_bitmap_from_surface: GetDesc failed.\n");
      return NULL;
   }

   if (IDirect3DSurface9_LockRect(surface, &surf_locked_rect, 0, D3DLOCK_READONLY) != D3D_OK) {
      TRACE("d3d_create_bitmap_from_surface: LockRect failed.\n");
      return NULL;
   }

   _al_push_bitmap_parameters();

   format = _al_d3d_format_to_allegro(desc.Format);

   al_set_new_bitmap_format(format);
   al_set_new_bitmap_flags(flags);

   bitmap = al_create_bitmap(desc.Width, desc.Height);
   d3d_bmp = (AL_BITMAP_D3D *)bitmap;

   _al_pop_bitmap_parameters();

   if (!bitmap) {
      IDirect3DSurface9_UnlockRect(surface);
      return NULL;
   }

   if (IDirect3DTexture9_LockRect(d3d_bmp->system_texture, 0, &sys_locked_rect, 0, 0) != D3D_OK) {
      IDirect3DSurface9_UnlockRect(surface);
      al_destroy_bitmap(bitmap);
      TRACE("d3d_create_bitmap_from_surface: Lock system texture failed.\n");
      return NULL;
   }

   for (y = 0; y < desc.Height; y++) {
      memcpy(
         sys_locked_rect.pBits+(sys_locked_rect.Pitch*y),
         surf_locked_rect.pBits+(surf_locked_rect.Pitch*y),
         desc.Width*4
      );
   }

   IDirect3DSurface9_UnlockRect(surface);
   IDirect3DTexture9_UnlockRect(d3d_bmp->system_texture, 0);

   if (IDirect3DDevice9_UpdateTexture(_al_d3d_device,
         (IDirect3DBaseTexture9 *)d3d_bmp->system_texture,
         (IDirect3DBaseTexture9 *)d3d_bmp->video_texture) != D3D_OK) {
      TRACE("d3d_create_bitmap_from_texture: Couldn't update texture.\n");
   }

   return bitmap;
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

static bool d3d_upload_bitmap(AL_BITMAP *bitmap, int x, int y,
   int width, int height)
{
   AL_BITMAP_D3D *d3d_bmp = (void *)bitmap;
   int w = bitmap->w;
   int h = bitmap->h;

   if (_al_d3d_is_device_lost()) return false;
   _al_d3d_lock_device();

   if (d3d_bmp->initialized != true) {
      d3d_bmp->texture_w = pot(w);
      d3d_bmp->texture_h = pot(h);
      if (d3d_bmp->video_texture == 0)
         if (!d3d_create_textures(d3d_bmp->texture_w,
               d3d_bmp->texture_h,
               &d3d_bmp->video_texture,
               &d3d_bmp->system_texture,
               bitmap->format)) {
            _al_d3d_unlock_device();
            return false;
         }

      /*
       * Keep track of created bitmaps, in case the display is lost
       * or resized.
       */
      *(AL_BITMAP_D3D **)_al_vector_alloc_back(&created_bitmaps) = d3d_bmp;

      d3d_bmp->initialized = true;
   }

   _al_d3d_unlock_device();

   d3d_do_upload(d3d_bmp, x, y, width, height, true);

   return true;
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
   DWORD light_color = 0xFFFFFFFF;

   if (_al_d3d_is_device_lost()) return;

   angle = -angle;

   if (d3d_src->is_backbuffer) {
      AL_DISPLAY_D3D *d3d_display = (AL_DISPLAY_D3D *)src->display;
      AL_BITMAP *tmp_bmp =
         d3d_create_bitmap_from_surface(
            d3d_display->render_target,
            src->flags);
      d3d_blit_real(tmp_bmp, sx, sy, sw, sh,
         source_center_x, source_center_y,
         dx, dy, dw, dh,
         angle, flags);
      al_destroy_bitmap(tmp_bmp);
      return;
   }

   if (flags & AL_MASK_SOURCE) {
      AL_BITMAP *tmp_bmp = _al_d3d_create_masked_bitmap(src);
      if (tmp_bmp) {
         d3d_blit_real(tmp_bmp, sx, sy, sw, sh,
            source_center_x, source_center_y,
            dx, dy, dw, dh,
            angle, flags & ~AL_MASK_SOURCE);
         al_destroy_bitmap(tmp_bmp);
      }
      return;
   }

   _al_d3d_lock_device();

   if (src && (src->flags & AL_USE_ALPHA)) {
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   }

   _al_d3d_draw_textured_quad(d3d_src,
      sx, sy, sw, sh,
      dx, dy, dw, dh,
      source_center_x, source_center_y,
      angle, light_color,
      flags);

   if (src && (src->flags & AL_USE_ALPHA)) {
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, FALSE);
   }

   _al_d3d_unlock_device();

   /* 
    * If we're rendering to a texture, and the bitmap has the
    * AL_SYNC_MEMORY_COPY flag, sync the texture to system memory.
    */
   if (!d3d_dest->is_backbuffer &&
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

      IDirect3DSurface9_Release(system_texture_surface);
      IDirect3DSurface9_Release(video_texture_surface);
      
      IDirect3DDevice9_BeginScene(_al_d3d_device);

      d3d_sync_bitmap_memory(dest);

      _al_d3d_unlock_device();
   }
}

/* Blitting functions */

static void d3d_draw_bitmap(AL_BITMAP *bitmap, float dx, float dy, int flags)
{
   d3d_blit_real(bitmap, 0.0f, 0.0f, bitmap->w, bitmap->h,
      bitmap->w/2, bitmap->h/2,
      dx, dy, bitmap->w, bitmap->h,
      0.0f, flags);
}

static void d3d_draw_bitmap_region(AL_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, int flags)
{
   d3d_blit_real(bitmap,
      sx, sy, sw, sh,
      0.0f, 0.0f,
      dx, dy, sw, sh,
      0.0f, flags);
}

void d3d_draw_scaled_bitmap(AL_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   d3d_blit_real(bitmap,
      sx, sy, sw, sh, (sw-sx)/2, (sh-sy)/2,
      dx, dy, dw, dh, 0.0f,
      flags);
}

void d3d_draw_rotated_bitmap(AL_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float angle, int flags)
{
   d3d_blit_real(bitmap,
      0.0f, 0.0f, bitmap->w, bitmap->h,
      cx, cy,
      dx, dy, bitmap->w, bitmap->h,
      angle, flags);
}

void d3d_draw_rotated_scaled_bitmap(AL_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float xscale, float yscale, float angle,
   float flags)
{
   d3d_blit_real(bitmap,
      0.0f, 0.0f, bitmap->w, bitmap->h,
      cx, cy,
      dx, dy, bitmap->w*xscale, bitmap->h*yscale,
      angle, flags);
}

static void d3d_destroy_bitmap(AL_BITMAP *bitmap)
{
   AL_BITMAP_D3D *d3d_bmp = (void *)bitmap;

   if (d3d_bmp->video_texture) {
      if (IDirect3DTexture9_Release(d3d_bmp->video_texture) != D3D_OK) {
      TRACE("d3d_destroy_bitmap: Release video texture failed.\n");
   }
   }
   if (d3d_bmp->system_texture) {
      if (IDirect3DTexture9_Release(d3d_bmp->system_texture) != D3D_OK) {
      TRACE("d3d_destroy_bitmap: Release video texture failed.\n");
   }
   }

   _al_vector_find_and_delete(&created_bitmaps, &d3d_bmp);
}

static AL_LOCKED_REGION *d3d_lock_region(AL_BITMAP *bitmap,
   int x, int y, int w, int h, AL_LOCKED_REGION *locked_region,
   int flags)
{
   AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;
   RECT rect;
   DWORD Flags = flags & AL_LOCK_READONLY ? D3DLOCK_READONLY : 0;

   rect.left = x;
   rect.right = x + w;
   rect.top = y;
   rect.bottom = y + h;

   if (d3d_bmp->is_backbuffer) {
      AL_DISPLAY_D3D *d3d_disp = (AL_DISPLAY_D3D *)bitmap->display;
      if (IDirect3DSurface9_LockRect(d3d_disp->render_target, &d3d_bmp->locked_rect, &rect, Flags) != D3D_OK) {
         TRACE("LockRect failed in d3d_lock_region.\n");
         return NULL;
      }
   }
   else {
      if (IDirect3DTexture9_LockRect(d3d_bmp->system_texture, 0, &d3d_bmp->locked_rect, &rect, Flags) != D3D_OK) {
         TRACE("LockRect failed in d3d_lock_region.\n");
         return NULL;
      }
   }

   locked_region->data = d3d_bmp->locked_rect.pBits;
   locked_region->format = bitmap->format;
   locked_region->pitch = d3d_bmp->locked_rect.Pitch;

   return locked_region;
}

static void d3d_unlock_region(AL_BITMAP *bitmap)
{
   AL_BITMAP_D3D *d3d_bmp = (AL_BITMAP_D3D *)bitmap;
   bool sync;


   if (d3d_bmp->is_backbuffer) {
      AL_DISPLAY_D3D *d3d_disp = (AL_DISPLAY_D3D *)bitmap->display;
      IDirect3DSurface9_UnlockRect(d3d_disp->render_target);
   }
   else {
      IDirect3DTexture9_UnlockRect(d3d_bmp->system_texture, 0);
      d3d_do_upload(d3d_bmp, bitmap->lock_x, bitmap->lock_y,
         bitmap->lock_w, bitmap->lock_h, sync);
   }

   if (bitmap->lock_flags & AL_LOCK_READONLY)
      return;

   if (bitmap->flags & AL_SYNC_MEMORY_COPY)
      sync = true;
   else
      sync = false;

}

/* Obtain a reference to this driver. */
AL_BITMAP_INTERFACE *_al_bitmap_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->draw_bitmap = d3d_draw_bitmap;
   vt->draw_bitmap_region = d3d_draw_bitmap_region;
   vt->draw_scaled_bitmap = d3d_draw_scaled_bitmap;
   vt->draw_rotated_bitmap = d3d_draw_rotated_bitmap;
   vt->draw_rotated_scaled_bitmap = d3d_draw_rotated_scaled_bitmap;
   vt->upload_bitmap = d3d_upload_bitmap;
   vt->destroy_bitmap = d3d_destroy_bitmap;
   vt->lock_region = d3d_lock_region;
   vt->unlock_region = d3d_unlock_region;

   return vt;
}

