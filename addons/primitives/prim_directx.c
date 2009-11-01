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

D3DVERTEXELEMENT9 allegro_vertex_decl[] =
{
  {0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
  {0, 8, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
  {0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
  D3DDECL_END()
};

IDirect3DVertexDeclaration9* allegro_vertex_def;

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
   int src, dst, alpha_src, alpha_dst;
   ALLEGRO_COLOR color;
   LPDIRECT3DDEVICE9 device = al_d3d_get_device(display);

   al_get_separate_blender(&src, &dst, &alpha_src, &alpha_dst, &color);

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

/*
 * Workabouts for the pre-SM3 cards
 */
#define A5V_FVF (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

typedef struct ALLEGRO_FVF_VERTEX ALLEGRO_FVF_VERTEX;

struct ALLEGRO_FVF_VERTEX {
  float x, y, z;
  DWORD c;
  float u, v;
};

ALLEGRO_FVF_VERTEX* fvf_buffer;
int fvf_buffer_size = 0;
bool use_vertex_twiddler = false;

static void* twiddle_vertices(const void* vtxs, int num_vertices)
{
   const ALLEGRO_VERTEX* vtx = vtxs;
   int ii;
   
   if(fvf_buffer == 0) {
      fvf_buffer = malloc(num_vertices * sizeof(ALLEGRO_FVF_VERTEX));
      fvf_buffer_size = num_vertices;
   } else if (num_vertices > fvf_buffer_size) {
      fvf_buffer = realloc(fvf_buffer, num_vertices * sizeof(ALLEGRO_FVF_VERTEX));
      fvf_buffer_size = num_vertices;
   }
   
   for(ii = 0; ii < num_vertices; ii++) {
      fvf_buffer[ii].x = vtx[ii].x;
      fvf_buffer[ii].y = vtx[ii].y;
      fvf_buffer[ii].z = 0;
      
      fvf_buffer[ii].u = vtx[ii].u;
      fvf_buffer[ii].v = vtx[ii].v;
      
      fvf_buffer[ii].c = vtx[ii].color.d3d_color;
   }
   return fvf_buffer;
}

#endif

void* lbuff;
int lbuff_size = 0;

int _al_draw_prim_directx(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_D3D
   int num_primitives = 0;
   int num_vtx;
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   const void *vtx;
   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DBASETEXTURE9 d3d_texture;
   DWORD old_wrap_state[2];
   DWORD old_ttf_state;

   display = al_get_current_display();
   device = al_d3d_get_device(display);
   num_vtx = end - start;
   vtx = ((const char*)vtxs + start * stride);
   
   if(decl) {
      if(decl->d3d_decl) {
         IDirect3DDevice9_SetVertexDeclaration(device, (IDirect3DVertexDeclaration9*)decl->d3d_decl);
      } else {
         return _al_draw_prim_soft(texture, vtxs, decl, start, end, type);
      }
   } else {
      if(!allegro_vertex_def && !use_vertex_twiddler) {
         D3DCAPS9 caps;
         IDirect3DDevice9_GetDeviceCaps(device, &caps);
         if(caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
            use_vertex_twiddler = true;
         else
            IDirect3DDevice9_CreateVertexDeclaration(device, allegro_vertex_decl, &allegro_vertex_def);
      }
      if(use_vertex_twiddler) {
         stride = sizeof(ALLEGRO_FVF_VERTEX);
         IDirect3DDevice9_SetFVF(device, A5V_FVF);
         vtx = twiddle_vertices(vtx, num_vtx);
      } else {
         IDirect3DDevice9_SetVertexDeclaration(device, allegro_vertex_def);
      }
   }

   set_blender(display);

   if (texture) {
      D3DSURFACE_DESC desc;
      float mat[4][4] = {
         {1, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 0, 1, 0},
         {0, 0, 0, 1}
      };      
      IDirect3DTexture9_GetLevelDesc(al_d3d_get_video_texture(texture), 0, &desc);

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

      IDirect3DDevice9_GetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, &old_ttf_state);
      IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
      IDirect3DDevice9_SetTransform(device, D3DTS_TEXTURE0, (D3DMATRIX *)&mat);

      d3d_texture = (LPDIRECT3DBASETEXTURE9)al_d3d_get_video_texture(texture);
      IDirect3DDevice9_SetTexture(device, 0, d3d_texture);
   } else {
      IDirect3DDevice9_SetTexture(device, 0, NULL);
   }
   
   IDirect3DDevice9_GetSamplerState(device, 0, D3DSAMP_ADDRESSU, &old_wrap_state[0]);
   IDirect3DDevice9_GetSamplerState(device, 0, D3DSAMP_ADDRESSV, &old_wrap_state[1]);
   
   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

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
         int size = 2 * stride;
         if(lbuff == 0) {
            lbuff = malloc(size);
            lbuff_size = size;
         } else if (size > lbuff_size) {
            lbuff = realloc(lbuff, size);
            lbuff_size = size;
         }
         
         memcpy(lbuff, vtx, stride);
         memcpy((char*)lbuff + stride, (const char*)vtx + stride * (num_vtx - 1), stride);
      
         num_primitives = num_vtx;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINESTRIP, num_primitives-1, vtx, stride);
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINELIST, 1, lbuff, stride);
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

   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, old_wrap_state[0]);
   IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, old_wrap_state[1]);

   if(texture) {
      IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_TEXTURETRANSFORMFLAGS, old_ttf_state);
   }

   return num_primitives;
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

void* cbuff;
int cbuff_size = 0;

int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{
#ifdef ALLEGRO_CFG_D3D
   /* FIXME: IDirect3DDevice9_DrawIndexedPrimitiveUP is incompatible with the freedom we allow for the contents of the indices array
    * The function requires that the first index is the smallest index in the indices array. No such requirement is made at the
    * primitives addon API level, so this function cannot be implemented directly.
    */
   
   if(decl) {
      int size = decl->stride * num_vtx;
      int ii;
      int ret;
      
      if(cbuff == 0) {
         cbuff = malloc(size);
         cbuff_size = size;
      } else if (size > cbuff_size) {
         cbuff = realloc(cbuff, size);
         cbuff_size = size;
      }
      
      for(ii = 0; ii < num_vtx; ii++) {
         memcpy((char*)cbuff + ii * decl->stride, (const char*)vtxs + decl->stride * indices[ii], decl->stride);
      }
      ret = _al_draw_prim_directx(texture, cbuff, decl, 0, num_vtx, type);
      return 0;
   } else {
      int size = sizeof(ALLEGRO_VERTEX) * num_vtx;
      int ii;
      int ret;
      
      if(cbuff == 0) {
         cbuff = malloc(size);
         cbuff_size = size;
      } else if (size > cbuff_size) {
         cbuff = realloc(cbuff, size);
         cbuff_size = size;
      }
      
      for(ii = 0; ii < num_vtx; ii++) {
         ((ALLEGRO_VERTEX*)cbuff)[ii] = ((const ALLEGRO_VERTEX*)vtxs)[indices[ii]];
      }
      ret = _al_draw_prim_directx(texture, cbuff, decl, 0, num_vtx, type);
      return ret;  
   }
   
   /*
   int num_primitives = 0;
   ALLEGRO_VERTEX *vtx;
   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DBASETEXTURE9 d3d_texture;

   display = al_get_current_display();
   device = al_d3d_get_device(display);
   vtx = vtxs;

   set_blender(display);

   if (texture) {
      d3d_texture = (LPDIRECT3DBASETEXTURE9)al_d3d_get_video_texture(texture);
      IDirect3DDevice9_SetTexture(device, 0, d3d_texture);
   }
   else {
      IDirect3DDevice9_SetTexture(device, 0, NULL);
   }
   
   IDirect3DDevice9_SetFVF(device, A5V_FVF);

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         num_primitives = num_vtx / 2;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         num_primitives = num_vtx - 1;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINESTRIP, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         int in[2];
         num_primitives = num_vtx;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINESTRIP, 0, num_vtx, num_primitives-1, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         in[0] = indices[0];
         in[1] = indices[num_vtx-1];
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, 0, 2, 1, in, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         num_primitives = num_vtx / 3;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         num_primitives = num_vtx - 2;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         num_primitives = num_vtx - 2;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLEFAN, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
   }

   return num_primitives;
   */
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

void _al_use_transform_directx(const ALLEGRO_TRANSFORM* trans)
{
#ifdef ALLEGRO_CFG_D3D
   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;
   D3DMATRIX matrix;
   memcpy(matrix.m[0], trans->m[0], 16 * sizeof(float));
   matrix.m[3][0] -= 0.5f;
   matrix.m[3][1] -= 0.5f;

   display = al_get_current_display();
   device = al_d3d_get_device(display);

   IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, &matrix);
#else
   (void)trans;
#endif
}
