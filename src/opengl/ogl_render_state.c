#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"

A5O_DEBUG_CHANNEL("opengl")

/* Note: synched to A5O_RENDER_FUNCTION values as array indices */
static int _gl_funcs[] = {
   GL_NEVER,
   GL_ALWAYS,
   GL_LESS,
   GL_EQUAL,
   GL_LEQUAL,
   GL_GREATER,
   GL_NOTEQUAL,
   GL_GEQUAL
};

void _al_ogl_update_render_state(A5O_DISPLAY *display)
{
   _A5O_RENDER_STATE *r = &display->render_state;

   /* TODO: We could store the previous state and/or mark updated states to
    * avoid so many redundant OpenGL calls.
    */

   if (display->flags & A5O_PROGRAMMABLE_PIPELINE) {
#ifdef A5O_CFG_SHADER_GLSL
      GLint atloc = display->ogl_extras->varlocs.alpha_test_loc;
      GLint floc = display->ogl_extras->varlocs.alpha_func_loc;
      GLint tvloc = display->ogl_extras->varlocs.alpha_test_val_loc;

      if (display->ogl_extras->program_object > 0 && floc >= 0 && tvloc >= 0) {
         glUniform1i(atloc, r->alpha_test);
         glUniform1i(floc, r->alpha_function);
         glUniform1f(tvloc, (float)r->alpha_test_value / 255.0);
      }
#endif
   }
   else {
#ifdef A5O_CFG_OPENGL_FIXED_FUNCTION
      if (r->alpha_test == 0)
         glDisable(GL_ALPHA_TEST);
      else
         glEnable(GL_ALPHA_TEST);
      glAlphaFunc(_gl_funcs[r->alpha_function], (float)r->alpha_test_value / 255.0);
#endif
   }

   if (r->depth_test == 0)
      glDisable(GL_DEPTH_TEST);
   else
      glEnable(GL_DEPTH_TEST);
   glDepthFunc(_gl_funcs[r->depth_function]);

   glDepthMask((r->write_mask & A5O_MASK_DEPTH) ? GL_TRUE : GL_FALSE);
   glColorMask(
      (r->write_mask & A5O_MASK_RED) ? GL_TRUE : GL_FALSE,
      (r->write_mask & A5O_MASK_GREEN) ? GL_TRUE : GL_FALSE,
      (r->write_mask & A5O_MASK_BLUE) ? GL_TRUE : GL_FALSE,
      (r->write_mask & A5O_MASK_ALPHA) ? GL_TRUE : GL_FALSE);
}
