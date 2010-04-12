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
 *      Implementation by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/internal/aintern_prim.h"

#include "allegro5/platform/alplatf.h"

#ifdef ALLEGRO_CFG_D3D

#include "allegro5/allegro_direct3d.h"

static int al_blender_to_d3d(int al_mode)
{
   int num_modes = 4;

   int allegro_modes[] = {
      ALLEGRO_ZERO,
      ALLEGRO_ONE,
      ALLEGRO_ALPHA,
      ALLEGRO_INVERSE_ALPHA
   };

   int d3d_modes[] = {
      D3DBLEND_ZERO,
      D3DBLEND_ONE,
      D3DBLEND_SRCALPHA,
      D3DBLEND_INVSRCALPHA
   };

   int i;

   for (i = 0; i < num_modes; i++) {
      if (al_mode == allegro_modes[i]) {
         return d3d_modes[i];
      }
   }

   return D3DBLEND_ONE;
}

static void set_blender(ALLEGRO_DISPLAY *display)
{
   int op, src, dst, alpha_op, alpha_src, alpha_dst;
   ALLEGRO_COLOR color;
   LPDIRECT3DDEVICE9 device = al_get_d3d_device(display);

   al_get_separate_blender(&op, &src, &dst, &alpha_op, &alpha_src, &alpha_dst, &color);

   src = al_blender_to_d3d(src);
   dst = al_blender_to_d3d(dst);
   alpha_src = al_blender_to_d3d(alpha_src);
   alpha_dst = al_blender_to_d3d(alpha_dst);

   IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, src);
   IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, dst);

   IDirect3DDevice9_SetRenderState(device, D3DRS_SEPARATEALPHABLENDENABLE, true);
   IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLENDALPHA, alpha_src);
   IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLENDALPHA, alpha_dst);

   IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
}

#define A5V_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static int _al_draw_prim_raw(ALLEGRO_BITMAP* texture, const void* vtx, const ALLEGRO_VERTEX_DECL* decl, 
   const int* indices, int num_vtx, int type)
{
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   int num_primitives = 0;

   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DBASETEXTURE9 d3d_texture;
   DWORD old_wrap_state[2];
   DWORD old_ttf_state;
   int min_idx = 0, max_idx = num_vtx - 1;
   ALLEGRO_BITMAP* target = al_get_target_bitmap();
   D3DMATRIX new_trans;
   const ALLEGRO_TRANSFORM* cur_trans = al_get_current_transform();
   IDirect3DVertexShader9* old_vtx_shader;
   IDirect3DPixelShader9* old_pix_shader;
   
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

   display = al_get_current_display();
   device = al_get_d3d_device(display);
   IDirect3DDevice9_GetVertexShader(device, &old_vtx_shader);
   IDirect3DDevice9_GetPixelShader(device, &old_pix_shader);
   
   if(decl) {
      if(decl->d3d_decl) {
         IDirect3DDevice9_SetVertexDeclaration(device, (IDirect3DVertexDeclaration9*)decl->d3d_decl);
      } else {
         return _al_draw_prim_soft(texture, vtx, decl, 0, num_vtx, type);
      }
   } else {
      IDirect3DDevice9_SetFVF(device, A5V_FVF);
   }

   if(!old_pix_shader)
      set_blender(display);

   if(!old_vtx_shader) {
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

         if (decl) {
            _al_set_texture_matrix(device, decl, mat[0]);
         }
         else
         {
            IDirect3DDevice9_GetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, &old_ttf_state);
            IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
            IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *)&mat);
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
   
   /* For sub-bitmaps */
   if (al_is_sub_bitmap(target)) {
      int xofs, yofs;
      al_get_d3d_texture_position(target, &xofs, &yofs);
      memcpy(new_trans.m[0], cur_trans->m[0], 16 * sizeof(float));
      new_trans.m[3][0] += xofs - 0.5;
      new_trans.m[3][1] += yofs - 0.5;
      IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, &new_trans);
   }
   
   if(!old_vtx_shader && decl)
      _al_setup_shader(device, decl);

   if(!indices)
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
            int in[2];
            in[0] = 0;
            in[1] = num_vtx-1;
         
            num_primitives = num_vtx - 1;
            IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINESTRIP, num_primitives, vtx, stride);
            IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, 0, num_vtx, 1, in, D3DFMT_INDEX32, vtx, stride);
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
   
   if (al_is_sub_bitmap(target)) {
      new_trans.m[3][0] = cur_trans->m[3][0] - 0.5;
      new_trans.m[3][1] = cur_trans->m[3][1] - 0.5;
      IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, &new_trans);
   }

   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, old_wrap_state[0]);
   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, old_wrap_state[1]);

   if(!old_vtx_shader && texture && !decl) {
      IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, old_ttf_state);
   }
   
   if(!old_vtx_shader)
      IDirect3DDevice9_SetVertexShader(device, 0);

   return num_primitives;
}

#endif

int _al_draw_prim_directx(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_D3D
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   return _al_draw_prim_raw(texture, (const char*)vtxs + start * stride, decl, 0, end - start, type);
#else
   (void)texture;
   (void)vtxs;
   (void)start;
   (void)end;
   (void)type;
   (void)decl;

   return 0;
#endif
}

int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{
#ifdef ALLEGRO_CFG_D3D
   return _al_draw_prim_raw(texture, vtxs, decl, indices, num_vtx, type);
#else
   (void)texture;
   (void)vtxs;
   (void)indices;
   (void)num_vtx;
   (void)type;
   (void)decl;

   return 0;
#endif
}

void _al_set_d3d_decl(ALLEGRO_VERTEX_DECL* ret)
{
#ifdef ALLEGRO_CFG_D3D
   int flags = al_get_display_flags();
   if (flags & ALLEGRO_DIRECT3D) {
      ALLEGRO_DISPLAY *display;
      LPDIRECT3DDEVICE9 device;
      D3DVERTEXELEMENT9 d3delements[ALLEGRO_PRIM_ATTR_NUM + 1];
      int idx = 0;
      ALLEGRO_VERTEX_ELEMENT* e;
      D3DCAPS9 caps;
      
      display = al_get_current_display();
      device = al_get_d3d_device(display);
      
      IDirect3DDevice9_GetDeviceCaps(device, &caps);
      if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0)) {
         ret->d3d_decl = 0;
      } else {
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
               case ALLEGRO_PRIM_FLOAT_3:
                  type = D3DDECLTYPE_FLOAT2;
               break;
               case ALLEGRO_PRIM_SHORT_2:
                  type = D3DDECLTYPE_SHORT2;
               break;
            }
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = type;
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_TEXCOORD;
            d3delements[idx].UsageIndex = 0;
            idx++;
         }

         e = &ret->elements[ALLEGRO_PRIM_COLOR_ATTR];
         if(e->attribute) {
            d3delements[idx].Stream = 0;
            d3delements[idx].Offset = e->offset;
            d3delements[idx].Type = D3DDECLTYPE_D3DCOLOR;
            d3delements[idx].Method = D3DDECLMETHOD_DEFAULT;
            d3delements[idx].Usage = D3DDECLUSAGE_COLOR;
            d3delements[idx].UsageIndex = 0;
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
      
      _al_create_shader(ret);
   }
#else
   ret->d3d_decl = 0;
#endif
}
