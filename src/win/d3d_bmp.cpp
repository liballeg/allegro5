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
#include "allegro5/internal/aintern_tri_soft.h" // For ALLEGRO_VERTEX
#include "allegro5/platform/aintwin.h"

#include "d3d.h"

ALLEGRO_DEBUG_CHANNEL("d3d")


static ALLEGRO_BITMAP_INTERFACE *vt;

// C++ needs to cast void pointers
#define get_extra(b) ((ALLEGRO_BITMAP_EXTRA_D3D *)\
   (b->parent ? b->parent->extra : b->extra))

static bool convert_compressed(LPDIRECT3DTEXTURE9 dest, LPDIRECT3DTEXTURE9 src,
   int x, int y, int width, int height) {
#ifdef ALLEGRO_CFG_D3DX9
   bool ok = true;
   LPDIRECT3DSURFACE9 dest_texture_surface = NULL;
   LPDIRECT3DSURFACE9 src_texture_surface = NULL;

   if (dest->GetSurfaceLevel(0, &dest_texture_surface) != D3D_OK) {
      ALLEGRO_ERROR("convert_compressed: GetSurfaceLevel failed on dest.\n");
      ok = false;
   }

   if (ok && src->GetSurfaceLevel(0, &src_texture_surface) != D3D_OK) {
      ALLEGRO_ERROR("convert_compressed: GetSurfaceLevel failed on src.\n");
      ok = false;
   }

   RECT rect;
   rect.left = x;
   rect.top = y;
   rect.right = x + width;
   rect.bottom = y + height;

   if (ok && _al_imp_D3DXLoadSurfaceFromSurface &&
       _al_imp_D3DXLoadSurfaceFromSurface(dest_texture_surface,
                                          NULL,
                                          &rect,
                                          src_texture_surface,
                                          NULL,
                                          &rect,
                                          D3DX_FILTER_NONE,
                                          0) != D3D_OK) {
      ALLEGRO_ERROR("convert_compressed: D3DXLoadSurfaceFromSurface failed.\n");
      ok = false;
   }

   int i;
   if (src_texture_surface) {
       if ((i = src_texture_surface->Release()) != 0) {
          ALLEGRO_DEBUG("convert_compressed (src) ref count == %d\n", i);
       }
   }
   if (dest_texture_surface) {
       if ((i = dest_texture_surface->Release()) != 0) {
          // This can be non-zero
          ALLEGRO_DEBUG("convert_compressed (dest) ref count == %d\n", i);
       }
   }
   return ok;
#else
   (void)dest;
   (void)src;
   (void)x;
   (void)y;
   (void)width;
   (void)height;
   return false;
#endif
}

/* Function: al_get_d3d_texture_size
 */
bool al_get_d3d_texture_size(ALLEGRO_BITMAP *bitmap, int *width, int *height)
{
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   ASSERT(bitmap);
   ASSERT(width);
   ASSERT(height);

   if (!(bitmap_flags & _ALLEGRO_INTERNAL_OPENGL) &&
         !(bitmap_flags & ALLEGRO_MEMORY_BITMAP)) {
      ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
      *width = d3d_bmp->texture_w;
      *height = d3d_bmp->texture_h;
      return true;
   }
   else {
      *width = 0;
      *height = 0;
      return false;
   }
}

void _al_d3d_bmp_destroy(void)
{
   al_free(vt);
   vt = NULL;
}

static INLINE void transform_vertex(float* x, float* y, float* z)
{
   al_transform_coordinates_3d(al_get_current_transform(), x, y, z);
}

/*
 * Draw a textured quad
 */
static void d3d_draw_textured_quad(
   ALLEGRO_DISPLAY_D3D *disp, ALLEGRO_BITMAP *bmp, ALLEGRO_COLOR tint,
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
   ALLEGRO_BITMAP_EXTRA_D3D *extra = get_extra(bmp);

   const float z = 0.0f;

   ALLEGRO_DISPLAY* aldisp = (ALLEGRO_DISPLAY*)disp;

   if (aldisp->num_cache_vertices != 0 && (uintptr_t)bmp != aldisp->cache_texture) {
      aldisp->vt->flush_vertex_cache(aldisp);
   }
   aldisp->cache_texture = (uintptr_t)bmp;

   right  = sw;
   bottom = sh;

   texture_w = extra->texture_w;
   texture_h = extra->texture_h;
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

#define ALLEGRO_COLOR_TO_D3D(c) D3DCOLOR_COLORVALUE(c.r, c.g, c.b, c.a)

#define SET(f) \
   vertices[0].x = 0; \
   vertices[0].y = 0; \
   vertices[0].z = z; \
   vertices[0].color = f(tint); \
   vertices[0].u = tu_start; \
   vertices[0].v = tv_start; \
 \
   vertices[1].x = right; \
   vertices[1].y = 0; \
   vertices[1].z = z; \
   vertices[1].color = f(tint); \
   vertices[1].u = tu_end; \
   vertices[1].v = tv_start; \
 \
   vertices[2].x = right; \
   vertices[2].y = bottom; \
   vertices[2].z = z; \
   vertices[2].color = f(tint); \
   vertices[2].u = tu_end; \
   vertices[2].v = tv_end; \
 \
   vertices[5].x = 0; \
   vertices[5].y = bottom; \
   vertices[5].z = z; \
   vertices[5].color = f(tint); \
   vertices[5].u = tu_start; \
   vertices[5].v = tv_end; \
\
   if (aldisp->cache_enabled) { \
      transform_vertex(&vertices[0].x, &vertices[0].y, &vertices[0].z); \
      transform_vertex(&vertices[1].x, &vertices[1].y, &vertices[1].z); \
      transform_vertex(&vertices[2].x, &vertices[2].y, &vertices[2].z); \
      transform_vertex(&vertices[5].x, &vertices[5].y, &vertices[5].z); \
   } \
    \
   vertices[3] = vertices[0]; \
   vertices[4] = vertices[2];

   bool pp = (aldisp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) != 0;

   if (pp) {
           ALLEGRO_VERTEX *vertices = (ALLEGRO_VERTEX *)aldisp->vt->prepare_vertex_cache(aldisp, 6);
        SET(ALLEGRO_COLOR)
   }
   else {
           D3D_FIXED_VERTEX *vertices = (D3D_FIXED_VERTEX *)aldisp->vt->prepare_vertex_cache(aldisp, 6);
        SET(ALLEGRO_COLOR_TO_D3D)
   }

   if (!aldisp->cache_enabled)
      aldisp->vt->flush_vertex_cache(aldisp);
}

static void d3d_draw_textured_quad_new(
   ALLEGRO_DISPLAY *disp, ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, int flags)
{
   float tex_l, tex_t, tex_r, tex_b, w, h, true_w, true_h;
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bitmap = get_extra(bitmap);
   ALLEGRO_VERTEX *vtx;
   int *idx;

   (void)flags;

   int first_idx = disp->vt->prepare_batch(disp, bitmap, ALLEGRO_PRIM_TRIANGLE_LIST, 4, 6, (void**)&vtx, (void**)&idx);

   float texture_w = d3d_bitmap->texture_w;
   float texture_h = d3d_bitmap->texture_h;

   tex_l = sx / texture_w;
   tex_t = sy / texture_h;
   tex_r = (sx + sw) / texture_w;
   tex_b = (sy + sh) / texture_h;

   vtx[0].x = 0;
   vtx[0].y = sh;
   vtx[0].z = 0;
   vtx[0].u = tex_l;
   vtx[0].v = tex_b;
   vtx[0].color = tint;

   vtx[1].x = 0;
   vtx[1].y = 0;
   vtx[1].z = 0;
   vtx[1].u = tex_l;
   vtx[1].v = tex_t;
   vtx[1].color = tint;

   vtx[2].x = sw;
   vtx[2].y = sh;
   vtx[2].z = 0;
   vtx[2].u = tex_r;
   vtx[2].v = tex_b;
   vtx[2].color = tint;

   vtx[3].x = sw;
   vtx[3].y = 0;
   vtx[3].z = 0;
   vtx[3].u = tex_r;
   vtx[3].v = tex_t;
   vtx[3].color = tint;

   if (disp->cache_enabled) {
      /* If drawing is batched, we apply transformations manually. */
      transform_vertex(&vtx[0].x, &vtx[0].y, &vtx[0].z);
      transform_vertex(&vtx[1].x, &vtx[1].y, &vtx[1].z);
      transform_vertex(&vtx[2].x, &vtx[2].y, &vtx[2].z);
      transform_vertex(&vtx[3].x, &vtx[3].y, &vtx[3].z);
   }

   idx[0] = first_idx + 0;
   idx[1] = first_idx + 1;
   idx[2] = first_idx + 2;
   idx[3] = first_idx + 1;
   idx[4] = first_idx + 2;
   idx[5] = first_idx + 3;

   if (!disp->cache_enabled) {
      disp->vt->draw_batch(disp);
   }
}

/* Copy texture memory to bitmap->memory */
static void d3d_sync_bitmap_memory(ALLEGRO_BITMAP *bitmap)
{
   D3DLOCKED_RECT locked_rect;
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   LPDIRECT3DTEXTURE9 texture;
   int bitmap_format = al_get_bitmap_format(bitmap);

   if (_al_d3d_render_to_texture_supported() &&
       !_al_pixel_format_is_compressed(bitmap_format))
      texture = d3d_bmp->system_texture;
   else
      texture = d3d_bmp->video_texture;

   if (texture->LockRect(0, &locked_rect, NULL, 0) == D3D_OK) {
      int block_size = al_get_pixel_block_size(bitmap_format);
      int block_width = al_get_pixel_block_width(bitmap_format);
      int block_height = al_get_pixel_block_height(bitmap_format);
      int mem_pitch = _al_get_least_multiple(bitmap->w, block_width) *
         block_size / block_width;
      _al_copy_bitmap_data(locked_rect.pBits, locked_rect.Pitch,
         bitmap->memory, mem_pitch,
         0, 0, 0, 0, _al_get_least_multiple(bitmap->w, block_width),
         _al_get_least_multiple(bitmap->h, block_height), bitmap_format);
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
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   LPDIRECT3DTEXTURE9 texture;
   int bitmap_format = al_get_bitmap_format(bitmap);

   if (bitmap->parent) return;

   rect.left = x;
   rect.top = y;
   rect.right = x + width;
   rect.bottom = y + height;

   if (_al_d3d_render_to_texture_supported() &&
       !_al_pixel_format_is_compressed(bitmap_format))
      texture = d3d_bmp->system_texture;
   else
      texture = d3d_bmp->video_texture;

   if (texture->LockRect(0, &locked_rect, &rect, 0) == D3D_OK) {
      int block_size = al_get_pixel_block_size(bitmap_format);
      int block_width = al_get_pixel_block_width(bitmap_format);
      int mem_pitch = _al_get_least_multiple(bitmap->w, block_width) *
         block_size / block_width;
      _al_copy_bitmap_data(bitmap->memory, mem_pitch,
         locked_rect.pBits, locked_rect.Pitch,
         x, y, 0, 0, width, height, bitmap_format);
      texture->UnlockRect(0);
   }
   else {
      ALLEGRO_ERROR("d3d_sync_bitmap_texture: Couldn't lock texture to upload.\n");
   }
}

static void d3d_do_upload(ALLEGRO_BITMAP *bmp, int x, int y, int width,
   int height, bool sync_from_memory)
{
   if (sync_from_memory) {
      d3d_sync_bitmap_texture(bmp, x, y, width, height);
   }

   if (_al_d3d_render_to_texture_supported()
         && !_al_pixel_format_is_compressed(al_get_bitmap_format(bmp))) {
      ALLEGRO_BITMAP_EXTRA_D3D *d3d_bitmap = get_extra(bmp);
      if (d3d_bitmap->display->device->UpdateTexture(
            (IDirect3DBaseTexture9 *)d3d_bitmap->system_texture,
            (IDirect3DBaseTexture9 *)d3d_bitmap->video_texture) != D3D_OK) {
         ALLEGRO_ERROR("d3d_do_upload: Couldn't update texture.\n");
         return;
      }
   }
}

/*
 * Release all default pool textures. This must be done before
 * resetting the device.
 */
void _al_d3d_release_default_pool_textures(ALLEGRO_DISPLAY *disp)
{
   unsigned int i;

   for (i = 0; i < disp->bitmaps._size; i++) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&disp->bitmaps, i);
      ALLEGRO_BITMAP *albmp = *bptr;

      if ((al_get_bitmap_flags(albmp) & ALLEGRO_MEMORY_BITMAP) || (albmp->parent))
         continue;

      ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(albmp);
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
   int video_format, int system_format)
{
   int levels;
   int autogenmipmap;
   int err;
   bool compressed = _al_pixel_format_is_compressed(video_format);

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
         int usage = compressed ? 0 : D3DUSAGE_RENDERTARGET;
         /* XXX: Compressed video bitmaps are managed, so in principle
          * there is no need to manually sync them for device loss.
          * It is still necessary to do so for resizing purposes,
          * however... not sure there's any real savings to be had here.
          */
         D3DPOOL pool = compressed ? D3DPOOL_MANAGED : D3DPOOL_DEFAULT;
         err = disp->device->CreateTexture(w, h, levels,
            usage | autogenmipmap,
            (D3DFORMAT)_al_pixel_format_to_d3d(video_format), pool,
            video_texture, NULL);
         if (err != D3D_OK && err != D3DOK_NOAUTOGEN) {
            ALLEGRO_ERROR("d3d_create_textures: Unable to create video texture.\n");
            return false;
         }
      }

      if (system_texture) {
         err = disp->device->CreateTexture(w, h, 1,
            0, (D3DFORMAT)_al_pixel_format_to_d3d(system_format), D3DPOOL_SYSTEMMEM,
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
            0, (D3DFORMAT)_al_pixel_format_to_d3d(video_format), D3DPOOL_MANAGED,
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
   ALLEGRO_BITMAP_EXTRA_D3D *extra;
   D3DSURFACE_DESC desc;
   D3DLOCKED_RECT surf_locked_rect;
   D3DLOCKED_RECT sys_locked_rect;
   unsigned int y;
   ASSERT(!_al_pixel_format_is_compressed(format));

   if (surface->GetDesc(&desc) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_bitmap_from_surface: GetDesc failed.\n");
      return NULL;
   }

   if (surface->LockRect(&surf_locked_rect, 0, D3DLOCK_READONLY) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_bitmap_from_surface: LockRect failed.\n");
      return NULL;
   }

   bitmap = _al_create_bitmap_params(al_get_current_display(),
      desc.Width, desc.Height, format, flags, 0, 0);
   if (!bitmap) {
      surface->UnlockRect();
      return NULL;
   }

   extra = get_extra(bitmap);

   if (extra->system_texture->LockRect(0, &sys_locked_rect, 0, 0) != D3D_OK) {
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
   extra->system_texture->UnlockRect(0);


   if (extra->display->device->UpdateTexture(
         (IDirect3DBaseTexture9 *)extra->system_texture,
         (IDirect3DBaseTexture9 *)extra->video_texture) != D3D_OK) {
      ALLEGRO_ERROR("d3d_create_bitmap_from_surface: Couldn't update texture.\n");
   }

   return bitmap;
}

/* Copies video texture to system texture and bitmap->memory */
static bool _al_d3d_sync_bitmap(ALLEGRO_BITMAP *dest)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_dest;
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

   d3d_dest = get_extra(dest);

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
      ALLEGRO_ERROR("_al_d3d_sync_bitmap: GetSurfaceLevel failed while updating system texture.\n");
      ok = false;
   }

   if (ok && d3d_dest->video_texture->GetSurfaceLevel(
         0, &video_texture_surface) != D3D_OK) {
      ALLEGRO_ERROR("_al_d3d_sync_bitmap: GetSurfaceLevel failed while updating system texture.\n");
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

// Copy texture to system memory
static void d3d_backup_dirty_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY *display = bitmap->_display;
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)display;

   if (d3d_display->device_lost)
      return;

   if (!_al_d3d_render_to_texture_supported())
      return;

   al_lock_mutex(_al_d3d_lost_device_mutex);

   ALLEGRO_BITMAP_EXTRA_D3D *extra = get_extra(bitmap);
   int bitmap_flags = al_get_bitmap_flags(bitmap);
   if (!(
      (bitmap_flags & ALLEGRO_MEMORY_BITMAP) ||
      (bitmap_flags & ALLEGRO_NO_PRESERVE_TEXTURE) ||
      !bitmap->dirty ||
      extra->is_backbuffer ||
      bitmap->parent
      ))
   {
      if (_al_pixel_format_is_compressed(al_get_bitmap_format(bitmap)))
         d3d_sync_bitmap_memory(bitmap);
      else
         _al_d3d_sync_bitmap(bitmap);
   }

   bitmap->dirty = false;

   al_unlock_mutex(_al_d3d_lost_device_mutex);
}

/*
 * Called after the resize is done.
 */
bool _al_d3d_recreate_bitmap_textures(ALLEGRO_DISPLAY_D3D *disp)
{
   unsigned int i;
   ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY *)disp;

   for (i = 0; i < display->bitmaps._size; i++) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&display->bitmaps, i);
      ALLEGRO_BITMAP *bmp = *bptr;
      ALLEGRO_BITMAP_EXTRA_D3D *extra = get_extra(bmp);

      if ((void *)_al_get_bitmap_display(bmp) == (void *)disp) {
         int block_width =
            al_get_pixel_block_width(al_get_bitmap_format(bmp));
         int block_height =
            al_get_pixel_block_height(al_get_bitmap_format(bmp));
         if (!d3d_create_textures(disp, extra->texture_w,
            extra->texture_h,
            al_get_bitmap_flags(bmp),
            &extra->video_texture,
            &extra->system_texture,
            al_get_bitmap_format(bmp),
            extra->system_format))
            return false;
         d3d_do_upload(bmp, 0, 0,
            _al_get_least_multiple(bmp->w, block_width),
            _al_get_least_multiple(bmp->h, block_height), true);
      }
   }

   return true;
}

/*
 * Refresh the texture memory. This must be done after a device is
 * lost or after it is reset.
 */
void _al_d3d_refresh_texture_memory(ALLEGRO_DISPLAY *display)
{
   unsigned int i;

   /* Refresh video hardware textures */
   for (i = 0; i < display->bitmaps._size; i++) {
      ALLEGRO_BITMAP **bptr = (ALLEGRO_BITMAP **)_al_vector_ref(&display->bitmaps, i);
      ALLEGRO_BITMAP *bmp = *bptr;
      ALLEGRO_BITMAP_EXTRA_D3D *extra = get_extra(bmp);
      ALLEGRO_DISPLAY_D3D *bmps_display =
         (ALLEGRO_DISPLAY_D3D *)_al_get_bitmap_display(bmp);
      int bitmap_flags = al_get_bitmap_flags(bmp);

      if ((bitmap_flags & ALLEGRO_MEMORY_BITMAP) || bmp->parent) {
         continue;
      }

      d3d_create_textures(bmps_display, extra->texture_w,
         extra->texture_h, bitmap_flags,
         &extra->video_texture, /*&bmp->system_texture*/0, al_get_bitmap_format(bmp), 0);
      if (!(bitmap_flags & ALLEGRO_NO_PRESERVE_TEXTURE)) {
         int block_width = al_get_pixel_block_width(al_get_bitmap_format(bmp));
         int block_height = al_get_pixel_block_height(al_get_bitmap_format(bmp));
         d3d_sync_bitmap_texture(bmp,
            0, 0,
            _al_get_least_multiple(bmp->w, block_width),
            _al_get_least_multiple(bmp->h, block_height));
         if (_al_d3d_render_to_texture_supported()
               && !_al_pixel_format_is_compressed(al_get_bitmap_format(bmp))) {
            extra->display->device->UpdateTexture(
               (IDirect3DBaseTexture9 *)extra->system_texture,
               (IDirect3DBaseTexture9 *)extra->video_texture);
         }
      }
   }
}

static bool d3d_upload_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   ALLEGRO_SYSTEM *system = al_get_system_driver();
   int bitmap_format = al_get_bitmap_format(bitmap);
   int system_format = d3d_bmp->system_format;
   int block_width = al_get_pixel_block_width(bitmap_format);
   int block_height = al_get_pixel_block_height(bitmap_format);
   int w = _al_get_least_multiple(bitmap->w, block_width);
   int h = _al_get_least_multiple(bitmap->h, block_height);

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
         // Must be square
         int max = _ALLEGRO_MAX(w, h);
         d3d_bmp->texture_w = max;
         d3d_bmp->texture_h = max;
      }
      else {
         // Must be POW2
         d3d_bmp->texture_w = pot(w);
         d3d_bmp->texture_h = pot(h);
      }

      // Some cards/drivers don't like small textures
      if ((int)d3d_bmp->texture_w < system->min_bitmap_size) d3d_bmp->texture_w = system->min_bitmap_size;
      if ((int)d3d_bmp->texture_h < system->min_bitmap_size) d3d_bmp->texture_h = system->min_bitmap_size;

      ASSERT(d3d_bmp->texture_w % block_width == 0);
      ASSERT(d3d_bmp->texture_h % block_height == 0);

      if (d3d_bmp->video_texture == 0)
         if (!d3d_create_textures(d3d_bmp->display,
               d3d_bmp->texture_w,
               d3d_bmp->texture_h,
               al_get_bitmap_flags(bitmap),
               &d3d_bmp->video_texture,
               &d3d_bmp->system_texture,
               bitmap_format,
               system_format)) {
            return false;
         }

      d3d_bmp->initialized = true;
   }

   d3d_do_upload(bitmap, 0, 0, w, h, false);

   return true;
}

static void d3d_draw_bitmap_region(
   ALLEGRO_BITMAP *src,
   ALLEGRO_COLOR tint,
   float sx, float sy, float sw, float sh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_dest = get_extra(dest);
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_src = get_extra(src);

   ASSERT(src->parent == NULL);

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
   if (dest->parent) {
      dest = dest->parent;
      d3d_dest = get_extra(dest);
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
         al_get_bitmap_format(src),
         al_get_bitmap_flags(src)
      );
      if (tmp_bmp) {
         d3d_draw_bitmap_region(tmp_bmp, tint,
            sx, sy, sw, sh, flags);
         al_destroy_bitmap(tmp_bmp);
         if (desc.MultiSampleType != D3DMULTISAMPLE_NONE) {
            surface->Release();
         }
      }
      return;
   }

   _al_d3d_set_blender(d3d_dest->display);

   ALLEGRO_DISPLAY* al_disp = (ALLEGRO_DISPLAY*)d3d_dest->display;
   if (al_disp->use_legacy_drawing_api) {
      d3d_draw_textured_quad(
         d3d_dest->display, src, tint,
         sx, sy, sw, sh, flags);
   }
   else {
      d3d_draw_textured_quad_new(
         al_disp, src, tint,
         sx, sy, sw, sh, flags);
   }
}

static ALLEGRO_LOCKED_REGION *d3d_lock_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, int format,
   int flags)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   int bitmap_format = al_get_bitmap_format(bitmap);
   int system_format = d3d_bmp->system_format;

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
      ALLEGRO_DISPLAY_D3D *d3d_disp =
         (ALLEGRO_DISPLAY_D3D *)_al_get_bitmap_display(bitmap);
      if (d3d_disp->render_target->LockRect(&d3d_bmp->locked_rect, &rect, Flags) != D3D_OK) {
         ALLEGRO_ERROR("LockRect failed in d3d_lock_region.\n");
         return NULL;
      }
   }
   else {
      LPDIRECT3DTEXTURE9 texture;
      if (_al_pixel_format_is_compressed(bitmap_format)) {
         if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
            if(!convert_compressed(
                  d3d_bmp->system_texture, d3d_bmp->video_texture,
                  x, y, w, h)) {
               ALLEGRO_ERROR("Could not decompress.\n");
               return NULL;
            }
         }
         texture = d3d_bmp->system_texture;
      }
      else if (_al_d3d_render_to_texture_supported()) {
         /*
          * Sync bitmap->memory with texture
          */
         bitmap->locked = false;
         if (!(flags & ALLEGRO_LOCK_WRITEONLY) && !_al_d3d_sync_bitmap(bitmap)) {
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

   if (format == ALLEGRO_PIXEL_FORMAT_ANY || system_format == format || system_format == f) {
      bitmap->locked_region.data = d3d_bmp->locked_rect.pBits;
      bitmap->locked_region.format = system_format;
      bitmap->locked_region.pitch = d3d_bmp->locked_rect.Pitch;
      bitmap->locked_region.pixel_size = al_get_pixel_size(system_format);
   }
   else {
      bitmap->locked_region.pitch = al_get_pixel_size(f) * w;
      bitmap->locked_region.data = al_malloc(bitmap->locked_region.pitch*h);
      bitmap->locked_region.format = f;
      bitmap->locked_region.pixel_size = al_get_pixel_size(f);
      if (!(bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY)) {
         _al_convert_bitmap_data(
            d3d_bmp->locked_rect.pBits, system_format, d3d_bmp->locked_rect.Pitch,
            bitmap->locked_region.data, f, bitmap->locked_region.pitch,
            0, 0, 0, 0, w, h);
      }
   }

   return &bitmap->locked_region;
}

static void d3d_unlock_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   int system_format = d3d_bmp->system_format;

   if (bitmap->locked_region.format != 0 && bitmap->locked_region.format != system_format) {
      if (!(bitmap->lock_flags & ALLEGRO_LOCK_READONLY)) {
         _al_convert_bitmap_data(
            bitmap->locked_region.data, bitmap->locked_region.format, bitmap->locked_region.pitch,
            d3d_bmp->locked_rect.pBits, system_format, d3d_bmp->locked_rect.Pitch,
            0, 0, 0, 0, bitmap->lock_w, bitmap->lock_h);
      }
      al_free(bitmap->locked_region.data);
   }

   if (d3d_bmp->is_backbuffer) {
      ALLEGRO_DISPLAY_D3D *d3d_disp =
         (ALLEGRO_DISPLAY_D3D *)_al_get_bitmap_display(bitmap);
      d3d_disp->render_target->UnlockRect();
   }
   else {
      LPDIRECT3DTEXTURE9 texture;
      int bitmap_format = al_get_bitmap_format(bitmap);
      bool compressed = _al_pixel_format_is_compressed(bitmap_format);
      if (_al_d3d_render_to_texture_supported() || compressed)
         texture = d3d_bmp->system_texture;
      else
         texture = d3d_bmp->video_texture;
      texture->UnlockRect(0);
      if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY)
         return;

      if (compressed) {
         int block_width = al_get_pixel_block_width(bitmap_format);
         int block_height = al_get_pixel_block_height(bitmap_format);
         int xc = (bitmap->lock_x / block_width) * block_width;
         int yc = (bitmap->lock_y / block_height) * block_height;
         int wc =
            _al_get_least_multiple(bitmap->lock_x + bitmap->lock_w, block_width) - xc;
         int hc =
            _al_get_least_multiple(bitmap->lock_y + bitmap->lock_h, block_height) - yc;
         if(!convert_compressed(
            d3d_bmp->video_texture, d3d_bmp->system_texture, xc, yc, wc, hc)) {
            ALLEGRO_ERROR("Could not compress.\n");
         }
      }
      else {
         d3d_do_upload(bitmap, bitmap->lock_x, bitmap->lock_y,
            bitmap->lock_w, bitmap->lock_h, false);
      }
   }
}


static void d3d_update_clipping_rectangle(ALLEGRO_BITMAP *bitmap)
{
   _al_d3d_set_bitmap_clip(bitmap);
}


static ALLEGRO_LOCKED_REGION *d3d_lock_compressed_region(
   ALLEGRO_BITMAP *bitmap, int x, int y, int w, int h,
   int flags)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   int bitmap_format = al_get_bitmap_format(bitmap);

   if (d3d_bmp->display->device_lost)
      return NULL;

   ASSERT(_al_pixel_format_is_compressed(bitmap_format));

   RECT rect;
   DWORD Flags = flags & ALLEGRO_LOCK_READONLY ? D3DLOCK_READONLY : 0;

   rect.left = x;
   rect.right = x + w;
   rect.top = y;
   rect.bottom = y + h;

   if (d3d_bmp->video_texture->LockRect(0, &d3d_bmp->locked_rect, &rect, Flags) != D3D_OK) {
      ALLEGRO_ERROR("LockRect failed in d3d_lock_region.\n");
      return NULL;
   }

   bitmap->locked_region.data = d3d_bmp->locked_rect.pBits;
   bitmap->locked_region.format = bitmap_format;
   bitmap->locked_region.pitch = d3d_bmp->locked_rect.Pitch;
   bitmap->locked_region.pixel_size = al_get_pixel_block_size(bitmap_format);

   return &bitmap->locked_region;
}


static void d3d_unlock_compressed_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_EXTRA_D3D *d3d_bmp = get_extra(bitmap);
   ASSERT(_al_pixel_format_is_compressed(al_get_bitmap_format(bitmap)));

   d3d_bmp->video_texture->UnlockRect(0);
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
   vt->destroy_bitmap = _al_d3d_destroy_bitmap;
   vt->lock_region = d3d_lock_region;
   vt->unlock_region = d3d_unlock_region;
   vt->lock_compressed_region = d3d_lock_compressed_region;
   vt->unlock_compressed_region = d3d_unlock_compressed_region;
   vt->update_clipping_rectangle = d3d_update_clipping_rectangle;
   vt->backup_dirty_bitmap = d3d_backup_dirty_bitmap;

   return vt;
}

