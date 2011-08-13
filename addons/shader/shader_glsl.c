#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_shader_glsl.h"
#include "allegro5/internal/aintern_shader_glsl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/transformations.h"
#include "shader.h"
#include "shader_glsl.h"

ALLEGRO_DEBUG_CHANNEL("shader")

#define LOG_GL_ERROR(name)                                   \
   do {                                                      \
      GLenum err = glGetError();                             \
      if (err != 0) {                                        \
         ALLEGRO_WARN("%s (%s)\n", name, _al_gl_error_string(err)); \
      }                                                      \
   } while (0)

typedef struct GLSL_DEFERRED_SET {
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
} GLSL_DEFERRED_SET;

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
   
   (void)platform;

   if (!shader)
      return NULL;

   shader->deferred_sets = al_malloc(sizeof(_AL_VECTOR));
   _al_vector_init(shader->deferred_sets, sizeof(GLSL_DEFERRED_SET));

   return (ALLEGRO_SHADER *)shader;
}

bool _al_link_shader_glsl(ALLEGRO_SHADER *shader)
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

bool _al_attach_shader_source_glsl(
   ALLEGRO_SHADER *shader,
   ALLEGRO_SHADER_TYPE type,
   const char *source)
{
   GLint status;
   GLchar error_buf[4096];
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

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
      _al_vector_delete_at(vec, _al_vector_size(vec) - 1);
   }

   if (free_vector) {
      _al_vector_free(vec);
   }
}

void _al_destroy_shader_glsl(ALLEGRO_SHADER *shader)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

   glDeleteShader(gl_shader->vertex_shader);
   glDeleteShader(gl_shader->pixel_shader);
   glDeleteProgram(gl_shader->program_object);
   free_deferred_sets(gl_shader->deferred_sets, true);
   al_free(shader);
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

void _al_use_shader_glsl(ALLEGRO_SHADER *shader, bool use)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   unsigned i;

   if (use) {
      ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;
      GLint handle;

      glUseProgram(gl_shader->program_object);

      handle = glGetUniformLocation(gl_shader->program_object, "projview_matrix");
      if (handle >= 0) {
         ALLEGRO_TRANSFORM t;
         al_copy_transform(&t, &display->view_transform);
         al_compose_transform(&t, &display->proj_transform);
         glUniformMatrix4fv(handle, 1, false, (float *)t.m);
      }

      // Apply all deferred sets
      for (i = 0; i < _al_vector_size(gl_shader->deferred_sets); i++) {
         GLSL_DEFERRED_SET *sptr = _al_vector_ref(gl_shader->deferred_sets, i);
         (*(sptr->fptr))(sptr);
      }
      free_deferred_sets(gl_shader->deferred_sets, false);
   }
   else {
      glUseProgram(0);
   }
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

bool _al_set_shader_sampler_glsl(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_BITMAP *bitmap, int unit)
{
   if (bitmap && bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      ALLEGRO_WARN("Cannot use memory bitmap for sampler\n");
      return false;
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

bool _al_set_shader_matrix_glsl(ALLEGRO_SHADER *shader, const char *name,
   ALLEGRO_TRANSFORM *matrix)
{
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

bool _al_set_shader_int_glsl(ALLEGRO_SHADER *shader, const char *name, int i)
{
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

bool _al_set_shader_float_glsl(ALLEGRO_SHADER *shader, const char *name,
   float f)
{
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

bool _al_set_shader_int_vector_glsl(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, int *i, int num_elems)
{
   return shader_add_deferred_set(
      shader_set_int_vector, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      elem_size,    // int size;
      num_elems,
      NULL, // float *fp;
      NULL, // unsigned char *cp;
      i, // int *ip;
      0.0f, // float f;
      0,    // int i;
      NULL, // ALLEGRO_TRANSFORM *transform;
      NULL, // ALLEGRO_BITMAP *bitmap;
      0,    // int unit;
      shader
   );
}

bool _al_set_shader_float_vector_glsl(ALLEGRO_SHADER *shader, const char *name,
   int elem_size, float *f, int num_elems)
{
   return shader_add_deferred_set(
      shader_set_float_vector, // void (*fptr)(GLSL_DEFERRED_SET *s)
      name,
      elem_size,    // int elem_size;
      num_elems,
      f, // float *fp;
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

bool _al_set_shader_bool_glsl(ALLEGRO_SHADER *shader, const char *name, bool b)
{
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

bool _al_set_shader_vertex_array_glsl(ALLEGRO_SHADER *shader, float *v, int stride)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

   GLint loc = glGetAttribLocation(gl_shader->program_object, "pos");
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

bool _al_set_shader_color_array_glsl(ALLEGRO_SHADER *shader, unsigned char *c, int stride)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

   GLint loc = glGetAttribLocation(gl_shader->program_object, "color");
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

bool _al_set_shader_texcoord_array_glsl(ALLEGRO_SHADER *shader, float *u, int stride)
{
   ALLEGRO_SHADER_GLSL_S *gl_shader = (ALLEGRO_SHADER_GLSL_S *)shader;

   GLint loc = glGetAttribLocation(gl_shader->program_object, "texcoord");
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

/* Function: al_get_opengl_program_object
 */
GLuint al_get_opengl_program_object(ALLEGRO_SHADER *shader)
{
   return ((ALLEGRO_SHADER_GLSL_S *)shader)->program_object;
}

void _al_set_shader_glsl(ALLEGRO_DISPLAY *display, ALLEGRO_SHADER *shader)
{
   al_set_opengl_program_object(display, al_get_opengl_program_object(shader));
}

/* vim: set sts=3 sw=3 et: */
