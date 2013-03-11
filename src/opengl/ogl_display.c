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

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/transformations.h"	

#ifdef ALLEGRO_IPHONE
#include "allegro5/internal/aintern_iphone.h"
#endif

#ifdef ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#include "ogl_helpers.h"

ALLEGRO_DEBUG_CHANNEL("opengl")


#ifdef ALLEGRO_CFG_SHADER_GLSL
static GLuint create_default_program(void)
{
   GLuint vshader = 0;
   GLuint pshader = 0;
   GLuint program = 0;
   const char *vsource;
   const char *psource;
   GLint status;
   GLenum err;

   vsource = al_get_default_shader_source(ALLEGRO_SHADER_GLSL,
      ALLEGRO_VERTEX_SHADER);
   psource = al_get_default_shader_source(ALLEGRO_SHADER_GLSL,
      ALLEGRO_PIXEL_SHADER);
   if (!vsource || !psource) {
      ALLEGRO_ERROR("missing default shader source\n");
      goto fail;
   }

   vshader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vshader, 1, &vsource, NULL);
   glCompileShader(vshader);
   glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
   if (status == GL_FALSE) {
      ALLEGRO_ERROR("error compiling vertex shader\n");
      goto fail;
   }

   pshader = glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(pshader, 1, &psource, NULL);
   glCompileShader(pshader);
   glGetShaderiv(vshader, GL_COMPILE_STATUS, &status);
   if (status == GL_FALSE) {
      ALLEGRO_ERROR("error compiling fragment shader\n");
      goto fail;
   }

   program = glCreateProgram();
   if (program == 0) {
      ALLEGRO_ERROR("error creating program\n");
      goto fail;
   }

   glAttachShader(program, vshader);
   glAttachShader(program, pshader);

   glGetError(); /* clear error */
   glLinkProgram(program);
   err = glGetError();
   if (err != GL_NO_ERROR) {
      ALLEGRO_ERROR("error linking program\n");
      goto fail;
   }

   /* Flag shaders for automatic deletion with the program. */
   glDeleteShader(pshader);
   glDeleteShader(vshader);

   return program;

fail:

   glDeleteShader(pshader);
   glDeleteShader(vshader);
   glDeleteProgram(program);
   return 0;
}
#endif


/* Helper to set up GL state as we want it. */
void _al_ogl_setup_gl(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

   glViewport(0, 0, d->w, d->h);

   if (d->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_SHADER_GLSL
      GLenum err;

      /* XXX check for and return errors */
      ogl->default_program = create_default_program();
      ogl->program_object = ogl->default_program;

      glGetError(); /* clear error */
      glUseProgram(ogl->program_object);
      err = glGetError();
      if (err != GL_NO_ERROR) {
         ALLEGRO_ERROR("glUseProgram failed\n");
      }
      else {
         _al_glsl_lookup_locations(ogl, ogl->program_object);
      }
#endif
   }
   else {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, d->w, d->h, 0, -1, 1);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
   }

   al_identity_transform(&d->proj_transform);
   al_orthographic_transform(&d->proj_transform, 0, 0, -1, d->w, d->h, 1);
   d->vt->set_projection(d);

   if (ogl->backbuffer)
      _al_ogl_resize_backbuffer(ogl->backbuffer, d->w, d->h);
   else
      ogl->backbuffer = _al_ogl_create_backbuffer(d);
}


void _al_ogl_delete_default_program(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;
   (void)ogl;

#ifdef ALLEGRO_CFG_SHADER_GLSL
   /* Flag the program for deletion; it does not matter if it is still used. */
   glDeleteProgram(ogl->default_program);
   ogl->program_object = ogl->default_program = 0;
#endif
}


void _al_ogl_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *target = bitmap;
   if (bitmap->parent)
      target = bitmap->parent;

   /* if either this bitmap or its parent (in the case of subbitmaps)
    * is locked then don't do anything
    */
   if (!(bitmap->locked ||
        (bitmap->parent && bitmap->parent->locked))) {
      _al_ogl_setup_fbo(display, bitmap);
      if (display->ogl_extras->opengl_target == target) {
         _al_ogl_setup_bitmap_clipping(bitmap);
      }
   }
}


/* Function: al_set_current_opengl_context
 */
void al_set_current_opengl_context(ALLEGRO_DISPLAY *display)
{
   ASSERT(display);

   if (!(display->flags & ALLEGRO_OPENGL))
      return;

   if (display) {
      ALLEGRO_BITMAP *bmp = al_get_target_bitmap();
      if (bmp && bmp->display && bmp->display != display) {
         al_set_target_bitmap(NULL);
      }
   }

   _al_set_current_display_only(display);
}


void _al_ogl_setup_bitmap_clipping(const ALLEGRO_BITMAP *bitmap)
{
   int x_1, y_1, x_2, y_2, h;
   bool use_scissor = true;

   x_1 = bitmap->cl;
   y_1 = bitmap->ct;
   x_2 = bitmap->cr_excl;
   y_2 = bitmap->cb_excl;
   h = bitmap->h;

   /* Drawing onto the sub bitmap is handled by clipping the parent. */
   if (bitmap->parent) {
      x_1 += bitmap->xofs;
      y_1 += bitmap->yofs;
      x_2 += bitmap->xofs;
      y_2 += bitmap->yofs;
      h = bitmap->parent->h;
   }

   if (x_1 == 0 &&  y_1 == 0 && x_2 == bitmap->w && y_2 == bitmap->h) {
      if (bitmap->parent) {
         /* Can only disable scissor if the sub-bitmap covers the
          * complete parent.
          */
         if (bitmap->xofs == 0 && bitmap->yofs == 0 && bitmap->w ==
            bitmap->parent->w && bitmap->h == bitmap->parent->h) {
               use_scissor = false;
            }
      }
      else
         use_scissor = false;
   }
   if (!use_scissor) {
      glDisable(GL_SCISSOR_TEST);
   }
   else {
      glEnable(GL_SCISSOR_TEST);
       
      #ifdef ALLEGRO_IPHONE
      _al_iphone_clip(bitmap, x_1, y_1, x_2, y_2);
      #else
      /* OpenGL is upside down, so must adjust y_2 to the height. */
      glScissor(x_1, h - y_2, x_2 - x_1, y_2 - y_1);
      #endif
   }
}


ALLEGRO_BITMAP *_al_ogl_get_backbuffer(ALLEGRO_DISPLAY *d)
{
   return (ALLEGRO_BITMAP *)d->ogl_extras->backbuffer;
}


bool _al_ogl_resize_backbuffer(ALLEGRO_BITMAP *b, int w, int h)
{
   int pitch;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra = b->extra;
         
   pitch = w * al_get_pixel_size(b->format);

   b->w = w;
   b->h = h;
   b->pitch = pitch;
   b->cl = 0;
   b->ct = 0;
   b->cr_excl = w;
   b->cb_excl = h;

   /* There is no texture associated with the backbuffer so no need to care
    * about texture size limitations. */
   extra->true_w = w;
   extra->true_h = h;

   if (!IS_IPHONE) {
      b->memory = NULL;
   }
   else {
      /* iPhone port still expects the buffer to be present. */
      /* FIXME: lazily manage memory */
      size_t bytes = pitch * h;
      al_free(b->memory);
      b->memory = al_calloc(1, bytes);
   }

   return true;
}


ALLEGRO_BITMAP* _al_ogl_create_backbuffer(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_backbuffer;
   ALLEGRO_BITMAP *backbuffer;
   ALLEGRO_STATE backup;
   int format;

   ALLEGRO_DEBUG("Creating backbuffer\n");

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

   // FIXME: _al_deduce_color_format would work fine if the display paramerers
   // are filled in, for OpenGL ES
   if (IS_OPENGLES) {
      if (disp->extra_settings.settings[ALLEGRO_COLOR_SIZE] == 16) {
         format = ALLEGRO_PIXEL_FORMAT_RGB_565;
      }
      else {
         format = ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE;
      }
   }
   else {
      format = _al_deduce_color_format(&disp->extra_settings);
      /* Eww. No OpenGL hardware in the world does that - let's just
       * switch to some default.
       */
      if (al_get_pixel_size(format) == 3) {
         /* Or should we use RGBA? Maybe only if not Nvidia cards? */
         format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
      }
   }
   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)("Deduced format %s for backbuffer.\n",
      _al_format_name(format));

   /* Now that the display backbuffer has a format, update extra_settings so
    * the user can query it back.
    */
   _al_set_color_components(format, &disp->extra_settings, ALLEGRO_REQUIRE);
   disp->backbuffer_format = format;

   ALLEGRO_DEBUG("Creating backbuffer bitmap\n");
   al_set_new_bitmap_format(format);
   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   backbuffer = _al_ogl_create_bitmap(disp, disp->w, disp->h);
   al_restore_state(&backup);

   if (!backbuffer) {
      ALLEGRO_DEBUG("Backbuffer bitmap creation failed.\n");
      return NULL;
   }

   backbuffer->w = disp->w;
   backbuffer->h = disp->h;
   backbuffer->cl = 0;
   backbuffer->ct = 0;
   backbuffer->cr_excl = disp->w;
   backbuffer->cb_excl = disp->h;

   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)(
      "Created backbuffer bitmap (actual format: %s)\n",
      _al_format_name(backbuffer->format));

   ogl_backbuffer = backbuffer->extra;
   ogl_backbuffer->is_backbuffer = 1;
   backbuffer->display = disp;

   al_identity_transform(&disp->view_transform);

   return backbuffer;
}


void _al_ogl_destroy_backbuffer(ALLEGRO_BITMAP *b)
{
   al_destroy_bitmap(b);
}


/* vi: set sts=3 sw=3 et: */
