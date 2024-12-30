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

#define ALLEGRO_INTERNAL_UNSTABLE

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

ALLEGRO_DEBUG_CHANNEL("d3d_primitives")

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

static bool is_legacy_card(void)
{
   if (!know_card_type) {
      ALLEGRO_CONFIG* sys_cfg = al_get_system_config();
      const char* detection_setting = al_get_config_value(sys_cfg, "graphics", "prim_d3d_legacy_detection");
      detection_setting = detection_setting ? detection_setting : "default";
      if (strcmp(detection_setting, "default") == 0) {
         D3DCAPS9 caps;
         LPDIRECT3DDEVICE9 device = al_get_d3d_device(al_get_current_display());
         device->GetDeviceCaps(&caps);
         if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
            legacy_card = true;
      } else if(strcmp(detection_setting, "force_legacy") == 0) {
         legacy_card = true;
      } else if(strcmp(detection_setting, "force_modern") == 0) {
          legacy_card = false;
      } else {
         ALLEGRO_WARN("Invalid setting for prim_d3d_legacy_detection.\n");
         legacy_card = false;
      }
      if (legacy_card) {
         ALLEGRO_WARN("Your GPU is considered legacy! Some of the features of the primitives addon will be slower/disabled.\n");
      }
      know_card_type = true;
   }
   return legacy_card;
}

struct LEGACY_VERTEX
{
   float x, y, z;
   DWORD color;
   float u, v;
};

static uint8_t* legacy_buffer;
static size_t legacy_buffer_size = 0;
#define A5V_FVF (D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE4(1))
#define A5V_LEGACY_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct DISPLAY_LOCAL_DATA
{
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DVERTEXSHADER9 shader;
   ALLEGRO_INDEX_BUFFER* loop_index_buffer;
};

static DISPLAY_LOCAL_DATA* display_local_data;
static int display_local_data_size = 0;

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

   for(ii = 0; ii < display_local_data_size; ii++)
   {
      if(display_local_data[ii].device == device) {
         display_local_data[ii].shader->Release();
         display_local_data[ii] = display_local_data[display_local_data_size - 1];
         display_local_data_size--;
         break;
      }
   }

   al_unlock_mutex(d3d_mutex);
}

static DISPLAY_LOCAL_DATA get_display_local_data(ALLEGRO_DISPLAY* display)
{
   LPDIRECT3DDEVICE9 device = al_get_d3d_device(display);
   DISPLAY_LOCAL_DATA ret;
   bool create_new = false;

   /*
    * Lock the mutex so that the entries are not messed up by a
    * display blowing up/being created
    */
   al_lock_mutex(d3d_mutex);

   if (display_local_data_size == 0) {
      display_local_data = (DISPLAY_LOCAL_DATA*)al_malloc(sizeof(DISPLAY_LOCAL_DATA));
      display_local_data_size = 1;
      create_new = true;
   }
   else if (display_local_data[0].device != device) {
      int ii;
      bool found = false;
      for(ii = 1; ii < display_local_data_size; ii++)
      {
         if(display_local_data[ii].device == device) {
            /*
             * Move this entry to the front, so the search goes faster
             * next time - presumably the al_draw_prim will be called
             * several times for each display before switching again
             */
            DISPLAY_LOCAL_DATA t = display_local_data[0];
            display_local_data[0] = display_local_data[ii];
            display_local_data[ii] = t;

            found = true;

            break;
         }
      }

      if (!found) {
         DISPLAY_LOCAL_DATA t = display_local_data[0];

         display_local_data_size++;
         display_local_data = (DISPLAY_LOCAL_DATA *)al_realloc(display_local_data, sizeof(DISPLAY_LOCAL_DATA) * display_local_data_size);

         display_local_data[display_local_data_size - 1] = t;
         create_new = true;
      }
   }

   if (create_new) {
      int initial_indices[2] = {0, 0};
      display_local_data[0].device = device;
      display_local_data[0].shader = (LPDIRECT3DVERTEXSHADER9)_al_create_default_primitives_shader(device);
      display_local_data[0].loop_index_buffer = al_create_index_buffer(sizeof(int), initial_indices, 2, 0);

      _al_add_display_invalidated_callback(display, &display_invalidated);
   }

   ret = display_local_data[0];

   al_unlock_mutex(d3d_mutex);

   return ret;
}

static void destroy_display_local_data(void)
{
   int ii;
   for(ii = 0; ii < display_local_data_size; ii++)
   {
      display_local_data[ii].shader->Release();
      al_destroy_index_buffer(display_local_data[ii].loop_index_buffer);
   }
   display_local_data_size = 0;
   al_free(display_local_data);
   display_local_data = NULL;
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

   destroy_display_local_data();

   legacy_card = false;
   know_card_type = false;
   legacy_buffer_size = 0;
   #endif
}

#ifdef ALLEGRO_CFG_D3D

static void* convert_to_legacy_vertices(const void* vtxs, int num_vertices, const int* indices, bool loop, bool pp)
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

   needed_size = num_needed_vertices * (pp ? sizeof(ALLEGRO_VERTEX) : sizeof(LEGACY_VERTEX));

   if(legacy_buffer == 0) {
      legacy_buffer = (uint8_t *)al_malloc(needed_size);
      legacy_buffer_size = needed_size;
   } else if (needed_size > legacy_buffer_size) {
      size_t new_size = needed_size * 1.5;
      legacy_buffer = (uint8_t *)al_realloc(legacy_buffer, new_size);
      legacy_buffer_size = new_size;
   }

   if (pp) {
      ALLEGRO_VERTEX *buffer = (ALLEGRO_VERTEX *)legacy_buffer;
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
      LEGACY_VERTEX *buffer = (LEGACY_VERTEX *)legacy_buffer;
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

   return legacy_buffer;
}

struct D3D_STATE
{
   DWORD old_wrap_state[2];
   DWORD old_ttf_state;
   IDirect3DVertexShader9* old_vtx_shader;
};

static D3D_STATE setup_state(LPDIRECT3DDEVICE9 device, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, DISPLAY_LOCAL_DATA data)
{
   D3D_STATE state;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;

   if (!(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
      IDirect3DPixelShader9* old_pix_shader;
      device->GetVertexShader(&state.old_vtx_shader);
      device->GetPixelShader(&old_pix_shader);

      if(!old_pix_shader) {
         _al_d3d_set_blender(d3d_disp);
      }

      if(!state.old_vtx_shader) {
         /* Prepare the default shader */
         if(!is_legacy_card()) {
            if(decl)
               _al_setup_primitives_shader(device, decl);
            else
               _al_setup_default_primitives_shader(device, data.shader);
         }
      }
   }
   else {
      state.old_vtx_shader = NULL;
      _al_d3d_set_blender(d3d_disp);
   }

   /* Set the vertex declarations */
   if(is_legacy_card() && !(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
      device->SetFVF(A5V_LEGACY_FVF);
   } else {
      if(decl) {
         device->SetVertexDeclaration((IDirect3DVertexDeclaration9*)decl->d3d_decl);
      } else {
         device->SetFVF(A5V_FVF);
      }
   }

   if(!state.old_vtx_shader || (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
      /* Set up the texture */
      if (texture) {
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


         if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
            d3d_disp->effect->SetMatrix(ALLEGRO_SHADER_VAR_TEX_MATRIX, (D3DXMATRIX *)mat);
            d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX_MATRIX, true);
            d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, true);
            d3d_disp->effect->SetTexture(ALLEGRO_SHADER_VAR_TEX, d3d_texture);
#endif
         }
         else {
            if(is_legacy_card()) {
               device->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &state.old_ttf_state);
               device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
               device->SetTransform(D3DTS_TEXTURE0, (D3DMATRIX *)&mat);
            } else {
               _al_set_texture_matrix(device, mat[0]);
            }
         }

         device->SetTexture(0, d3d_texture);

      } else {
         /* Don't unbind the texture here if shaders are used, since the user may
          * have set the 0'th texture unit manually via the shader API. */
         if (!(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
            device->SetTexture(0, NULL);
         }
      }
   }

   if (texture) {
      device->GetSamplerState(0, D3DSAMP_ADDRESSU, &state.old_wrap_state[0]);
      device->GetSamplerState(0, D3DSAMP_ADDRESSV, &state.old_wrap_state[1]);
      _al_set_d3d_sampler_state(device, 0, texture, true);
   }

   return state;
}

static void revert_state(D3D_STATE state, LPDIRECT3DDEVICE9 device, ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture)
{
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;
   (void)d3d_disp;
#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->effect->End();
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX_MATRIX, false);
      d3d_disp->effect->SetBool(ALLEGRO_SHADER_VAR_USE_TEX, false);
   }
#endif

   if (texture) {
      device->SetSamplerState(0, D3DSAMP_ADDRESSU, state.old_wrap_state[0]);
      device->SetSamplerState(0, D3DSAMP_ADDRESSV, state.old_wrap_state[1]);
   }

   if(!state.old_vtx_shader && is_legacy_card() && texture) {
      device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, state.old_ttf_state);
   }

   if (!(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
      if(!state.old_vtx_shader)
         device->SetVertexShader(0);
   }
}

static int draw_prim_raw(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
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
   DISPLAY_LOCAL_DATA data;
   (void)d3d_disp;

   if (al_is_d3d_device_lost(disp)) {
      return 0;
   }

   if (is_legacy_card() && !(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
      stride = (int)sizeof(LEGACY_VERTEX);
   }
   else {
      stride = (decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX));
   }

   /* Check for early exit */
   if((is_legacy_card() && decl) || (decl && decl->d3d_decl == 0)) {
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

   data = get_display_local_data(disp);

   state = setup_state(device, decl, target, texture, data);

   /* Convert vertices for legacy cards */
   if(is_legacy_card()) {
      al_lock_mutex(d3d_mutex);
      vtx = convert_to_legacy_vertices(vtx, num_vtx, indices, type == ALLEGRO_PRIM_LINE_LOOP,
         disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE);
   }

#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      d3d_disp->effect->Begin(&required_passes, 0);
   }
#endif

   for (i = 0; i < required_passes; i++) {
#ifdef ALLEGRO_CFG_SHADER_HLSL
      if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
         d3d_disp->effect->BeginPass(i);
      }
#endif

      if (!indices || is_legacy_card())
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
               if(!is_legacy_card()) {
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
      if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
         d3d_disp->effect->EndPass();
      }
#endif
   }

   if(is_legacy_card())
      al_unlock_mutex(d3d_mutex);

   revert_state(state, device, target, texture);

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

#ifdef ALLEGRO_CFG_D3D
static int draw_buffer_raw(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
   int num_primitives = 0;
   int num_vtx = end - start;
   LPDIRECT3DDEVICE9 device;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)disp;
   UINT required_passes = 1;
   unsigned int i;
   D3D_STATE state;
   DISPLAY_LOCAL_DATA data;
   (void)d3d_disp;

   if (al_is_d3d_device_lost(disp)) {
      return 0;
   }

   /* Check for early exit for legacy cards */
   if (vertex_buffer->decl && vertex_buffer->decl->d3d_decl == 0) {
      return _al_draw_buffer_common_soft(vertex_buffer, texture, index_buffer, start, end, type);
   }

   device = al_get_d3d_device(disp);

   data = get_display_local_data(disp);

   state = setup_state(device, vertex_buffer->decl, target, texture, data);

   device->SetStreamSource(0, (IDirect3DVertexBuffer9*)vertex_buffer->common.handle, 0, vertex_buffer->decl ? vertex_buffer->decl->stride : (int)sizeof(ALLEGRO_VERTEX));

   if (index_buffer) {
      device->SetIndices((IDirect3DIndexBuffer9*)index_buffer->common.handle);
   }

#ifdef ALLEGRO_CFG_SHADER_HLSL
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
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
               int* indices;
               num_primitives = num_vtx - 1;
               device->DrawPrimitive(D3DPT_LINESTRIP, start, num_primitives);

               if (data.loop_index_buffer) {
                  al_lock_mutex(d3d_mutex);
                  indices = (int*)al_lock_index_buffer(data.loop_index_buffer, 0, 2, ALLEGRO_LOCK_WRITEONLY);
                  ASSERT(indices);
                  indices[0] = start;
                  indices[1] = start + num_vtx - 1;
                  al_unlock_index_buffer(data.loop_index_buffer);
                  device->SetIndices((IDirect3DIndexBuffer9*)data.loop_index_buffer->common.handle);
                  device->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, al_get_vertex_buffer_size(vertex_buffer), 0, 1);
                  al_unlock_mutex(d3d_mutex);
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
         int vbuff_size = al_get_vertex_buffer_size(vertex_buffer);
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

   if (is_legacy_card())
      al_unlock_mutex(d3d_mutex);

   revert_state(state, device, target, texture);

   return num_primitives;
}
#endif

int _al_draw_vertex_buffer_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_D3D
   return draw_buffer_raw(target, texture, vertex_buffer, NULL, start, end, type);
#else
   (void)target;
   (void)texture;
   (void)vertex_buffer;
   (void)start;
   (void)end;
   (void)type;

   return 0;
#endif
}

int _al_draw_indexed_buffer_directx(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_D3D
   return draw_buffer_raw(target, texture, vertex_buffer, index_buffer, start, end, type);
#else
   (void)target;
   (void)texture;
   (void)vertex_buffer;
   (void)index_buffer;
   (void)start;
   (void)end;
   (void)type;

   return 0;
#endif
}

#ifdef ALLEGRO_CFG_D3D
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
#endif

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

      device->GetDeviceCaps(&caps);
      if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
         ret->d3d_decl = 0;
      } else {
         int i;
         e = &ret->elements[ALLEGRO_PRIM_POSITION];
         if(e->attribute) {
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = convert_storage(e->storage);
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_POSITION;
            d3delements[idx].UsageIndex = 0;
            idx++;
         }

         e = &ret->elements[ALLEGRO_PRIM_TEX_COORD];
         if(!e->attribute)
            e = &ret->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
         if(e->attribute) {
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = convert_storage(e->storage);
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
            d3delements[idx].UsageIndex = 0;
            idx++;
         }

         e = &ret->elements[ALLEGRO_PRIM_COLOR_ATTR];
         if(e->attribute) {
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = D3DDECLTYPE_FLOAT4;
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
            d3delements[idx].UsageIndex = 1;
            idx++;
         }

         for (i = 0; i < _ALLEGRO_PRIM_MAX_USER_ATTR; i++) {
            e = &ret->elements[ALLEGRO_PRIM_USER_ATTR + i];
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

         device->CreateVertexDeclaration(d3delements, (IDirect3DVertexDeclaration9**)&ret->d3d_decl);
      }

      _al_create_primitives_shader(device, ret);
   }
#else
   (void)display;
   ret->d3d_decl = 0;
#endif
}

bool _al_create_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int flags)
{
#ifdef ALLEGRO_CFG_D3D
   LPDIRECT3DDEVICE9 device;
   IDirect3DVertexBuffer9* d3d_vbuff;
   DWORD fvf = A5V_FVF;
   int stride = buf->decl ? buf->decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   HRESULT res;
   void* locked_memory;

   /* There's just no point */
   if (is_legacy_card()) {
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
#else
   (void)buf;
   (void)initial_data;
   (void)num_vertices;
   (void)flags;

   return false;
#endif
}

bool _al_create_index_buffer_directx(ALLEGRO_INDEX_BUFFER* buf, const void* initial_data, size_t num_indices, int flags)
{
#ifdef ALLEGRO_CFG_D3D
   LPDIRECT3DDEVICE9 device;
   IDirect3DIndexBuffer9* d3d_ibuff;
   HRESULT res;
   void* locked_memory;

   /* There's just no point */
   if (is_legacy_card()) {
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
#else
   (void)buf;
   (void)initial_data;
   (void)num_indices;
   (void)flags;

   return false;
#endif
}

void _al_destroy_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_D3D
   ((IDirect3DVertexBuffer9*)buf->common.handle)->Release();
#else
   (void)buf;
#endif
}

void _al_destroy_index_buffer_directx(ALLEGRO_INDEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_D3D
   ((IDirect3DIndexBuffer9*)buf->common.handle)->Release();
#else
   (void)buf;
#endif
}

void* _al_lock_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_D3D
   DWORD flags = buf->common.lock_flags == ALLEGRO_LOCK_READONLY ? D3DLOCK_READONLY : 0;
   HRESULT res;

   res = ((IDirect3DVertexBuffer9*)buf->common.handle)->Lock((UINT)buf->common.lock_offset, (UINT)buf->common.lock_length, &buf->common.locked_memory, flags);
   if (res != D3D_OK) {
      ALLEGRO_WARN("Locking vertex buffer failed: %ld.\n", res);
      return 0;
   }

   return buf->common.locked_memory;
#else
   (void)buf;

   return 0;
#endif
}

void* _al_lock_index_buffer_directx(ALLEGRO_INDEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_D3D
   DWORD flags = buf->common.lock_flags == ALLEGRO_LOCK_READONLY ? D3DLOCK_READONLY : 0;
   HRESULT res;

   res = ((IDirect3DIndexBuffer9*)buf->common.handle)->Lock((UINT)buf->common.lock_offset, (UINT)buf->common.lock_length, &buf->common.locked_memory, flags);

   if (res != D3D_OK) {
      ALLEGRO_WARN("Locking index buffer failed: %ld.\n", res);
      return 0;
   }

   return buf->common.locked_memory;
#else
   (void)buf;

   return 0;
#endif
}

void _al_unlock_vertex_buffer_directx(ALLEGRO_VERTEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_D3D
   ((IDirect3DVertexBuffer9*)buf->common.handle)->Unlock();
#else
   (void)buf;
#endif
}

void _al_unlock_index_buffer_directx(ALLEGRO_INDEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_D3D
   ((IDirect3DIndexBuffer9*)buf->common.handle)->Unlock();
#else
   (void)buf;
#endif
}
