#include "allegro5/allegro.h"
#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>
#include <stdio.h>
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

typedef HRESULT (WINAPI *D3DXCREATEEFFECTPROC)(LPDIRECT3DDEVICE9, LPCVOID, UINT,
   CONST D3DXMACRO*, LPD3DXINCLUDE, DWORD, LPD3DXEFFECTPOOL, LPD3DXEFFECT*,
   LPD3DXBUFFER*);

static HMODULE _imp_d3dx9_module = 0;
static D3DXCREATEEFFECTPROC _imp_D3DXCreateEffect = NULL;

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

static void _imp_unload_d3dx9_module(void* handle)
{
   _al_unregister_destructor(_al_dtor_list, handle);

   FreeLibrary(_imp_d3dx9_module);
   _imp_d3dx9_module = NULL;
}

static bool _imp_load_d3dx9_module_version(int version)
{
   char module_name[16];

   // Sanity check
   // Comented out, to not reject choice of the user if any new version
   // appears. See force_d3dx9_version entry in config file.
   /*
   if (version < 24 || version > 43) {
      ALLEGRO_ERROR("Error: Requested version (%d) of D3DX9 library is invalid.\n", version);
      return false;
   }
   */

   sprintf(module_name, "d3dx9_%d.dll", version);

   _imp_d3dx9_module = _al_win_safe_load_library(module_name);
   if (NULL == _imp_d3dx9_module)
      return false;

   _imp_D3DXCreateEffect = (D3DXCREATEEFFECTPROC)GetProcAddress(_imp_d3dx9_module, "D3DXCreateEffect");
   if (NULL == _imp_D3DXCreateEffect) {
      FreeLibrary(_imp_d3dx9_module);
      _imp_d3dx9_module = NULL;
      return false;
   }

   _al_register_destructor(_al_dtor_list, (void*)_imp_d3dx9_module, _imp_unload_d3dx9_module);

   ALLEGRO_INFO("Module \"%s\" loaded.\n", module_name);

   return true;
}

static bool _imp_load_d3dx9_module()
{
   ALLEGRO_CONFIG *cfg;
   const char *str;
   long version;

   cfg = al_get_system_config();
   if (cfg) {
      char const *value = al_get_config_value(cfg,
         "shader", "force_d3dx9_version");
      if (value) {
         errno = 0;
         version = strtol(value, NULL, 10);
         if (errno) {
            ALLEGRO_ERROR("Failed to override D3DX9 version. \"%s\" is not valid integer number.", value);
            return false;
         }
         else
            return _imp_load_d3dx9_module_version((int)version);
      }
   }

   // Iterate over all valid versions.
   //
   // DXSDK redistributable install d3dx9_xx.dll from version
   // 24 to 43. It's unlikely that any new version will come,
   // since HLSL compiler was moved to D3DCompiler_xx.dll and
   // most recent versions of this utility library are bound
   // to DirectX 11.
   //
   // However, if any new version appears anyway, range in this
   // loop should be changed.
   for (version = 43; version >= 24; --version)
      if (_imp_load_d3dx9_module_version((int)version))
         return true;

   ALLEGRO_ERROR("Failed to load D3DX9 library. Library is not installed.");

   return false;
}

ALLEGRO_SHADER *_al_create_shader_hlsl(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER_HLSL_S *shader = (ALLEGRO_SHADER_HLSL_S *)al_malloc(
      sizeof(ALLEGRO_SHADER_HLSL_S)
   );

   (void)platform;

   if (!shader)
      return NULL;

   if (NULL == _imp_D3DXCreateEffect && !_imp_load_d3dx9_module())
      return false;

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

   DWORD ok = _imp_D3DXCreateEffect(
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
