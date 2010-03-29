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

#include "allegro5/allegro5.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_memory.h"


static bool set_opengl_blending(ALLEGRO_DISPLAY *d,
   ALLEGRO_COLOR *color)
{
   const int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };
   const int blend_equations[3] = {
      GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT
   };
   ALLEGRO_COLOR *bc;
   int op, src_color, dst_color, op_alpha, src_alpha, dst_alpha;
   float r, g, b, a;

   (void)d;

   al_unmap_rgba_f(*color, &r, &g, &b, &a);

   al_get_separate_blender(&op, &src_color, &dst_color, &op_alpha,
      &src_alpha, &dst_alpha, NULL);
#if defined ALLEGRO_IPHONE
   glEnable(GL_BLEND);
   glBlendFuncSeparate(blend_modes[src_color],
      blend_modes[dst_color], blend_modes[src_alpha],
      blend_modes[dst_alpha]);
   glBlendEquationSeparate(
      blend_equations[op],
      blend_equations[op_alpha]);
   bc = _al_get_blend_color();
   glColor4f(r * bc->r, g * bc->g, b * bc->b, a * bc->a);
   return true;
#elif defined ALLEGRO_GP2XWIZ
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
   glBlendEquation(blend_equations[op]);
   bc = _al_get_blend_color();
   glColor4f(r * bc->r, g * bc->g, b * bc->b, a * bc->a);
   return true;
#else
   if (d->ogl_extras->ogl_info.version >= 1.4) {
      glEnable(GL_BLEND);
      glBlendFuncSeparate(blend_modes[src_color],
         blend_modes[dst_color], blend_modes[src_alpha],
         blend_modes[dst_alpha]);
      if (d->ogl_extras->ogl_info.version >= 2.0) {
         glBlendEquationSeparate(
            blend_equations[op],
            blend_equations[op_alpha]);
      }
      else
         glBlendEquation(blend_equations[op]);
      bc = _al_get_blend_color();
      glColor4f(r * bc->r, g * bc->g, b * bc->b, a * bc->a);
      return true;
   }
   else {
      if (src_color == src_alpha && dst_color == dst_alpha) {
         glEnable(GL_BLEND);
         glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
         glBlendEquation(blend_equations[op]);
         bc = _al_get_blend_color();
         glColor4f(r * bc->r, g * bc->g, b * bc->b, a * bc->a);
         return true;
      }
   }
#endif
   return false;
}



/* Dummy implementation of clear. */
static void ogl_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY *ogl_disp = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target;
   float r, g, b, a;
   
   if (target->parent) target = target->parent;
   
   ogl_target = (void *)target;

   if ((!ogl_target->is_backbuffer &&
      ogl_disp->ogl_extras->opengl_target != ogl_target) ||
      target->locked) {
      _al_clear_memory(color);
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
   ALLEGRO_BITMAP_OGL *ogl_target = (void *)target;
   GLfloat vert[2];

   if ((!ogl_target->is_backbuffer &&
      d->ogl_extras->opengl_target != ogl_target) ||
      target->locked || !set_opengl_blending(d, color))  {
      _al_draw_pixel_memory(target, x, y, color);
      return;
   }

   /* For sub bitmaps. */
   if(target->parent) {
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glTranslatef(target->xofs, target->yofs, 0);
      glMultMatrixf((float*)(d->cur_transform.m));
   }

   vert[0] = x;
   vert[1] = y;

   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, 0, vert);

   glDrawArrays(GL_POINTS, 0, 1);

   glDisableClientState(GL_VERTEX_ARRAY);
   
   if(target->parent) {
      glPopMatrix();
   }
}

static void* ogl_prepare_vertex_cache(ALLEGRO_DISPLAY* disp, 
                                      int num_new_vertices)
{
   disp->num_cache_vertices += num_new_vertices;
   if(!disp->vertex_cache) {  
      disp->vertex_cache = _AL_MALLOC(num_new_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));
      
      disp->vertex_cache_size = num_new_vertices;
   } else if (disp->num_cache_vertices > disp->vertex_cache_size) {
      disp->vertex_cache = _AL_REALLOC(disp->vertex_cache, 
                              2 * disp->num_cache_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));
                              
      disp->vertex_cache_size = 2 * disp->num_cache_vertices;
   }
   return (ALLEGRO_OGL_BITMAP_VERTEX*)disp->vertex_cache + 
         (disp->num_cache_vertices - num_new_vertices);
}

static void ogl_flush_vertex_cache(ALLEGRO_DISPLAY* disp)
{
   ALLEGRO_COLOR *bc;
   GLboolean on;
   GLuint current_texture;
   ALLEGRO_BITMAP* target;
   if(!disp->vertex_cache)
      return;
   if(disp->num_cache_vertices == 0)
      return;
      
   target = al_get_target_bitmap();
   if(target->parent) {
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glTranslatef(target->xofs, target->yofs, 0);
      glMultMatrixf((float*)(disp->cur_transform.m));
   }
   
   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on) {
      glEnable(GL_TEXTURE_2D);
   }
   
   glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&current_texture);
   if (current_texture != disp->cache_texture) {
      glBindTexture(GL_TEXTURE_2D, disp->cache_texture);
   }
      
   bc = _al_get_blend_color();
   glColor4f(bc->r, bc->g, bc->b, bc->a);

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
   
   if(target->parent) {
      glPopMatrix();
   }
   
   disp->num_cache_vertices = 0;
   if (!on) {
      glDisable(GL_TEXTURE_2D);
   }
}

static void ogl_update_transformation(ALLEGRO_DISPLAY* disp)
{
   glMatrixMode(GL_MODELVIEW);
   glLoadMatrixf((float*)(disp->cur_transform.m));
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
