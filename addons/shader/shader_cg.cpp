#include "allegro5/allegro.h"
#ifdef ALLEGRO_WINDOWS
#include <Cg/cgD3D9.h>
#include "allegro5/allegro_direct3d.h"
#include <d3dx9.h>
#endif
#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_shader.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_system.h"

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#include "shader.h"

ALLEGRO_DEBUG_CHANNEL("shader")


typedef struct ALLEGRO_SHADER_CG_S ALLEGRO_SHADER_CG_S;

struct ALLEGRO_SHADER_CG_S
{
   ALLEGRO_SHADER shader;
   CGcontext context;
   CGprogram vertex_program;
   CGprogram pixel_program;
   CGparameter name_pos;
   CGparameter name_col;
   CGparameter name_tex;
   CGparameter name_projview;
#ifdef ALLEGRO_WINDOWS
   LPDIRECT3DVERTEXDECLARATION9 vertex_decl;
#endif
};


/* Module loading only used on Windows, at least for now. */
#ifdef ALLEGRO_WINDOWS
   #define CG_MODULE_LOADING
#endif

typedef CGcontext (CGENTRY *CGCREATECONTEXTPROC)(void);
typedef CGprogram (CGENTRY *CGCREATEPROGRAMPROC)(CGcontext context, CGenum program_type, const char *program, CGprofile profile, const char *entry, const char **args);
typedef void (CGENTRY *CGDESTROYCONTEXTPROC)(CGcontext context);
typedef void (CGENTRY *CGDESTROYPROGRAMPROC)(CGprogram program);
typedef const char * (CGENTRY *CGGETLASTLISTINGPROC)(CGcontext context);
typedef CGparameter (CGENTRY *CGGETNAMEDPARAMETERPROC)(CGprogram program, const char *name);
typedef void (CGENTRY *CGSETMATRIXPARAMETERFRPROC)(CGparameter param, const float *matrix);
typedef void (CGENTRY *CGSETPARAMETER1IPROC)(CGparameter param, int x);
typedef void (CGENTRY *CGSETPARAMETERVALUEFCPROC)(CGparameter param, int nelements, const float *vals);
typedef void (CGENTRY *CGSETPARAMETERVALUEICPROC)(CGparameter param, int nelements, const int *vals);

typedef void (CGGLENTRY *CGGLBINDPROGRAMPROC)(CGprogram program);
typedef void (CGGLENTRY *CGGLENABLECLIENTSTATEPROC)(CGparameter param);
typedef void (CGGLENTRY *CGGLENABLEPROFILEPROC)(CGprofile profile);
typedef void (CGGLENTRY *CGGLLOADPROGRAMPROC)(CGprogram program);
typedef void (CGGLENTRY *CGGLSETTEXTUREPARAMETERPROC)(CGparameter param, GLuint texobj);
typedef void (CGGLENTRY *CGGLUNBINDPROGRAMPROC)(CGprofile profile);

#ifdef ALLEGRO_WINDOWS
typedef HRESULT (CGD3D9ENTRY *CGD3D9BINDPROGRAMPROC)(CGprogram prog);
typedef HRESULT (CGD3D9ENTRY *CGD3D9LOADPROGRAMPROC)(CGprogram prog, CGbool paramShadowing, DWORD assemFlags);
typedef HRESULT (CGD3D9ENTRY *CGD3D9SETDEVICEPROC)(IDirect3DDevice9 *pDevice);
typedef HRESULT (CGD3D9ENTRY *CGD3D9SETTEXTUREPROC)(CGparameter param, IDirect3DBaseTexture9 *tex);
typedef HRESULT (CGD3D9ENTRY *CGD3D9SETUNIFORMPROC)(CGparameter param, const void *floats);
#endif

#ifdef CG_MODULE_LOADING
   #define INITSYM(sym) (NULL)
   static void* _imp_cg_module = 0;
   static void* _imp_cggl_module = 0;
   static void* _imp_cgd3d9_module = 0;
#else
   #define INITSYM(sym) (sym)
#endif

CGCREATECONTEXTPROC           _imp_cgCreateContext = INITSYM(cgCreateContext);
CGCREATEPROGRAMPROC           _imp_cgCreateProgram = INITSYM(cgCreateProgram);
CGDESTROYCONTEXTPROC          _imp_cgDestroyContext = INITSYM(cgDestroyContext);
CGDESTROYPROGRAMPROC          _imp_cgDestroyProgram = INITSYM(cgDestroyProgram);
CGGETLASTLISTINGPROC          _imp_cgGetLastListing = INITSYM(cgGetLastListing);
CGGETNAMEDPARAMETERPROC       _imp_cgGetNamedParameter = INITSYM(cgGetNamedParameter);
CGSETMATRIXPARAMETERFRPROC    _imp_cgSetMatrixParameterfr = INITSYM(cgSetMatrixParameterfr);
CGSETPARAMETER1IPROC          _imp_cgSetParameter1i = INITSYM(cgSetParameter1i);
CGSETPARAMETERVALUEFCPROC     _imp_cgSetParameterValuefc = INITSYM(cgSetParameterValuefc);
CGSETPARAMETERVALUEICPROC     _imp_cgSetParameterValueic = INITSYM(cgSetParameterValueic);

CGGLBINDPROGRAMPROC           _imp_cgGLBindProgram = INITSYM(cgGLBindProgram);
CGGLENABLECLIENTSTATEPROC     _imp_cgGLEnableClientState = INITSYM(cgGLEnableClientState);
CGGLENABLEPROFILEPROC         _imp_cgGLEnableProfile = INITSYM(cgGLEnableProfile);
CGGLLOADPROGRAMPROC           _imp_cgGLLoadProgram = INITSYM(cgGLLoadProgram);
CGGLSETTEXTUREPARAMETERPROC   _imp_cgGLSetTextureParameter = INITSYM(cgGLSetTextureParameter);
CGGLUNBINDPROGRAMPROC         _imp_cgGLUnbindProgram = INITSYM(cgGLUnbindProgram);

#ifdef ALLEGRO_WINDOWS
CGD3D9BINDPROGRAMPROC         _imp_cgD3D9BindProgram = INITSYM(cgD3D9BindProgram);
CGD3D9LOADPROGRAMPROC         _imp_cgD3D9LoadProgram = INITSYM(cgD3D9LoadProgram);
CGD3D9SETDEVICEPROC           _imp_cgD3D9SetDevice = INITSYM(cgD3D9SetDevice);
CGD3D9SETTEXTUREPROC          _imp_cgD3D9SetTexture = INITSYM(cgD3D9SetTexture);
CGD3D9SETUNIFORMPROC          _imp_cgD3D9SetUniform = INITSYM(cgD3D9SetUniform);
#endif

#ifdef CG_MODULE_LOADING
static void _imp_unload_cg_module(void* module)
{
   _al_unregister_destructor(_al_dtor_list, module);

   _al_close_library(_imp_cg_module);
   _imp_cg_module = NULL;

   _imp_cgCreateContext = NULL;
   _imp_cgCreateProgram = NULL;
   _imp_cgDestroyContext = NULL;
   _imp_cgDestroyProgram = NULL;
   _imp_cgGetLastListing = NULL;
   _imp_cgGetNamedParameter = NULL;
   _imp_cgSetMatrixParameterfr = NULL;
   _imp_cgSetParameter1i = NULL;
   _imp_cgSetParameterValuefc = NULL;
   _imp_cgSetParameterValueic = NULL;
}

static bool _imp_load_cg_module()
{
   _imp_cg_module = _al_open_library("cg.dll");
   if (NULL == _imp_cg_module) {
      ALLEGRO_ERROR("Failed to load cg.dll.");
      return false;
   }

   _imp_cgCreateContext        = (CGCREATECONTEXTPROC)       _al_import_symbol(_imp_cg_module, "cgCreateContext");
   _imp_cgCreateProgram        = (CGCREATEPROGRAMPROC)       _al_import_symbol(_imp_cg_module, "cgCreateProgram");
   _imp_cgDestroyContext       = (CGDESTROYCONTEXTPROC)      _al_import_symbol(_imp_cg_module, "cgDestroyContext");
   _imp_cgDestroyProgram       = (CGDESTROYPROGRAMPROC)      _al_import_symbol(_imp_cg_module, "cgDestroyProgram");
   _imp_cgGetLastListing       = (CGGETLASTLISTINGPROC)      _al_import_symbol(_imp_cg_module, "cgGetLastListing");
   _imp_cgGetNamedParameter    = (CGGETNAMEDPARAMETERPROC)   _al_import_symbol(_imp_cg_module, "cgGetNamedParameter");
   _imp_cgSetMatrixParameterfr = (CGSETMATRIXPARAMETERFRPROC)_al_import_symbol(_imp_cg_module, "cgSetMatrixParameterfr");
   _imp_cgSetParameter1i       = (CGSETPARAMETER1IPROC)      _al_import_symbol(_imp_cg_module, "cgSetParameter1i");
   _imp_cgSetParameterValuefc  = (CGSETPARAMETERVALUEFCPROC) _al_import_symbol(_imp_cg_module, "cgSetParameterValuefc");
   _imp_cgSetParameterValueic  = (CGSETPARAMETERVALUEICPROC) _al_import_symbol(_imp_cg_module, "cgSetParameterValueic");

   if (!_imp_cgCreateContext || !_imp_cgCreateProgram || !_imp_cgDestroyContext ||
       !_imp_cgDestroyProgram || !_imp_cgGetLastListing || !_imp_cgGetNamedParameter ||
       !_imp_cgSetMatrixParameterfr || !_imp_cgSetParameter1i || !_imp_cgSetParameterValuefc ||
       !_imp_cgSetParameterValueic) {
      ALLEGRO_ERROR("Failed to import required symbols from cg.dll.");
      _imp_unload_cg_module(_imp_cg_module);
      return false;
   }

   _al_register_destructor(_al_dtor_list, _imp_cg_module, _imp_unload_cg_module);

   return true;
}

static void _imp_unload_cggl_module(void* module)
{
   _al_unregister_destructor(_al_dtor_list, module);

   _al_close_library(_imp_cggl_module);
   _imp_cggl_module = NULL;

   _imp_cgGLBindProgram = NULL;
   _imp_cgGLEnableClientState = NULL;
   _imp_cgGLEnableProfile = NULL;
   _imp_cgGLLoadProgram = NULL;
   _imp_cgGLSetTextureParameter = NULL;
   _imp_cgGLUnbindProgram = NULL;
}

static bool _imp_load_cggl_module()
{
   _imp_cggl_module = _al_open_library("cgGL.dll");
   if (NULL == _imp_cggl_module) {
      ALLEGRO_ERROR("Failed to load cgGL.dll.");
      return false;
   }

   _imp_cgGLBindProgram         = (CGGLBINDPROGRAMPROC)        _al_import_symbol(_imp_cggl_module, "cgGLBindProgram");
   _imp_cgGLEnableClientState   = (CGGLENABLECLIENTSTATEPROC)  _al_import_symbol(_imp_cggl_module, "cgGLEnableClientState");
   _imp_cgGLEnableProfile       = (CGGLENABLEPROFILEPROC)      _al_import_symbol(_imp_cggl_module, "cgGLEnableProfile");
   _imp_cgGLLoadProgram         = (CGGLLOADPROGRAMPROC)        _al_import_symbol(_imp_cggl_module, "cgGLLoadProgram");
   _imp_cgGLSetTextureParameter = (CGGLSETTEXTUREPARAMETERPROC)_al_import_symbol(_imp_cggl_module, "cgGLSetTextureParameter");
   _imp_cgGLUnbindProgram       = (CGGLUNBINDPROGRAMPROC)      _al_import_symbol(_imp_cggl_module, "cgGLUnbindProgram");

   if (!_imp_cgGLBindProgram || !_imp_cgGLEnableClientState || !_imp_cgGLEnableProfile ||
       !_imp_cgGLLoadProgram || !_imp_cgGLSetTextureParameter || !_imp_cgGLUnbindProgram) {
      ALLEGRO_ERROR("Failed to import required symbols from cgGL.dll.");
      _imp_unload_cggl_module(_imp_cggl_module);
      return false;
   }

   _al_register_destructor(_al_dtor_list, _imp_cggl_module, _imp_unload_cggl_module);

   return true;
}

#ifdef ALLEGRO_WINDOWS
static void _imp_unload_cgd3d9_module(void* module)
{
   _al_unregister_destructor(_al_dtor_list, module);

   _al_close_library(_imp_cgd3d9_module);
   _imp_cgd3d9_module = NULL;

   _imp_cgD3D9BindProgram = NULL;
   _imp_cgD3D9LoadProgram = NULL;
   _imp_cgD3D9SetDevice = NULL;
   _imp_cgD3D9SetTexture = NULL;
   _imp_cgD3D9SetUniform = NULL;
}

static bool _imp_load_cgd3d9_module()
{
   _imp_cgd3d9_module = _al_open_library("cgD3D9.dll");
   if (NULL == _imp_cgd3d9_module) {
      ALLEGRO_ERROR("Failed to load cgD3D9.dll.");
      return false;
   }

    _imp_cgD3D9BindProgram = (CGD3D9BINDPROGRAMPROC)_al_import_symbol(_imp_cgd3d9_module, "cgD3D9BindProgram");
    _imp_cgD3D9LoadProgram = (CGD3D9LOADPROGRAMPROC)_al_import_symbol(_imp_cgd3d9_module, "cgD3D9LoadProgram");
    _imp_cgD3D9SetDevice   = (CGD3D9SETDEVICEPROC)  _al_import_symbol(_imp_cgd3d9_module, "cgD3D9SetDevice");
    _imp_cgD3D9SetTexture  = (CGD3D9SETTEXTUREPROC) _al_import_symbol(_imp_cgd3d9_module, "cgD3D9SetTexture");
    _imp_cgD3D9SetUniform  = (CGD3D9SETUNIFORMPROC) _al_import_symbol(_imp_cgd3d9_module, "cgD3D9SetUniform");

   if (!_imp_cgD3D9BindProgram || !_imp_cgD3D9LoadProgram || !_imp_cgD3D9SetDevice ||
       !_imp_cgD3D9SetTexture || !_imp_cgD3D9SetUniform) {
      ALLEGRO_ERROR("Failed to import required symbols from cgD3D9.dll.");
      _imp_unload_cgd3d9_module(_imp_cgd3d9_module);
      return false;
   }

   _al_register_destructor(_al_dtor_list, _imp_cgd3d9_module, _imp_unload_cgd3d9_module);

   return true;
}
#endif /* ALLEGRO_WINDOWS */
#endif /* CG_MODULE_LOADING */


static bool cg_link_shader(ALLEGRO_SHADER *shader);
static bool cg_attach_shader_source(ALLEGRO_SHADER *shader,
               ALLEGRO_SHADER_TYPE type, const char *source);
static void cg_use_shader(ALLEGRO_SHADER *shader, bool use);
static void cg_destroy_shader(ALLEGRO_SHADER *shader);
static bool cg_set_shader_sampler(ALLEGRO_SHADER *shader,
               const char *name, ALLEGRO_BITMAP *bitmap, int unit);
static bool cg_set_shader_matrix(ALLEGRO_SHADER *shader,
               const char *name, ALLEGRO_TRANSFORM *matrix);
static bool cg_set_shader_int(ALLEGRO_SHADER *shader,
               const char *name, int i);
static bool cg_set_shader_float(ALLEGRO_SHADER *shader,
               const char *name, float f);
static bool cg_set_shader_int_vector(ALLEGRO_SHADER *shader,
               const char *name, int elem_size, int *i, int num_elems);
static bool cg_set_shader_float_vector(ALLEGRO_SHADER *shader,
               const char *name, int elem_size, float *f, int num_elems);
static bool cg_set_shader_bool(ALLEGRO_SHADER *shader,
               const char *name, bool b);
static bool cg_set_shader_vertex_array(ALLEGRO_SHADER *shader,
               float *v, int stride);
static bool cg_set_shader_color_array(ALLEGRO_SHADER *shader,
               unsigned char *c, int stride);
static bool cg_set_shader_texcoord_array(ALLEGRO_SHADER *shader,
               float *u, int stride);
static void cg_set_shader(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader);

static struct ALLEGRO_SHADER_INTERFACE shader_cg_vt =
{
   cg_link_shader,
   cg_attach_shader_source,
   cg_use_shader,
   cg_destroy_shader,
   cg_set_shader_sampler,
   cg_set_shader_matrix,
   cg_set_shader_int,
   cg_set_shader_float,
   cg_set_shader_int_vector,
   cg_set_shader_float_vector,
   cg_set_shader_bool,
   cg_set_shader_vertex_array,
   cg_set_shader_color_array,
   cg_set_shader_texcoord_array,
   cg_set_shader
};


ALLEGRO_SHADER *_al_create_shader_cg(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER_CG_S *shader;

   shader = (ALLEGRO_SHADER_CG_S *)al_malloc(sizeof(ALLEGRO_SHADER_CG_S));
   if (!shader)
      return NULL;

#ifdef CG_MODULE_LOADING
   if (!_imp_cg_module && !_imp_load_cg_module())
      return NULL;

   if (platform & ALLEGRO_SHADER_GLSL) {
      if (!_imp_cggl_module && !_imp_load_cggl_module())
         return NULL;
   }

   #ifdef ALLEGRO_WINDOWS
   if (platform & ALLEGRO_SHADER_HLSL) {
      if (!_imp_cgd3d9_module && !_imp_load_cgd3d9_module())
         return NULL;
   }
   #endif /* ALLEGRO_WINDOWS */
#endif /* CG_MODULE_LOADING */

   memset(shader, 0, sizeof(ALLEGRO_SHADER_CG_S));
   shader->shader.platform = platform;
   shader->shader.vt = &shader_cg_vt;

   shader->context = _imp_cgCreateContext();
   if (shader->context == 0) {
      al_free(shader);
      return NULL;
   }

#ifdef ALLEGRO_WINDOWS
   if (platform & ALLEGRO_SHADER_HLSL) {
      ALLEGRO_DISPLAY *display = al_get_current_display();
      _imp_cgD3D9SetDevice(al_get_d3d_device(display));
   }
#endif

   return (ALLEGRO_SHADER *)shader;
}

static bool cg_attach_shader_source(ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *source)
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

   *program = _imp_cgCreateProgram(
      cg_shader->context,
      CG_SOURCE,
      source,
      profile,
      type == ALLEGRO_VERTEX_SHADER ? "vs_main" : "ps_main",
      NULL
   );

   if (*program == 0) {
      const char *msg = _imp_cgGetLastListing(cg_shader->context);
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
      _imp_cgD3D9LoadProgram(*program, CG_FALSE, 0);
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      _imp_cgGLLoadProgram(*program);
   }

   if (cg_shader->vertex_program) {
      // get named params
      cg_shader->name_pos =
         _imp_cgGetNamedParameter(cg_shader->vertex_program, ALLEGRO_SHADER_VAR_POS);
      cg_shader->name_col =
         _imp_cgGetNamedParameter(cg_shader->vertex_program, ALLEGRO_SHADER_VAR_COLOR);
      cg_shader->name_tex =
         _imp_cgGetNamedParameter(cg_shader->vertex_program, ALLEGRO_SHADER_VAR_TEXCOORD);
      cg_shader->name_projview =
         _imp_cgGetNamedParameter(cg_shader->vertex_program, ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX);
   }

   return true;
}

static bool cg_link_shader(ALLEGRO_SHADER *shader)
{
   (void)shader;
   return true;
}

static void do_set_shader_matrix(ALLEGRO_SHADER *shader,
   CGparameter name, float *m)
{
#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      _imp_cgD3D9SetUniform(name, m);
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      _imp_cgSetMatrixParameterfr(name, m);
   }
}

static void cg_use_shader(ALLEGRO_SHADER *shader, bool use)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   ALLEGRO_DISPLAY *display = al_get_current_display();

   if (use) {
      if (cg_shader->name_projview) {
         ALLEGRO_TRANSFORM t;
	 al_copy_transform(&t, &display->view_transform);
	 al_compose_transform(&t, &display->proj_transform);
         do_set_shader_matrix(shader, cg_shader->name_projview, (float *)t.m);
      }
      if (shader->platform & ALLEGRO_SHADER_GLSL) {
         if (cg_shader->vertex_program) {
            _imp_cgGLBindProgram(cg_shader->vertex_program);
            _imp_cgGLEnableProfile(CG_PROFILE_ARBVP1);
            if (cg_shader->name_pos)
               _imp_cgGLEnableClientState(cg_shader->name_pos);
            if (cg_shader->name_col)
               _imp_cgGLEnableClientState(cg_shader->name_col);
            if (cg_shader->name_tex)
               _imp_cgGLEnableClientState(cg_shader->name_tex);
         }
         if (cg_shader->pixel_program) {
            _imp_cgGLBindProgram(cg_shader->pixel_program);
            _imp_cgGLEnableProfile(CG_PROFILE_ARBFP1);
         }
      }
#ifdef ALLEGRO_WINDOWS
      else {
         if (cg_shader->vertex_program) {
	    //LPDIRECT3DDEVICE9 dev = al_get_d3d_device(al_get_current_display());
	    //IDirect3DDevice9_SetVertexDeclaration(dev, cg_shader->vertex_decl);
            _imp_cgD3D9BindProgram(cg_shader->vertex_program);
         }
         if (cg_shader->pixel_program) {
            _imp_cgD3D9BindProgram(cg_shader->pixel_program);
         }
      }
#endif
   }
   else {
      if (shader->platform & ALLEGRO_SHADER_GLSL) {
         if (cg_shader->vertex_program)
            _imp_cgGLUnbindProgram(CG_PROFILE_ARBVP1);
         if (cg_shader->pixel_program)
            _imp_cgGLUnbindProgram(CG_PROFILE_ARBFP1);
      }
#ifdef ALLEGRO_WINDOWS
      else {
         // FIXME: There doesn't seem to be a way to unbind a d3d9 program...
      }
#endif
   }
}

static void cg_destroy_shader(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;

#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      _imp_cgD3D9SetDevice(0);
   }
#endif

   if (cg_shader->vertex_program)
      _imp_cgDestroyProgram(cg_shader->vertex_program);
   if (cg_shader->pixel_program)
      _imp_cgDestroyProgram(cg_shader->pixel_program);

   _imp_cgDestroyContext(cg_shader->context);

   al_free(shader);
}

static bool cg_set_shader_sampler(ALLEGRO_SHADER *shader,
   const char *name, ALLEGRO_BITMAP *bitmap, int unit)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   (void)unit;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP)
      return false;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = _imp_cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

#ifdef ALLEGRO_WINDOWS
   if (shader->platform & ALLEGRO_SHADER_HLSL) {
      if (v_param != 0) {
         _imp_cgD3D9SetTexture(v_param, al_get_d3d_video_texture(bitmap));
      }
      if (p_param != 0) {
         _imp_cgD3D9SetTexture(p_param, al_get_d3d_video_texture(bitmap));
      }
   }
#endif
   if (shader->platform & ALLEGRO_SHADER_GLSL) {
      if (v_param != 0) {
         _imp_cgGLSetTextureParameter(v_param, al_get_opengl_texture(bitmap));
      }
      if (p_param != 0) {
         _imp_cgGLSetTextureParameter(p_param, al_get_opengl_texture(bitmap));
      }
   }

   return true;
}

static bool cg_set_shader_matrix(ALLEGRO_SHADER *shader,
   const char *name, ALLEGRO_TRANSFORM *matrix)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);

   if (v_param == 0) {
      return false;
   }

   do_set_shader_matrix(shader, v_param, (float *)matrix->m);

   return true;
}

static bool cg_set_shader_int(ALLEGRO_SHADER *shader, const char *name, int i)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = _imp_cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      _imp_cgSetParameterValueic(v_param, 1, &i);
   }
   if (p_param != 0) {
      _imp_cgSetParameterValueic(p_param, 1, &i);
   }

   return true;
}

static bool cg_set_shader_float(ALLEGRO_SHADER *shader,
   const char *name, float f)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = _imp_cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      _imp_cgSetParameterValuefc(v_param, 1, &f);
   }
   if (p_param != 0) {
      _imp_cgSetParameterValuefc(p_param, 1, &f);
   }

   return true;
}

static bool cg_set_shader_int_vector(ALLEGRO_SHADER *shader,
   const char *name, int elem_size, int *i, int num_elems)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = _imp_cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      _imp_cgSetParameterValueic(v_param, elem_size * num_elems, i);
   }
   if (p_param != 0) {
      _imp_cgSetParameterValueic(p_param, elem_size, i);
   }

   return true;
}

static bool cg_set_shader_float_vector(ALLEGRO_SHADER *shader,
   const char *name, int elem_size, float *f, int num_elems)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = _imp_cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      _imp_cgSetParameterValuefc(v_param, elem_size * num_elems, f);
   }
   if (p_param != 0) {
      _imp_cgSetParameterValuefc(p_param, elem_size * num_elems, f);
   }

   return true;
}

static bool cg_set_shader_bool(ALLEGRO_SHADER *shader,
   const char *name, bool b)
{
   ALLEGRO_SHADER_CG_S *cg_shader = (ALLEGRO_SHADER_CG_S *)shader;
   CGparameter v_param, p_param;

   v_param = _imp_cgGetNamedParameter(cg_shader->vertex_program, name);
   p_param = _imp_cgGetNamedParameter(cg_shader->pixel_program, name);

   if (v_param == 0 && p_param == 0)
      return false;

   if (v_param != 0) {
      _imp_cgSetParameter1i(v_param, b);
   }
   if (p_param != 0) {
      _imp_cgSetParameter1i(p_param, b);
   }

   return true;
}

static bool cg_set_shader_vertex_array(ALLEGRO_SHADER *shader,
   float *v, int stride)
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

static bool cg_set_shader_color_array(ALLEGRO_SHADER *shader,
   unsigned char *c, int stride)
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

static bool cg_set_shader_texcoord_array(ALLEGRO_SHADER *shader,
   float *u, int stride)
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

static void cg_set_shader(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader)
{
   /* XXX this was never implemented */
   (void)display;
   (void)shader;
   ALLEGRO_WARN("cg_set_shader not implemented\n");
}

/* vim: set sts=3 sw=3 et: */
