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
 * 	  except making D3D happy when you pass it vertices with non-FVF
 * 	  memory layout.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include <string.h>
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/platform/alplatf.h"

#ifdef ALLEGRO_CFG_D3D

#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>

static char* strappend(char* dest, size_t* dest_size, const char* str)
{
   size_t dest_len = strlen(dest);
   size_t str_len = strlen(str);
      
   if(dest_len + str_len + 1 > *dest_size) {
      *dest_size = dest_len + str_len + 1;
      dest = (char*)realloc(dest, *dest_size);
   }
   
   strcat(dest, str);
   
   return dest;
}

#endif

extern "C"
{

void _al_create_shader(ALLEGRO_VERTEX_DECL* decl)
{
#ifdef ALLEGRO_CFG_D3D
   char* header_text = (char*)malloc(4);
   header_text[0] = 0;
   size_t header_size = 4;
   
   char* body_text = (char*)malloc(4);
   body_text[0] = 0;
   size_t body_size = 4;
   
   const char* header_str = 0;
   const char* body_str = 0;
   
   ALLEGRO_VERTEX_ELEMENT* e;
   
   header_text = strappend(header_text, &header_size, 
      "float4x4 WorldViewProj;\n"
      "float4x4 TextureProj;\n"
      "void main(\n"
      );
                                          
   e = &decl->elements[ALLEGRO_PRIM_POSITION];
   if(e->attribute) {
      switch(e->storage) {
         case ALLEGRO_PRIM_SHORT_2:
         case ALLEGRO_PRIM_FLOAT_2:
            header_str = "float2 in_pos : POSITION,\n";
            body_str = "pos = float4(in_pos.x, in_pos.y, 0.0f, 1.0f);\n";
         break;
         case ALLEGRO_PRIM_FLOAT_3:
            header_str = "float3 in_pos : POSITION,\n";
            body_str = "pos = float4(in_pos.x, in_pos.y, in_pos.z, 1.0f);\n";
         break;
      }     
      header_text = strappend(header_text, &header_size, header_str);
      body_text = strappend(body_text, &body_size, body_str);
      
      header_text = strappend(header_text, &header_size, "out float4 pos : POSITION,\n");
      body_text = strappend(body_text, &body_size, "pos = mul(pos, WorldViewProj);\n");
   } else {
      // Must output some position, even if it is nonsensical
      header_text = strappend(header_text, &header_size, "out float4 pos : POSITION,\n");
      body_text = strappend(body_text, &body_size, "pos = float4(0.0f, 0.0f, 0.0f, 1.0f);\n");
   }
   
   e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
   if(!e->attribute)
      e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
   if(e->attribute) {
      header_text = strappend(header_text, &header_size, 
      "float2 in_texcoord0 : TEXCOORD0,\n"
      "out float2 texcoord0 : TEXCOORD0,\n"
      );
      body_text = strappend(body_text, &body_size,
      "texcoord0 = float4(in_texcoord0.x, in_texcoord0.y, 0, 1.0f);\n"
      "texcoord0 = mul(texcoord0, TextureProj);\n"
      );
   }
   
   e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
   if(e->attribute) {
      header_text = strappend(header_text, &header_size, 
      "float4 in_color0 : COLOR0,\n"
      "out float4 color0 : COLOR0,\n"
      );
      body_text = strappend(body_text, &body_size,
      "color0 = in_color0;\n"
      );
   } else {
      // By default, the color is white
      header_text = strappend(header_text, &header_size, 
      "out float4 color0 : COLOR0,\n"
      );
      body_text = strappend(body_text, &body_size,
      "color0 = float4(1.0f, 1.0f, 1.0f, 1.0f);\n"
      );
   }
   
   header_text[strlen(header_text) - 2] = '\0'; // Remove the trailing comma
   
   header_text = strappend(header_text, &header_size, ")\n{\n");
   header_text = strappend(header_text, &header_size, body_text);
   header_text = strappend(header_text, &header_size, "}\n");
   
   IDirect3DDevice9* device = al_get_d3d_device(al_get_current_display());
   IDirect3DVertexShader9* ret = 0;
   ID3DXConstantTable* table = 0;
   ID3DXBuffer *code = 0;
   
   HRESULT result = D3DXCompileShader(header_text, strlen(header_text), 0, 0, "main", "vs_1_1", 0, &code, 0, &table); 
   
   if(result == D3D_OK)
   {
      IDirect3DDevice9_CreateVertexShader(device, (DWORD*)code->GetBufferPointer(), &ret);
      code->Release();
   }
   
   decl->d3d_shader_table = table;
   decl->d3d_dummy_shader = ret;
   
   free(header_text);
   free(body_text);
#else
   (void)decl;
#endif
}

void _al_set_texture_matrix(void* dev, const ALLEGRO_VERTEX_DECL* decl, float* mat)
{
#ifdef ALLEGRO_CFG_D3D
	((ID3DXConstantTable*)decl->d3d_shader_table)->SetMatrix((IDirect3DDevice9*)dev, "TextureProj", (D3DXMATRIX*)mat);
#else
   (void)dev;
   (void)decl;
   (void)mat;
#endif
}

void _al_setup_shader(void* dev, const ALLEGRO_VERTEX_DECL* decl)
{
#ifdef ALLEGRO_CFG_D3D
	IDirect3DDevice9* device = (IDirect3DDevice9*)dev;
	D3DXMATRIX matWorld, matView, matProj, matTex;
	IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &matWorld);
	IDirect3DDevice9_GetTransform(device, D3DTS_VIEW, &matView);
	IDirect3DDevice9_GetTransform(device, D3DTS_PROJECTION, &matProj);

	D3DXMATRIX matWorldViewProj = matWorld * matView * matProj;
	((ID3DXConstantTable*)decl->d3d_shader_table)->SetMatrix(device, "WorldViewProj", &matWorldViewProj);
	IDirect3DDevice9_SetVertexShader(device, (IDirect3DVertexShader9*)decl->d3d_dummy_shader);
#else
   (void)dev;
   (void)decl;
#endif
}

}
