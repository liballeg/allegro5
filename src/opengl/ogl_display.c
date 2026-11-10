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

/* Helper to set up GL state as we want it. */
void _al_ogl_setup_gl(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

   if (ogl->backbuffer) {
      ALLEGRO_BITMAP *target = al_get_target_bitmap();
      _al_ogl_resize_backbuffer(ogl->backbuffer, d->w, d->h);
      /* If we are currently targetting the backbuffer, we need to update the
       * transformations. */
      if (target && (target == ogl->backbuffer ||
                     target->parent == ogl->backbuffer)) {
         /* vt should be set at this point, but doesn't hurt to check */
         ASSERT(d->vt);
         d->vt->update_transformation(d, target);
      }
   } else {
      ogl->backbuffer = _al_ogl_create_backbuffer(d);
   }
}


void _al_ogl_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP *target = bitmap;
   if (bitmap->parent)
      target = bitmap->parent;

   /* if either this bitmap or its parent (in the case of subbitmaps)
    * is locked then don't do anything
    */
   if (bitmap->locked)
      return;
   if (bitmap->parent && bitmap->parent->locked)
      return;

   _al_ogl_setup_fbo(display, bitmap);
   if (display->ogl_extras->opengl_target == target) {
      _al_ogl_setup_bitmap_clipping(bitmap);
   }
}


void _al_ogl_unset_target_bitmap(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *target)
{
   if (!target)
      return;
   _al_ogl_finalize_fbo(display, target);
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
      if (bmp && _al_get_bitmap_display(bmp) &&
            _al_get_bitmap_display(bmp) != display) {
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
         if (bitmap->xofs == 0 && bitmap->yofs == 0 &&
            bitmap->w == bitmap->parent->w && bitmap->h == bitmap->parent->h)
         {
            use_scissor = false;
         }
      }
      else {
         use_scissor = false;
      }
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

   pitch = w * al_get_pixel_size(al_get_bitmap_format(b));

   b->w = w;
   b->h = h;
   b->pitch = pitch;
   b->cl = 0;
   b->ct = 0;
   b->cr_excl = w;
   b->cb_excl = h;
   al_identity_transform(&b->proj_transform);
   al_orthographic_transform(&b->proj_transform, 0, 0, -1.0, w, h, 1.0);

   /* There is no texture associated with the backbuffer so no need to care
    * about texture size limitations. */
   extra->true_w = w;
   extra->true_h = h;

   b->memory = NULL;

   return true;
}


ALLEGRO_BITMAP* _al_ogl_create_backbuffer(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_backbuffer;
   ALLEGRO_BITMAP *backbuffer;
   int format;

   ALLEGRO_DEBUG("Creating backbuffer\n");

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
      _al_pixel_format_name(format));

   /* Now that the display backbuffer has a format, update extra_settings so
    * the user can query it back.
    */
   _al_set_color_components(format, &disp->extra_settings, ALLEGRO_REQUIRE);
   disp->backbuffer_format = format;

   ALLEGRO_DEBUG("Creating backbuffer bitmap\n");
   /* Using ALLEGRO_NO_PRESERVE_TEXTURE prevents extra memory being allocated */
   backbuffer = _al_ogl_create_bitmap(disp, disp->w, disp->h,
      format, ALLEGRO_VIDEO_BITMAP | ALLEGRO_NO_PRESERVE_TEXTURE);
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
   al_identity_transform(&backbuffer->transform);
   al_identity_transform(&backbuffer->proj_transform);
   al_orthographic_transform(&backbuffer->proj_transform, 0, 0, -1.0, disp->w, disp->h, 1.0);

   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)(
      "Created backbuffer bitmap (actual format: %s)\n",
      _al_pixel_format_name(al_get_bitmap_format(backbuffer)));

   ogl_backbuffer = backbuffer->extra;
   ogl_backbuffer->true_w = disp->w;
   ogl_backbuffer->true_h = disp->h;
   ogl_backbuffer->is_backbuffer = 1;
   backbuffer->_display = disp;

   return backbuffer;
}


void _al_ogl_destroy_backbuffer(ALLEGRO_BITMAP *b)
{
   al_destroy_bitmap(b);
}


/* vi: set sts=3 sw=3 et: */
