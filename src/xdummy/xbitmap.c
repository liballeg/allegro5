/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * bitmap driver.
 */

#include <GL/gl.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "system_new.h"
#include "internal/aintern.h"
#include "platform/aintunix.h"
#include "internal/aintern2.h"
#include "internal/system_new.h"
#include "internal/display_new.h"
#include "internal/bitmap_new.h"

#include "xdummy.h"

static AL_BITMAP_INTERFACE *vt;

/* Draw the bitmap at the specified position. */
static void draw_bitmap(AL_BITMAP *bitmap, float x, float y)
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
static void upload_bitmap(AL_BITMAP *bitmap)
{
   int w = bitmap->w;
   int h = bitmap->h;
   
   // Some OpenGL drivers (like mine) need power of two textures.
   int tw = pot(w);
   int th = pot(h);
   AL_BITMAP_XDUMMY *xbitmap = (void *)bitmap;
   AL_BITMAP *prepare = al_create_memory_bitmap(tw, th, 0);
   
   _al_blit_memory_bitmap(bitmap, prepare, 0, 0, 0, 0, w, h);
   // HACK: To avoid seams, double last pixel. This is no issue when not using
   // power of two textures - any maybe there's a texture mode or something to
   // do the same? AllegroGL scales textures, but that looks really ugly.
   _al_blit_memory_bitmap(bitmap, prepare, w, 0, w, 0, 1, h + 1);
   _al_blit_memory_bitmap(bitmap, prepare, 0, h, 0, h, w, 1);

   if (xbitmap->texture == 0)
      glGenTextures(1, &xbitmap->texture);
   glBindTexture(GL_TEXTURE_2D, xbitmap->texture);
   glTexImage2D(GL_TEXTURE_2D, 0, 4, tw, th, 0, GL_RGBA,
      GL_UNSIGNED_BYTE, prepare->memory);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   al_destroy_bitmap(prepare);

   xbitmap->left = 0;
   xbitmap->right = (float)w / tw;
   xbitmap->top = 0;
   xbitmap->bottom = (float)h / th;
}

static void download_bitmap(AL_BITMAP *bitmap)
{
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
   vt->download_bitmap = download_bitmap;
   vt->destroy_bitmap = xdummy_destroy_bitmap;

   return vt;
}
