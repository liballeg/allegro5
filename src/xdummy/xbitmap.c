/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * bitmap driver.
 */

#include "xdummy.h"

static AL_BITMAP_INTERFACE *vt;

/* Draw the bitmap at the specified position. */
static void draw_bitmap(AL_BITMAP *bitmap, float x, float y, int flags)
{
   /* C-Hack to get polymorphism, we know the AL_BITMAP has to be an
    * AL_BITMAP_XDUMMY really. */
   // FIXME: Need to add an id field to the vtable and assert that it actually
   // has the right id.
   GLboolean on;
   glGetBooleanv(GL_TEXTURE_2D, &on);
   if (!on)
      glEnable(GL_TEXTURE_2D);
   AL_BITMAP_XDUMMY *xbitmap = (void *)bitmap;
   float l = xbitmap->left;
   float t = xbitmap->top;
   float r = xbitmap->right;
   float b = xbitmap->bottom;
   float w = bitmap->w;
   float h = bitmap->h;
   glBindTexture(GL_TEXTURE_2D, xbitmap->texture);
   glColor4f(1, 1, 1, 1);
   glBegin(GL_QUADS);
   glTexCoord2f(l, t);
   glVertex2f(x, y);
   glTexCoord2f(r, t);
   glVertex2f(x + w, y);
   glTexCoord2f(r, b);
   glVertex2f(x + w, y + h);
   glTexCoord2f(l, b);
   glVertex2f(x, y + h);
   glEnd();
   if (!on)
      glDisable(GL_TEXTURE_2D);
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
static void upload_bitmap(AL_BITMAP *bitmap, int x, int y, int w, int h)
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

   return vt;
}
