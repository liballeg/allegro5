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

#define LOG_GL_ERROR(name)                                   \
   do {                                                      \
      GLenum err = glGetError();                             \
      if (err != 0) {                                        \
         ALLEGRO_WARN("%s (%s)\n", name, _al_gl_error_string(err)); \
      }                                                      \
   } while (0)


typedef struct ALLEGRO_SHADER_GLSL_S ALLEGRO_SHADER_GLSL_S;
typedef struct GLSL_DEFERRED_SET GLSL_DEFERRED_SET;

struct ALLEGRO_SHADER_GLSL_S
{
   ALLEGRO_SHADER shader;
   GLuint vertex_shader;
   GLuint pixel_shader;
   GLuint program_object;
   _AL_VECTOR *deferred_sets;
};

struct GLSL_DEFERRED_SET {
   void (*fptr)(struct GLSL_DEFERRED_SET *s);
   // dump every parameter possible from the setters below
   char *name;
   int elem_size;
   int num_elems;
   float *fp;
   unsigned char *cp;
   int *ip;
   float f;
   int i;
   ALLEGRO_TRANSFORM *transform;
   ALLEGRO_BITMAP *bitmap;
   int unit;
   ALLEGRO_SHADER *shader;
};


/* forward declaration */
static struct ALLEGRO_SHADER_INTERFACE shader_glsl_vt;


static char *my_strdup(const char *src)
{
   size_t n = strlen(src) + 1; /* including NUL */
   char *dst = al_malloc(n);
   memcpy(dst, src, n);
   return dst;
}

ALLEGRO_SHADER *_al_create_shader_glsl(ALLEGRO_SHADER_PLATFORM platform)
{
   ALLEGRO_SHADER_GLSL_S *shader = al_calloc(1, sizeof(ALLEGRO_SHADER_GLSL_S));

   if (!shader)
      return NULL;

   shader->shader.platform = platform;
   shader->shader.vt = &shader_glsl_vt;

   shader->deferred_sets = al_malloc(sizeof(_AL_VECTOR));
   _al_vector_init(shader->deferred_sets, sizeof(GLSL_DEFERRED_SET));

   return (ALLEGRO_SHADER *)shader;
}

static bool glsl_link_shader(ALLEGRO_SHADER *shader)
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
   
   return true;
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

static void free_deferred_sets(_AL_VECTOR *vec, bool free_vector)
{
   while (!_al_vector_is_empty(vec)) {
      GLSL_DEFERRED_SET *sptr = _al_vector_ref_back(vec);
      al_free(sptr->name);
      al_free(sptr->fp);
      al_free(sptr->ip);
      _al_vector_delete_at(vec, _al_vector_size(vec) - 1);
   }

   if (free_vector) {
      _al_vector_free(vec);
   }
}

static void glsl_destroy_shader(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

   glDeleteShader(gl_shader->vertex_shader);
   glDeleteShader(gl_shader->pixel_shader);
   glDeleteProgram(gl_shader->program_object);
   free_deferred_sets(gl_shader->deferred_sets, true);
   al_free(gl_shader->deferred_sets);
   al_free(shader);
}

static bool shader_effective(ALLEGRO_SHADER *shader)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();
   return (bitmap && bitmap->shader == shader);
}

// real setters
static void shader_set_sampler(GLSL_DEFERRED_SET *s)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)s->shader;
   GLint handle;
   GLuint texture;

   handle = glGetUniformLocation(gl_shader->program_object, s->name);

   glActiveTexture(GL_TEXTURE0 + s->unit);

   texture = s->bitmap ? al_get_opengl_texture(s->bitmap) : 0;
   glBindTexture(GL_TEXTURE_2D, texture);

   glUniform1i(handle, s->unit);

   LOG_GL_ERROR(s->name);
}

static void shader_set_matrix(GLSL_DEFERRED_SET *s)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)s->shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, s->name);

   glUniformMatrix4fv(handle, 1, false, (float *)s->transform->m);
   LOG_GL_ERROR(s->name);
}

static void shader_set_int(GLSL_DEFERRED_SET *s)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)s->shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, s->name);

   glUniform1i(handle, s->i);
   LOG_GL_ERROR(s->name);
}

static void shader_set_float(GLSL_DEFERRED_SET *s)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)s->shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, s->name);

   glUniform1f(handle, s->f);
   LOG_GL_ERROR(s->name);
}

static void shader_set_int_vector(GLSL_DEFERRED_SET *s)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)s->shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, s->name);

   switch (s->elem_size) {
      case 1:
         glUniform1iv(handle, s->num_elems, s->ip);
         break;
      case 2:
         glUniform2iv(handle, s->num_elems, s->ip);
         break;
      case 3:
         glUniform3iv(handle, s->num_elems, s->ip);
         break;
      case 4:
         glUniform4iv(handle, s->num_elems, s->ip);
         break;
      default:
         ASSERT(false);
         break;
   }

   LOG_GL_ERROR(s->name);
}

static void shader_set_float_vector(GLSL_DEFERRED_SET *s)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)s->shader;
   GLint handle;

   handle = glGetUniformLocation(gl_shader->program_object, s->name);

   switch (s->elem_size) {
      case 1:
         glUniform1fv(handle, s->num_elems, s->fp);
         break;
      case 2:
         glUniform2fv(handle, s->num_elems, s->fp);
         break;
      case 3:
         glUniform3fv(handle, s->num_elems, s->fp);
         break;
      case 4:
         glUniform4fv(handle, s->num_elems, s->fp);
         break;
      default:
         ASSERT(false);
         break;
   }

   LOG_GL_ERROR(s->name);
}

static bool glsl_use_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display,
   bool set_projview_matrix_from_display)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader;
   GLuint program_object;
   GLenum err;
   unsigned i;

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

   /* Optionally set projview matrix.  We skip this when it is known that the
    * matrices in the display are out of date and are about to be clobbered
    * itself.
    */
   if (set_projview_matrix_from_display) {
      ALLEGRO_TRANSFORM t;
      al_copy_transform(&t, &display->view_transform);
      al_compose_transform(&t, &display->proj_transform);
      _al_glsl_set_projview_matrix(program_object, &t);
   }

   /* Look up variable locations. */
   _al_glsl_lookup_locations(display->ogl_extras, program_object);

   for (i = 0; i < _ALLEGRO_PRIM_MAX_USER_ATTR; i++) {
      /* al_user_attr_##0 */
      char user_attr_name[sizeof(ALLEGRO_SHADER_VAR_USER_ATTR "999")];

      snprintf(user_attr_name, sizeof(user_attr_name), ALLEGRO_SHADER_VAR_USER_ATTR "%d", i);
      display->ogl_extras->user_attr_loc[i] = glGetAttribLocation(program_object, user_attr_name);
   }

   /* Apply all deferred sets. */
   for (i = 0; i < _al_vector_size(gl_shader->deferred_sets); i++) {
      GLSL_DEFERRED_SET *sptr = _al_vector_ref(gl_shader->deferred_sets, i);
      (*(sptr->fptr))(sptr);
   }
   free_deferred_sets(gl_shader->deferred_sets, false);

   return true;
}

static void glsl_unuse_shader(ALLEGRO_SHADER *shader, ALLEGRO_DISPLAY *display)
{
   (void)shader;
   display->ogl_extras->program_object = display->ogl_extras->default_program;
   glUseProgram(display->ogl_extras->program_object);
}

static bool shader_add_deferred_set(
   void (*fptr)(GLSL_DEFERRED_SET *s),
   const char *name,
   int elem_size,
   int num_elems,
   float *fp,
   unsigned char *cp,
   int *ip,
   float f,
   int i,
   ALLEGRO_TRANSFORM *transform,
   ALLEGRO_BITMAP *bitmap,
   int unit,
   ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint handle;
   GLSL_DEFERRED_SET s;
   GLSL_DEFERRED_SET *sptr;

   // First check if the variable name exists, as errors cannot be
   // reported later. Could possibly do some other error checking here.
   // We don't have to check it later.
   handle = glGetUniformLocation(gl_shader->program_object, name);

   if (handle < 0) {
      ALLEGRO_WARN("No uniform variable '%s' in shader program\n", name);
      return false;
   }

   s.fptr = fptr;
   s.name = my_strdup(name);
   s.elem_size = elem_size;
   s.num_elems = num_elems;
   s.fp = fp;
   s.cp = cp;
   s.ip = ip;
   s.f = f;
   s.i = i;
   s.transform = transform;
   s.bitmap = bitmap;
   s.unit = unit;
   s.shader = shader;

   sptr = _al_vector_alloc_back(gl_shader->deferred_sets);
   if (!sptr)
      return false;

   *sptr = s;

   return true;
}

static bool glsl_set_shader_sampler(ALLEGRO_SHADER *shader,
   const char *name, ALLEGRO_BITMAP *bitmap, int unit)
{
   if (bitmap && bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      ALLEGRO_WARN("Cannot use memory bitmap for sampler\n");
      return false;
   }

   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.bitmap = bitmap;
      set.unit = unit;
      set.shader = shader;
      shader_set_sampler(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_sampler, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      0,    // int elem_size;
      0,    // int num_elems
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      NULL, // int *ip;
      0.0f, // float f;
      0,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      bitmap, // ALLEGRO_BITMAP *bitmap;
      unit,    // int unit;
      shader
   );
}

static bool glsl_set_shader_matrix(ALLEGRO_SHADER *shader,
   const char *name, ALLEGRO_TRANSFORM *matrix)
{
   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.transform = matrix;
      set.shader = shader;
      shader_set_matrix(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_matrix, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      0,    // int elem_size;
      0,    // int num_elems
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      NULL, // int *ip;
      0.0f, // float f;
      0,    // int i;
      matrix, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

static bool glsl_set_shader_int(ALLEGRO_SHADER *shader,
   const char *name, int i)
{
   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.i = i;
      set.shader = shader;
      shader_set_int(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_int, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      0,    // int elem_size;
      0,    // int num_elems
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      NULL, // int *ip;
      0.0f, // float f;
      i,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

static bool glsl_set_shader_float(ALLEGRO_SHADER *shader,
   const char *name, float f)
{
   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.f = f;
      set.shader = shader;
      shader_set_float(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_float, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      0,    // int elem_size;
      0,    // int num_elems
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      NULL, // int *ip;
      f, // float f;
      0,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

static bool glsl_set_shader_int_vector(ALLEGRO_SHADER *shader,
   const char *name, int elem_size, int *i, int num_elems)
{
   int *copy = al_malloc(sizeof(int) * elem_size * num_elems);
   memcpy(copy, i, sizeof(int) * elem_size * num_elems);

   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.elem_size = elem_size;
      set.num_elems = num_elems;
      set.ip = copy;
      set.shader = shader;
      shader_set_int_vector(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_int_vector, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      elem_size,    // int size;
      num_elems,
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      copy, // int *ip;
      0.0f, // float f;
      0,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

static bool glsl_set_shader_float_vector(ALLEGRO_SHADER *shader,
   const char *name, int elem_size, float *f, int num_elems)
{
   float *copy = al_malloc(sizeof(float) * elem_size * num_elems);
   memcpy(copy, f, sizeof(float) * elem_size * num_elems);

   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.elem_size = elem_size;
      set.num_elems = num_elems;
      set.fp = copy;
      set.shader = shader;
      shader_set_float_vector(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_float_vector, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      elem_size,    // int elem_size;
      num_elems,
      copy, // float *fp;
      NULL, // unsigned char *cp;
      NULL, // int *ip;
      0.0f, // float f;
      0,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

static bool glsl_set_shader_bool(ALLEGRO_SHADER *shader,
   const char *name, bool b)
{
   if (shader_effective(shader)) {
      GLSL_DEFERRED_SET set;
      set.name = my_strdup(name);
      set.i = b;
      set.shader = shader;
      shader_set_int(&set);
      return true;
   }

   return shader_add_deferred_set(
      shader_set_int, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      0,    // int elem_size;
      0,    // int num_elems
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      NULL, // int *ip;
      0.0f, // float f;
      b,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

static bool glsl_set_shader_vertex_array(ALLEGRO_SHADER *shader,
   float *v, int stride)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint loc;

   if (!shader_effective(shader))
      return false;

   loc = glGetAttribLocation(gl_shader->program_object, ALLEGRO_SHADER_VAR_POS);
   if (loc < 0)
      return false;

   if (v == NULL) {
      glDisableVertexAttribArray(loc);
      return true;
   }

   glVertexAttribPointer(loc, 3, GL_FLOAT, false, stride, v);
   glEnableVertexAttribArray(loc);

   return true;
}

static bool glsl_set_shader_color_array(ALLEGRO_SHADER *shader,
   unsigned char *c, int stride)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint loc;

   if (!shader_effective(shader))
      return false;

   loc = glGetAttribLocation(gl_shader->program_object, ALLEGRO_SHADER_VAR_COLOR);
   if (loc < 0)
      return false;

   if (c == NULL) {
      glDisableVertexAttribArray(loc);
      return true;
   }

   glVertexAttribPointer(loc, 4, GL_FLOAT, true, stride, (float *)c);
   glEnableVertexAttribArray(loc);

   return true;
}

static bool glsl_set_shader_texcoord_array(ALLEGRO_SHADER *shader,
   float *u, int stride)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
   GLint loc;

   if (!shader_effective(shader))
      return false;

   loc = glGetAttribLocation(gl_shader->program_object, ALLEGRO_SHADER_VAR_TEXCOORD);
   if (loc < 0)
      return false;

   if (u == NULL) {
      glDisableVertexAttribArray(loc);
      return true;
   }

   glVertexAttribPointer(loc, 2, GL_FLOAT, false, stride, u);
   glEnableVertexAttribArray(loc);

   return true;
}

static struct ALLEGRO_SHADER_INTERFACE shader_glsl_vt =
{
   glsl_link_shader,
   glsl_attach_shader_source,
   glsl_use_shader,
   glsl_unuse_shader,
   glsl_destroy_shader,
   glsl_set_shader_sampler,
   glsl_set_shader_matrix,
   glsl_set_shader_int,
   glsl_set_shader_float,
   glsl_set_shader_int_vector,
   glsl_set_shader_float_vector,
   glsl_set_shader_bool,
   glsl_set_shader_vertex_array,
   glsl_set_shader_color_array,
   glsl_set_shader_texcoord_array
};

void _al_glsl_lookup_locations(ALLEGRO_OGL_EXTRAS *ogl_extras, GLuint program)
{
   ogl_extras->pos_loc = glGetAttribLocation(program, ALLEGRO_SHADER_VAR_POS);
   ogl_extras->color_loc = glGetAttribLocation(program, ALLEGRO_SHADER_VAR_COLOR);
   ogl_extras->texcoord_loc = glGetAttribLocation(program, ALLEGRO_SHADER_VAR_TEXCOORD);
   ogl_extras->use_tex_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_USE_TEX);
   ogl_extras->tex_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_TEX);
   ogl_extras->use_tex_matrix_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_USE_TEX_MATRIX);
   ogl_extras->tex_matrix_loc = glGetUniformLocation(program, ALLEGRO_SHADER_VAR_TEX_MATRIX);

   LOG_GL_ERROR("glGetAttribLocation, glGetUniformLocation");
}

bool _al_glsl_set_projview_matrix(GLuint program_object, const ALLEGRO_TRANSFORM *t)
{
   GLint handle;

   handle = glGetUniformLocation(program_object,
      ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX);
   if (handle >= 0) {
      glUniformMatrix4fv(handle, 1, false, (float *)t->m);
      return true;
   }

   return false;
}

#endif


/* vim: set sts=3 sw=3 et: */
