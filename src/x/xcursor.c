#include "xglx.h"

#include <X11/cursorfont.h>

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#else
/* This requirement could be lifted for compatibility with older systems at the
 * expense of functionality, but it's probably not worthwhile.
 */
#error This file requires Xcursor.
#endif

static ALLEGRO_MOUSE_CURSOR *xdpy_create_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bmp, int x_focus, int y_focus)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   int bmp_w;
   int bmp_h;
   ALLEGRO_LOCKED_REGION lr;
   ALLEGRO_MOUSE_CURSOR_XGLX *xcursor;
   XcursorImage *image;
   int c, ix, iy;

   bmp_w = al_get_bitmap_width(bmp);
   bmp_h = al_get_bitmap_height(bmp);
   if (!al_lock_bitmap(bmp, &lr, ALLEGRO_LOCK_READONLY)) {
      return NULL;
   }

   xcursor = _AL_MALLOC(sizeof *xcursor);
   if (!xcursor) {
      return NULL;
   }

   image = XcursorImageCreate(bmp->w, bmp->h);
   if (image == None) {
      _AL_FREE(xcursor);
      return NULL;
   }

   c = 0;
   for (iy = 0; iy < bmp_h; iy++) {
      for (ix = 0; ix < bmp_w; ix++) {
         ALLEGRO_COLOR col;
         unsigned char r, g, b, a;

         col = al_get_pixel(bmp, ix, iy);
         al_unmap_rgb(col, &r, &g, &b);
         a = 255;
         image->pixels[c++] = (a<<24) | (r<<16) | (g<<8) | (b);
      }
   }

   image->xhot = x_focus;
   image->yhot = y_focus;

   _al_mutex_lock(&system->lock);
   xcursor->cursor = XcursorImageLoadCursor(xdisplay, image);
   _al_mutex_unlock(&system->lock);

   XcursorImageDestroy(image);

   al_unlock_bitmap(bmp);

   return (ALLEGRO_MOUSE_CURSOR *)xcursor;
}



static void xdpy_destroy_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   ALLEGRO_MOUSE_CURSOR_XGLX *xcursor = (ALLEGRO_MOUSE_CURSOR_XGLX *)cursor;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   _al_mutex_lock(&system->lock);

   if (glx->current_cursor == xcursor->cursor) {
      XUndefineCursor(xdisplay, xwindow);
      glx->current_cursor = None;
   }

   XFreeCursor(xdisplay, xcursor->cursor);
   _AL_FREE(xcursor);

   _al_mutex_unlock(&system->lock);
}



static bool xdpy_set_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   ALLEGRO_MOUSE_CURSOR_XGLX *xcursor = (ALLEGRO_MOUSE_CURSOR_XGLX *)cursor;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   glx->current_cursor = xcursor->cursor;

   if (!glx->cursor_hidden) {
      _al_mutex_lock(&system->lock);
      XDefineCursor(xdisplay, xwindow, glx->current_cursor);
      _al_mutex_unlock(&system->lock);
   }

   return true;
}



static bool xdpy_set_system_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;
   unsigned int cursor_shape;

   switch (cursor_id) {
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
         cursor_shape = XC_left_ptr;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY:
         cursor_shape = XC_watch;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION:
         cursor_shape = XC_question_arrow;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT:
         cursor_shape = XC_xterm;
         break;
      default:
         return false;
   }

   _al_mutex_lock(&system->lock);

   glx->current_cursor = XCreateFontCursor(xdisplay, cursor_shape);
   /* XXX: leak? */

   if (!glx->cursor_hidden) {
      XDefineCursor(xdisplay, xwindow, glx->current_cursor);
   }

   _al_mutex_unlock(&system->lock);

   return true;
}



/* Show the system mouse cursor. */
static bool xdpy_show_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_XGLX *glx = (void *)display;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   if (!glx->cursor_hidden)
      return true;

   _al_mutex_lock(&system->lock);
   XDefineCursor(xdisplay, xwindow, glx->current_cursor);
   glx->cursor_hidden = false;
   _al_mutex_unlock(&system->lock);
   return true;
}



/* Hide the system mouse cursor. */
static bool xdpy_hide_mouse_cursor(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_XGLX *glx = (void *)display;
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;

   if (glx->cursor_hidden)
      return true;

   _al_mutex_lock(&system->lock);

   if (glx->invisible_cursor == None) {
      unsigned long gcmask;
      XGCValues gcvalues;

      Pixmap pixmap = XCreatePixmap(xdisplay, xwindow, 1, 1, 1);

      GC temp_gc;
      XColor color;

      gcmask = GCFunction | GCForeground | GCBackground;
      gcvalues.function = GXcopy;
      gcvalues.foreground = 0;
      gcvalues.background = 0;
      temp_gc = XCreateGC(xdisplay, pixmap, gcmask, &gcvalues);
      XDrawPoint(xdisplay, pixmap, temp_gc, 0, 0);
      XFreeGC(xdisplay, temp_gc);
      color.pixel = 0;
      color.red = color.green = color.blue = 0;
      color.flags = DoRed | DoGreen | DoBlue;
      glx->invisible_cursor = XCreatePixmapCursor(xdisplay, pixmap,
         pixmap, &color, &color, 0, 0);
      XFreePixmap(xdisplay, pixmap);
   }

   XDefineCursor(xdisplay, xwindow, glx->invisible_cursor);
   glx->cursor_hidden = true;

   _al_mutex_unlock(&system->lock);

   return true;
}



void _al_xglx_add_cursor_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->create_mouse_cursor = xdpy_create_mouse_cursor;
   vt->destroy_mouse_cursor = xdpy_destroy_mouse_cursor;
   vt->set_mouse_cursor = xdpy_set_mouse_cursor;
   vt->set_system_mouse_cursor = xdpy_set_system_mouse_cursor;
   vt->show_mouse_cursor = xdpy_show_mouse_cursor;
   vt->hide_mouse_cursor = xdpy_hide_mouse_cursor;
}
