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

ALLEGRO_DEBUG_CHANNEL("opengl")

bool _al_opengl_set_blender(ALLEGRO_DISPLAY *ogl_disp)
{
   int op, src_color, dst_color, op_alpha, src_alpha, dst_alpha;
   const int blend_modes[8] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
      GL_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR,
      GL_ONE_MINUS_DST_COLOR
   };
   const int blend_equations[3] = {
      GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT
   };

   al_get_separate_blender(&op, &src_color, &dst_color, &op_alpha,
      &src_alpha, &dst_alpha);
   /* glBlendFuncSeparate was only included with OpenGL 1.4 */
   /* (And not in OpenGL ES) */
#if defined ALLEGRO_IPHONE || defined ALLEGRO_GP2XWIZ
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_1_4) {
#else
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
#endif
      glEnable(GL_BLEND);
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



static void ogl_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY *ogl_disp = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target;
   float r, g, b, a;
   
   if (target->parent) target = target->parent;
   
   ogl_target = (void *)target;

   if ((!ogl_target->is_backbuffer &&
         ogl_disp->ogl_extras->opengl_target != ogl_target)
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
   ALLEGRO_BITMAP_OGL *ogl_target;
   GLfloat vert[2];

   /* For sub-bitmaps */
   if (target->parent) {
      target = target->parent;
   }

   ogl_target = (void *)target;

   if ((!ogl_target->is_backbuffer &&
      d->ogl_extras->opengl_target != ogl_target) ||
      target->locked || !_al_opengl_set_blender(d))  {
      _al_draw_pixel_memory(target, x, y, color);
      return;
   }

   vert[0] = x;
   vert[1] = y;

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);
   glVertexPointer(2, GL_FLOAT, 0, vert);
   glColorPointer(4, GL_FLOAT, 0, color);

   glDrawArrays(GL_POINTS, 0, 1);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);
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

static void ogl_flush_vertex_cache(ALLEGRO_DISPLAY* disp)
{
   GLboolean on;
   GLuint current_texture;
   if (!disp->vertex_cache)
      return;
   if (disp->num_cache_vertices == 0)
      return;

   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on) {
      glEnable(GL_TEXTURE_2D);
   }
   
   glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&current_texture);
   if (current_texture != disp->cache_texture) {
      glBindTexture(GL_TEXTURE_2D, disp->cache_texture);
   }

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glEnableClientState(GL_COLOR_ARRAY);
   
   glVertexPointer(2, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX), disp->vertex_cache);
   glTexCoordPointer(2, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX), 
                  (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, tx));
   glColorPointer(4, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX), (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, r));

   glDrawArrays(GL_TRIANGLES, 0, disp->num_cache_vertices);

   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisableClientState(GL_COLOR_ARRAY);
   
   disp->num_cache_vertices = 0;
   if (!on) {
      glDisable(GL_TEXTURE_2D);
   }
}

static void ogl_update_transformation(ALLEGRO_DISPLAY* disp,
   ALLEGRO_BITMAP *target)
{
   (void)disp;

   glMatrixMode(GL_MODELVIEW);
   if (target->parent) {
      /* Sub-bitmaps have an additional offset. */
      glLoadIdentity();
      glTranslatef(target->xofs, target->yofs, 0);
      glMultMatrixf((float*)(target->transform.m));
   }
   else {
      glLoadMatrixf((float *)target->transform.m);
   }
}

/* Add drawing commands to the vtable. */
void _al_ogl_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->clear = ogl_clear;
   vt->draw_pixel = ogl_draw_pixel;
   
   vt->flush_vertex_cache = ogl_flush_vertex_cache;
   vt->prepare_vertex_cache = ogl_prepare_vertex_cache;
   vt->update_transformation = ogl_update_transformation;
}

/* vim: set sts=3 sw=3 et: */
