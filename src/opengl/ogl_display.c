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
 *      OpenGL routines common to all OpenGL drivers.
 *
 *      By Elias Pschernig and Milan Mimica.
 *
 */

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"


void _al_ogl_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY_OGL *ogl_disp = (void *)display;
   /* If it is a memory bitmap, this display vtable entry would not even get
    * called, so the cast below is always safe.
    */
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   int x_1, y_1, x_2, y_2;

   if (!ogl_bitmap->is_backbuffer) {
      if (ogl_bitmap->fbo) {
         /* Bind to the FBO. */
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ogl_bitmap->fbo);

         /* Attach the texture. */
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D, ogl_bitmap->texture, 0);
         if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=
            GL_FRAMEBUFFER_COMPLETE_EXT) {
            // FIXME: handle this somehow!
         }

         ogl_disp->opengl_target = ogl_bitmap;
         glViewport(0, 0, bitmap->w, bitmap->h);

         glMatrixMode(GL_PROJECTION);
         glLoadIdentity();
         /* Allegro's bitmaps start at the top left pixel. OpenGL's bitmaps
          * at the bottom left. Therefore, Allegro's OpenGL drawing commands
          * all appear upside-down to OpenGL. To counter this when drawing to
          * a texture, we can actually use regular projection now, so we draw
          * upside down into it, and it appears right again when the texture
          * is drawn (upside-down) to the screen.
          * TODO: Verify this a bit more.. is there really no better way?
          */
         glOrtho(0, bitmap->w, 0, bitmap->h, -1, 1);

         glMatrixMode(GL_MODELVIEW);
         glLoadIdentity();
      }
   }
   else {
      if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object)
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      ogl_disp->opengl_target = ogl_bitmap;

      glViewport(0, 0, display->w, display->h);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      /* We use upside down coordinates compared to OpenGL, so the bottommost
       * coordinate is display->h not 0.
       */
      glOrtho(0, display->w, display->h, 0, -1, 1);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
   }

   if (ogl_disp->opengl_target == ogl_bitmap) {
      if (bitmap->cl == 0 && bitmap->cr == bitmap->w - 1 &&
         bitmap->ct == 0 && bitmap->cb == bitmap->h - 1) {
         glDisable(GL_SCISSOR_TEST);
      }
      else {
         glEnable(GL_SCISSOR_TEST);

         x_1 = bitmap->cl;
         y_1 = bitmap->ct;
         /* In OpenGL, coordinates are the top-left corner of pixels, so we need to
          * add one to the right and bottom edge.
          */
         x_2 = bitmap->cr + 1;
         y_2 = bitmap->cb + 1;

         /* OpenGL is upside down, so must adjust y_2 to the height. */
         glScissor(x_1, bitmap->h - y_2, x_2 - x_1, y_2 - y_1);
      }
   }
}


ALLEGRO_BITMAP *_al_ogl_get_backbuffer(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_OGL *ogl = (void *)d;
   return (ALLEGRO_BITMAP*)ogl->backbuffer;
}


ALLEGRO_BITMAP_OGL* _al_ogl_create_backbuffer(ALLEGRO_DISPLAY *disp) {
   ALLEGRO_BITMAP_OGL *ogl_backbuffer;
   ALLEGRO_BITMAP *backbuffer;

   _al_push_new_bitmap_parameters();
   al_set_new_bitmap_format(disp->format);
   backbuffer = _al_ogl_create_bitmap(disp, disp->w, disp->h);
   _al_pop_new_bitmap_parameters();

   ogl_backbuffer = (ALLEGRO_BITMAP_OGL*)backbuffer;
   ogl_backbuffer->is_backbuffer = 1;
   
   /* Create a memory cache for the whole screen. */
   //TODO: Maybe we should do this lazily and defer to lock_bitmap_region
   //FIXME: need to resize this on resizing
   if (!backbuffer->memory) {
      int n = disp->w * disp->h * al_get_pixel_size(backbuffer->format);
      backbuffer->memory = _AL_MALLOC(n);
      memset(backbuffer->memory, 0, n);
   }

   return ogl_backbuffer;
}
