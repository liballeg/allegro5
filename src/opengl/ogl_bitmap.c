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
 *      OpenGL bitmap vtable
 *
 *      By Elias Pschernig.
 *
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_opengl.h"

/* OpenGL does not support "locking", i.e. direct access to a memory
 * buffer with pixel data. Instead, the data can be copied from/to
 * client memory. Because OpenGL stores pixel data starting with the
 * pixel in the lower left corner, that's also how we return locked
 * data. Otherwise (and as was done in earlier versions of this code)
 * we would have to flip all locked memory after receiving and before
 * sending it to OpenGL.
 * 
 * Also, we do support old OpenGL drivers where textures must have
 * power-of-two dimensions. If a non-power-of-two bitmap is created in
 * such a case, we use a texture with the next larger POT dimensions,
 * and just keep some unused padding space to the right/bottom of the
 * pixel data.
 * 
 * Putting it all together, if we have an Allegro bitmap like this,
 * with Allegro's y-coordinates:
 * 0 ###########
 * 1 #111      #
 * 2 #222      #
 * 3 #333      #
 * 4 ###########
 *
 * Then the corresponding texture looks like this with OpenGL
 * y-coordinates (assuming we use an old driver which needs padding to
 * POT):
 * 7 ................
 * 6 ................
 * 5 ................
 * 4 ###########.....
 * 3 #333      #.....
 * 2 #222      #.....
 * 1 #111      #.....
 * 0 ###########.....
 */

/* Conversion table from Allegro's pixel formats to corresponding OpenGL
 * formats. The three entries are GL internal format, GL type, GL format.
 */
static const int glformats[ALLEGRO_NUM_PIXEL_FORMATS][3] = {
   /* Skip pseudo formats */
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   /* Actual formats */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* ARGB_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_8888 */
   {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_BGRA}, /* ARGB_4444 */
   {GL_RGB8, GL_UNSIGNED_BYTE, GL_BGR}, /* RGB_888 */
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB}, /* RGB_565 */
   {0, 0, 0}, /* RGB_555 */ //FIXME: how?
   {GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA}, /* RGBA_5551 */
   {0, 0, 0}, /* ARGB_1555 */ //FIXME: how?
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* ABGR_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* XBGR_8888 */
   {GL_RGB8, GL_UNSIGNED_BYTE, GL_RGB}, /* BGR_888 */
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_BGR}, /* BGR_565 */
   {0, 0, 0}, /* BGR_555 */ //FIXME: how?
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBX_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* XRGB_8888 */
   {GL_RGBA32F_ARB, GL_FLOAT, GL_RGBA} /* ABGR_F32 */
};


static ALLEGRO_BITMAP_INTERFACE *glbmp_vt;


#define SWAP(type, x, y) {type temp = x; x = y; y = temp;}

/* Helper function to draw a bitmap with an internal OpenGL texture as
 * a textured OpenGL quad.
 */
static void draw_quad(ALLEGRO_BITMAP *bitmap,
    float sx, float sy, float sw, float sh,
    float cx, float cy, float dx, float dy, float dw, float dh,
    float xscale, float yscale, float angle, int flags)
{
   float tex_l, tex_t, tex_r, tex_b, w, h, tex_w, tex_h;
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   GLboolean on;
   GLuint current_texture;
   ALLEGRO_COLOR *bc;
   int src_color, dst_color, src_alpha, dst_alpha;
   int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };

   al_get_separate_blender(&src_color, &dst_color, &src_alpha,
      &dst_alpha, NULL);
   glEnable(GL_BLEND);
   glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
      blend_modes[src_alpha], blend_modes[dst_alpha]);

   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on) {
      glEnable(GL_TEXTURE_2D);
   }

   glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, (GLint*)&current_texture);
   if (current_texture != ogl_bitmap->texture) {
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   }
   
   tex_l = ogl_bitmap->left;
   tex_r = ogl_bitmap->right;
   tex_t = ogl_bitmap->top;
   tex_b = ogl_bitmap->bottom;

   w = bitmap->w;
   h = bitmap->h;
   tex_w = 1.0 / ogl_bitmap->true_w;
   tex_h = 1.0 / ogl_bitmap->true_h;

   tex_l += sx * tex_w;
   tex_t -= sy * tex_h;
   tex_r -= (w - sx - sw) * tex_w;
   tex_b += (h - sy - sh) * tex_h;
   
   if (flags & ALLEGRO_FLIP_HORIZONTAL)
      SWAP(float, tex_l, tex_r);

   if (flags & ALLEGRO_FLIP_VERTICAL)
      SWAP(float, tex_t, tex_b);

   bc = _al_get_blend_color();
   glColor4f(bc->r, bc->g, bc->b, bc->a);

   glPushMatrix();
   glTranslatef(dx, dy, 0);
   glRotatef(angle * 180 / ALLEGRO_PI, 0, 0, -1);
   glScalef(xscale, yscale, 1);
   glTranslatef(-dx - cx, -dy - cy, 0);

   glBegin(GL_QUADS);
   glTexCoord2f(tex_l, tex_b);
   glVertex2f(dx, dy + dh);
   glTexCoord2f(tex_r, tex_b);
   glVertex2f(dx + dw, dy + dh);
   glTexCoord2f(tex_r, tex_t);
   glVertex2f(dx + dw, dy);
   glTexCoord2f(tex_l, tex_t);
   glVertex2f(dx, dy);
   glEnd();

   glPopMatrix();

   if (!on) {
      glDisable(GL_TEXTURE_2D);
   }
}
#undef SWAP



/* Draw the bitmap at the specified position. */
static void ogl_draw_bitmap(ALLEGRO_BITMAP *bitmap, float x, float y,
   int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = (void *)al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target || target->locked) {
      _al_draw_bitmap_memory(bitmap, x, y, flags);
      return;
   }

   /* For sub-bitmaps */
   if (target->parent) {
      x += target->xofs;
      y += target->yofs;
   }

   draw_quad(bitmap, 0, 0, bitmap->w, bitmap->h,
      0, 0, x, y, bitmap->w, bitmap->h, 1, 1, 0, flags);
}



static void ogl_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = (void *)al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target || target->locked) {
      _al_draw_scaled_bitmap_memory(bitmap, sx, sy, sw, sh, dx, dy, dw, dh,
                                    flags);
      return;
   }

   /* For sub-bitmaps */
   if (target->parent) {
      dx += target->xofs;
      dy += target->yofs;
   }

   draw_quad(bitmap, sx, sy, sw, sh, 0, 0, dx, dy, dw, dh, 1, 1, 0, flags);
}



static void ogl_draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = (void *)al_get_current_display();
   
   /* For sub-bitmaps */
   if (target->parent) {
      dx += target->xofs;
      dy += target->yofs;
   }
   
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
      ALLEGRO_BITMAP_OGL *ogl_source = (void *)bitmap;
      if (ogl_source->is_backbuffer) {
         if (ogl_target->is_backbuffer) {
            /* Oh fun. Someone draws the screen to itself. */
            // FIXME: What if the target is locked?
            // FIXME: OpenGL refuses to do clipping with CopyPixels,
            // have to do it ourselves.
            glRasterPos2f(dx, dy + sh);
            glCopyPixels(sx, bitmap->h - sy - sh, sw, sh, GL_COLOR);
            return;
         }
         else {
            /* Our source bitmap is the OpenGL backbuffer, the target
             * is an OpenGL texture.
             */
            // FIXME: What if the target is locked?
            /* In general, we can't modify the texture while it's
             * FBO bound - so we temporarily disable the FBO.
             */
            if (ogl_target->fbo)
               _al_ogl_set_target_bitmap(disp, bitmap);
            glBindTexture(GL_TEXTURE_2D, ogl_target->texture);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
                dx, target->h - dy - sh,
                sx, bitmap->h - sy - sh,
                sw, sh);
            /* Fix up FBO again after the copy. */
            if (ogl_target->fbo)
               _al_ogl_set_target_bitmap(disp, target);
            return;
         }
      }
   }
   if (disp->ogl_extras->opengl_target != ogl_target || target->locked) {
      _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
      return;
   }

   draw_quad(bitmap, sx, sy, sw, sh, 0, 0, dx, dy, sw, sh, 1, 1, 0, flags);
}



static void ogl_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float angle, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = (void *)al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target || target->locked) {
      _al_draw_rotated_bitmap_memory(bitmap, cx, cy, dx, dy, angle, flags);
      return;
   }

   /* For sub-bitmaps */
   if (target->parent) {
      dx += target->xofs;
      dy += target->yofs;
   }

   draw_quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy,
      dx, dy, bitmap->w, bitmap->h, 1, 1, angle, flags);
}



static void ogl_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = (void *)al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target || target->locked) {
      _al_draw_rotated_scaled_bitmap_memory(bitmap, cx, cy, dx, dy,
                                            xscale, yscale, angle, flags);
      return;
   }

   /* For sub-bitmaps */
   if (target->parent) {
      dx += target->xofs;
      dy += target->yofs;
   }

   draw_quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy, dx, dy,
      bitmap->w, bitmap->h, xscale, yscale, angle, flags);
}



/* Helper to get smallest fitting power of two. */
static int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}



// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
static bool ogl_upload_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   int w = bitmap->w;
   int h = bitmap->h;

   if (ogl_bitmap->texture == 0) {
      glGenTextures(1, &ogl_bitmap->texture);
   }
   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   if (glGetError()) {
      TRACE("ogl_bitmap: glBindTexture for texture %d failed.\n",
         ogl_bitmap->texture);
   }

   glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
      ogl_bitmap->true_w, ogl_bitmap->true_h, 0, glformats[bitmap->format][2],
      glformats[bitmap->format][1], bitmap->memory);
   if (glGetError()) {
      TRACE("ogl_bitmap: glTexImage2D for format %d, size %dx%d failed\n",
         bitmap->format, ogl_bitmap->true_w, ogl_bitmap->true_h);
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
      // FIXME: Should we convert it into a memory bitmap? Or if the size is
      // the problem try to use multiple textures?
      return false;
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   ogl_bitmap->left = 0;
   ogl_bitmap->right = (float) w / ogl_bitmap->true_w;
   ogl_bitmap->top = (float) h / ogl_bitmap->true_h;
   ogl_bitmap->bottom = 0;

   /* We try to create a frame buffer object for each bitmap we upload to
    * OpenGL.
    * TODO: Probably it's better to just create it on demand, i.e. the first
    * time drawing to it.
    */
   if (ogl_bitmap->fbo == 0 && !(bitmap->flags & ALLEGRO_FORCE_LOCKING)) {
      if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object) {
         glGenFramebuffersEXT(1, &ogl_bitmap->fbo);
      }
   }

   return true;
}



static void ogl_update_clipping_rectangle(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ALLEGRO_DISPLAY *ogl_disp = (void *)display;
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

   if (ogl_disp->ogl_extras->opengl_target == ogl_bitmap) {
      _al_ogl_setup_bitmap_clipping(bitmap);
   }
}



/* OpenGL cannot "lock" pixels, so instead we update our memory copy and
 * return a pointer into that.
 */
static ALLEGRO_LOCKED_REGION *ogl_lock_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, int format, int flags)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   int pixel_size;
   int pitch = 0;
   ALLEGRO_DISPLAY *old_disp = NULL;
   GLint gl_y = bitmap->h - y - h;

   if (format == ALLEGRO_PIXEL_FORMAT_ANY)
      format = bitmap->format;

   format = _al_get_real_pixel_format(format);

   pixel_size = al_get_pixel_size(format);

   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != al_get_current_display()) {
      old_disp = al_get_current_display();
      al_set_current_display(bitmap->display);
   }

   if (ogl_bitmap->is_backbuffer) {
      pitch = w * pixel_size;
      ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * h);

      if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
         glReadPixels(x, gl_y, w, h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         if (glGetError()) {
            TRACE("ogl_bitmap: glReadPixels for format %d failed.\n",
               format);
         }
      }
      bitmap->locked_region.data = ogl_bitmap->lock_buffer +
         pitch * (h - 1);
   }
   else {
      if (flags & ALLEGRO_LOCK_WRITEONLY) {
         /* For write-only locking, we allocate a buffer just big enough
          * to later be passed to glTexSubImage2D.
          */
         pitch = w * pixel_size;
         ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * h);
         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (h - 1);
      }
      else {
         // FIXME: Using glGetTexImage means we always read the complete
         // texture - even when only a single pixel is locked. Likely
         // using FBO and glReadPixels to just read the locked part
         // would be faster.
         pitch = ogl_bitmap->true_w * pixel_size;
         ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * ogl_bitmap->true_h);

         glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
         glGetTexImage(GL_TEXTURE_2D, 0, glformats[format][2],
            glformats[format][1], ogl_bitmap->lock_buffer);
         if (glGetError()) {
            TRACE("ogl_bitmap: glGetTexImage for format %d failed.\n",
               format);
         }

         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (gl_y + h - 1) + pixel_size * x;
      }
   }

   bitmap->locked_region.format = format;
   bitmap->locked_region.pitch = -pitch;

   if (old_disp) {
      al_set_current_display(old_disp);
   }
   return &bitmap->locked_region;
}



/* Synchronizes the texture back to the (possibly modified) bitmap data.
 */
static void ogl_unlock_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   const int format = bitmap->locked_region.format;
   ALLEGRO_DISPLAY *old_disp = NULL;

   if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY) {
      _AL_FREE(ogl_bitmap->lock_buffer);
      return;
   }

   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != al_get_current_display()) {
      old_disp = al_get_current_display();
      al_set_current_display(bitmap->display);
   }

   if (ogl_bitmap->is_backbuffer) {
      GLint gl_y = bitmap->h - bitmap->lock_y - bitmap->lock_h;
      /* glWindowPos2i may not be available. */
      if (al_opengl_version() >= 1.4) {
         glWindowPos2i(bitmap->lock_x, gl_y);
      }
      else {
         /* The offset is to keep the coordinate within bounds, which was at
          * least needed on my machine. --pw
          */
         glRasterPos2f(bitmap->lock_x,
            bitmap->lock_y + bitmap->lock_h - 1e-4f);
      }
      glDrawPixels(bitmap->lock_w, bitmap->lock_h,
         glformats[format][2],
         glformats[format][1],
         ogl_bitmap->lock_buffer);
      if (glGetError()) {
         TRACE("ogl_bitmap: glDrawPixels for format %d failed.\n",
            format);
      }
      //glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
   }
   else {
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      if (bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY) {
         GLint gl_y = bitmap->h - bitmap->lock_y - bitmap->lock_h;
         glTexSubImage2D(GL_TEXTURE_2D, 0,
            bitmap->lock_x, gl_y,
            bitmap->lock_w, bitmap->lock_h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         if (glGetError()) {
            TRACE("ogl_bitmap: glTexSubImage2D for format %d failed.\n",
               format);
         }
      }
      else {
         // FIXME: Don't copy the whole bitmap. For example use
         // FBO and glDrawPixels to draw the locked area back
         // into the texture.

         /* We don't copy anything past bitmap->h on purpose. */
         glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            ogl_bitmap->true_w, bitmap->h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         if (glGetError()) {
            TRACE("ogl_bitmap: glTexSubImage2D for format %d failed.\n",
               format);
         }
      }
   }

   if (old_disp) {
      al_set_current_display(old_disp);
   }

   _AL_FREE(ogl_bitmap->lock_buffer);
}



static void ogl_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   ALLEGRO_DISPLAY *old_disp = NULL;

   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != al_get_current_display()) {
      old_disp = al_get_current_display();
      al_set_current_display(bitmap->display);
   }

   if (ogl_bitmap->texture) {
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
   }

   if (ogl_bitmap->fbo) {
      glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo);
      ogl_bitmap->fbo = 0;
   }

   if (old_disp) {
      al_set_current_display(old_disp);
   }
}



/* Obtain a reference to this driver. */
static ALLEGRO_BITMAP_INTERFACE *ogl_bitmap_driver(void)
{
   if (glbmp_vt)
      return glbmp_vt;

   glbmp_vt = _AL_MALLOC(sizeof *glbmp_vt);
   memset(glbmp_vt, 0, sizeof *glbmp_vt);

   glbmp_vt->draw_bitmap = ogl_draw_bitmap;
   glbmp_vt->draw_bitmap_region = ogl_draw_bitmap_region;
   glbmp_vt->draw_scaled_bitmap = ogl_draw_scaled_bitmap;
   glbmp_vt->draw_rotated_bitmap = ogl_draw_rotated_bitmap;
   glbmp_vt->draw_rotated_scaled_bitmap = ogl_draw_rotated_scaled_bitmap;
   glbmp_vt->upload_bitmap = ogl_upload_bitmap;
   glbmp_vt->update_clipping_rectangle = ogl_update_clipping_rectangle;
   glbmp_vt->destroy_bitmap = ogl_destroy_bitmap;
   glbmp_vt->lock_region = ogl_lock_region;
   glbmp_vt->unlock_region = ogl_unlock_region;

   return glbmp_vt;
}



ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h)
{
   const ALLEGRO_DISPLAY *ogl_dpy = (void *)d;
   ALLEGRO_BITMAP_OGL *bitmap;
   int format = al_get_new_bitmap_format();
   const int flags = al_get_new_bitmap_flags();
   int true_w;
   int true_h;
   int pitch;
   size_t bytes;

   if (ogl_dpy->ogl_extras->extension_list->ALLEGRO_GL_ARB_texture_non_power_of_two) {
      true_w = w;
      true_h = h;
   }
   else {
      true_w = pot(w);
      true_h = pot(h);
   }

   if (! _al_pixel_format_is_real(format)) {
      if (format == ALLEGRO_PIXEL_FORMAT_ANY)
         format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_XBGR_8888;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA)
         return NULL;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_BGR_565;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_BGR_888;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_XBGR_8888;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA)
         return NULL;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_RGBA_5551;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA)
         return NULL;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
      else if (format == ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA)
         format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
   }

   pitch = true_w * al_get_pixel_size(format);

   bitmap = _AL_MALLOC(sizeof *bitmap);
   ASSERT(bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->bitmap.size = sizeof *bitmap;
   bitmap->bitmap.vt = ogl_bitmap_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.pitch = pitch;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;
   bitmap->bitmap.cl = 0;
   bitmap->bitmap.ct = 0;
   bitmap->bitmap.cr = w;
   bitmap->bitmap.cb = h;

   bitmap->true_w = true_w;
   bitmap->true_h = true_h;

   bytes = pitch * true_h;
   bitmap->bitmap.memory = _AL_MALLOC_ATOMIC(bytes);

   return &bitmap->bitmap;
}



ALLEGRO_BITMAP *_al_ogl_create_sub_bitmap(ALLEGRO_DISPLAY *d,
                                          ALLEGRO_BITMAP *parent,
                                          int x, int y, int w, int h)
{
   ALLEGRO_BITMAP_OGL* ogl_bmp;
   ALLEGRO_BITMAP_OGL* ogl_parent = (void*)parent;
   (void)d;

   ogl_bmp = _AL_MALLOC(sizeof *ogl_bmp);
   memset(ogl_bmp, 0, sizeof *ogl_bmp);

   ogl_bmp->true_w = ogl_parent->true_w;
   ogl_bmp->true_h = ogl_parent->true_h;
   ogl_bmp->texture = ogl_parent->texture;
   ogl_bmp->fbo = ogl_parent->fbo;

   ogl_bmp->left = x / (float)ogl_parent->true_w;
   ogl_bmp->right = (x + w) / (float)ogl_parent->true_w;
   ogl_bmp->top = (parent->h - y) / (float)ogl_parent->true_h;
   ogl_bmp->bottom = (parent->h - y - h) / (float)ogl_parent->true_h;

   ogl_bmp->is_backbuffer = ogl_parent->is_backbuffer;

   ogl_bmp->bitmap.vt = parent->vt;

   return (void*)ogl_bmp;
}

/* Function: al_get_opengl_texture
 */
GLuint al_get_opengl_texture(ALLEGRO_BITMAP *bitmap)
{
   // FIXME: Check if this is an OpenGL bitmap, if not, return 0
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   return ogl_bitmap->texture;
}

/* vim: set sts=3 sw=3 et: */
