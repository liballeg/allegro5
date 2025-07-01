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
 *      OpenGL shader support.
 *
 *      See LICENSE.txt for copyright information.
 */

#include <stdio.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_shader.h"

#ifdef ALLEGRO_MSVC
   #define snprintf _snprintf
#endif

#ifdef ALLEGRO_CFG_SHADER_GLSL

ALLEGRO_DEBUG_CHANNEL("shader")

static _AL_VECTOR shaders;
static ALLEGRO_MUTEX *shaders_mutex;

typedef struct ALLEGRO_SHADER_GLSL_S ALLEGRO_SHADER_GLSL_S;

struct ALLEGRO_SHADER_GLSL_S
{
   ALLEGRO_SHADER shader;
   GLuint vertex_shader;
   GLuint pixel_shader;
   GLuint program_object;
   ALLEGRO_OGL_VARLOCS varlocs;
};


/* forward declarations */
static struct ALLEGRO_SHADER_INTERFACE shader_glsl_vt;
static void lookup_varlocs(ALLEGRO_OGL_VARLOCS *varlocs, GLuint program);


static bool check_gl_error(const char* name)
{
   GLenum err = glGetError();
   if (err != 0) {
      ALLEGRO_WARN("%s (%s)\n", name, _al_gl_error_string(err));
      return false;
   }
   return true;
}


ALLEGRO_SHADER *_al_create_shader_glsl(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER_GLSL_S *shader = al_calloc(1, sizeof(ALLEGRO_SHADER_GLSL_S));

   if (!shader)
      return NULL;

   shader->shader.platform = platform;
   shader->shader.vt = &shader_glsl_vt;
   _al_vector_init(&shader->shader.bitmaps, sizeof(ALLEGRO_BITMAP *));

   al_lock_mutex(shaders_mutex);
   {
      ALLEGRO_SHADER **back = (ALLEGRO_SHADER **)_al_vector_alloc_back(&shaders);
      *back = (ALLEGRO_SHADER *)shader;
   }
   al_unlock_mutex(shaders_mutex);

   return (ALLEGRO_SHADER *)shader;
}

static bool glsl_attach_shader_source(ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type, const char *source)
{
   GLint status;
   GLchar error_buf[4096];
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ASSERT(display);
   ASSERT(display->flags & ALLEGRO_OPENGL);

   if (source == NULL) {
      if (type == ALLEGRO_VERTEX_SHADER) {
         if (gl_shader->vertex_shader) {
            glDetachShader(gl_shader->program_object, gl_shader->vertex_shader);
            glDeleteShader(gl_shader->vertex_shader);
            gl_shader->vertex_shader = 0;
         }
      }
      else {
         if (gl_shader->pixel_shader) {
            glDetachShader(gl_shader->program_object, gl_shader->pixel_shader);
            glDeleteShader(gl_shader->pixel_shader);
            gl_shader->pixel_shader = 0;
         }
      }
      return true;
   }
   else {
      GLuint *handle;
      GLenum gl_type;
      if (type == ALLEGRO_VERTEX_SHADER) {
         handle = &(gl_shader->vertex_shader);
         gl_type = GL_VERTEX_SHADER;
      }
      else {
         handle = &(gl_shader->pixel_shader);
         gl_type = GL_FRAGMENT_SHADER;
      }
      *handle = glCreateShader(gl_type);
      if ((*handle) == 0) {
         return false;
      }
      glShaderSource(*handle, 1, &source, NULL);
      glCompileShader(*handle);
      glGetShaderiv(*handle, GL_COMPILE_STATUS, &status);
      if (status == 0) {
         glGetShaderInfoLog(*handle, sizeof(error_buf), NULL, error_buf);
         if (shader->log) {
            al_ustr_truncate(shader->log, 0);
            al_ustr_append_cstr(shader->log, error_buf);
         }
         else {
            shader->log = al_ustr_new(error_buf);
         }
         ALLEGRO_ERROR("Compile error: %s\n", error_buf);
         glDeleteShader(*handle);
         return false;
      }
   }

   return true;
}

static bool glsl_build_shader(ALLEGRO_SHADER *shader)
{
   GLint status;
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLchar error_buf[4096];

   if (gl_shader->vertex_shader == 0 && gl_shader->pixel_shader == 0)
      return false;

   if (gl_shader->program_object != 0) {
      glDeleteProgram(gl_shader->program_object);
   }

   gl_shader->program_object = glCreateProgram();
   if (gl_shader->program_object == 0)
      return false;

   if (gl_shader->vertex_shader)
      glAttachShader(gl_shader->program_object, gl_shader->vertex_shader);
   if (gl_shader->pixel_shader)
      glAttachShader(gl_shader->program_object, gl_shader->pixel_shader);

   glLinkProgram(gl_shader->program_object);

   glGetProgramiv(gl_shader->program_object, GL_LINK_STATUS, &status);

   if (status == 0) {
      glGetProgramInfoLog(gl_shader->program_object, sizeof(error_buf), NULL,
         error_buf);
      if (shader->log) {
         al_ustr_truncate(shader->log, 0);
         al_ustr_append_cstr(shader->log, error_buf);
      }
      else {
         shader->log = al_ustr_new(error_buf);
      }
      ALLEGRO_ERROR("Link error: %s\n", error_buf);
      glDeleteProgram(gl_shader->program_object);
      return false;
   }

   /* Look up variable locations. */
   lookup_varlocs(&gl_shader->varlocs, gl_shader->program_object);

   return true;
}

static bool glsl_use_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display,
   bool set_projview_matrix_from_display)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader;
   GLuint program_object;
   GLenum err;

   if (!(display->flags & ALLEGRO_OPENGL)) {
      return false;
   }

   gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   program_object = gl_shader->program_object;

   glGetError(); /* clear error */
   glUseProgram(program_object);
   err = glGetError();
   if (err != GL_NO_ERROR) {
      ALLEGRO_WARN("glUseProgram(%u) failed: %s\n", program_object,
         _al_gl_error_string(err));
      display->ogl_extras->program_object = 0;
      return false;
   }

   display->ogl_extras->program_object = program_object;

   /* Copy variable locations. */
   display->ogl_extras->varlocs = gl_shader->varlocs;

   /* Optionally set projview matrix.  We skip this when it is known that the
    * matrices in the display are out of date and are about to be clobbered
    * itself.
    */
   if (set_projview_matrix_from_display) {
      _al_glsl_set_projview_matrix(
         display->ogl_extras->varlocs.projview_matrix_loc, &display->projview_transform);
   }

   /* Alpha testing may be done in the shader and so when a shader is
    * set the uniforms need to be synchronized.
    */
   _al_ogl_update_render_state(display);

   return true;
}

static void glsl_unuse_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display)
{
   (void)shader;
   (void)display;
   glUseProgram(0);
}

static void glsl_destroy_shader(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

   al_lock_mutex(shaders_mutex);
   _al_vector_find_and_delete(&shaders, &shader);
   al_unlock_mutex(shaders_mutex);

   glDeleteShader(gl_shader->vertex_shader);
   glDeleteShader(gl_shader->pixel_shader);
   glDeleteProgram(gl_shader->program_object);
   al_free(shader);
}

static bool glsl_set_shader_sampler(ALLEGRO_SHADER *shader,
   const char *name, ALLEGRO_BITMAP *bitmap, int unit)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;
   GLuint texture;

   if (bitmap && al_get_bitmap_flags(bitmap) & ALLEGRO_MEMORY_BITMAP) {
      ALLEGRO_WARN("Cannot use memory bitmap for sampler\n");
      return false;
   }

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   glActiveTexture(GL_TEXTURE0 + unit);

   texture = bitmap ? al_get_opengl_texture(bitmap) : 0;
   glBindTexture(GL_TEXTURE_2D, texture);

   glUniform1i(handle, unit);

   return check_gl_error(name);
}

static bool glsl_set_shader_matrix(ALLEGRO_SHADER *shader,
   const char *name, const ALLEGRO_TRANSFORM *matrix)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   glUniformMatrix4fv(handle, 1, false, (const float *)matrix->m);

   return check_gl_error(name);
}

static bool glsl_set_shader_int(ALLEGRO_SHADER *shader,
   const char *name, int i)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   glUniform1i(handle, i);

   return check_gl_error(name);
}

static bool glsl_set_shader_float(ALLEGRO_SHADER *shader,
   const char *name, float f)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   glUniform1f(handle, f);

   return check_gl_error(name);
}

static bool glsl_set_shader_double(ALLEGRO_SHADER *shader,
   const char *name, double d)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   glUniform1d(handle, d);

   return check_gl_error(name);
}

static bool glsl_set_shader_int_vector(ALLEGRO_SHADER *shader,
   const char *name, int num_components, const int *i, int num_elems)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   switch (num_components) {
      case 1:
         glUniform1iv(handle, num_elems, i);
         break;
      case 2:
         glUniform2iv(handle, num_elems, i);
         break;
      case 3:
         glUniform3iv(handle, num_elems, i);
         break;
      case 4:
         glUniform4iv(handle, num_elems, i);
         break;
      default:
         ASSERT(false);
         break;
   }

   return check_gl_error(name);
}

static bool glsl_set_shader_float_vector(ALLEGRO_SHADER *shader,
   const char *name, int num_components, const float *f, int num_elems)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   switch (num_components) {
      case 1:
         glUniform1fv(handle, num_elems, f);
         break;
      case 2:
         glUniform2fv(handle, num_elems, f);
         break;
      case 3:
         glUniform3fv(handle, num_elems, f);
         break;
      case 4:
         glUniform4fv(handle, num_elems, f);
         break;
      default:
         ASSERT(false);
         break;
   }

   return check_gl_error(name);
}


static bool glsl_set_shader_double_vector(ALLEGRO_SHADER *shader,
   const char *name, int num_components, const double *d, int num_elems)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   switch (num_components) {
      case 1:
         glUniform1dv(handle, num_elems, d);
         break;
      case 2:
         glUniform2dv(handle, num_elems, d);
         break;
      case 3:
         glUniform3dv(handle, num_elems, d);
         break;
      case 4:
         glUniform4dv(handle, num_elems, d);
         break;
      default:
         ASSERT(false);
         break;
   }

   return check_gl_error(name);
}

static bool glsl_set_shader_bool(ALLEGRO_SHADER *shader,
   const char *name, bool b)
{
   return glsl_set_shader_int(shader, name, b);
}

static struct ALLEGRO_SHADER_INTERFACE shader_glsl_vt =
{
   glsl_attach_shader_source,
   glsl_build_shader,
   glsl_use_shader,
   glsl_unuse_shader,
   glsl_destroy_shader,
   NULL, /* on_lost_device */
   NULL, /* on_reset_device */
   glsl_set_shader_sampler,
   glsl_set_shader_matrix,
   glsl_set_shader_int,
   glsl_set_shader_float,
   glsl_set_shader_double,
   glsl_set_shader_int_vector,
   glsl_set_shader_float_vector,
   glsl_set_shader_double_vector,
   glsl_set_shader_bool
};

static void lookup_varlocs(ALLEGRO_OGL_VARLOCS *varlocs, GLuint program)
{
   unsigned i;

   varlocs->pos_loc = glGetAttribLocation(program, ALLEGRO_SHADER_VAR_POS);
   varlocs->color_loc = glGetAttribLocation(program, ALLEGRO_SHADER_VAR_COLOR);
   varlocs->projview_matrix_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX);
   varlocs->texcoord_loc = glGetAttribLocation(program, ALLEGRO_SHADER_VAR_TEXCOORD);
   varlocs->use_tex_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_USE_TEX);
   varlocs->tex_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_TEX);
   varlocs->use_tex_matrix_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_USE_TEX_MATRIX);
   varlocs->tex_matrix_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_TEX_MATRIX);
   varlocs->alpha_test_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_ALPHA_TEST);
   varlocs->alpha_func_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_ALPHA_FUNCTION);
   varlocs->alpha_test_val_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_ALPHA_TEST_VALUE);

   for (i = 0; i < _ALLEGRO_PRIM_MAX_USER_ATTR; i++) {
      /* al_user_attr_##0 */
      char user_attr_name[sizeof(ALLEGRO_SHADER_VAR_USER_ATTR "999")];

      snprintf(user_attr_name, sizeof(user_attr_name), ALLEGRO_SHADER_VAR_USER_ATTR "%d", i);
      varlocs->user_attr_loc[i] = glGetAttribLocation(program, user_attr_name);
   }

   check_gl_error("glGetAttribLocation, glGetUniformLocation");
}

bool _al_glsl_set_projview_matrix(GLint projview_matrix_loc,
   const ALLEGRO_TRANSFORM *t)
{
   if (projview_matrix_loc >= 0) {
      glUniformMatrix4fv(projview_matrix_loc, 1, false, (float *)t->m);
      return true;
   }

   return false;
}

void _al_glsl_init_shaders(void)
{
   _al_vector_init(&shaders, sizeof(ALLEGRO_SHADER *));
   shaders_mutex = al_create_mutex();
}

void _al_glsl_shutdown_shaders(void)
{
   _al_vector_free(&shaders);
   al_destroy_mutex(shaders_mutex);
   shaders_mutex = NULL;
}

/* Look through all the bitmaps associated with all the shaders and c;ear their
 * shader field */
void _al_glsl_unuse_shaders(void)
{
   unsigned i;
   al_lock_mutex(shaders_mutex);
   for (i = 0; i < _al_vector_size(&shaders); i++) {
      unsigned j;
      ALLEGRO_SHADER *shader = *((ALLEGRO_SHADER **)_al_vector_ref(&shaders, i));

      for (j = 0; j < _al_vector_size(&shader->bitmaps); j++) {
         ALLEGRO_BITMAP *bitmap =
            *((ALLEGRO_BITMAP **)_al_vector_ref(&shader->bitmaps, j));
         _al_set_bitmap_shader_field(bitmap, NULL);
      }
   }
   al_unlock_mutex(shaders_mutex);
}

#endif

/* Function: al_get_opengl_program_object
 */
GLuint al_get_opengl_program_object(ALLEGRO_SHADER *shader)
{
   ASSERT(shader);
#ifdef ALLEGRO_CFG_SHADER_GLSL
   if (shader->platform != ALLEGRO_SHADER_GLSL)
      return 0;

   return ((ALLEGRO_SHADER_GLSL_S *)shader)->program_object;
#else
   return 0;
#endif
}


/* vim: set sts=3 sw=3 et: */
