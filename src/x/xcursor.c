#include <X11/Xlib.h>
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_x.h"
#include "allegro5/internal/aintern_xcursor.h"
#include "allegro5/internal/aintern_xdisplay.h"
#include "allegro5/internal/aintern_xsystem.h"

#include <X11/cursorfont.h>

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#else
/* This requirement could be lifted for compatibility with older systems at the
 * expense of functionality, but it's probably not worthwhile.
 */
#error This file requires Xcursor.
#endif

ALLEGRO_MOUSE_CURSOR *_al_xwin_create_mouse_cursor(ALLEGRO_BITMAP *bmp,
   int x_focus, int y_focus)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   Display *xdisplay = system->x11display;

   int bmp_w;
   int bmp_h;
   ALLEGRO_MOUSE_CURSOR_XWIN *xcursor;
   XcursorImage *image;
   int c, ix, iy;
   bool was_locked;

   bmp_w = al_get_bitmap_width(bmp);
   bmp_h = al_get_bitmap_height(bmp);

   xcursor = al_malloc(sizeof *xcursor);
   if (!xcursor) {
      return NULL;
   }

   image = XcursorImageCreate(bmp->w, bmp->h);
   if (image == None) {
      al_free(xcursor);
      return NULL;
   }

   was_locked = al_is_bitmap_locked(bmp);
   if (!was_locked) {
      al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
   }

   c = 0;
   for (iy = 0; iy < bmp_h; iy++) {
      for (ix = 0; ix < bmp_w; ix++) {
         ALLEGRO_COLOR col;
         unsigned char r, g, b, a;

         col = al_get_pixel(bmp, ix, iy);
         al_unmap_rgba(col, &r, &g, &b, &a);
         image->pixels[c++] = (a<<24) | (r<<16) | (g<<8) | (b);
      }
   }

   if (!was_locked) {
      al_unlock_bitmap(bmp);
   }

   image->xhot = x_focus;
   image->yhot = y_focus;

   _al_mutex_lock(&system->lock);
   xcursor->cursor = XcursorImageLoadCursor(xdisplay, image);
   _al_mutex_unlock(&system->lock);

   XcursorImageDestroy(image);

   return (ALLEGRO_MOUSE_CURSOR *)xcursor;
}



void _al_xwin_destroy_mouse_cursor(ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_MOUSE_CURSOR_XWIN *xcursor = (ALLEGRO_MOUSE_CURSOR_XWIN *)cursor;
   ALLEGRO_SYSTEM *sys = al_get_system_driver();
   ALLEGRO_SYSTEM_XGLX *sysx = (ALLEGRO_SYSTEM_XGLX *)sys;
   unsigned i;

   _al_mutex_lock(&sysx->lock);

   for (i = 0; i < _al_vector_size(&sys->displays); i++) {
      ALLEGRO_DISPLAY_XGLX **slot = _al_vector_ref(&sys->displays, i);
      ALLEGRO_DISPLAY_XGLX *glx = *slot;

      if (glx->current_cursor == xcursor->cursor) {
         if (!glx->cursor_hidden)
            XUndefineCursor(sysx->x11display, glx->window);
         glx->current_cursor = None;
      }
   }

   XFreeCursor(sysx->x11display, xcursor->cursor);
   al_free(xcursor);

   _al_mutex_unlock(&sysx->lock);
}



static bool xdpy_set_mouse_cursor(ALLEGRO_DISPLAY *display,
   ALLEGRO_MOUSE_CURSOR *cursor)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   ALLEGRO_MOUSE_CURSOR_XWIN *xcursor = (ALLEGRO_MOUSE_CURSOR_XWIN *)cursor;
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
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
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_get_system_driver();
   Display *xdisplay = system->x11display;
   Window xwindow = glx->window;
   unsigned int cursor_shape;

   switch (cursor_id) {
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_DEFAULT:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW:
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_PROGRESS:
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
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_MOVE:
         cursor_shape = XC_fleur;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_N:
         cursor_shape = XC_top_side;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_S:
         cursor_shape = XC_bottom_side;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_E:
         cursor_shape = XC_right_side;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_W:
         cursor_shape = XC_left_side;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NE:
         cursor_shape = XC_top_right_corner;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SW:
         cursor_shape = XC_bottom_left_corner;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_NW:
         cursor_shape = XC_top_left_corner;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_RESIZE_SE:
         cursor_shape = XC_bottom_right_corner;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_PRECISION:
         cursor_shape = XC_crosshair;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_LINK:
         cursor_shape = XC_hand2;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_ALT_SELECT:
         cursor_shape = XC_hand1;
         break;
      case ALLEGRO_SYSTEM_MOUSE_CURSOR_UNAVAILABLE:
         cursor_shape = XC_X_cursor;
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
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
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
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_get_system_driver();
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



void _al_xwin_add_cursor_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->set_mouse_cursor = xdpy_set_mouse_cursor;
   vt->set_system_mouse_cursor = xdpy_set_system_mouse_cursor;
   vt->show_mouse_cursor = xdpy_show_mouse_cursor;
   vt->hide_mouse_cursor = xdpy_hide_mouse_cursor;
}


/* vim: set sts=3 sw=3 et: */
