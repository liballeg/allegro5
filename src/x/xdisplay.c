#include "xglx.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_opengl.h"

#include <X11/cursorfont.h>



static ALLEGRO_DISPLAY_INTERFACE *xdpy_vt;



/* Helper to set up GL state as we want it. */
static void setup_gl(ALLEGRO_DISPLAY *d)
{
   glViewport(0, 0, d->w, d->h);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, d->w, d->h, 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}



static void set_size_hints(ALLEGRO_DISPLAY *d, int w, int h)
{
   if (!(d->flags & ALLEGRO_RESIZABLE)) {
      ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
      ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
      XSizeHints *hints = XAllocSizeHints();;

      hints->flags = PMinSize | PMaxSize | PBaseSize;
      hints->min_width  = hints->max_width  = hints->base_width  = w;
      hints->min_height = hints->max_height = hints->base_height = h;
      XSetWMNormalHints(system->x11display, glx->window, hints);

      XFree(hints);
   }
}



/* Helper to set a window icon. */
static void xdpy_set_icon(ALLEGRO_DISPLAY *d, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *display = (void *)d;
   XWMHints wm_hints;
   int x, y;

   _al_mutex_lock(&system->lock);

   int w = al_get_bitmap_width(bitmap);
   int h = al_get_bitmap_height(bitmap);

   XWindowAttributes attributes;
   XGetWindowAttributes(system->x11display, display->window,
      &attributes);

   // FIXME: Do we need to check for other depths? Just 32 now..
   XImage *image = XCreateImage(system->x11display, attributes.visual,
      attributes.depth, ZPixmap, 0, NULL, w, h, 32, 0);
   // FIXME: Must check for errors
   // TODO: Is this really freed by XDestroyImage?
   image->data = _AL_MALLOC_ATOMIC(image->bytes_per_line * h);

   // FIXME: Do this properly.
   ALLEGRO_LOCKED_REGION lr;
   if (al_lock_bitmap(bitmap, &lr, ALLEGRO_LOCK_READONLY)) {
      const char *src;
      char *dst;
      int i;

      src = lr.data;
      dst = image->data;
      for (i = 0; i < h; i++) {
         memcpy(dst, src, w * 4);
         src += lr.pitch;
         dst += w * 4;
      }

      al_unlock_bitmap(bitmap);
   }
   else {
       /* XXX what should we do here? */
   }

   display->icon = XCreatePixmap(system->x11display, display->window,
      bitmap->w, bitmap->h, attributes.depth);

   GC gc = XCreateGC(system->x11display, display->icon, 0, NULL);
   XPutImage(system->x11display, display->icon, gc, image, 0, 0, 0, 0, w, h);
   XFreeGC(system->x11display, gc);
   XDestroyImage(image);

   wm_hints.flags = IconPixmapHint | IconMaskHint;
   wm_hints.icon_pixmap = display->icon;
   // FIXME: Does X11 support apha values? In any case, we need a separate
   // mask here!
   wm_hints.icon_mask = display->icon;
   XSetWMHints(system->x11display, display->window, &wm_hints);
   // FIXME: Do we have to destroy the icon pixmap, or is it owned by X11 now?

   XFlush(system->x11display);

   _al_mutex_unlock(&system->lock);
}



static void xdpy_set_window_position(ALLEGRO_DISPLAY *display, int x, int y)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   Window root, parent, child, *children;
   unsigned int n;

   _al_mutex_lock(&system->lock);

   /* To account for the window border, we have to find the parent window which
    * draws the border. If the parent is the root though, then we should not
    * translate.
    */
   XQueryTree(system->x11display, glx->window, &root, &parent, &children, &n);
   if (parent != root) {
      XTranslateCoordinates(system->x11display, parent, glx->window,
         x, y, &x, &y, &child);
   }

   XMoveWindow(system->x11display, glx->window, x, y);
   XFlush(system->x11display);
   _al_mutex_unlock(&system->lock);
}



static void xdpy_toggle_frame(ALLEGRO_DISPLAY *display, bool onoff)
{
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   Display *x11 = system->x11display;
   Atom hints;

   _al_mutex_lock(&system->lock);

#if 1
   /* This code is taken from the GDK sources. So it works perfectly in Gnome,
    * no idea if it will work anywhere else. X11 documentation itself only
    * describes a way how to make the window completely unmanaged, but that
    * would also require special care in the event handler.
    */
   hints = XInternAtom(x11, "_MOTIF_WM_HINTS", True);
   if (hints) { 
      struct {
         unsigned long flags;
         unsigned long functions;
         unsigned long decorations;
         long input_mode;
         unsigned long status;
      } motif = {2, 0, onoff, 0, 0};
      XChangeProperty(x11, glx->window, hints, hints, 32, PropModeReplace,
         (void *)&motif, sizeof motif / 4);
   }
#endif

   _al_mutex_unlock(&system->lock);
}



/* Create a new X11 dummy display, which maps directly to a GLX window. */
static ALLEGRO_DISPLAY *xdpy_create_display(int w, int h)
{
   ALLEGRO_DISPLAY_XGLX *d = _AL_MALLOC(sizeof *d);
   ALLEGRO_DISPLAY *display = (void*)d;
   ALLEGRO_OGL_EXTRAS *ogl = _AL_MALLOC(sizeof *ogl);
   memset(d, 0, sizeof *d);
   memset(ogl, 0, sizeof *ogl);
   display->ogl_extras = ogl;
   
   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();

   _al_mutex_lock(&system->lock);

   int major, minor;
   glXQueryVersion(system->x11display, &major, &minor);
   d->glx_version = major * 100 + minor * 10;
   TRACE("xdisplay: GLX %.1f.\n", d->glx_version / 100.f);

   display->w = w;
   display->h = h;
   display->vt = xdpy_vt;
   //FIXME
   //display->format = al_get_new_display_format();
   display->format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;

   display->refresh_rate = al_get_new_display_refresh_rate();
   display->flags = al_get_new_display_flags();

   // FIXME: default? Is this the right place to set this?
   display->flags |= ALLEGRO_OPENGL;

   // TODO: What is this?
   d->xscreen = DefaultScreen(system->x11display);

   if (display->flags & ALLEGRO_FULLSCREEN)
      _al_xglx_fullscreen_set_mode(system, w, h, 0, 0);

   //FIXME
   //d->display.flags |= ALLEGRO_WINDOWED;

   /* Add ourself to the list of displays. */
   ALLEGRO_DISPLAY_XGLX **add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   _al_xglx_config_select_visual(d);

   TRACE("xdisplay: Selected visual %lx.\n", d->xvinfo->visualid);

   /* Create a colormap. */
   Colormap cmap = XCreateColormap(system->x11display,
      RootWindow(system->x11display, d->xvinfo->screen),
      d->xvinfo->visual, AllocNone);

   /* Create an X11 window */
   XSetWindowAttributes swa;
   int mask = CWBorderPixel | CWColormap | CWEventMask;
   swa.colormap = cmap;
   swa.border_pixel = 0;
   swa.event_mask =
      KeyPressMask |
      KeyReleaseMask |
      StructureNotifyMask |
      EnterWindowMask |
      LeaveWindowMask |
      FocusChangeMask |
      ExposureMask |
      PropertyChangeMask |
      ButtonPressMask |
      ButtonReleaseMask |
      PointerMotionMask;

   d->window = XCreateWindow(system->x11display, RootWindow(
      system->x11display, d->xvinfo->screen), 0, 0, w, h, 0, d->xvinfo->depth,
      InputOutput, d->xvinfo->visual, mask, &swa);
   
   if (display->flags & ALLEGRO_NOFRAME)
      xdpy_toggle_frame(display, false);

   TRACE("xdisplay: X11 window created.\n");

   set_size_hints(display, w, h);

   d->wm_delete_window_atom = XInternAtom (system->x11display,
      "WM_DELETE_WINDOW", False);
   XSetWMProtocols(system->x11display, d->window, &d->wm_delete_window_atom, 1);

   XMapWindow(system->x11display, d->window);
   TRACE("xdisplay: X11 window mapped.\n");

   /* Send the pending request to the X server. */
   XSync(system->x11display, False);
   /* To avoid race conditions where some X11 functions fail before the window
    * is mapped, we wait here until it is mapped. Note that the thread is
    * locked, so the event could not possibly have been processed yet in the
    * events thread. So as long as no other map events occur, the condition
    * should only be signalled when our window gets mapped.
    */
   _al_cond_wait(&system->mapped, &system->lock);

   _al_xglx_config_create_context(d);

   if (display->flags & ALLEGRO_FULLSCREEN) {
      _al_xglx_fullscreen_to_display(system, d);
   }

   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   if (d->fbc) {
      glXMakeContextCurrent(system->gfxdisplay, d->glxwindow, d->glxwindow,
         d->context);
   }
   else {
      glXMakeCurrent(system->gfxdisplay, d->glxwindow, d->context);
   }

   _al_ogl_manage_extensions(display);
   _al_ogl_set_extensions(ogl->extension_api);

   setup_gl(display);

   ogl->backbuffer = _al_ogl_create_backbuffer(display);

   d->invisible_cursor = None;
   d->current_cursor = XC_left_ptr;
   d->cursor_hidden = false;

   d->icon = None;
   d->icon_mask = None;

   _al_mutex_unlock(&system->lock);

   return display;
}



static void xdpy_destroy_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *s = (void *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

   /* If we're the last display, convert all bitmpas to display independent
    * (memory) bitmaps. */
   if (s->system.displays._size == 1) {
      while (d->bitmaps._size > 0) {
         ALLEGRO_BITMAP **bptr = _al_vector_ref_back(&d->bitmaps);
         ALLEGRO_BITMAP *b = *bptr;
         _al_convert_to_memory_bitmap(b);
      }
   }
   else {
      /* Pass all bitmaps to any other living display. (We assume all displays
       * are compatible.) */
      size_t i;
      ALLEGRO_DISPLAY **living = NULL;
      ASSERT(s->system.displays._size > 1);

      for (i = 0; i < s->system.displays._size; i++) {
         living = _al_vector_ref(&s->system.displays, i);
         if (*living != d)
            break;
      }

      for (i = 0; i < d->bitmaps._size; i++) {
         ALLEGRO_BITMAP **add = _al_vector_alloc_back(&(*living)->bitmaps);
         ALLEGRO_BITMAP **ref = _al_vector_ref(&d->bitmaps, i);
         *add = *ref;
         (*add)->display = *living;
      }
   }

   _al_ogl_unmanage_extensions(d);

   _al_mutex_lock(&s->lock);
   _al_vector_find_and_delete(&s->system.displays, &d);
   XDestroyWindow(s->x11display, glx->window);

   if (d->flags & ALLEGRO_FULLSCREEN)
      _al_xglx_restore_video_mode(s);

   if (ogl->backbuffer) {
      _al_ogl_destroy_backbuffer(ogl->backbuffer);
      ogl->backbuffer = NULL;
   }

   _al_vector_free(&d->bitmaps);
   _al_event_source_free(&d->es);

   _AL_FREE(d->ogl_extras);
   _AL_FREE(d);

   _al_mutex_unlock(&s->lock);
}



static bool xdpy_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   ALLEGRO_OGL_EXTRAS *ogl = d->ogl_extras;

   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   if (glx->fbc) {
      if (!glXMakeContextCurrent(system->gfxdisplay, glx->glxwindow,
                                glx->glxwindow, glx->context))
         return false;
   }
   else {
      if (!glXMakeCurrent(system->gfxdisplay, glx->glxwindow, glx->context))
         return false;
   }

   _al_ogl_set_extensions(ogl->extension_api);

   return true;
}



/* Dummy implementation of flip. */
static void xdpy_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   glFlush();
   glXSwapBuffers(system->gfxdisplay, glx->glxwindow);
}



static bool xdpy_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   XWindowAttributes xwa;
   unsigned int w, h;
   
   _al_mutex_lock(&system->lock);

   /* glXQueryDrawable is GLX 1.3+. */
   /*
   glXQueryDrawable(system->x11display, glx->glxwindow, GLX_WIDTH, &w);
   glXQueryDrawable(system->x11display, glx->glxwindow, GLX_HEIGHT, &h);
   */

   XGetWindowAttributes(system->x11display, glx->window, &xwa);
   w = xwa.width;
   h = xwa.height;

   d->w = w;
   d->h = h;

   setup_gl(d);
   
   _al_mutex_unlock(&system->lock);

   return true;
}



static bool xdpy_resize_display(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   _al_mutex_lock(&system->lock);

   set_size_hints(d, w, h);
   XResizeWindow(system->x11display, glx->window, w, h);
   XSync(system->x11display, False);

   /* Wait until we are actually resized. There might be a better way.. */
   _al_cond_wait(&system->resized, &system->lock);

   /* The user can expect the display to already be resized when
    * xdpy_resize_display returns.
    * TODO: Right now, we still generate a resize event - maybe we should not.
    */
   xdpy_acknowledge_resize(d);

   if (d->flags & ALLEGRO_FULLSCREEN) {
      _al_xglx_fullscreen_set_mode(system, w, h, 0, 0);
      _al_xglx_fullscreen_to_display(system, glx);
   }

   _al_mutex_unlock(&system->lock);
   return true;
}



/* Handle an X11 configure event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
void _al_display_xglx_configure(ALLEGRO_DISPLAY *d, XEvent *xevent)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   ALLEGRO_EVENT_SOURCE *es = &glx->display.es;
   _al_event_source_lock(es);

   /* Generate a resize event if the size has changed. We cannot asynchronously
    * change the display size here yet, since the user will only know about a
    * changed size after receiving the resize event. Here we merely add the
    * event to the queue.
    */
   if (d->w != xevent->xconfigure.width ||
      d->h != xevent->xconfigure.height) {
      if (_al_event_source_needs_to_generate_event(es)) {
         ALLEGRO_EVENT *event = _al_event_source_get_unused_event(es);
         if (event) {
            event->display.type = ALLEGRO_EVENT_DISPLAY_RESIZE;
            event->display.timestamp = al_current_time();
            event->display.x = xevent->xconfigure.x;
            event->display.y = xevent->xconfigure.y;
            event->display.width = xevent->xconfigure.width;
            event->display.height = xevent->xconfigure.height;
            _al_event_source_emit_event(es, event);
         }
      }
   }

   /* Generate an expose event. */
   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT *event = _al_event_source_get_unused_event(es);
      if (event) {
         event->display.type = ALLEGRO_EVENT_DISPLAY_EXPOSE;
         event->display.timestamp = al_current_time();
         event->display.x = xevent->xconfigure.x;
         event->display.y = xevent->xconfigure.y;
         event->display.width = xevent->xconfigure.width;
         event->display.height = xevent->xconfigure.height;
         _al_event_source_emit_event(es, event);
      }
   }

   /* This ignores the event when changing the border (which has bogus
    * coordinates).
    */
   if (xevent->xconfigure.send_event) {
      glx->x = xevent->xconfigure.x;
      glx->y = xevent->xconfigure.y;
   }

   _al_event_source_unlock(es);
}



/* Handle an X11 close button event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
void _al_display_xglx_closebutton(ALLEGRO_DISPLAY *d, XEvent *xevent)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   ALLEGRO_EVENT_SOURCE *es = &d->es;
   _al_event_source_lock(es);

   if (_al_event_source_needs_to_generate_event(es)) {
      ALLEGRO_EVENT *event = _al_event_source_get_unused_event(es);
      if (event) {
         event->display.type = ALLEGRO_EVENT_DISPLAY_CLOSE;
         event->display.timestamp = al_current_time();
         _al_event_source_emit_event(es, event);
      }
   }
   _al_event_source_unlock(es);
}



static bool xdpy_is_compatible_bitmap(ALLEGRO_DISPLAY *display,
   ALLEGRO_BITMAP *bitmap)
{
   /* All GLX bitmaps are compatible. */
   return true;
}



static void xdpy_get_window_position(ALLEGRO_DISPLAY *display, int *x, int *y)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   /* We could also query the X11 server, but it just would take longer, and
    * would not be synchronized to our events. The latter can be an advantage
    * or disadvantage.
    */
   *x = glx->x;
   *y = glx->y;
}



static void xdpy_set_window_title(
   ALLEGRO_DISPLAY *display, AL_CONST char *title)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;

   _al_mutex_lock(&system->lock);
   XStoreName(system->x11display, glx->window, title);
   _al_mutex_unlock(&system->lock);
}



/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_xglx_driver(void)
{
   if (xdpy_vt)
      return xdpy_vt;

   xdpy_vt = _AL_MALLOC(sizeof *xdpy_vt);
   memset(xdpy_vt, 0, sizeof *xdpy_vt);

   xdpy_vt->create_display = xdpy_create_display;
   xdpy_vt->destroy_display = xdpy_destroy_display;
   xdpy_vt->set_current_display = xdpy_set_current_display;
   xdpy_vt->flip_display = xdpy_flip_display;
   xdpy_vt->acknowledge_resize = xdpy_acknowledge_resize;
   xdpy_vt->create_bitmap = _al_ogl_create_bitmap;
   xdpy_vt->create_sub_bitmap = _al_ogl_create_sub_bitmap;
   xdpy_vt->get_backbuffer = _al_ogl_get_backbuffer;
   xdpy_vt->get_frontbuffer = _al_ogl_get_backbuffer;
   xdpy_vt->set_target_bitmap = _al_ogl_set_target_bitmap;
   xdpy_vt->is_compatible_bitmap = xdpy_is_compatible_bitmap;
   xdpy_vt->resize_display = xdpy_resize_display;
   xdpy_vt->set_icon = xdpy_set_icon;
   xdpy_vt->set_window_title = xdpy_set_window_title;
   xdpy_vt->set_window_position = xdpy_set_window_position;
   xdpy_vt->get_window_position = xdpy_get_window_position;
   xdpy_vt->toggle_frame = xdpy_toggle_frame;
   
   _al_xglx_add_cursor_functions(xdpy_vt);
   _al_ogl_add_drawing_functions(xdpy_vt);

   return xdpy_vt;
}

/* vi: set sts=3 sw=3 et: */
