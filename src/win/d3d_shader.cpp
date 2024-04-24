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
 *      Direct3D shader support.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_direct3d.h"
#include "allegro5/internal/aintern_shader.h"

#ifdef ALLEGRO_CFG_SHADER_HLSL

#include <d3dx9.h>
#include <stdio.h>

#include "d3d.h"

ALLEGRO_DEBUG_CHANNEL("shader")

static _AL_VECTOR shaders;

struct ALLEGRO_SHADER_HLSL_S
{
   ALLEGRO_SHADER shader;
   LPD3DXEFFECT hlsl_shader;
   int shader_model;
};

static const char *null_source = "";

static const char *technique_source_vertex_v2 =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = compile vs_2_0 vs_main();\n"
   "      PixelShader = null;\n"
   "   }\n"
   "}\n";

static const char *technique_source_pixel_v2 =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = null;\n"
   "      PixelShader = compile ps_2_0 ps_main();\n"
   "   }\n"
   "}\n\n";

static const char *technique_source_both_v2 =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = compile vs_2_0 vs_main();\n"
   "      PixelShader = compile ps_2_0 ps_main();\n"
   "   }\n"
   "}\n";

static const char *technique_source_vertex_v3 =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = compile vs_3_0 vs_main();\n"
   "      PixelShader = null;\n"
   "   }\n"
   "}\n";

static const char *technique_source_pixel_v3 =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = null;\n"
   "      PixelShader = compile ps_3_0 ps_main();\n"
   "   }\n"
   "}\n\n";

static const char *technique_source_both_v3 =
   "technique TECH\n"
   "{\n"
   "   pass p1\n"
   "   {\n"
   "      VertexShader = compile vs_3_0 vs_main();\n"
   "      PixelShader = compile ps_3_0 ps_main();\n"
   "   }\n"
   "}\n";


static bool hlsl_attach_shader_source(ALLEGRO_SHADER *shader,
               ALLEGRO_SHADER_TYPE type, const char *source);
static bool hlsl_build_shader(ALLEGRO_SHADER *shader);
static bool hlsl_use_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display,
               bool set_projview_matrix_from_display);
static void hlsl_unuse_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display);
static void hlsl_destroy_shader(ALLEGRO_SHADER *shader);
static void hlsl_on_lost_device(ALLEGRO_SHADER *shader);
static void hlsl_on_reset_device(ALLEGRO_SHADER *shader);
static bool hlsl_set_shader_sampler(ALLEGRO_SHADER *shader,
               const char *name, ALLEGRO_BITMAP *bitmap, int unit);
static bool hlsl_set_shader_matrix(ALLEGRO_SHADER *shader,
               const char *name, const ALLEGRO_TRANSFORM *matrix);
static bool hlsl_set_shader_int(ALLEGRO_SHADER *shader,
               const char *name, int i);
static bool hlsl_set_shader_float(ALLEGRO_SHADER *shader,
               const char *name, float f);
static bool hlsl_set_shader_int_vector(ALLEGRO_SHADER *shader,
               const char *name, int num_components, const int *i, int num_elems);
static bool hlsl_set_shader_float_vector(ALLEGRO_SHADER *shader,
               const char *name, int num_components, const float *f, int num_elems);
static bool hlsl_set_shader_bool(ALLEGRO_SHADER *shader,
               const char *name, bool b);

static struct ALLEGRO_SHADER_INTERFACE shader_hlsl_vt =
{
   hlsl_attach_shader_source,
   hlsl_build_shader,
   hlsl_use_shader,
   hlsl_unuse_shader,
   hlsl_destroy_shader,
   hlsl_on_lost_device,
   hlsl_on_reset_device,
   hlsl_set_shader_sampler,
   hlsl_set_shader_matrix,
   hlsl_set_shader_int,
   hlsl_set_shader_float,
   hlsl_set_shader_int_vector,
   hlsl_set_shader_float_vector,
   hlsl_set_shader_bool
};

void _al_d3d_on_lost_shaders(ALLEGRO_DISPLAY *display)
{
   unsigned i;
   (void)display;

   for (i = 0; i < _al_vector_size(&shaders); i++) {
      ALLEGRO_SHADER **shader = (ALLEGRO_SHADER **)_al_vector_ref(&shaders, i);
      (*shader)->vt->on_lost_device(*shader);
   }
}

void _al_d3d_on_reset_shaders(ALLEGRO_DISPLAY *display)
{
   unsigned i;
   (void)display;

   for (i = 0; i < _al_vector_size(&shaders); i++) {
      ALLEGRO_SHADER **shader = (ALLEGRO_SHADER **)_al_vector_ref(&shaders, i);
      (*shader)->vt->on_reset_device(*shader);
   }
}

ALLEGRO_SHADER *_al_create_shader_hlsl(ALLEGRO_SHADER_PLATFORM platform, int shader_model)
{
   ALLEGRO_SHADER_HLSL_S *shader;

   if (NULL == _al_imp_D3DXCreateEffect) {
      ALLEGRO_ERROR("D3DXCreateEffect unavailable\n");
      return NULL;
   }

   shader = (ALLEGRO_SHADER_HLSL_S *)al_calloc(1, sizeof(ALLEGRO_SHADER_HLSL_S));
   if (!shader)
      return NULL;
   shader->shader.platform = platform;
   shader->shader.vt = &shader_hlsl_vt;
   shader->shader_model = shader_model;
   _al_vector_init(&shader->shader.bitmaps, sizeof(ALLEGRO_BITMAP *));

   // For simplicity, these fields are never NULL in this backend.
   shader->shader.pixel_copy = al_ustr_new("");
   shader->shader.vertex_copy = al_ustr_new("");

   ALLEGRO_SHADER **back = (ALLEGRO_SHADER **)_al_vector_alloc_back(&shaders);
   *back = (ALLEGRO_SHADER *)shader;

   _al_add_display_invalidated_callback(al_get_current_display(), _al_d3d_on_lost_shaders);
   _al_add_display_validated_callback(al_get_current_display(), _al_d3d_on_reset_shaders);

   return (ALLEGRO_SHADER *)shader;
}

static bool hlsl_attach_shader_source(ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *source)
{
   bool add_technique;
   ALLEGRO_USTR *full_source;
   LPD3DXBUFFER errors;
   const char *vertex_source, *pixel_source, *technique_source;
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);
   ASSERT(display->flags & ALLEGRO_DIRECT3D);

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

   if (hlsl_shader->shader_model == 3) {
      if (add_technique) {
         if (vertex_source[0] == 0) {
            technique_source = technique_source_pixel_v3;
         }
         else if (pixel_source[0] == 0) {
            technique_source = technique_source_vertex_v3;
         }
         else {
            technique_source = technique_source_both_v3;
         }
      }
      else {
         technique_source = null_source;
      }
   }
   else {
      if (add_technique) {
         if (vertex_source[0] == 0) {
            technique_source = technique_source_pixel_v2;
         }
         else if (pixel_source[0] == 0) {
            technique_source = technique_source_vertex_v2;
         }
         else {
            technique_source = technique_source_both_v2;
         }
      }
      else {
         technique_source = null_source;
      }
   }

   // HLSL likes the source in one buffer
   full_source = al_ustr_newf("%s\n#line 1\n%s\n%s\n",
      vertex_source, pixel_source, technique_source);

   // Release the shader if we already created it
   if (hlsl_shader->hlsl_shader)
      hlsl_shader->hlsl_shader->Release();

   DWORD ok = _al_imp_D3DXCreateEffect(
      al_get_d3d_device(display),
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
      hlsl_shader->hlsl_shader = NULL;
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

static bool hlsl_build_shader(ALLEGRO_SHADER *shader)
{
   (void)shader;
   return true;
}

static bool hlsl_use_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display,
   bool set_projview_matrix_from_display)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   LPD3DXEFFECT effect = hlsl_shader->hlsl_shader;
   ALLEGRO_DISPLAY_D3D *d3d_disp;

   if (!(display->flags & ALLEGRO_DIRECT3D)) {
      return false;
   }
   d3d_disp = (ALLEGRO_DISPLAY_D3D *)display;

   if (set_projview_matrix_from_display) {
      if (!_al_hlsl_set_projview_matrix(effect, &display->projview_transform)) {
         d3d_disp->effect = NULL;
         return false;
      }
   }

   d3d_disp->effect = hlsl_shader->hlsl_shader;
   return true;
}

static void hlsl_unuse_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)display;

   (void)shader;
   //effect->EndPass();
   //effect->End();
   d3d_disp->effect = NULL;
}

static void hlsl_destroy_shader(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;

   if (hlsl_shader->hlsl_shader)
      hlsl_shader->hlsl_shader->Release();

   _al_vector_find_and_delete(&shaders, &shader);

   al_free(shader);
}

static void hlsl_on_lost_device(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   hlsl_shader->hlsl_shader->OnLostDevice();
}

static void hlsl_on_reset_device(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   hlsl_shader->hlsl_shader->OnResetDevice();
}

static bool hlsl_set_shader_sampler(ALLEGRO_SHADER *shader,
   const char *name, ALLEGRO_BITMAP *bitmap, int unit)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   if (al_get_bitmap_flags(bitmap) & ALLEGRO_MEMORY_BITMAP)
      return false;

   LPDIRECT3DTEXTURE9 vid_texture = al_get_d3d_video_texture(bitmap);
   result = hlsl_shader->hlsl_shader->SetTexture(name, vid_texture);
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)_al_get_bitmap_display(bitmap);
   d3d_disp->device->SetTexture(unit, vid_texture);
   _al_set_d3d_sampler_state(d3d_disp->device, unit, bitmap, false);

   return result == D3D_OK;
}

static bool hlsl_set_shader_matrix(ALLEGRO_SHADER *shader,
   const char *name, const ALLEGRO_TRANSFORM *matrix)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   D3DXMATRIX m;

   memcpy(m.m, matrix->m, sizeof(float)*16);

   result = hlsl_shader->hlsl_shader->SetMatrix(name, &m);

   return result == D3D_OK;
}

static bool hlsl_set_shader_int(ALLEGRO_SHADER *shader,
   const char *name, int i)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetInt(name, i);

   return result == D3D_OK;
}

static bool hlsl_set_shader_float(ALLEGRO_SHADER *shader,
   const char *name, float f)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetFloat(name, f);

   return result == D3D_OK;
}

static bool hlsl_set_shader_int_vector(ALLEGRO_SHADER *shader,
   const char *name, int num_components, const int *i, int num_elems)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetIntArray(name, i,
      num_components * num_elems);

   return result == D3D_OK;
}

static bool hlsl_set_shader_float_vector(ALLEGRO_SHADER *shader,
   const char *name, int num_components, const float *f, int num_elems)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetFloatArray(name, f,
      num_components * num_elems);

   return result == D3D_OK;
}

static bool hlsl_set_shader_bool(ALLEGRO_SHADER *shader,
   const char *name, bool b)
{
   ALLEGRO_SHADER_HLSL_S *hlsl_shader = (ALLEGRO_SHADER_HLSL_S *)shader;
   HRESULT result;

   result = hlsl_shader->hlsl_shader->SetBool(name, b);

   return result == D3D_OK;
}

bool _al_hlsl_set_projview_matrix(
   LPD3DXEFFECT effect, const ALLEGRO_TRANSFORM *t)
{
   HRESULT result = effect->SetMatrix(ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX,
      (LPD3DXMATRIX)t->m);
   return result == D3D_OK;
}

void _al_d3d_init_shaders(void)
{
   _al_vector_init(&shaders, sizeof(ALLEGRO_SHADER *));
}

void _al_d3d_shutdown_shaders(void)
{
   _al_vector_free(&shaders);
}

#endif

/* vim: set sts=3 sw=3 et: */
