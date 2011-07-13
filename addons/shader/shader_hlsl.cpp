#include "allegro5/allegro.h"
#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>
#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_shader_hlsl.h"
#include "allegro5/transformations.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_shader_hlsl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_direct3d.h"

#include "shader.h"
#include "shader_hlsl.h"

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

   // For simplicity, these fields are never NULL in this backend.
   shader->shader.pixel_copy = al_ustr_new("");
   shader->shader.vertex_copy = al_ustr_new("");

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
   bool add_technique;
   ALLEGRO_USTR *full_source;
   LPD3DXBUFFER errors;
   const char *vertex_source, *pixel_source, *technique_source;
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;

   if (source == NULL) {
      if (type == ALLEGRO_VERTEX_SHADER) {
         if (shader->vertex_copy) {
            al_ustr_truncate(shader->vertex_copy, 0);
            hlsl_shader->hlsl_shader->Release();
         }
         vertex_source = null_source;
         pixel_source = al_cstr(shader->pixel_copy);
      }
      else {
         if (shader->pixel_copy) {
            al_ustr_truncate(shader->pixel_copy, 0);
            hlsl_shader->hlsl_shader->Release();
         }
         pixel_source = null_source;
         vertex_source = al_cstr(shader->vertex_copy);
      }
   }
   else {
      if (type == ALLEGRO_VERTEX_SHADER) {
         vertex_source = source;
         al_ustr_truncate(shader->vertex_copy, 0);
         al_ustr_append_cstr(shader->vertex_copy, vertex_source);
         pixel_source = al_cstr(shader->pixel_copy);
      }
      else {
         pixel_source = source;
         al_ustr_truncate(shader->pixel_copy, 0);
         al_ustr_append_cstr(shader->pixel_copy, pixel_source);
         vertex_source = al_cstr(shader->vertex_copy);
      }
   }

   if (vertex_source[0] == 0 && pixel_source[0] == 0) {
      return false;
   }

   if (strstr(vertex_source, "technique") || strstr(pixel_source, "technique"))
      add_technique = false;
   else
      add_technique = true;

   if (add_technique) {
      if (vertex_source[0] == 0) {
         technique_source = technique_source_pixel;
      }
      else if (pixel_source[0] == 0) {
         technique_source = technique_source_vertex;
      }
      else {
         technique_source = technique_source_both;
      }
   }
   else {
      technique_source = null_source;
   }

   // HLSL likes the source in one buffer
   full_source = al_ustr_newf("%s\n%s\n%s\n",
      vertex_source, pixel_source, technique_source);

   DWORD ok = D3DXCreateEffect(
      al_get_d3d_device(al_get_current_display()),
      al_cstr(full_source),
      al_ustr_size(full_source),
      NULL,
      NULL,
      D3DXSHADER_PACKMATRIX_ROWMAJOR,
      NULL,
      &hlsl_shader->hlsl_shader,
      &errors
   );

   al_ustr_free(full_source);

   if (ok != D3D_OK) {
      char *msg = (char *)errors->GetBufferPointer();
      if (shader->log) {
         al_ustr_truncate(shader->log, 0);
         al_ustr_append_cstr(shader->log, msg);
      } else {
         shader->log = al_ustr_new(msg);
      }
      ALLEGRO_ERROR("Error: %s\n", msg);
      return false;
   }
   
   D3DXHANDLE hTech;
   hTech = hlsl_shader->hlsl_shader->GetTechniqueByName("TECH");
   hlsl_shader->hlsl_shader->ValidateTechnique(hTech);
   hlsl_shader->hlsl_shader->SetTechnique(hTech);

   return true;
}

void _al_use_shader_hlsl(ALLEGRO_SHADER *shader, bool use)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   LPD3DXEFFECT effect = hlsl_shader->hlsl_shader;
   ALLEGRO_DISPLAY *display = al_get_current_display();

   if (use) {
      ALLEGRO_TRANSFORM t;
      al_copy_transform(&t, &display->view_transform);
      al_compose_transform(&t, &display->proj_transform);
      effect->SetMatrix("proj_matrix", (LPD3DXMATRIX)&t.m);
   }
   else {
      //effect->EndPass();
      //effect->End();
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

   LPDIRECT3DTEXTURE9 vid_texture = al_get_d3d_video_texture(bitmap);
   result = hlsl_shader->hlsl_shader->SetTexture(name, vid_texture);
   ((ALLEGRO_DISPLAY_D3D *)bitmap->display)->device->SetTexture(0, vid_texture);

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
   int elem_size, int *i, int num_elems)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetIntArray(name, i,
      elem_size * num_elems);

   return result == D3D_OK;
}

bool _al_set_shader_float_vector_hlsl(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetFloatArray(name, f,
      elem_size * num_elems);

   return result == D3D_OK;
}

bool _al_set_shader_bool_hlsl(ALLEGRO_SHADER *shader, const char *name, bool b)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetBool(name, b);

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

/* Function: al_get_direct3d_effect
 */
LPD3DXEFFECT al_get_direct3d_effect(ALLEGRO_SHADER *shader)
{
   return ((ALLEGRO_SHADER_HLSL_S *)shader)->hlsl_shader;
}

void _al_set_shader_hlsl(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader)
{
   al_set_direct3d_effect(display, al_get_direct3d_effect(shader));
}

/* vim: set sts=3 sw=3 et: */
