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
 *      DirectX dummy shader support. Dummy shader doesn't do anything
 *      except making D3D happy when you pass it vertices with non-FVF
 *      memory layout.
 *
 *      By Pavel Sountsov.
 *
 *      Thanks to Michał Cichoń for the pre-compiled shader code.
 *
 *      See readme.txt for copyright information.
 */

#include <string.h>
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/platform/alplatf.h"

#ifdef ALLEGRO_CFG_D3D

#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern_prim_directx.h"

#include "precompiled_shaders.inc"

void _al_create_primitives_shader(void* dev, ALLEGRO_VERTEX_DECL* decl)
{
   LPDIRECT3DDEVICE9 device = (LPDIRECT3DDEVICE9)dev;
   LPDIRECT3DVERTEXSHADER9 ret = 0;

   ALLEGRO_VERTEX_ELEMENT* e;

   int position = 0;
   int texture = 0;
   int color = 0;

   const uint8_t* shader = NULL;

   /*position, texture, color*/
   const uint8_t* shaders[3][2][2] =
   {
      {
         {_al_vs_pos0_tex0_col0, _al_vs_pos0_tex0_col4},
         {_al_vs_pos0_tex2_col0, _al_vs_pos0_tex2_col4}
      },
      {
         {_al_vs_pos2_tex0_col0, _al_vs_pos2_tex0_col4},
         {_al_vs_pos2_tex2_col0, _al_vs_pos2_tex2_col4}
      },
      {
         {_al_vs_pos3_tex0_col0, _al_vs_pos3_tex0_col4},
         {_al_vs_pos3_tex2_col0, _al_vs_pos3_tex2_col4}
      }
   };

   e = &decl->elements[ALLEGRO_PRIM_POSITION];
   if (e->attribute) {
      switch(e->storage) {
         case ALLEGRO_PRIM_SHORT_2:
         case ALLEGRO_PRIM_FLOAT_2:
            position = 1;
         break;
         case ALLEGRO_PRIM_FLOAT_3:
            position = 2;
         break;
      }
   }

   e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
   if(!e->attribute)
      e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
   if(e->attribute) {
      texture = 1;
   }

   e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
   if(e->attribute) {
      color = 1;
   }

   shader = shaders[position][texture][color];

   IDirect3DDevice9_CreateVertexShader(device, (const DWORD*)shader, &ret);

   decl->d3d_dummy_shader = ret;
}

void* _al_create_default_primitives_shader(void* dev)
{
   LPDIRECT3DDEVICE9 device = (LPDIRECT3DDEVICE9)dev;
   LPDIRECT3DVERTEXSHADER9 shader;
   IDirect3DDevice9_CreateVertexShader(device, (const DWORD*)_al_vs_pos3_tex2_col4, &shader);
   return shader;
}

static void _al_swap(float* l, float* r)
{
  float temp = *r;
  *r = *l;
  *l = temp;
}

static void _al_transpose_d3d_matrix(D3DMATRIX* m)
{
   int i, j;
   float* m_ptr = (float*)m;

   for (i = 1; i < 4; i++) {
      for (j = 0; j < i; j++) {
         _al_swap(m_ptr + (i * 4 + j), m_ptr + (j * 4 + i));
      }
   }
}

static void _al_multiply_d3d_matrix(D3DMATRIX* result, const D3DMATRIX* a, const D3DMATRIX* b)
{
   int i, j, k;
   float* r_ptr = (float*)result;
   float* a_ptr = (float*)a;
   float* b_ptr = (float*)b;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         r_ptr[i * 4 + j] = 0.0f;
         for (k = 0; k < 4; k++) {
            r_ptr[i * 4 + j] += a_ptr[i * 4 + k] * b_ptr[k * 4 + j];
         }
      }
   }
}

static void _al_multiply_transpose_d3d_matrix(D3DMATRIX* result, const D3DMATRIX* a, const D3DMATRIX* b)
{
   int i, j, k;
   float* r_ptr = (float*)result;
   float* a_ptr = (float*)a;
   float* b_ptr = (float*)b;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         r_ptr[i + 4 * j] = 0.0f;
         for (k = 0; k < 4; k++) {
            r_ptr[i + 4 * j] += a_ptr[i * 4 + k] * b_ptr[k * 4 + j];
         }
      }
   }
}

/* note: passed matrix may be modified by this function */
void _al_set_texture_matrix(void* dev, float* mat)
{
   _al_transpose_d3d_matrix((D3DMATRIX*)mat);
   IDirect3DDevice9_SetVertexShaderConstantF((IDirect3DDevice9*)dev, 4, mat, 4);
}

static void setup_transforms(IDirect3DDevice9* device)
{
   D3DMATRIX matWorld, matView, matProj, matWorldView, matWorldViewProj;
   IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &matWorld);
   IDirect3DDevice9_GetTransform(device, D3DTS_VIEW, &matView);
   IDirect3DDevice9_GetTransform(device, D3DTS_PROJECTION, &matProj);
   _al_multiply_d3d_matrix(&matWorldView, &matWorld, &matView);
   _al_multiply_transpose_d3d_matrix(&matWorldViewProj, &matWorldView, &matProj);
   IDirect3DDevice9_SetVertexShaderConstantF(device, 0, (float*)&matWorldViewProj, 4);
}

void _al_setup_primitives_shader(void* dev, const ALLEGRO_VERTEX_DECL* decl)
{
   IDirect3DDevice9* device = (IDirect3DDevice9*)dev;
   setup_transforms(device);
   IDirect3DDevice9_SetVertexShader(device, (IDirect3DVertexShader9*)decl->d3d_dummy_shader);
}

void _al_setup_default_primitives_shader(void* dev, void* shader)
{
   IDirect3DDevice9* device = (IDirect3DDevice9*)dev;
   setup_transforms(device);
   IDirect3DDevice9_SetVertexShader(device, (LPDIRECT3DVERTEXSHADER9)shader);
}

#endif /* ALLEGRO_CFG_D3D */

/* vim: set sts=3 sw=3 et: */
