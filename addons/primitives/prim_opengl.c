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
 *      OpenGL implementation of some of the primitive routines.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_primitives.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_prim.h"

#ifdef ALLEGRO_CFG_OPENGL

#include "allegro5/a5_opengl.h"
#include "allegro5/internal/aintern_opengl.h"

static void setup_blending(void)
{
   const int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };
   /*   ALLEGRO_COLOR *bc;*/
   int src_color, dst_color, src_alpha, dst_alpha;
   
   al_get_separate_blender(&src_color, &dst_color, &src_alpha,
                           &dst_alpha, NULL);
   glEnable(GL_BLEND);
   glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
                       blend_modes[src_alpha], blend_modes[dst_alpha]);
                       
   /*
   TODO: Should do something about colour shading... I think disabling it is simplest
   since reintroducing it would require shaders, I believe
   */
}

static void setup_state(ALLEGRO_VERTEX* vtx, ALLEGRO_BITMAP* texture)
{
   if (!glIsEnabled(GL_COLOR_ARRAY))
      glEnableClientState(GL_COLOR_ARRAY);
   if (!glIsEnabled(GL_VERTEX_ARRAY))
      glEnableClientState(GL_VERTEX_ARRAY);
   if (texture && !glIsEnabled(GL_TEXTURE_COORD_ARRAY))
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      
   if (al_get_prim_flag(ALLEGRO_PRIM_3D))
      glVertexPointer(3, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].x);
   else
      glVertexPointer(2, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].x);
   glColorPointer(4, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].r);
   
   if (texture)
      glTexCoordPointer(2, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].u);
}

static int draw_soft_vbuff(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int start, int end, int type)
{   
   int num_primitives = 0;
   ALLEGRO_DISPLAY *ogl_disp = al_get_current_display();
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (void *)target;
   ALLEGRO_VERTEX* vtx;
   int num_vtx;

   ASSERT(!al_vbuff_is_locked(vbuff));
  
   if ((!ogl_target->is_backbuffer && ogl_disp->ogl_extras->opengl_target != ogl_target) || al_is_bitmap_locked(target)) {
      return _al_draw_prim_soft(texture, vbuff, start, end, type);
   }
   
   /* We'll dispense with the formality here
   if(!al_lock_vbuff_range(vbuff, start, end, ALLEGRO_VBUFFER_READ))
       return 0;
   */
   
   vtx = ((ALLEGRO_VERTEX*)vbuff->data) + start;
   num_vtx = end - start;
   
   setup_blending();
   setup_state(vtx, texture);
   
   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         glDrawArrays(GL_LINES, 0, num_vtx);
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         glDrawArrays(GL_LINE_STRIP, 0, num_vtx);
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         glDrawArrays(GL_LINE_LOOP, 0, num_vtx);
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         glDrawArrays(GL_TRIANGLES, 0, num_vtx);
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vtx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         glDrawArrays(GL_TRIANGLE_FAN, 0, num_vtx);
         num_primitives = num_vtx - 2;
         break;
      };
   }
   
   /*al_unlock_vbuff(vbuff);*/
   glFlush();
   return num_primitives;
}

static int draw_indexed_soft_vbuff(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int* indices, int num_vtx, int type)
{   
   int num_primitives = 0;
   ALLEGRO_DISPLAY *ogl_disp = al_get_current_display();
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (void *)target;
   ALLEGRO_VERTEX* vtx;
 
   ASSERT(!al_vbuff_is_locked(vbuff));

   if ((!ogl_target->is_backbuffer && ogl_disp->ogl_extras->opengl_target != ogl_target) || al_is_bitmap_locked(target)) {
      return _al_draw_prim_indexed_soft(texture, vbuff, indices, num_vtx, type);
   }
   
   /* We'll dispense with the formality here
   In this case we'd have to find the min/max indices, which would be annoying
   if(!al_lock_vbuff_range(vbuff, start, end, ALLEGRO_VBUFFER_READ))
      return 0;
   */
   
   vtx = ((ALLEGRO_VERTEX*)vbuff->data);
   
   setup_blending();
   setup_state(vtx, texture);
   
   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         glDrawElements(GL_LINES, num_vtx, GL_UNSIGNED_INT, indices);
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         glDrawElements(GL_LINE_STRIP, num_vtx, GL_UNSIGNED_INT, indices);
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         glDrawElements(GL_LINE_LOOP, num_vtx, GL_UNSIGNED_INT, indices);
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         glDrawElements(GL_TRIANGLES, num_vtx, GL_UNSIGNED_INT, indices);
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         glDrawElements(GL_TRIANGLE_STRIP, num_vtx, GL_UNSIGNED_INT, indices);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         glDrawElements(GL_TRIANGLE_FAN, num_vtx, GL_UNSIGNED_INT, indices);
         num_primitives = num_vtx - 2;
         break;
      };
   }
   
   /*
   al_unlock_vbuff(vbuff);
   */
   glFlush();
   return num_primitives;
}

#endif

int _al_draw_prim_opengl(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_OPENGL
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      return draw_soft_vbuff(texture, vbuff, start, end, type);
   }
#endif
   return 0;
}

int _al_draw_prim_indexed_opengl(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int* indices, int num_vtx, int type)
{
#ifdef ALLEGRO_CFG_OPENGL
   if (vbuff->flags & ALLEGRO_VBUFFER_SOFT) {
      return draw_indexed_soft_vbuff(texture, vbuff, indices, num_vtx, type);
   }
   return 0;
#endif
}

void _al_use_transform_opengl(ALLEGRO_TRANSFORM* trans)
{
#ifdef ALLEGRO_CFG_OPENGL
   glLoadMatrixf((float*)trans);
#endif
}
