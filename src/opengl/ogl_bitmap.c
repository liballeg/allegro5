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
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"


/* Conversion table from Allegro's pixel formats to corresponding OpenGL
 * formats. The three entries are GL internal format, GL type, GL format.
 */
static const int glformats[][3] = {
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
   /* Actual formats */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* ARGB_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_8888 */
   {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_BGRA}, /* ARGB_4444 */
   {GL_RGB8, GL_UNSIGNED_BYTE, GL_BGR}, /* RGB_888 */
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB}, /* RGB_565 */
   {0, 0, 0}, /* RGB_555 */ //FIXME: how?
   {GL_INTENSITY, GL_UNSIGNED_BYTE, GL_COLOR_INDEX}, /* PALETTE_8 */
   {GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA}, /* RGBA_5551 */
   {0, 0, 0}, /* ARGB_1555 */ //FIXME: how?
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* ABGR_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* XBGR_8888 */
   {GL_RGB8, GL_UNSIGNED_BYTE, GL_RGB}, /* BGR_888 */
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_BGR}, /* BGR_565 */
   {0, 0, 0}, /* BGR_555 */ //FIXME: how?
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBX_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* XRGB_8888 */
};


static ALLEGRO_BITMAP_INTERFACE *glbmp_vt;



static void draw_quad(ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh,
    float cx, float cy, float dx, float dy, float dw, float dh,
    float xscale, float yscale, float angle, int flags)
{
   float l, t, r, b, w, h;
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   GLboolean on;
   GLint current_texture;
   ALLEGRO_COLOR *bc;
   int src_mode, dst_mode;
   int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };

   // FIXME: hack
   // FIXME: need format conversion if they don't match
   // FIXME: won't work that way if it is scaled or rotated
   ALLEGRO_DISPLAY_OGL *disp = (void *)al_get_current_display();
   if (disp->opengl_target != ogl_target) {
      _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
      return;
   }

   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on)
      glEnable(GL_TEXTURE_2D);

   al_get_blender(&src_mode, &dst_mode, NULL);
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_mode], blend_modes[dst_mode]);

   glGetIntegerv(GL_TEXTURE_2D_BINDING_EXT, &current_texture);
   if (current_texture != ogl_bitmap->texture) {
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   }

   l = ogl_bitmap->left;
   t = ogl_bitmap->top;
   r = ogl_bitmap->right;
   b = ogl_bitmap->bottom;
   w = bitmap->w;
   h = bitmap->h;

   l += sx / w * r;
   t += sy / h * b;
   r -= (w - sx - sw) / w * r;
   b -= (h - sy - sh) / h * b;

   bc = _al_get_blend_color();
   glColor4f(bc->r, bc->g, bc->b, bc->a);

   glPushMatrix();
   glTranslatef(dx, dy, 0);
   glRotatef(angle * 180 / AL_PI, 0, 0, -1);
   glScalef(xscale, yscale, 1);
   glTranslatef(-dx - cx, -dy - cy, 0);

   glBegin(GL_QUADS);
   glTexCoord2f(l, t);
   glVertex2f(dx, dy);
   glTexCoord2f(r, t);
   glVertex2f(dx + dw, dy);
   glTexCoord2f(r, b);
   glVertex2f(dx + dw, dy + dh);
   glTexCoord2f(l, b);
   glVertex2f(dx, dy + dh);
   glEnd();

   glPopMatrix();

   if (!on)
      glDisable(GL_TEXTURE_2D);
}



/* Draw the bitmap at the specified position. */
static void ogl_draw_bitmap(ALLEGRO_BITMAP *bitmap, float x, float y,
   int flags)
{
   draw_quad(bitmap, 0, 0, bitmap->w, bitmap->h,
      0, 0, x, y, bitmap->w, bitmap->h, 1, 1, 0, flags);
}



static void ogl_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   draw_quad(bitmap, sx, sy, sw, sh, 0, 0, dx, dy, dw, dh, 1, 1, 0, flags);
}



static void ogl_draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, int flags)
{
   draw_quad(bitmap, sx, sy, sw, sh, 0, 0, dx, dy, sw, sh, 1, 1, 0, flags);
}



static void ogl_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float angle, int flags)
{
   draw_quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy,
      dx, dy, bitmap->w, bitmap->h, 1, 1, angle, flags);
}



static void ogl_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap,
   float cx, float cy, float dx, float dy, float xscale, float yscale,
   float angle, float flags)
{
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



/* TODO: use AllegrGL's version which doesn't involve memcpy. */
static void upside_down(ALLEGRO_BITMAP *bitmap, int x, int y, int w, int h)
{
   const int pixel_size = al_get_pixel_size(bitmap->format);
   const int pitch = bitmap->pitch;
   const size_t bytes = w * pixel_size;
   unsigned char *temp = malloc(bytes);
   unsigned char *ptr = bitmap->memory + pitch * y + pixel_size * x;
   int i;
   for (i = 0; i < h / 2; i++) {
      memcpy(temp, ptr + pitch * i, bytes);
      memcpy(ptr + pitch * i, ptr + pitch * (h - 1 - i), bytes);
      memcpy(ptr + pitch * (h - 1 - i), temp, bytes);
   }
   free(temp);
}



// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
static bool ogl_upload_bitmap(ALLEGRO_BITMAP *bitmap, int x, int y,
   int w, int h)
{
   // FIXME: Some OpenGL drivers need power of two textures.
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

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
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

   /* XXX take into account (x,y) */
   ogl_bitmap->left = 0;
   ogl_bitmap->right = (float) w / ogl_bitmap->true_w;
   ogl_bitmap->top = 0;
   ogl_bitmap->bottom = (float) h / ogl_bitmap->true_h;

   /* We try to create a frame buffer object for each bitmap we upload to
    * OpenGL.
    * TODO: Probably it's better to just create it on demand, i.e. the first
    * time drawing to it.
    */
   if (ogl_bitmap->fbo == 0 && !(bitmap->flags & ALLEGRO_FORCE_LOCKING)) {
      if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object) {

         /* Note: I had a buggy nvidia driver which would just malfunction
          * after around 170 FBOs were allocated.. but not much we can do
          * against driver bugs like that. We should have a way to disable
          * use of individual OpenGL extensions though I guess.
          */
         if (ogl_bitmap->fbo == 0) {
            glGenFramebuffersEXT(1, &ogl_bitmap->fbo);
         }
      }
   }

   return true;
}



/* OpenGL cannot "lock" pixels, so instead we update our memory copy and
 * return a pointer into that.
 */
static ALLEGRO_LOCKED_REGION *ogl_lock_region(ALLEGRO_BITMAP *bitmap,
   int x, int y, int w, int h, ALLEGRO_LOCKED_REGION *locked_region, int flags)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   const int pixel_size = al_get_pixel_size(bitmap->format);
   const int pitch = bitmap->pitch;

   if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
      if (ogl_bitmap->is_backbuffer) {
         GLint pack_row_length;
         glGetIntegerv(GL_PACK_ROW_LENGTH, &pack_row_length);
         glPixelStorei(GL_PACK_ROW_LENGTH, ogl_bitmap->true_w);
         glReadPixels(x, bitmap->h - y - h, w, h,
            glformats[bitmap->format][2],
            glformats[bitmap->format][1],
            bitmap->memory + pitch * y + pixel_size * x);
         if (glGetError()) {
            TRACE("ogl_bitmap: glReadPixels for format %d failed.\n",
               bitmap->format);
         }
         glPixelStorei(GL_PACK_ROW_LENGTH, pack_row_length);
         //FIXME: ugh. isn't there a better way?
         upside_down(bitmap, x, y, w, h);
      }
      else {
         //FIXME: use glPixelStore or similar to only synchronize the required
         //region
         glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
         glGetTexImage(GL_TEXTURE_2D, 0, glformats[bitmap->format][2],
            glformats[bitmap->format][1], bitmap->memory);
         if (glGetError()) {
            TRACE("ogl_bitmap: glGetTexImage for format %d failed.\n",
               bitmap->format);
         }
      }
   }

   locked_region->data = bitmap->memory + pitch * y + pixel_size * x;
   locked_region->format = bitmap->format;
   locked_region->pitch = pitch;

   return locked_region;
}



/* Synchronizes the texture back to the (possibly modified) bitmap data.
 */
static void ogl_unlock_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   const int pixel_size = al_get_pixel_size(bitmap->format);
   const int pitch = bitmap->pitch; 

   if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY)
      return;

   if (ogl_bitmap->is_backbuffer) {
      GLint unpack_row_length;
      //FIXME: ugh. isn't there a better way?
      upside_down(bitmap, bitmap->lock_x, bitmap->lock_y, bitmap->lock_w, bitmap->lock_h);

      /* glWindowPos2i may not be available. */
      if (al_opengl_version() >= 1.4) {
         glWindowPos2i(bitmap->lock_x, bitmap->h - bitmap->lock_y - bitmap->lock_h);
      }
      else {
         glRasterPos2i(bitmap->lock_x, bitmap->lock_y + bitmap->lock_h);
      }

      /* XXX would it be more efficient to avoid copying padding,
       * when true_w > w?
       */
      glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpack_row_length);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, ogl_bitmap->true_w);
      glDrawPixels(bitmap->lock_w, bitmap->lock_h,
         glformats[bitmap->format][2],
         glformats[bitmap->format][1],
         bitmap->memory + bitmap->lock_y * pitch + pixel_size * bitmap->lock_x);
      if (glGetError()) {
         TRACE("ogl_bitmap: glDrawPixels for format %d failed.\n",
            bitmap->format);
      }
      glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length);
   }
   else {
      // FIXME: don't copy the whole bitmap
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      /* We don't copy anything past bitmap->h on purpose. */
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
         ogl_bitmap->true_w, bitmap->h, glformats[bitmap->format][2],
         glformats[bitmap->format][1],
         bitmap->memory);
      if (glGetError()) {
         TRACE("ogl_bitmap: glTexSubImage2D for format %d failed.\n",
            bitmap->format);
      }
   }
}



static void ogl_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

   if (ogl_bitmap->texture) {
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
   }

   if (ogl_bitmap->fbo) {
      glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo);
      ogl_bitmap->fbo = 0;
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
   glbmp_vt->upload_bitmap = ogl_upload_bitmap;
   glbmp_vt->destroy_bitmap = ogl_destroy_bitmap;
   glbmp_vt->draw_scaled_bitmap = ogl_draw_scaled_bitmap;
   glbmp_vt->draw_bitmap_region = ogl_draw_bitmap_region;
   glbmp_vt->draw_rotated_scaled_bitmap = ogl_draw_rotated_scaled_bitmap;
   glbmp_vt->draw_rotated_bitmap = ogl_draw_rotated_bitmap;
   glbmp_vt->lock_region = ogl_lock_region;
   glbmp_vt->unlock_region = ogl_unlock_region;

   return glbmp_vt;
}



ALLEGRO_BITMAP *_al_ogl_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h)
{
   const ALLEGRO_DISPLAY_OGL *ogl_dpy = (void *)d;
   ALLEGRO_BITMAP_OGL *bitmap;
   int format = al_get_new_bitmap_format();
   const int flags = al_get_new_bitmap_flags();
   int true_w;
   int true_h;
   int pitch;
   size_t bytes;

   if (ogl_dpy->extension_list->ALLEGRO_GL_ARB_texture_non_power_of_two) {
      true_w = w;
      true_h = h;
   }
   else {
      true_w = pot(w);
      true_h = pot(h);
   }

   /* FIXME: do this right */
   if (! _al_pixel_format_is_real(format)) {
      format = d->format;
      //if (_al_format_has_alpha(format))
      //   format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
      //else
      //   format = ALLEGRO_PIXEL_FORMAT_XBGR_8888;
   }

   pitch = true_w * al_get_pixel_size(format);

   bitmap = _AL_MALLOC(sizeof *bitmap);
   ASSERT(bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->bitmap.vt = ogl_bitmap_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.pitch = pitch;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;
   bitmap->bitmap.cl = 0;
   bitmap->bitmap.ct = 0;
   bitmap->bitmap.cr = w - 1;
   bitmap->bitmap.cb = h - 1;

   bitmap->true_w = true_w;
   bitmap->true_h = true_h;

   bytes = pitch * true_h;
   bitmap->bitmap.memory = _AL_MALLOC_ATOMIC(bytes);
   memset(bitmap->bitmap.memory, 0, bytes);

   return &bitmap->bitmap;
}



ALLEGRO_BITMAP *_al_ogl_create_sub_bitmap(ALLEGRO_DISPLAY *d,
                                          ALLEGRO_BITMAP *parent,
                                          int x, int y, int w, int h)
{
   ALLEGRO_BITMAP_OGL* ogl_bmp;
   ALLEGRO_BITMAP_OGL* ogl_parent = (void*)parent;

   ogl_bmp = _AL_MALLOC(sizeof *ogl_bmp);
   memset(ogl_bmp, 0, sizeof *ogl_bmp);

   ogl_bmp->true_w = ogl_parent->true_w;
   ogl_bmp->true_h = ogl_parent->true_h;
   ogl_bmp->texture = ogl_parent->texture;
   ogl_bmp->fbo = ogl_parent->fbo;

   ogl_bmp->left = x / (float)ogl_parent->true_w;
   ogl_bmp->right = (x + w) / (float)ogl_parent->true_w;
   ogl_bmp->top = y / (float)ogl_parent->true_h;
   ogl_bmp->bottom = (y + h) / (float)ogl_parent->true_h;

   ogl_bmp->is_backbuffer = ogl_parent->is_backbuffer;

   ogl_bmp->bitmap.vt = parent->vt;

   return (void*)ogl_bmp;
}

/* vim: set sts=3 sw=3 et: */
