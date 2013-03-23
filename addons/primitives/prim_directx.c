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
 *      DirectX implementation of some of the primitive routines.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/internal/aintern_display.h"

#include "allegro5/platform/alplatf.h"

#ifdef ALLEGRO_CFG_D3D

#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern_direct3d.h"

static ALLEGRO_MUTEX *d3d_mutex;
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
static bool legacy_card = false;
static bool know_card_type = false;

typedef struct LEGACY_VERTEX
{
   float x, y, z;
   DWORD color;
   float u, v;
} LEGACY_VERTEX;

static LEGACY_VERTEX* legacy_buffer;
static int legacy_buffer_size = 0;
#define A5V_FVF (D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE4(1))
#define A5V_LEGACY_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

typedef struct SHADER_ENTRY
{
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DVERTEXSHADER9 shader;
} SHADER_ENTRY;

static SHADER_ENTRY* shader_entries;
static int num_shader_entries = 0;

static void display_invalidated(ALLEGRO_DISPLAY* display)
{
   int ii;
   LPDIRECT3DDEVICE9 device = al_get_d3d_device(display);
   /*
    * If there is no mutex, the addon has been shutdown earlier
    */
   if(!d3d_mutex)
      return;

   al_lock_mutex(d3d_mutex);

   for(ii = 0; ii < num_shader_entries; ii++)
   {
      if(shader_entries[ii].device == device) {
         IDirect3DVertexShader9_Release(shader_entries[ii].shader);
         shader_entries[ii] = shader_entries[num_shader_entries - 1];
         num_shader_entries--;
         break;
      }
   }

   al_unlock_mutex(d3d_mutex);
}

static void setup_default_shader(ALLEGRO_DISPLAY* display)
{
   LPDIRECT3DDEVICE9 device = al_get_d3d_device(display);

   /*
    * Lock the mutex so that the entries are not messed up by a
    * display blowing up/being created
    */
   al_lock_mutex(d3d_mutex);

   if(num_shader_entries == 0) {
      shader_entries = al_malloc(sizeof(SHADER_ENTRY));
      num_shader_entries = 1;

      shader_entries[0].device = device;
      shader_entries[0].shader = _al_create_default_shader(device);

      _al_set_display_invalidated_callback(display, &display_invalidated);
   }

   if(shader_entries[0].device != device) {
      int ii;
      bool found = false;
      for(ii = 1; ii < num_shader_entries; ii++)
      {
         if(shader_entries[ii].device == device) {
            /*
             * Move this entry to the front, so the search goes faster
             * next time - presumably the al_draw_prim will be called
             * several times for each display before switching again
             */
            SHADER_ENTRY t = shader_entries[0];
            shader_entries[0] = shader_entries[ii];
            shader_entries[ii] = t;

            found = true;

            break;
         }
      }

      if(!found) {
         SHADER_ENTRY t = shader_entries[0];

         num_shader_entries++;
         shader_entries = al_realloc(shader_entries, sizeof(SHADER_ENTRY) * num_shader_entries);

         shader_entries[num_shader_entries - 1] = t;
         shader_entries[0].device = device;
         shader_entries[0].shader = _al_create_default_shader(device);

         _al_set_display_invalidated_callback(display, &display_invalidated);
      }
   }

   _al_setup_default_shader(device, shader_entries[0].shader);

   al_unlock_mutex(d3d_mutex);
}

static void destroy_default_shaders(void)
{
   int ii;
   for(ii = 0; ii < num_shader_entries; ii++)
   {
      IDirect3DVertexShader9_Release(shader_entries[ii].shader);
   }
   num_shader_entries = 0;
   al_free(shader_entries);
   shader_entries = NULL;
}

#endif

bool _al_init_d3d_driver(void)
{
   #ifdef ALLEGRO_CFG_D3D
   d3d_mutex = al_create_mutex();
   #endif
   return true;
}

void _al_shutdown_d3d_driver(void)
{
   #ifdef ALLEGRO_CFG_D3D
   al_destroy_mutex(d3d_mutex);
   al_free(legacy_buffer);
   d3d_mutex = NULL;
   legacy_buffer = NULL;

   destroy_default_shaders();

   legacy_card = false;
   know_card_type = false;
   legacy_buffer_size = 0;
   #endif
}

#ifdef ALLEGRO_CFG_D3D

static void* convert_to_legacy_vertices(const void* vtxs, int num_vertices, const int* indices, bool loop)
{
   const ALLEGRO_VERTEX* vtx = vtxs;
   int ii;
   int num_needed_vertices = num_vertices;

   if(loop)
      num_needed_vertices++;

   if(legacy_buffer == 0) {
      legacy_buffer = al_malloc(num_needed_vertices * sizeof(LEGACY_VERTEX));
      legacy_buffer_size = num_needed_vertices;
   } else if (num_needed_vertices > legacy_buffer_size) {
      legacy_buffer = al_realloc(legacy_buffer, num_needed_vertices * 1.5 * sizeof(LEGACY_VERTEX));
      legacy_buffer_size = num_needed_vertices * 1.5;
   }
   for(ii = 0; ii < num_vertices; ii++) {
      ALLEGRO_VERTEX vertex;

      if(indices)
         vertex = vtx[indices[ii]];
      else
         vertex = vtx[ii];

      legacy_buffer[ii].x = vertex.x;
      legacy_buffer[ii].y = vertex.y;
      legacy_buffer[ii].z = vertex.z;

      legacy_buffer[ii].u = vertex.u;
      legacy_buffer[ii].v = vertex.v;

      legacy_buffer[ii].color = D3DCOLOR_COLORVALUE(vertex.color.r, vertex.color.g, vertex.color.b, vertex.color.a);
   }

   if(loop)
      legacy_buffer[num_vertices] = legacy_buffer[0];

   return legacy_buffer;
}


static int draw_prim_raw(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
   const void* vtx, const ALLEGRO_VERTEX_DECL* decl,
   const int* indices, int num_vtx, int type)
{
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   int num_primitives = 0;

   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DBASETEXTURE9 d3d_texture;
   DWORD old_wrap_state[2];
   DWORD old_ttf_state;
   int min_idx = 0, max_idx = num_vtx - 1;
   IDirect3DVertexShader9* old_vtx_shader;
   IDirect3DPixelShader9* old_pix_shader;

   if (al_is_d3d_device_lost(target->display)) {
      return 0;
   }

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
   }

   device = al_get_d3d_device(target->display);
   IDirect3DDevice9_GetVertexShader(device, &old_vtx_shader);
   IDirect3DDevice9_GetPixelShader(device, &old_pix_shader);

   if(!know_card_type) {
      D3DCAPS9 caps;
      IDirect3DDevice9_GetDeviceCaps(device, &caps);
      if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
         legacy_card = true;
      know_card_type = true;
   }

   /* Check for early exit */
   if((legacy_card && decl) || (decl && decl->d3d_decl == 0)) {
      if(!indices)
         return _al_draw_prim_soft(texture, vtx, decl, 0, num_vtx, type);
      else
         return _al_draw_prim_indexed_soft(texture, vtx, decl, indices, num_vtx, type);
   }

   if(!old_pix_shader) {
      _al_d3d_set_blender((struct ALLEGRO_DISPLAY_D3D *)target->display);
   }

   if(!old_vtx_shader) {
      /* Prepare the default shader */
      if(!legacy_card) {
         if(decl)
            _al_setup_shader(device, decl);
         else
            setup_default_shader(target->display);
      }
   }

   /* Set the vertex declarations */
   if(legacy_card) {
      IDirect3DDevice9_SetFVF(device, A5V_LEGACY_FVF);
      stride = sizeof(LEGACY_VERTEX);
   } else {
      if(decl) {
         IDirect3DDevice9_SetVertexDeclaration(device, (IDirect3DVertexDeclaration9*)decl->d3d_decl);
      } else {
         IDirect3DDevice9_SetFVF(device, A5V_FVF);
      }
   }

   if(!old_vtx_shader) {
      /* Set up the texture */
      if (texture) {
         int tex_x, tex_y;
         D3DSURFACE_DESC desc;
         float mat[4][4] = {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1}
         };
         IDirect3DTexture9_GetLevelDesc(al_get_d3d_video_texture(texture), 0, &desc);

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

         if(legacy_card) {
            IDirect3DDevice9_GetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, &old_ttf_state);
            IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
            IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *)&mat);
         } else {
            _al_set_texture_matrix(device, mat[0]);
         }

         d3d_texture = (LPDIRECT3DBASETEXTURE9)al_get_d3d_video_texture(texture);
         IDirect3DDevice9_SetTexture(device, 0, d3d_texture);
      } else {
         IDirect3DDevice9_SetTexture(device, 0, NULL);
      }
   }

   IDirect3DDevice9_GetSamplerState(device, 0, D3DSAMP_ADDRESSU, &old_wrap_state[0]);
   IDirect3DDevice9_GetSamplerState(device, 0, D3DSAMP_ADDRESSV, &old_wrap_state[1]);

   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

   if (texture) {
      if (texture->flags & ALLEGRO_MIN_LINEAR) {
         IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
      }
      else {
         IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
      }
      if (texture->flags & ALLEGRO_MAG_LINEAR) {
         IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
      }
      else {
         IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
      }
      if (texture->flags & ALLEGRO_MIPMAP) {
         IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
      }
      else {
         IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
      }
   }

   /* Convert vertices for legacy cards */
   if(legacy_card) {
      al_lock_mutex(d3d_mutex);
      vtx = convert_to_legacy_vertices(vtx, num_vtx, indices, type == ALLEGRO_PRIM_LINE_LOOP);
   }

   if(!indices || legacy_card)
   {
      switch (type) {
         case ALLEGRO_PRIM_LINE_LIST: {
            num_primitives = num_vtx / 2;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINELIST, num_primitives, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_LINE_STRIP: {
            num_primitives = num_vtx - 1;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINESTRIP, num_primitives, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_LINE_LOOP: {
            num_primitives = num_vtx - 1;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINESTRIP, num_primitives, vtx, stride);
            if(!legacy_card) {
               int in[2];
               in[0] = 0;
               in[1] = num_vtx-1;

               IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, 0, num_vtx, 1, in, D3DFMT_INDEX32, vtx, stride);
            } else {
               IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINELIST, 1, (char*)vtx + stride * (num_vtx - 1), stride);
            }
            break;
         };
         case ALLEGRO_PRIM_TRIANGLE_LIST: {
            num_primitives = num_vtx / 3;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, num_primitives, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_TRIANGLE_STRIP: {
            num_primitives = num_vtx - 2;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, num_primitives, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_TRIANGLE_FAN: {
            num_primitives = num_vtx - 2;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLEFAN, num_primitives, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_POINT_LIST: {
            num_primitives = num_vtx;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, num_primitives, vtx, stride);
            break;
         };
      }
   } else {
      switch (type) {
         case ALLEGRO_PRIM_LINE_LIST: {
            num_primitives = num_vtx / 2;
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, min_idx, max_idx + 1, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_LINE_STRIP: {
            num_primitives = num_vtx - 1;
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINESTRIP, min_idx, max_idx + 1, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_LINE_LOOP: {
            int in[2];
            num_primitives = num_vtx - 1;
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINESTRIP, min_idx, max_idx + 1, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);

            in[0] = indices[0];
            in[1] = indices[num_vtx-1];
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, min_idx, max_idx + 1, 1, in, D3DFMT_INDEX32, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_TRIANGLE_LIST: {
            num_primitives = num_vtx / 3;
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, min_idx, max_idx + 1, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_TRIANGLE_STRIP: {
            num_primitives = num_vtx - 2;
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLESTRIP, min_idx, max_idx + 1, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
            break;
         };
         case ALLEGRO_PRIM_TRIANGLE_FAN: {
            num_primitives = num_vtx - 2;
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLEFAN, min_idx, max_idx + 1, num_primitives, indices, D3DFMT_INDEX32, vtx, stride);
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
                  IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, run_length, (const char*)vtx + start_idx * stride, stride);
                  if(ii != num_vtx - 1)
                     start_idx = indices[ii + 1];
                  run_length = 0;
               }
            }
            break;
         };
      }
   }

   if(legacy_card)
      al_unlock_mutex(d3d_mutex);

   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, old_wrap_state[0]);
   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, old_wrap_state[1]);

   if(!old_vtx_shader && legacy_card && texture) {
      IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, old_ttf_state);
   }

   if(!old_vtx_shader)
      IDirect3DDevice9_SetVertexShader(device, 0);

   return num_primitives;
}

#endif

int _al_draw_prim_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_D3D
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   return draw_prim_raw(target, texture, (const char*)vtxs + start * stride, decl, 0, end - start, type);
#else
   (void)target;
   (void)texture;
   (void)vtxs;
   (void)start;
   (void)end;
   (void)type;
   (void)decl;

   return 0;
#endif
}

int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{
#ifdef ALLEGRO_CFG_D3D
   return draw_prim_raw(target, texture, vtxs, decl, indices, num_vtx, type);
#else
   (void)target;
   (void)texture;
   (void)vtxs;
   (void)indices;
   (void)num_vtx;
   (void)type;
   (void)decl;

   return 0;
#endif
}

void _al_set_d3d_decl(ALLEGRO_DISPLAY* display, ALLEGRO_VERTEX_DECL* ret)
{
#ifdef ALLEGRO_CFG_D3D
   {
      LPDIRECT3DDEVICE9 device;
      D3DVERTEXELEMENT9 d3delements[ALLEGRO_PRIM_ATTR_NUM + 1];
      int idx = 0;
      ALLEGRO_VERTEX_ELEMENT* e;
      D3DCAPS9 caps;

      device = al_get_d3d_device(display);

      IDirect3DDevice9_GetDeviceCaps(device, &caps);
      if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
         ret->d3d_decl = 0;
      } else {
         int color_idx = 0;
         e = &ret->elements[ALLEGRO_PRIM_POSITION];
         if(e->attribute) {
            int type = 0;
            switch(e->storage) {
               case ALLEGRO_PRIM_FLOAT_2:
                  type = D3DDECLTYPE_FLOAT2;
               break;
               case ALLEGRO_PRIM_FLOAT_3:
                  type = D3DDECLTYPE_FLOAT3;
               break;
               case ALLEGRO_PRIM_SHORT_2:
                  type = D3DDECLTYPE_SHORT2;
               break;
               default:
                  ASSERT(0);
            }
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = type;
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_POSITION;
            d3delements[idx].UsageIndex = 0;
            idx++;
         }

         e = &ret->elements[ALLEGRO_PRIM_TEX_COORD];
         if(!e->attribute)
            e = &ret->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
         if(e->attribute) {
            int type = 0;
            switch(e->storage) {
               case ALLEGRO_PRIM_FLOAT_2:
                  type = D3DDECLTYPE_FLOAT2;
               break;
               case ALLEGRO_PRIM_SHORT_2:
                  type = D3DDECLTYPE_SHORT2;
               break;
               default:
                  ASSERT(0);
            }
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = type;
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
            d3delements[idx].UsageIndex = 0;
            idx++;
            color_idx++;
         }

         e = &ret->elements[ALLEGRO_PRIM_COLOR_ATTR];
         if(e->attribute) {
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = D3DDECLTYPE_FLOAT4;
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
            d3delements[idx].UsageIndex = color_idx;
            idx++;
         }

         d3delements[idx].Stream = 0xFF;
         d3delements[idx].Offset = 0;
         d3delements[idx].Type = D3DDECLTYPE_UNUSED;
         d3delements[idx].Method = 0;
         d3delements[idx].Usage = 0;
         d3delements[idx].UsageIndex = 0;

         IDirect3DDevice9_CreateVertexDeclaration(device, d3delements, (IDirect3DVertexDeclaration9**)&ret->d3d_decl);
      }

      _al_create_shader(device, ret);
   }
#else
   (void)display;
   ret->d3d_decl = 0;
#endif
}
