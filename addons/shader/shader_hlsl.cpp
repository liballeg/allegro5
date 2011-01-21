#include "allegro5/allegro5.h"
#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>
#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_shader_hlsl.h"
#include "allegro5/transformations.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_shader_hlsl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"

#include "shader.h"
#include "shader_hlsl.h"
#include <cstdio>

ALLEGRO_DEBUG_CHANNEL("shader")

static const char *null_source = "";

static const char *technique_source_vertex =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = compile vs_2_0 vs_main();\n"
   "      PixelShader = null;\n"
   "   }\n"
   "}\n";

static const char *technique_source_pixel =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = null;\n"
   "      PixelShader = compile ps_2_0 ps_main();\n"
   "   }\n"
   "}\n\n";

static const char *technique_source_both =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = compile vs_2_0 vs_main();\n"
   "      PixelShader = compile ps_2_0 ps_main();\n"
   "   }\n"
   "}\n";

ALLEGRO_SHADER *_al_create_shader_hlsl(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER_HLSL_S *shader = (ALLEGRO_SHADER_HLSL_S *)al_malloc(
      sizeof(ALLEGRO_SHADER_HLSL_S)
   );

   (void)platform;

   if (!shader)
      return NULL;

   memset(shader, 0, sizeof(ALLEGRO_SHADER_HLSL_S));

   return (ALLEGRO_SHADER *)shader;
}

bool _al_link_shader_hlsl(ALLEGRO_SHADER *shader)
{
   (void)shader;
   return true;
}

bool _al_attach_shader_source_hlsl(
   ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type,
   const char *source)
{
   bool add_technique = true;
   char *full_source;
   LPD3DXBUFFER errors;
   const char *vertex_source, *pixel_source, *technique_source;
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;

   if (source == NULL) {
      if (type == ALLEGRO_VERTEX_SHADER) {
         if (shader->vertex_copy) {
            free(shader->vertex_copy);
            shader->vertex_copy = NULL;
	    hlsl_shader->hlsl_shader->Release();
         }
         vertex_source = null_source;
         if (shader->pixel_copy) {
            pixel_source = shader->pixel_copy;
         }
         else {
            pixel_source = null_source;
         }
      }
      else {
         if (shader->pixel_copy) {
            free(shader->pixel_copy);
            shader->pixel_copy = NULL;
	    hlsl_shader->hlsl_shader->Release();
         }
         pixel_source = null_source;
         if (shader->vertex_copy) {
            vertex_source = shader->vertex_copy;
         }
         else {
            vertex_source = null_source;
         }
      }
   }
   else {
      if (type == ALLEGRO_VERTEX_SHADER) {
            vertex_source = source;
            if (shader->vertex_copy) {
               free(shader->vertex_copy);
            }
            shader->vertex_copy = strdup(vertex_source);
            if (shader->pixel_copy) {
               pixel_source = shader->pixel_copy;
            }
            else {
               pixel_source = null_source;
            }
      }
      else {
            pixel_source = source;
            if (shader->pixel_copy) {
               free(shader->pixel_copy);
            }
            shader->pixel_copy = strdup(pixel_source);
            if (shader->vertex_copy) {
               vertex_source = shader->vertex_copy;
            }
            else {
               vertex_source = null_source;
            }
      }
   }

   if (vertex_source[0] == 0 && pixel_source[0] == 0) {
      return false;
   }

   if (strstr(vertex_source, "technique"))
      add_technique = false;
   if (strstr(pixel_source, "technique"))
      add_technique = false;

   if (add_technique) {
      if (vertex_source[0] == 0) {
         technique_source = technique_source_pixel;
      }
      else if (pixel_source[0] == 0) {
         technique_source = technique_source_vertex;
      }
      else
      	technique_source = technique_source_both;
   }
   else {
      technique_source = null_source;
   }

   // HLSL likes the source in one buffer
   int vlen, plen, tlen;
   vlen = strlen(vertex_source);
   plen = strlen(pixel_source);
   if (add_technique)
      tlen = strlen(technique_source);
   else
      tlen = 0;
   full_source = (char *)al_malloc(vlen+plen+tlen+4);
   memcpy(full_source, vertex_source, vlen);
   full_source[vlen] = '\n';
   memcpy(full_source+vlen+1, pixel_source, plen);
   full_source[vlen+plen+1] = '\n';
   if (add_technique) {
      memcpy(full_source+vlen+plen+2, technique_source, tlen);
   }
   full_source[vlen+plen+tlen+2] = '\n';
   full_source[vlen+plen+tlen+3] = 0;

   DWORD ok = D3DXCreateEffect(
      al_get_d3d_device(al_get_current_display()),
      full_source,
      strlen(full_source),
      NULL,
      NULL,
      D3DXSHADER_PACKMATRIX_ROWMAJOR,
      NULL,
      &hlsl_shader->hlsl_shader,
      &errors
   );

   if (ok != D3D_OK) {
      fprintf(stderr, "%s\n", (char *)errors->GetBufferPointer());
   }
   
   D3DXHANDLE hTech;

   hTech = hlsl_shader->hlsl_shader->GetTechniqueByName("TECH");
   hlsl_shader->hlsl_shader->ValidateTechnique(hTech);
   hlsl_shader->hlsl_shader->SetTechnique(hTech);

   al_free(full_source);

   return true;
}

void _al_use_shader_hlsl(ALLEGRO_SHADER *shader, bool use)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   LPD3DXEFFECT effect = hlsl_shader->hlsl_shader;
   ALLEGRO_DISPLAY *display = al_get_current_display();

   if (use) {
      D3DXMATRIX mp, mv, mu;
      memcpy(mp.m, (float *)display->proj_transform.m, sizeof(float)*16);
      effect->SetMatrix("proj_matrix", &mp);
      memcpy(mp.m, (float *)display->view_transform.m, sizeof(float)*16);
      effect->SetMatrix("view_matrix", &mp);
      UINT required_passes;
      effect->Begin(&required_passes, 0);
      effect->BeginPass(0);
   }
   else {
      effect->EndPass();
      effect->End();
   }
}

void _al_destroy_shader_hlsl(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;

   hlsl_shader->hlsl_shader->Release();

   al_free(shader);
}

bool _al_set_shader_sampler_hlsl(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   (void)unit;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP)
      return false;

   result = hlsl_shader->hlsl_shader->SetTexture(name,
      al_get_d3d_video_texture(bitmap));

   return result == D3D_OK;
}

bool _al_set_shader_matrix_hlsl(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   D3DXMATRIX m;

   memcpy(m.m, matrix->m, sizeof(float)*16);

   result = hlsl_shader->hlsl_shader->SetMatrix(name, &m);

   return result == D3D_OK;
}

bool _al_set_shader_int_hlsl(ALLEGRO_SHADER *shader, const char *name, int i)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetInt(name, i);

   return result == D3D_OK;
}

bool _al_set_shader_float_hlsl(ALLEGRO_SHADER *shader, const char *name, float f)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetFloat(name, f);

   return result == D3D_OK;
}

bool _al_set_shader_int_vector_hlsl(ALLEGRO_SHADER *shader, const char *name,
   int size, int *i)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetIntArray(name, i, size);

   return result == D3D_OK;
}

bool _al_set_shader_float_vector_hlsl(ALLEGRO_SHADER *shader, const char *name,
   int size, float *f)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetFloatArray(name, f, size);

   return result == D3D_OK;
}

bool _al_set_shader_vertex_array_hlsl(ALLEGRO_SHADER *shader, float *v, int stride)
{
   (void)shader;
   (void)v;
   (void)stride;
   return true;
}

bool _al_set_shader_color_array_hlsl(ALLEGRO_SHADER *shader, unsigned char *c, int stride)
{
   (void)shader;
   (void)c;
   (void)stride;
   return true;
}

bool _al_set_shader_texcoord_array_hlsl(ALLEGRO_SHADER *shader, float *u, int stride)
{
   (void)shader;
   (void)u;
   (void)stride;
   return true;
}

LPD3DXEFFECT al_get_direct3d_effect(ALLEGRO_SHADER *shader)
{
   return ((ALLEGRO_SHADER_HLSL_S *)shader)->hlsl_shader;
}
