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
 *      OpenGL ES 1.1 support by Trent Gamblin.
 *
 */

/* FIXME: Add the other format for gp2xwiz 1555 or 5551 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_system.h"
#include <math.h>

#if defined ALLEGRO_GP2XWIZ
#include "allegro5/internal/aintern_gp2xwiz.h"
#endif

ALLEGRO_DEBUG_CHANNEL("opengl")

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

static GLint ogl_min_filter = GL_NEAREST;
static GLint ogl_mag_filter = GL_NEAREST;

static GLint ogl_get_filter(const char *s)
{
   if (!stricmp(s, "LINEAR"))
      return GL_LINEAR;
   if (!stricmp(s, "ANISOTROPIC"))
      return GL_LINEAR;
   return GL_NEAREST;
}

/* Conversion table from Allegro's pixel formats to corresponding OpenGL
 * formats. The three entries are GL internal format, GL type, GL format.
 */
#if !defined(ALLEGRO_GP2XWIZ) && !defined(ALLEGRO_IPHONE)
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
   /* Actual formats */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* ARGB_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBA_8888 */
   {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_BGRA}, /* ARGB_4444 */
   {GL_RGB8, GL_UNSIGNED_BYTE, GL_BGR}, /* RGB_888 */
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB}, /* RGB_565 */
   {GL_RGB5_A1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_BGRA}, /* RGB_555 */
   {GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA}, /* RGBA_5551 */
   {GL_RGB5_A1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_BGRA}, /* ARGB_1555 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* ABGR_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_RGBA}, /* XBGR_8888 */
   {GL_RGB8, GL_UNSIGNED_BYTE, GL_RGB}, /* BGR_888 */
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5_REV, GL_RGB}, /* BGR_565 */
   {GL_RGB5_A1, GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_RGBA}, /* BGR_555 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8, GL_RGBA}, /* RGBX_8888 */
   {GL_RGBA8, GL_UNSIGNED_INT_8_8_8_8_REV, GL_BGRA}, /* XRGB_8888 */
   {GL_RGBA32F_ARB, GL_FLOAT, GL_RGBA}, /* ABGR_F32 */
   {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA}, /* ABGR_8888_LE */
   {GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA} /* RGBA_4444 */
};
#else
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
   /* Actual formats */
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, GL_RGB}, /* RGB_565 */
   {0, 0, 0},
   {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, GL_RGBA}, /* RGBA_5551 */
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {0, 0, 0},
   {GL_RGBA, GL_UNSIGNED_BYTE, GL_RGBA}, /* ABGR_8888_LE */
   {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, GL_RGBA} /* RGBA_4444 */
};
#endif

static ALLEGRO_BITMAP_INTERFACE *glbmp_vt;


#define SWAP(type, x, y) {type temp = x; x = y; y = temp;}

#define ERR(e) case e: return #e;
static char const *error_string(GLenum e)
{
   switch (e) {
      ERR(GL_NO_ERROR)
      ERR(GL_INVALID_ENUM)
      ERR(GL_INVALID_VALUE)
      ERR(GL_INVALID_OPERATION)
      ERR(GL_STACK_OVERFLOW)
      ERR(GL_STACK_UNDERFLOW)
      ERR(GL_OUT_OF_MEMORY)
   }
   return "UNKNOWN";
}
#undef ERR

static INLINE bool setup_blending(ALLEGRO_DISPLAY *ogl_disp)
{
   int op, src_color, dst_color, op_alpha, src_alpha, dst_alpha;
   const int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };
   const int blend_equations[3] = {
      GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT
   };

   (void)ogl_disp;

   al_get_separate_blender(&op, &src_color, &dst_color,
      &op_alpha, &src_alpha, &dst_alpha, NULL);
   /* glBlendFuncSeparate was only included with OpenGL 1.4 */
   /* (And not in OpenGL ES) */
#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   if (ogl_disp->ogl_extras->ogl_info.version >= 1.4) {
      glEnable(GL_BLEND);
      glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
         blend_modes[src_alpha], blend_modes[dst_alpha]);
      if (ogl_disp->ogl_extras->ogl_info.version >= 2.0) {
         glBlendEquationSeparate(
            blend_equations[op],
            blend_equations[op_alpha]);
      }
      else
         glBlendEquation(blend_equations[op]);
   }
   else {
      if (src_color == src_alpha && dst_color == dst_alpha) {
         glEnable(GL_BLEND);
         glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
      }
      else {
         return false;
      }
   }

#elif defined(ALLEGRO_IPHONE)
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
   glBlendEquation(blend_equations[op]);
   /* FIXME: Only OpenGL ES 2.0 has both functions and the OES versions
    * Apple put into ES 1.0 seem to just crash. Should try if it's fixed
    * in their next update.
    */
   //glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
   //   blend_modes[src_alpha], blend_modes[dst_alpha]);
   //glBlendEquationSeparate(
   //   blend_equations[op],
   //   blend_equations[op_alpha]);
#else
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
#endif
   return true;
}

static INLINE void transform_vertex(float cx, float cy, float dx, float dy, 
    float xscale, float yscale, float c, float s, float* x, float* y)
{
   if(s == 0) {
      *x = xscale * (*x - dx - cx) + dx;
      *y = yscale * (*y - dy - cy) + dy;
   } else {
      float t;
      *x = xscale * (*x - dx - cx);
      *y = yscale * (*y - dy - cy);
      t = *x;
      *x = c * *x - s * *y + dx;
      *y = s * t + c * *y + dy;
   }
}

static void draw_quad(ALLEGRO_BITMAP *bitmap,
    float sx, float sy, float sw, float sh,
    float cx, float cy, float dx, float dy, float dw, float dh,
    float xscale, float yscale, float angle, int flags)
{
   float tex_l, tex_t, tex_r, tex_b, w, h, tex_w, tex_h;
   float c, s;
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   ALLEGRO_OGL_BITMAP_VERTEX* verts;
   ALLEGRO_DISPLAY* disp = al_get_current_display();
   ALLEGRO_COLOR* bc = _al_get_blend_color();

   if (disp->num_cache_vertices != 0 && ogl_bitmap->texture != disp->cache_texture) {
      disp->vt->flush_vertex_cache(disp);
   }
   disp->cache_texture = ogl_bitmap->texture;
   
   verts = disp->vt->prepare_vertex_cache(disp, 6);
   
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

   verts[0].x = dx;
   verts[0].y = dy+dh;
   verts[0].tx = tex_l;
   verts[0].ty = tex_b;
   verts[0].r = bc->r;
   verts[0].g = bc->g;
   verts[0].b = bc->b;
   verts[0].a = bc->a;
   
   verts[1].x = dx;
   verts[1].y = dy;
   verts[1].tx = tex_l;
   verts[1].ty = tex_t;
   verts[1].r = bc->r;
   verts[1].g = bc->g;
   verts[1].b = bc->b;
   verts[1].a = bc->a;
   
   verts[2].x = dx+dw;
   verts[2].y = dy+dh;
   verts[2].tx = tex_r;
   verts[2].ty = tex_b;
   verts[2].r = bc->r;
   verts[2].g = bc->g;
   verts[2].b = bc->b;
   verts[2].a = bc->a;
   
   verts[4].x = dx+dw;
   verts[4].y = dy;
   verts[4].tx = tex_r;
   verts[4].ty = tex_t;
   verts[4].r = bc->r;
   verts[4].g = bc->g;
   verts[4].b = bc->b;
   verts[4].a = bc->a;
   
   if(angle == 0) {
      c = 1;
      s = 0;
   } else {
      c = cosf(angle);
      s = sinf(angle);
   }
   
   transform_vertex(cx, cy, dx, dy, xscale, yscale, c, s, &verts[0].x, &verts[0].y);
   transform_vertex(cx, cy, dx, dy, xscale, yscale, c, s, &verts[1].x, &verts[1].y);
   transform_vertex(cx, cy, dx, dy, xscale, yscale, c, s, &verts[2].x, &verts[2].y);
   transform_vertex(cx, cy, dx, dy, xscale, yscale, c, s, &verts[4].x, &verts[4].y);
   
   verts[3] = verts[1];
   verts[5] = verts[2];
   
   if(!disp->cache_enabled)
      disp->vt->flush_vertex_cache(disp);
}
#undef SWAP



static void ogl_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target ||
      !setup_blending(disp) || target->locked) {
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
   ALLEGRO_DISPLAY *disp = al_get_current_display();
   
   /* For sub-bitmaps */
   if (target->parent) {
      dx += target->xofs;
      dy += target->yofs;
   }
   
   if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
#if !defined ALLEGRO_GP2XWIZ
      ALLEGRO_BITMAP_OGL *ogl_source = (void *)bitmap;
      if (ogl_source->is_backbuffer) {
         if (ogl_target->is_backbuffer) {
            #if !defined ALLEGRO_IPHONE
            /* Oh fun. Someone draws the screen to itself. */
            // FIXME: What if the target is locked?
            // FIXME: OpenGL refuses to do clipping with CopyPixels,
            // have to do it ourselves.
            if (setup_blending(disp)) {
               glRasterPos2f(dx, dy + sh);
               glCopyPixels(sx, bitmap->h - sy - sh, sw, sh, GL_COLOR);
               return;
            }
            #endif
         }
         else {
            /* Our source bitmap is the OpenGL backbuffer, the target
             * is an OpenGL texture.
             */
            // FIXME: What if the target is locked?
            // FIXME: This does no blending.
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
#elif defined ALLEGRO_GP2XWIZ
      /* FIXME: make this work somehow on Wiz */
      return;
#endif
   }
   if (disp->ogl_extras->opengl_target == ogl_target) {
      if (setup_blending(disp)) {
         draw_quad(bitmap, sx, sy, sw, sh, 0, 0, dx, dy, sw, sh, 1, 1, 0, flags);
         return;
      }
   }

   
   
   /* If all else fails, fall back to software implementation. */
   _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
}



/* Draw the bitmap at the specified position. */
static void ogl_draw_bitmap(ALLEGRO_BITMAP *bitmap, float x, float y,
   int flags)
{
   ogl_draw_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h, x, y, flags);
}



static void ogl_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float angle, int flags)
{
   // FIXME: hack
   // FIXME: need format conversion if they don't match
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (ALLEGRO_BITMAP_OGL *)target;
   ALLEGRO_DISPLAY *disp = al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target ||
      !setup_blending(disp) || target->locked) {
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
   ALLEGRO_DISPLAY *disp = al_get_current_display();
   if (disp->ogl_extras->opengl_target != ogl_target ||
      !setup_blending(disp) || target->locked) {
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
   static bool cfg_read = false;
   GLenum e;

   if (ogl_bitmap->texture == 0) {
      glGenTextures(1, &ogl_bitmap->texture);
   }
   glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glBindTexture for texture %d failed (%s).\n",
         ogl_bitmap->texture, error_string(e));
   }

    
   glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
      ogl_bitmap->true_w, ogl_bitmap->true_h, 0, glformats[bitmap->format][2],
      glformats[bitmap->format][1], bitmap->memory);
   e = glGetError();
   if (e) {
      ALLEGRO_ERROR("glTexImage2D for format %s, size %dx%d failed (%s)\n",
         _al_format_name(bitmap->format),
         ogl_bitmap->true_w, ogl_bitmap->true_h,
         error_string(e));
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
      // FIXME: Should we convert it into a memory bitmap? Or if the size is
      // the problem try to use multiple textures?
      return false;
   }
   
   if (!cfg_read) {         
      ALLEGRO_SYSTEM *sys;
      const char *s;

      cfg_read = true;

      sys = al_get_system_driver();

      if (sys->config) {
         s = al_get_config_value(sys->config, "graphics", "min_filter");
         if (s)
            ogl_min_filter = ogl_get_filter(s);
         s = al_get_config_value(sys->config, "graphics", "mag_filter");
         if (s)
            ogl_mag_filter = ogl_get_filter(s);
      }
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ogl_min_filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ogl_mag_filter);

   ogl_bitmap->left = 0;
   ogl_bitmap->right = (float) w / ogl_bitmap->true_w;
   ogl_bitmap->top = (float) h / ogl_bitmap->true_h;
   ogl_bitmap->bottom = 0;

   return true;
}



static void ogl_update_clipping_rectangle(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   ALLEGRO_DISPLAY *ogl_disp = display;
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;

   if (ogl_disp->ogl_extras->opengl_target == ogl_bitmap) {
      _al_ogl_setup_bitmap_clipping(bitmap);
   }
}



static int round_to_pack_alignment(int pitch)
{
   GLint alignment;

   glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
   return (pitch + alignment - 1) / alignment * alignment;
}



static int round_to_unpack_alignment(int pitch)
{
   GLint alignment;

   glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
   return (pitch + alignment - 1) / alignment * alignment;
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
   GLenum e;


   if (format == ALLEGRO_PIXEL_FORMAT_ANY)
      format = bitmap->format;

   format = _al_get_real_pixel_format(format);
/*#else
   format = bitmap->format;
   (void)x;
   (void)w;
   (void)gl_y;
   (void)e;
#endif
*/

   pixel_size = al_get_pixel_size(format);

   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != al_get_current_display()) {
      old_disp = al_get_current_display();
      al_set_current_display(bitmap->display);
   }

   if (ogl_bitmap->is_backbuffer) {
      pitch = round_to_unpack_alignment(w * pixel_size);
      ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * h);

      if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
         glReadPixels(x, gl_y, w, h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glReadPixels for format %s failed (%s).\n",
               _al_format_name(format), error_string(e));
         }
      }
      bitmap->locked_region.data = ogl_bitmap->lock_buffer +
         pitch * (h - 1);
   }
#if !defined ALLEGRO_GP2XWIZ
   else {
      if (flags & ALLEGRO_LOCK_WRITEONLY) {
         /* For write-only locking, we allocate a buffer just big enough
          * to later be passed to glTexSubImage2D.  The start of each row must
          * be aligned according to GL_UNPACK_ALIGNMENT.
          */
         pitch = round_to_unpack_alignment(w * pixel_size);
         ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * h);
         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (h - 1);
      }
      else {
         #ifdef ALLEGRO_IPHONE
            GLint current_fbo;
            pitch = round_to_unpack_alignment(w * pixel_size);
            ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * h);

            /* Create an FBO if there isn't one. */
            if (!ogl_bitmap->fbo) {
               ALLEGRO_STATE state;
               al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
               bitmap->locked = false; // hack :(
               al_set_target_bitmap(bitmap); // This creates the fbo
               bitmap->locked = true;
               al_restore_state(&state);
            }

            glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &current_fbo);
            glBindFramebufferOES(GL_FRAMEBUFFER_OES, ogl_bitmap->fbo);
            glReadPixels(x, gl_y, w, h,
               glformats[format][2],
               glformats[format][1],
               ogl_bitmap->lock_buffer);

            glBindFramebufferOES(GL_FRAMEBUFFER_OES, current_fbo);

            bitmap->locked_region.data = ogl_bitmap->lock_buffer +
               pitch * (h - 1);
         #else
         // FIXME: Using glGetTexImage means we always read the complete
         // texture - even when only a single pixel is locked. Likely
         // using FBO and glReadPixels to just read the locked part
         // would be faster.
         pitch = round_to_pack_alignment(ogl_bitmap->true_w * pixel_size);
         ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * ogl_bitmap->true_h);

         glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
         glGetTexImage(GL_TEXTURE_2D, 0, glformats[format][2],
            glformats[format][1], ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glGetTexImage for format %s failed (%s).\n",
               _al_format_name(format), error_string(e));
         }

         bitmap->locked_region.data = ogl_bitmap->lock_buffer +
            pitch * (gl_y + h - 1) + pixel_size * x;
         #endif
      }
   }
#else
   else {
      if (flags & ALLEGRO_LOCK_WRITEONLY) {
         pitch = round_to_unpack_alignment(w * pixel_size);
         ogl_bitmap->lock_buffer = _AL_MALLOC(pitch * h);
	 bitmap->locked_region.data = ogl_bitmap->lock_buffer;
	 pitch = -pitch;
         //bitmap->locked_region.data = ogl_bitmap->lock_buffer +
           // pitch * (h - 1);
      }
      else {
         /* FIXME: implement */
         return NULL;
      }
   }
#endif

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
   GLenum e;
   GLint gl_y = bitmap->h - bitmap->lock_y - bitmap->lock_h;
   (void)e;

   if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY) {
      _AL_FREE(ogl_bitmap->lock_buffer);
      return;
   }
//#endif

   if (bitmap->display->ogl_extras->is_shared == false &&
       bitmap->display != al_get_current_display()) {
      old_disp = al_get_current_display();
      al_set_current_display(bitmap->display);
   }

#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   if (ogl_bitmap->is_backbuffer) {
      /* glWindowPos2i may not be available. */
      if (al_get_opengl_version() >= 1.4) {
         glWindowPos2i(bitmap->lock_x, gl_y);
      }
      else {
         /* The offset is to keep the coordinate within bounds, which was at
          * least needed on my machine. --pw
          */
         glRasterPos2f(bitmap->lock_x,
            bitmap->lock_y + bitmap->lock_h - 1e-4f);
      }
      glDisable(GL_BLEND);
      glDrawPixels(bitmap->lock_w, bitmap->lock_h,
         glformats[format][2],
         glformats[format][1],
         ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         ALLEGRO_ERROR("glDrawPixels for format %s failed (%s).\n",
            _al_format_name(format), error_string(e));
      }
   }
   else {
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      if (bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY) {
         glTexSubImage2D(GL_TEXTURE_2D, 0,
            bitmap->lock_x, gl_y,
            bitmap->lock_w, bitmap->lock_h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glTexSubImage2D for format %d failed (%s).\n",
               format, error_string(e));
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
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glTexSubImage2D for format %s failed (%s).\n",
               _al_format_name(format), error_string(e));
         }
      }
   }
#else
   if (ogl_bitmap->is_backbuffer) {
      GLuint tmp_tex;
      glGenTextures(1, &tmp_tex);
      glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0], bitmap->lock_w, bitmap->lock_h,
                   0, glformats[bitmap->format][2], glformats[bitmap->format][1],
                   ogl_bitmap->lock_buffer);
      e = glGetError();
      if (e) {
         int printf(const char *, ...);
         printf("glTexImage2D failed: %d\n", e);
      }
      glDrawTexiOES(bitmap->lock_x, bitmap->lock_y, 0, bitmap->lock_w, bitmap->lock_h);
      e = glGetError();
      if (e) {
         int printf(const char *, ...);
         printf("glDrawTexiOES failed: %d\n", e);
      }
      glDeleteTextures(1, &tmp_tex);
   }
   else {
      glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      if (bitmap->lock_flags & ALLEGRO_LOCK_WRITEONLY) {
         int fake_pitch = round_to_unpack_alignment(bitmap->w *
            al_get_pixel_size(bitmap->format));
         _al_convert_bitmap_data(
            bitmap->locked_region.data, bitmap->locked_region.format,
            bitmap->locked_region.pitch,
	    bitmap->memory+fake_pitch*(bitmap->h-1), bitmap->format,
	    -fake_pitch,
            0, 0, bitmap->lock_x, bitmap->lock_y,
            bitmap->lock_w, bitmap->lock_h);
         glTexSubImage2D(GL_TEXTURE_2D, 0,
            bitmap->lock_x, gl_y,
            bitmap->lock_w, bitmap->lock_h,
            glformats[bitmap->format][2],
            glformats[bitmap->format][1],
            bitmap->memory);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glTexImage2D for format %d failed (%s).\n",
               format, error_string(e));
         }
      }
      else {
         #ifdef ALLEGRO_IPHONE
         /* We don't copy anything past bitmap->h on purpose. */
         glTexSubImage2D(GL_TEXTURE_2D, 0, bitmap->lock_x, gl_y,
            bitmap->lock_w, bitmap->lock_h,
            glformats[format][2],
            glformats[format][1],
            ogl_bitmap->lock_buffer);
         e = glGetError();
         if (e) {
            ALLEGRO_ERROR("glTexSubImage2D for format %s failed (%s).\n",
               _al_format_name(format), error_string(e));
         }
         #endif
      }
   }
#endif

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

#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   if (ogl_bitmap->fbo) {
      glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo);
      ogl_bitmap->fbo = 0;
   }
#elif defined ALLEGRO_IPHONE
   if (ogl_bitmap->fbo) {
      glDeleteFramebuffersOES(1, &ogl_bitmap->fbo);
      ogl_bitmap->fbo = 0;
   }
#endif

   if (ogl_bitmap->texture) {
      glDeleteTextures(1, &ogl_bitmap->texture);
      ogl_bitmap->texture = 0;
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
   ALLEGRO_BITMAP_OGL *bitmap;
   int format = al_get_new_bitmap_format();
   const int flags = al_get_new_bitmap_flags();
   int true_w;
   int true_h;
   int pitch;
   size_t bytes;
   (void)d;

   ALLEGRO_DEBUG("Creating OpenGL bitmap\n");

#if !defined ALLEGRO_GP2XWIZ
   if (d->ogl_extras->extension_list->ALLEGRO_GL_ARB_texture_non_power_of_two) {
      true_w = w;
      true_h = h;
   }
   else {
      true_w = pot(w);
      true_h = pot(h);
   }
#else
   true_w = pot(w);
   true_h = pot(h);
#endif

/* FBOs are 16x16 minimum on iPhone, this is a workaround */
#ifdef ALLEGRO_IPHONE
   if (true_w < 16) true_w = 16;
   if (true_h < 16) true_h = 16;
#endif

   ALLEGRO_DEBUG("Using dimensions: %d %d\n", true_w, true_h);

#if !defined ALLEGRO_GP2XWIZ
   format = _al_get_real_pixel_format(format);
#else
   if (format != ALLEGRO_PIXEL_FORMAT_RGB_565 && format != ALLEGRO_PIXEL_FORMAT_RGBA_4444)
      format = ALLEGRO_PIXEL_FORMAT_RGBA_4444;
#endif

   ALLEGRO_DEBUG("Chose format %s for OpenGL bitmap\n", _al_format_name(format));

   /* XXX do we need to take into account GL_PACK_ALIGNMENT or
    * GL_UNPACK_ALIGNMENT here?
    */
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
   bitmap->bitmap.cr_excl = w;
   bitmap->bitmap.cb_excl = h;

   bitmap->true_w = true_w;
   bitmap->true_h = true_h;

   bytes = pitch * true_h;
   bitmap->bitmap.memory = _AL_MALLOC(bytes);
   
   /* We never allow un-initialized memory for OpenGL bitmaps, if it
    * is uploaded to a floating point texture it can lead to Inf and
    * NaN values which break all subsequent blending.
    */
    // FIXME!~
   memset(bitmap->bitmap.memory, 0, bytes);

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

#if !defined ALLEGRO_GP2XWIZ
   ogl_bmp->fbo = ogl_parent->fbo;
#endif

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

/* Function: al_remove_opengl_fbo
 */
void al_remove_opengl_fbo(ALLEGRO_BITMAP *bitmap)
{
#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   // FIXME: Check if this is an OpenGL bitmap
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   if (ogl_bitmap->fbo) {
      glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo);
      ogl_bitmap->fbo = 0;
   }
#else
   (void)bitmap;
#endif
}

/* Function: al_get_opengl_fbo
 */
GLuint al_get_opengl_fbo(ALLEGRO_BITMAP *bitmap)
{
#if !defined ALLEGRO_GP2XWIZ
   // FIXME: Check if this is an OpenGL bitmap, if not, return 0
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   return ogl_bitmap->fbo;
#else
   (void)bitmap;
   return 0;
#endif
}

/* Function: al_get_opengl_texture_size
 */
void al_get_opengl_texture_size(ALLEGRO_BITMAP *bitmap, int *w, int *h)
{
   /* The designers of OpenGL ES 1.0 forgot to add a function to query
    * texture sizes, so this will be the only way there to get the texture
    * size. On normal OpenGL also glGetTexLevelParameter could be used.
    */
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   *w = ogl_bitmap->true_w;
   *h = ogl_bitmap->true_h;
}

/* Function: al_get_opengl_texture_position
 */
void al_get_opengl_texture_position(ALLEGRO_BITMAP *bitmap, int *u, int *v)
{
   *u = bitmap->xofs;
   *v = bitmap->yofs;
}

/* vim: set sts=3 sw=3 et: */
