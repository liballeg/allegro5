#include "allegro5/allegro.h"
#ifdef ALLEGRO_WINDOWS
#include <Cg/cgD3D9.h>
#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>
#endif
#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_shader.h"
#include "allegro5/transformations.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include "shader.h"
#include "shader_cg.h"
#include "allegro5/internal/aintern_shader_cg.h"

ALLEGRO_DEBUG_CHANNEL("shader")

ALLEGRO_SHADER *_al_create_shader_cg(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER_CG_S *shader = (ALLEGRO_SHADER_CG_S *)al_malloc(
      sizeof(ALLEGRO_SHADER_CG_S)
   );

   (void)platform;

   if (!shader)
      return NULL;

   memset(shader, 0, sizeof(ALLEGRO_SHADER_CG_S));

   shader->context = cgCreateContext();
   if (shader->context == 0) {
      al_free(shader);
      return NULL;
   }

#ifdef ALLEGRO_WINDOWS
   if (platform & ALLEGRO_SHADER_HLSL) {
      ALLEGRO_DISPLAY *display = al_get_current_display();
      cgD3D9SetDevice(al_get_d3d_device(display));
   }
#endif

   return (ALLEGRO_SHADER *)shader;
}

bool _al_link_shader_cg(ALLEGRO_SHADER *shader)
{
   (void)shader;
   return true;
}

bool _al_attach_shader_source_cg(
   ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type,
   const char *source)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGprofile profile = CG_PROFILE_VS_2_0; // initialize to silence warning
   CGprogram *program;

#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      if (type == ALLEGRO_VERTEX_SHADER) {
         profile = CG_PROFILE_VS_2_0;
      }
      else {
         profile = CG_PROFILE_PS_2_0;
      }
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      if (type == ALLEGRO_VERTEX_SHADER) {
         profile = CG_PROFILE_ARBVP1;
      }
      else {
         profile = CG_PROFILE_ARBFP1;
      }
   }

   if (type == ALLEGRO_VERTEX_SHADER) {
      program = &cg_shader->vertex_program;
   }
   else {
      program = &cg_shader->pixel_program;
   }
   
   *program = cgCreateProgram(
      cg_shader->context,
      CG_SOURCE,
      source,
      profile,
      type == ALLEGRO_VERTEX_SHADER ? "vs_main" : "ps_main",
      NULL
   );
   
   if (*program == 0) {
      const char *msg = cgGetLastListing(cg_shader->context);
      if (shader->log) {
         al_ustr_truncate(shader->log, 0);
         al_ustr_append_cstr(shader->log, msg);
      }
      else {
         shader->log = al_ustr_new(msg);
      }
      ALLEGRO_ERROR("Error: %s\n", msg);
      return false;
   }

#ifdef ALLEGRO_WINDOWS
	/*
   if (shader->platform & ALLEGRO_SHADER_HLSL && type == ALLEGRO_VERTEX_SHADER) {
      LPDIRECT3DDEVICE9 dev = al_get_d3d_device(al_get_current_display());
      const D3DVERTEXELEMENT9 declaration[] = {
         {
	    0, 0 * sizeof(float),
            D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,
            D3DDECLUSAGE_POSITION, 0
	 },
         {
            0, 3 * sizeof(float),
            D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,
            D3DDECLUSAGE_COLOR, 0
	 },
         {
            0, 4 * sizeof(float),
            D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT,
            D3DDECLUSAGE_TEXCOORD, 0
         },
         D3DDECL_END()
      };
      IDirect3DDevice9_CreateVertexDeclaration(dev, declaration,
         &cg_shader->vertex_decl);
   }
   */
#endif

#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      cgD3D9LoadProgram(*program, CG_FALSE, 0);
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      cgGLLoadProgram(*program);
   }
   
   if (cg_shader->vertex_program) {
      // get named params
      cg_shader->name_pos =
         cgGetNamedParameter(cg_shader->vertex_program, "pos");
      cg_shader->name_col =
         cgGetNamedParameter(cg_shader->vertex_program, "color");
      cg_shader->name_tex =
         cgGetNamedParameter(cg_shader->vertex_program, "texcoord");
      cg_shader->name_projview =
         cgGetNamedParameter(cg_shader->vertex_program, "projview_matrix");
   }

   return true;
}

static void _al_set_shader_matrix_cg_name(ALLEGRO_SHADER *shader,
   CGparameter name, float *m)
{
#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      cgD3D9SetUniform(name, m);
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      cgSetMatrixParameterfr(name, m);
   }
}


void _al_use_shader_cg(ALLEGRO_SHADER *shader, bool use)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   ALLEGRO_DISPLAY *display = al_get_current_display();

   if (use) {
      if (cg_shader->name_projview) {
         ALLEGRO_TRANSFORM t;
	 al_copy_transform(&t, &display->view_transform);
	 al_compose_transform(&t, &display->proj_transform);
         _al_set_shader_matrix_cg_name(
	    shader,
	    cg_shader->name_projview,
	    (float *)t.m
	 );
      }
      if (shader->platform & ALLEGRO_SHADER_GLSL) {
         if (cg_shader->vertex_program) {
            cgGLBindProgram(cg_shader->vertex_program);
            cgGLEnableProfile(CG_PROFILE_ARBVP1);
            if (cg_shader->name_pos)
               cgGLEnableClientState(cg_shader->name_pos);
            if (cg_shader->name_col)
               cgGLEnableClientState(cg_shader->name_col);
            if (cg_shader->name_tex)
               cgGLEnableClientState(cg_shader->name_tex);
         }
         if (cg_shader->pixel_program) {
            cgGLBindProgram(cg_shader->pixel_program);
            cgGLEnableProfile(CG_PROFILE_ARBFP1);
         }
      }
#ifdef ALLEGRO_WINDOWS
      else {
         if (cg_shader->vertex_program) {
	    //LPDIRECT3DDEVICE9 dev = al_get_d3d_device(al_get_current_display());
	    //IDirect3DDevice9_SetVertexDeclaration(dev, cg_shader->vertex_decl);
            cgD3D9BindProgram(cg_shader->vertex_program);
         }
         if (cg_shader->pixel_program) {
            cgD3D9BindProgram(cg_shader->pixel_program);
      }
    }
#endif
   }
   else {
      if (shader->platform & ALLEGRO_SHADER_GLSL) {
         if (cg_shader->vertex_program)
            cgGLUnbindProgram(CG_PROFILE_ARBVP1);
         if (cg_shader->pixel_program)
            cgGLUnbindProgram(CG_PROFILE_ARBFP1);
      }
#ifdef ALLEGRO_WINDOWS
      else {
         // FIXME: There doesn't seem to be a way to unbind a d3d9 program...
      }
#endif
   }
}

void _al_destroy_shader_cg(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;

#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      cgD3D9SetDevice(0);
   }
#endif

   if (cg_shader->vertex_program)
      cgDestroyProgram(cg_shader->vertex_program);
   if (cg_shader->pixel_program)
      cgDestroyProgram(cg_shader->pixel_program);

   cgDestroyContext(cg_shader->context);

   al_free(shader);
}

bool _al_set_shader_sampler_cg(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   (void)unit;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP)
      return false;
   
   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      if (v_param != 0) {
         cgD3D9SetTexture(v_param, al_get_d3d_video_texture(bitmap));
      }
      if (p_param != 0) {
         cgD3D9SetTexture(p_param, al_get_d3d_video_texture(bitmap));
      }
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      if (v_param != 0) {
         cgGLSetTextureParameter(v_param, al_get_opengl_texture(bitmap));
      }
      if (p_param != 0) {
         cgGLSetTextureParameter(p_param, al_get_opengl_texture(bitmap));
      }
   }
     
   return true;
}

bool _al_set_shader_matrix_cg(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param;

   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);

   if (v_param == 0) {
      return false;
   }

   _al_set_shader_matrix_cg_name(shader, v_param, (float *)matrix->m);
   
   return true;
}

bool _al_set_shader_int_cg(ALLEGRO_SHADER *shader, const char *name, int i)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      cgSetParameterValueic(v_param, 1, &i);
   }
   if (p_param != 0) {
      cgSetParameterValueic(p_param, 1, &i);
   }

   return true;
}

bool _al_set_shader_float_cg(ALLEGRO_SHADER *shader, const char *name, float f)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      cgSetParameterValuefc(v_param, 1, &f);
   }
   if (p_param != 0) {
      cgSetParameterValuefc(p_param, 1, &f);
   }

   return true;
}

bool _al_set_shader_int_vector_cg(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, int *i, int num_elems)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      cgSetParameterValueic(v_param, elem_size * num_elems, i);
   }
   if (p_param != 0) {
      cgSetParameterValueic(p_param, elem_size, i);
   }

   return true;
}

bool _al_set_shader_float_vector_cg(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      cgSetParameterValuefc(v_param, elem_size * num_elems, f);
   }
   if (p_param != 0) {
      cgSetParameterValuefc(p_param, elem_size * num_elems, f);
   }
   
   return true;
}

bool _al_set_shader_bool_cg(ALLEGRO_SHADER *shader, const char *name, bool b)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      cgSetParameter1i(v_param, b);
   }
   if (p_param != 0) {
      cgSetParameter1i(p_param, b);
   }

   return true;
}

bool _al_set_shader_vertex_array_cg(ALLEGRO_SHADER *shader, float *v, int stride)
{
  if (shader->platform & ALLEGRO_SHADER_GLSL) {
     if (v == NULL) {
        glDisableClientState(GL_VERTEX_ARRAY);
     }
     else {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, stride, v);
     }
  }

   return true;
}

bool _al_set_shader_color_array_cg(ALLEGRO_SHADER *shader, unsigned char *c, int stride)
{
  if (shader->platform & ALLEGRO_SHADER_GLSL) {
     if (c == NULL) {
        glDisableClientState(GL_COLOR_ARRAY);
     }
     else {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, stride, c);
     }
  }

   return true;
}

bool _al_set_shader_texcoord_array_cg(ALLEGRO_SHADER *shader, float *u, int stride)
{
  if (shader->platform & ALLEGRO_SHADER_GLSL) {
     if (u == NULL) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
     }
     else {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, stride, u);
     }
  }

   return true;
}

/* vim: set sts=3 sw=3 et: */
