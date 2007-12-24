/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * bitmap driver.
 */

#include "xglx.h"

static ALLEGRO_BITMAP_INTERFACE *vt;

extern void _al_draw_bitmap_region_memory_fast(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags);


static void quad(ALLEGRO_BITMAP *bitmap, float sx, float sy, float sw, float sh,
    float cx, float cy, float dx, float dy, float dw, float dh, float angle,
    int flags)
{
   float l, t, r, b, w, h;
   ALLEGRO_BITMAP_XGLX *xbitmap = (void *)bitmap;
   GLboolean on;
   ALLEGRO_INDEPENDANT_COLOR *bc;
   int src_mode, dst_mode;
   int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };

   // FIXME: hack
   // FIXME: need format conversion if they don't match
   // FIXME: won't work that way if it is scaled or rotated
   ALLEGRO_DISPLAY_XGLX *glx = (void *)al_get_current_display();
   if (glx->temporary_hack) {
      _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
      return;
   }

   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on)
      glEnable(GL_TEXTURE_2D);

   al_get_blender(&src_mode, &dst_mode, NULL);
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_mode], blend_modes[dst_mode]);

   glBindTexture(GL_TEXTURE_2D, xbitmap->texture);
   l = xbitmap->left;
   t = xbitmap->top;
   r = xbitmap->right;
   b = xbitmap->bottom;
   w = bitmap->w;
   h = bitmap->h;

   l += sx / w;
   t += sy / h;
   r -= (w - sx - sw) / w;
   b -= (h - sy - sh) / h;

   bc = _al_get_blend_color();
   glColor4f(bc->r, bc->g, bc->b, bc->a);

   glPushMatrix();
   glTranslatef(dx, dy, 0);
   glRotatef(angle * 180 / AL_PI, 0, 0, -1);
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
static void draw_bitmap(ALLEGRO_BITMAP *bitmap, float x, float y, int flags)
{
   quad(bitmap, 0, 0, bitmap->w, bitmap->h,
      0, 0, x, y, bitmap->w, bitmap->h, 0, flags);
}

static void draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   quad(bitmap, 0, 0, sw, sh, 0, 0, dx, dy, dw, dh, 0, flags);
}

static void draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, int flags)
{
   quad(bitmap, sx, sy, sw, sh, 0, 0, dx, dy, sw, sh, 0, flags);
}

static void draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float angle, int flags)
{
   quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy,
      dx, dy, bitmap->w, bitmap->h, angle, flags);
}

static void draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float xscale, float yscale, float angle,
	float flags)
{
   quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy, dx, dy,
      bitmap->w * xscale, bitmap->h * yscale, angle, flags);
}

/* Helper to get smallest fitting power of two. */
#if 0
static int pot(int x)
{
   int y = 1;
   while (y < x) y *= 2;
   return y;
}
#endif

static void upside_down(ALLEGRO_BITMAP *bitmap, int x, int y, int w, int h)
{
    int pixelsize = al_get_pixel_size(bitmap->format);
    int pitch = pixelsize * bitmap->w;
    int bytes = w * pixelsize;
    int i;
    unsigned char temp[pitch];
    unsigned char *ptr = bitmap->memory + pitch * y + pixelsize * x;
    for (i = 0; i < h / 2; i++)
    {
        memcpy(temp, ptr + pitch * i, bytes);
        memcpy(ptr + pitch * i, ptr + pitch * (h - 1 - i), bytes);
        memcpy(ptr + pitch * (h - 1 - i), temp, bytes);
    }
}

/* Conversion table from Allegro's pixel formats to corresponding OpenGL
 * formats. The three entries are GL internal format, GL type, GL format.
 */
static int glformats[][3] = {
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

// FIXME: need to do all the logic AllegroGL does, checking extensions,
// proxy textures, formats, limits ...
static bool upload_bitmap(ALLEGRO_BITMAP *bitmap, int x, int y, int w, int h)
{
   // FIXME: Some OpenGL drivers need power of two textures.
   ALLEGRO_BITMAP_XGLX *xbitmap = (void *)bitmap;

   if (xbitmap->texture == 0)
      glGenTextures(1, &xbitmap->texture);
   glBindTexture(GL_TEXTURE_2D, xbitmap->texture);

   glTexImage2D(GL_TEXTURE_2D, 0, glformats[bitmap->format][0],
      bitmap->w, bitmap->h, 0, glformats[bitmap->format][2],
      glformats[bitmap->format][1], bitmap->memory);
   if (glGetError())
      TRACE("xbitmap: glTexImage2D for format %d failed.\n", bitmap->format);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   xbitmap->left = 0;
   xbitmap->right = 1;
   xbitmap->top = 0;
   xbitmap->bottom = 1;

   // FIXME: TODO
   //if (xbitmap->fbo == 0) {
   //   glGenFramebuffersEXT(1, &xbitmap->fbo);
   //   glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, xbitmap->fbo);
   //}

   return 1;
}

/* OpenGL cannot "lock" pixels, so instead we update our memory copy and
 * return a pointer into that.
 */
static ALLEGRO_LOCKED_REGION *lock_region(ALLEGRO_BITMAP *bitmap,
	int x, int y, int w, int h, ALLEGRO_LOCKED_REGION *locked_region,
	int flags)
{
    ALLEGRO_BITMAP_XGLX *xbitmap = (void *)bitmap;
    int pixelsize = al_get_pixel_size(bitmap->format);
    int pitch = pixelsize * bitmap->w;

    if (!(flags & ALLEGRO_LOCK_WRITEONLY)) {
        if (xbitmap->is_backbuffer) {
            glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->w);
            glReadPixels(0, bitmap->h - y - h, w, h,
                GL_RGBA, GL_UNSIGNED_BYTE,
                bitmap->memory + pitch * y + pixelsize * x);
            if (glGetError())
               TRACE("xbitmap: glReadPixels for format %d failed.\n",
                  bitmap->format);
            //FIXME: ugh. isn't there a better way?
            upside_down(bitmap, x, y, w, h);
        }
        else {
            //FIXME: use glPixelStore or similar to only synchronize the required
            //region
            glBindTexture(GL_TEXTURE_2D, xbitmap->texture);
            glGetTexImage(GL_TEXTURE_2D, 0, glformats[bitmap->format][2],
               glformats[bitmap->format][1],
               bitmap->memory);
            if (glGetError())
               TRACE("xbitmap: glGetTexImage for format %d failed.\n",
                  bitmap->format);
        }
    }

    locked_region->data = bitmap->memory + pitch * y + pixelsize * x;
    locked_region->format = bitmap->format;
    locked_region->pitch = pitch;

    return locked_region;
}

/* Synchronizes the texture back to the (possibly modified) bitmap data.
 */
static void unlock_region(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_XGLX *xbitmap = (void *)bitmap;
   int pixelsize = al_get_pixel_size(bitmap->format);
   int pitch = pixelsize * bitmap->w;

   if (bitmap->lock_flags & ALLEGRO_LOCK_READONLY) return;
    
   if (xbitmap->is_backbuffer) {
      //FIXME: ugh. isn't there a better way?
      upside_down(bitmap, bitmap->lock_x, bitmap->lock_y, bitmap->lock_w, bitmap->lock_h);

      glRasterPos2i(0, bitmap->lock_y + bitmap->lock_h);
      glPixelStorei(GL_PACK_ROW_LENGTH, bitmap->w);
      glDrawPixels(bitmap->lock_w, bitmap->lock_h,
         glformats[bitmap->format][2],
         glformats[bitmap->format][1],
         bitmap->memory + bitmap->lock_y * pitch + pixelsize * bitmap->lock_x);
      if (glGetError())
         TRACE("xbitmap: glDrawPixels for format %d failed.\n",
            bitmap->format);
   }
   else {
      // FIXME: don't copy the whole bitmap
      glBindTexture(GL_TEXTURE_2D, xbitmap->texture);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
         bitmap->w, bitmap->h, glformats[bitmap->format][2],
         glformats[bitmap->format][1],
         bitmap->memory);
      if (glGetError())
         TRACE("xbitmap: glTexSubImage2D for format %d failed.\n",
            bitmap->format);
   }
}

static void xglx_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_XGLX *xbitmap = (void *)bitmap;
   if (xbitmap->texture) {
      glDeleteTextures(1, &xbitmap->texture);
      xbitmap->texture = 0;
   }
}

/* Obtain a reference to this driver. */
ALLEGRO_BITMAP_INTERFACE *_al_bitmap_xglx_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->draw_bitmap = draw_bitmap;
   vt->upload_bitmap = upload_bitmap;
   vt->destroy_bitmap = xglx_destroy_bitmap;
   vt->draw_scaled_bitmap = draw_scaled_bitmap;
   vt->draw_bitmap_region = draw_bitmap_region;
   vt->draw_rotated_scaled_bitmap = draw_rotated_scaled_bitmap;
   vt->draw_rotated_bitmap = draw_rotated_bitmap;
   vt->lock_region = lock_region;
   vt->unlock_region = unlock_region;

   return vt;
}
