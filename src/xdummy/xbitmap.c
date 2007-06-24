/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * bitmap driver.
 */

#include "xdummy.h"

static AL_BITMAP_INTERFACE *vt;

static void quad(AL_BITMAP *bitmap, float sx, float sy, float sw, float sh,
    float cx, float cy, float dx, float dy, float dw, float dh, float angle,
    int flags)
{
   float l, t, r, b, w, h;
   AL_BITMAP_XDUMMY *xbitmap = (void *)bitmap;
   GLboolean on;
   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on)
      glEnable(GL_TEXTURE_2D);
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

   glColor4f(1, 1, 1, 1);

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

   if (!on)
      glDisable(GL_TEXTURE_2D);
}

/* Draw the bitmap at the specified position. */
static void draw_bitmap(AL_BITMAP *bitmap, float x, float y, int flags)
{
   quad(bitmap, 0, 0, bitmap->w, bitmap->h,
      0, 0, x, y, bitmap->w, bitmap->h, 0, flags);
}

static void draw_scaled_bitmap(AL_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   quad(bitmap, 0, 0, sw, sh, 0, 0, dx, dy, dw, dh, 0, flags);
}

static void draw_bitmap_region(AL_BITMAP *bitmap, float sx, float sy,
   float sw, float sh, float dx, float dy, int flags)
{
   quad(bitmap, sy, sy, sw, sh, 0, 0, dx, dy, sw, sh, 0, flags);
}

static void draw_rotated_bitmap(AL_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float angle, int flags)
{
   quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy,
      dx, dy, bitmap->w, bitmap->h, angle, flags);
}

static void draw_rotated_scaled_bitmap(AL_BITMAP *bitmap, float cx, float cy,
   float dx, float dy, float xscale, float yscale, float angle,
	float flags)
{
   quad(bitmap, 0, 0, bitmap->w, bitmap->h, cx, cy, dx, dy,
      bitmap->w * xscale, bitmap->h * yscale, angle, flags);
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
static bool upload_bitmap(AL_BITMAP *bitmap, int x, int y, int w, int h)
{
   // FIXME: Some OpenGL drivers need power of two textures.
   AL_BITMAP_XDUMMY *xbitmap = (void *)bitmap;

   if (xbitmap->texture == 0)
      glGenTextures(1, &xbitmap->texture);
   glBindTexture(GL_TEXTURE_2D, xbitmap->texture);
   glTexImage2D(GL_TEXTURE_2D, 0, 4, bitmap->w, bitmap->h, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, bitmap->memory);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   xbitmap->left = 0;
   xbitmap->right = 1;
   xbitmap->top = 0;
   xbitmap->bottom = 1;

   return 1;
}

static AL_LOCKED_REGION *lock_region(AL_BITMAP *bitmap,
	int x, int y, int w, int h, AL_LOCKED_REGION *locked_region,
	int flags)
{
    // FIXME
    int pixelsize = _al_get_pixel_size(bitmap->format);
    int pitch = pixelsize * bitmap->w;
    locked_region->data = bitmap->memory + pitch * y + pixelsize * x;
    locked_region->format = bitmap->format;
    locked_region->pitch = pitch;
    return locked_region;
}

static void unlock_region(AL_BITMAP *bitmap)
{
    // FIXME
}

static void xdummy_destroy_bitmap(AL_BITMAP *bitmap)
{
   AL_BITMAP_XDUMMY *xbitmap = (void *)bitmap;
   if (xbitmap->texture) {
      glDeleteTextures(1, &xbitmap->texture);
      xbitmap->texture = 0;
   }
}

/* Obtain a reference to this driver. */
AL_BITMAP_INTERFACE *_al_bitmap_xdummy_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->draw_bitmap = draw_bitmap;
   vt->upload_bitmap = upload_bitmap;
   vt->destroy_bitmap = xdummy_destroy_bitmap;
   vt->draw_scaled_bitmap = draw_scaled_bitmap;
   vt->draw_bitmap_region = draw_bitmap_region;
   vt->draw_rotated_scaled_bitmap = draw_rotated_scaled_bitmap;
   vt->draw_rotated_bitmap = draw_rotated_bitmap;
   vt->lock_region = lock_region;
   vt->unlock_region = unlock_region;

   return vt;
}
