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
 *      OpenGL drawing routines
 *
 *      By Elias Pschernig.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memdraw.h"
#include "allegro5/internal/aintern_opengl.h"

#ifdef ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#include "ogl_helpers.h"

ALLEGRO_DEBUG_CHANNEL("opengl")

/* FIXME: For some reason x86_64 Android crashes for me when calling
 * glBlendColor - so adding this hack to disable it.
 */
#ifdef ALLEGRO_ANDROID
#if defined(__x86_64__) || defined(__i686__)
#define ALLEGRO_ANDROID_HACK_X86_64
#endif
#endif

static void try_const_color(ALLEGRO_DISPLAY *ogl_disp, ALLEGRO_COLOR *c)
{
   #ifdef ALLEGRO_CFG_OPENGLES
      #ifndef ALLEGRO_CFG_OPENGLES2
         return;
      #endif
      // Only OpenGL ES 2.0 has glBlendColor
      if (ogl_disp->ogl_extras->ogl_info.version < _ALLEGRO_OPENGL_VERSION_2_0) {
         return;
      }
   #else
   (void)ogl_disp;
   #endif
   glBlendColor(c->r, c->g, c->b, c->a);
}

bool _al_opengl_set_blender(ALLEGRO_DISPLAY *ogl_disp)
{
   int op, src_color, dst_color, op_alpha, src_alpha, dst_alpha;
   ALLEGRO_COLOR const_color;
   const int blend_modes[10] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
      GL_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR,
      GL_ONE_MINUS_DST_COLOR,
#if defined(ALLEGRO_CFG_OPENGLES2) || !defined(ALLEGRO_CFG_OPENGLES)
      GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR
#else
      GL_ONE, GL_ONE
#endif
   };
   const int blend_equations[3] = {
      GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT
   };

   (void)ogl_disp;

   al_get_separate_bitmap_blender(&op, &src_color, &dst_color,
      &op_alpha, &src_alpha, &dst_alpha);
   const_color = al_get_bitmap_blend_color();
   /* glBlendFuncSeparate was only included with OpenGL 1.4 */
#if !defined ALLEGRO_CFG_OPENGLES
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_1_4) {
#else
   /* FIXME: At this time (09/2014) there are a lot of Android phones that
    * don't support glBlendFuncSeparate even though they claim OpenGL ES 2.0
    * support. Rather than not work on 20-25% of phones, we just don't support
    * separate blending on Android for now.
    */
#if defined ALLEGRO_ANDROID && !defined ALLEGRO_CFG_OPENGLES3
   if (false) {
#else
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
#endif
#endif
      glEnable(GL_BLEND);
      try_const_color(ogl_disp, &const_color);
      glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
         blend_modes[src_alpha], blend_modes[dst_alpha]);
      if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
         glBlendEquationSeparate(
            blend_equations[op],
            blend_equations[op_alpha]);
      }
      else {
         glBlendEquation(blend_equations[op]);
      }
   }
   else {
      if (src_color == src_alpha && dst_color == dst_alpha) {
         glEnable(GL_BLEND);
         try_const_color(ogl_disp, &const_color);
         glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
      }
      else {
         ALLEGRO_ERROR("Blender unsupported with this OpenGL version (%d %d %d %d %d %d)\n",
            op, src_color, dst_color, op_alpha, src_alpha, dst_alpha);
         return false;
      }
   }
   return true;
}

/* These functions make drawing calls use shaders or the fixed pipeline
 * based on what the user has set up. FIXME: OpenGL only right now.
 */

static void vert_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
/* Only use this shader stuff with GLES2+ or equivalent */
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.pos_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->varlocs.pos_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(n, t, stride, v);
#endif
   }
}

static void vert_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.pos_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_VERTEX_ARRAY);
#endif
   }
}

static void color_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.color_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->varlocs.color_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(n, t, stride, v);
#endif
   }
}

static void color_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.color_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_COLOR_ARRAY);
#endif
   }
}

static void tex_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->varlocs.texcoord_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(n, t, stride, v);
#endif
   }
}

static void tex_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
   }
}

static void ogl_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY *ogl_disp = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target;
   float r, g, b, a;

   if (target->parent)
      target = target->parent;

   ogl_target = target->extra;

   if ((!ogl_target->is_backbuffer &&
         ogl_disp->ogl_extras->opengl_target != target)
      || target->locked)
   {
      _al_clear_bitmap_by_locking(target, color);
      return;
   }

   al_unmap_rgba_f(*color, &r, &g, &b, &a);

   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT);
}


static void ogl_draw_pixel(ALLEGRO_DISPLAY *d, float x, float y,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target;
   GLfloat vert[2];
   GLfloat color_array[4];

   /* For sub-bitmaps */
   if (target->parent) {
      target = target->parent;
   }

   ogl_target = target->extra;

   if ((!ogl_target->is_backbuffer &&
      d->ogl_extras->opengl_target != target) ||
      target->locked || !_al_opengl_set_blender(d))  {
      _al_draw_pixel_memory(target, x, y, color);
      return;
   }

   vert[0] = x;
   vert[1] = y;

   color_array[0] = color->r;
   color_array[1] = color->g;
   color_array[2] = color->b;
   color_array[3] = color->a;

   vert_ptr_on(d, 2, GL_FLOAT, 2*sizeof(float), vert);
   color_ptr_on(d, 4, GL_FLOAT, 4*sizeof(float), color_array);

   // Should this be here if it's in the if above?
   if (!_al_opengl_set_blender(d)) {
      return;
   }

   glDrawArrays(GL_POINTS, 0, 1);

   vert_ptr_off(d);
   color_ptr_off(d);
}

static void* ogl_prepare_vertex_cache(ALLEGRO_DISPLAY* disp,
                                      int num_new_vertices)
{
   disp->num_cache_vertices += num_new_vertices;
   if (!disp->vertex_cache) {
      disp->vertex_cache = al_malloc(num_new_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));

      disp->vertex_cache_size = num_new_vertices;
   } else if (disp->num_cache_vertices > disp->vertex_cache_size) {
      disp->vertex_cache = al_realloc(disp->vertex_cache,
                              2 * disp->num_cache_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));

      disp->vertex_cache_size = 2 * disp->num_cache_vertices;
   }
   return (ALLEGRO_OGL_BITMAP_VERTEX*)disp->vertex_cache +
         (disp->num_cache_vertices - num_new_vertices);
}

static void ogl_flush_vertex_cache(ALLEGRO_DISPLAY *disp)
{
   GLuint current_texture;
   ALLEGRO_OGL_EXTRAS *o = disp->ogl_extras;
   (void)o; /* not used in all ports */

   if (!disp->vertex_cache)
      return;
   if (disp->num_cache_vertices == 0)
      return;

   if (!_al_opengl_set_blender(disp)) {
      disp->num_cache_vertices = 0;
      return;
   }

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (disp->ogl_extras->varlocs.use_tex_loc >= 0) {
         glUniform1i(disp->ogl_extras->varlocs.use_tex_loc, 1);
      }
      if (disp->ogl_extras->varlocs.use_tex_matrix_loc >= 0) {
         glUniform1i(disp->ogl_extras->varlocs.use_tex_matrix_loc, 0);
      }
#endif
   }
   else {
      glEnable(GL_TEXTURE_2D);
   }

   glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&current_texture);
   if (current_texture != disp->cache_texture) {
      if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
         /* Use texture unit 0 */
         glActiveTexture(GL_TEXTURE0);
         if (disp->ogl_extras->varlocs.tex_loc >= 0)
            glUniform1i(disp->ogl_extras->varlocs.tex_loc, 0);
#endif
      }
      glBindTexture(GL_TEXTURE_2D, disp->cache_texture);
   }

#if !defined(ALLEGRO_CFG_OPENGLES)
#if defined(ALLEGRO_MACOSX)
   if (disp->flags & (ALLEGRO_PROGRAMMABLE_PIPELINE | ALLEGRO_OPENGL_3_0)) {
#else
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#endif
      int stride = sizeof(ALLEGRO_OGL_BITMAP_VERTEX);
      int bytes = disp->num_cache_vertices * stride;

      /* We create the VAO and VBO on first use. */
      if (o->vao == 0) {
         glGenVertexArrays(1, &o->vao);
         ALLEGRO_DEBUG("new VAO: %u\n", o->vao);
      }
      glBindVertexArray(o->vao);

      if (o->vbo == 0) {
         glGenBuffers(1, &o->vbo);
         ALLEGRO_DEBUG("new VBO: %u\n", o->vbo);
      }
      glBindBuffer(GL_ARRAY_BUFFER, o->vbo);

      /* Then we upload data into it. */
      glBufferData(GL_ARRAY_BUFFER, bytes, disp->vertex_cache, GL_STREAM_DRAW);

      /* Finally set the "pos", "texccord" and "color" attributes used by our
       * shader and enable them.
       */
      if (o->varlocs.pos_loc >= 0)  {
         glVertexAttribPointer(o->varlocs.pos_loc, 3, GL_FLOAT, false, stride,
            (void *)offsetof(ALLEGRO_OGL_BITMAP_VERTEX, x));
         glEnableVertexAttribArray(o->varlocs.pos_loc);
      }

      if (o->varlocs.texcoord_loc >= 0) {
         glVertexAttribPointer(o->varlocs.texcoord_loc, 2, GL_FLOAT, false, stride,
            (void *)offsetof(ALLEGRO_OGL_BITMAP_VERTEX, tx));
         glEnableVertexAttribArray(o->varlocs.texcoord_loc);
      }

      if (o->varlocs.color_loc >= 0) {
         glVertexAttribPointer(o->varlocs.color_loc, 4, GL_FLOAT, false, stride,
            (void *)offsetof(ALLEGRO_OGL_BITMAP_VERTEX, r));
         glEnableVertexAttribArray(o->varlocs.color_loc);
      }
   }
   else
#endif
   {
      vert_ptr_on(disp, 3, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX),
         (char *)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, x));
      tex_ptr_on(disp, 2, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX),
         (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, tx));
      color_ptr_on(disp, 4, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX),
         (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, r));

#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      if (!(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE))
         glDisableClientState(GL_NORMAL_ARRAY);
#endif
   }

   glGetError(); /* clear error */
   glDrawArrays(GL_TRIANGLES, 0, disp->num_cache_vertices);

#ifdef DEBUGMODE
   {
      int e = glGetError();
      if (e) {
         ALLEGRO_WARN("glDrawArrays failed: %s\n", _al_gl_error_string(e));
      }
   }
#endif

#if !defined ALLEGRO_CFG_OPENGLES && !defined ALLEGRO_MACOSX
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      if (o->varlocs.pos_loc >= 0)
         glDisableVertexAttribArray(o->varlocs.pos_loc);
      if (o->varlocs.texcoord_loc >= 0)
         glDisableVertexAttribArray(o->varlocs.texcoord_loc);
      if (o->varlocs.color_loc >= 0)
         glDisableVertexAttribArray(o->varlocs.color_loc);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }
   else
#endif
   {
      vert_ptr_off(disp);
      tex_ptr_off(disp);
      color_ptr_off(disp);
   }

   disp->num_cache_vertices = 0;

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (disp->ogl_extras->varlocs.use_tex_loc >= 0)
         glUniform1i(disp->ogl_extras->varlocs.use_tex_loc, 0);
#endif
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }
}

static void ogl_update_transformation(ALLEGRO_DISPLAY* disp,
   ALLEGRO_BITMAP *target)
{
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_SHADER_GLSL
      GLint loc = disp->ogl_extras->varlocs.projview_matrix_loc;
      ALLEGRO_TRANSFORM projview;
      al_copy_transform(&projview, &target->transform);
      al_compose_transform(&projview, &target->proj_transform);
      al_copy_transform(&disp->projview_transform, &projview);

      if (disp->ogl_extras->program_object > 0 && loc >= 0) {
         _al_glsl_set_projview_matrix(loc, &disp->projview_transform);
      }
#endif
   } else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glMatrixMode(GL_PROJECTION);
      glLoadMatrixf((float *)target->proj_transform.m);
      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixf((float *)target->transform.m);
#endif
   }

   if (target->parent) {
      ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_extra = target->parent->extra;
      /* glViewport requires the bottom-left coordinate of the corner. */
      glViewport(target->xofs, ogl_extra->true_h - (target->yofs + target->h), target->w, target->h);
   } else {
      glViewport(0, 0, target->w, target->h);
   }
}

static void ogl_clear_depth_buffer(ALLEGRO_DISPLAY *display, float x)
{
   (void)display;

#if defined(ALLEGRO_CFG_OPENGLES)
   glClearDepthf(x);
#else
   glClearDepth(x);
#endif

   /* We may want to defer this to the next glClear call as a combined
    * color/depth clear may be faster.
    */
   glClear(GL_DEPTH_BUFFER_BIT);
}

/* Add drawing commands to the vtable. */
void _al_ogl_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->clear = ogl_clear;
   vt->draw_pixel = ogl_draw_pixel;
   vt->clear_depth_buffer = ogl_clear_depth_buffer;

   vt->flush_vertex_cache = ogl_flush_vertex_cache;
   vt->prepare_vertex_cache = ogl_prepare_vertex_cache;
   vt->update_transformation = ogl_update_transformation;
}

/* vim: set sts=3 sw=3 et: */
