/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * display driver.
 */

#include "xdummy.h"

static AL_DISPLAY_INTERFACE *vt;

/* Helper to set up GL state as we want it. */
static void setup_gl(AL_DISPLAY *d)
{
   glViewport(0, 0, d->w, d->h);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, d->w, d->h, 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

static void set_size_hints(AL_DISPLAY *d, int w, int h)
{
   if (!(d->flags & AL_RESIZABLE)) {
      AL_SYSTEM_XDUMMY *system = (void *)al_system_driver();
      AL_DISPLAY_XDUMMY *glx = (void *)d;
      XSizeHints *hints = XAllocSizeHints();;

      hints->flags = PMinSize | PMaxSize | PBaseSize;
      hints->min_width  = hints->max_width  = hints->base_width  = w;
      hints->min_height = hints->max_height = hints->base_height = h;
      XSetWMNormalHints(system->xdisplay, glx->window, hints);

      XFree(hints);
   }
}

/* Create a new X11 dummy display, which maps directly to a GLX window. */
static AL_DISPLAY *create_display(int w, int h)
{
   AL_DISPLAY_XDUMMY *d = _AL_MALLOC(sizeof *d);
   memset(d, 0, sizeof *d);

   AL_SYSTEM_XDUMMY *system = (void *)al_system_driver();

   _al_mutex_lock(&system->lock);

   d->display.w = w;
   d->display.h = h;
   d->display.vt = vt;
   //FIXME
   //d->display.format = al_get_new_display_format();
   d->display.format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;

   d->display.refresh_rate = al_get_new_display_refresh_rate();
   d->display.flags = al_get_new_display_flags();

   if (d->display.flags | AL_FULLSCREEN)
      _al_xdummy_fullscreen_set_mode(system, w, h, 0, 0);

   //FIXME
   d->display.flags |= AL_WINDOWED;

   d->backbuffer = _al_xdummy_create_bitmap(&d->display, w, h);
   AL_BITMAP_XDUMMY *backbuffer = (void *)d->backbuffer;
   backbuffer->is_backbuffer = 1;
   /* Create a memory cache for the whole screen. */
   //TODO: Maybe we should do this lazily and defer to lock_bitmap_region
   if (!d->backbuffer->memory) {
      int n = w * h * al_get_pixel_size(d->backbuffer->format);
      d->backbuffer->memory = _AL_MALLOC(n);
      memset(d->backbuffer->memory, 0, n);
   }

   /* Add ourself to the list of displays. */
   AL_DISPLAY_XDUMMY **add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&d->display.es);

   /* Let GLX choose an appropriate visual. */
   int nelements;
   GLXFBConfig *fbc = glXChooseFBConfig(system->xdisplay,
      DefaultScreen(system->xdisplay), 0, &nelements);
   XVisualInfo *vi = glXGetVisualFromFBConfig(system->xdisplay, fbc[0]);

   TRACE("xdisplay: Selected visual %lu.\n", vi->visualid);

   /* Create a colormap. */
   Colormap cmap = XCreateColormap(system->xdisplay,
   RootWindow(system->xdisplay, vi->screen), vi->visual, AllocNone);

   /* Create an X11 window */
   XSetWindowAttributes swa;
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

   d->window = XCreateWindow(system->xdisplay, RootWindow(
      system->xdisplay, vi->screen), 0, 0, w, h, 0, vi->depth, InputOutput,
      vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

   TRACE("xdisplay: X11 window created.\n");
   
   set_size_hints(&d->display, w, h);

   d->wm_delete_window_atom = XInternAtom (system->xdisplay,
      "WM_DELETE_WINDOW", False);
   XSetWMProtocols (system->xdisplay, d->window, &d->wm_delete_window_atom, 1);

   XMapWindow(system->xdisplay, d->window);
   TRACE("xdisplay: X11 window mapped.\n");

   /* Send the pending request to the X server. */
   XSync(system->xdisplay, False);
   /* To avoid race conditions where some X11 functions fail before the window
    * is mapped, we wait here until it is mapped. Note that the thread is
    * locked, so the event could not possibly have processed yet in the
    * events thread. So as long as no other map events occur, the condition
    * should only be signalled when our window gets mapped.
    */
   _al_cond_wait(&system->mapped, &system->lock);

   /* Create a GLX subwindow inside our window. */
   d->glxwindow = glXCreateWindow(system->xdisplay, fbc[0], d->window, 0);

   TRACE("xdisplay: GLX window created.\n");

   if (d->display.flags | AL_FULLSCREEN) {
      int x, y;
      Window child;
      XTranslateCoordinates(system->xdisplay, d->window,
         RootWindow(system->xdisplay, 0), 0, 0, &x, &y, &child);
      _al_xdummy_fullscreen_set_origin(system, x, y);
   }

   /* Create a GLX context. */
   d->context = glXCreateNewContext(system->xdisplay, fbc[0], GLX_RGBA_TYPE,
      NULL, True);

   TRACE("xdisplay: Got GLX context.\n");

   _al_mutex_unlock(&system->lock);

   return &d->display;
}

static void destroy_display(AL_DISPLAY *d)
{
   int i;
   AL_SYSTEM_XDUMMY *s = (void *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (void *)d;
   _al_mutex_lock(&s->lock);
   for (i = 0; i < s->system.displays._size; i++) {
      AL_DISPLAY_XDUMMY **dptr = _al_vector_ref(&s->system.displays, i);
      if (glx == *dptr) {
         _al_vector_delete_at(&s->system.displays, i);
         break;
      }
   }
   XDestroyWindow(s->xdisplay, glx->window);

   if (d->flags | AL_FULLSCREEN)
      _al_xdummy_restore_video_mode(s);

   // FIXME: deallocate ourselves?

   _al_mutex_unlock(&s->lock);
}

static void set_current_display(AL_DISPLAY *d)
{
   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;
   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   glXMakeContextCurrent(system->xdisplay, glx->glxwindow, glx->glxwindow,
      glx->context);

   if (!glx->opengl_initialized) {
      setup_gl(d);
   }

   TRACE("xdisplay: GLX context made current.\n");
}

/* Dummy implementation of flip. */
static void flip_display(AL_DISPLAY *d)
{
   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;
   glFlush();
   glXSwapBuffers(system->xdisplay, glx->glxwindow);
}

static bool acknowledge_resize(AL_DISPLAY *d)
{
   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;

   unsigned int w, h;
   glXQueryDrawable(system->xdisplay, glx->glxwindow, GLX_WIDTH, &w);
   glXQueryDrawable(system->xdisplay, glx->glxwindow, GLX_HEIGHT, &h);

   d->w = w;
   d->h = h;

   setup_gl(d);

   return true;
}

static bool resize_display(AL_DISPLAY *d, int w, int h)
{
   AL_SYSTEM_XDUMMY *s = (AL_SYSTEM_XDUMMY *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;
   set_size_hints(d, w, h);
   XResizeWindow(s->xdisplay, glx->window, w, h);
   XSync(s->xdisplay, False);
   return true;
}

/* Handle an X11 configure event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
void _al_display_xdummy_configure(AL_DISPLAY *d, XEvent *xevent)
{
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;

   AL_EVENT_SOURCE *es = &glx->display.es;
   _al_event_source_lock(es);

   /* Generate a resize event if the size has changed. We cannot asynchronously
    * change the display size here yet, since the user will only know about a
    * changed size after receiving the resize event. Here we merely add the
    * event to the queue.
    */
   if (d->w != xevent->xconfigure.width ||
      d->h != xevent->xconfigure.height) {
      if (_al_event_source_needs_to_generate_event(es)) {
         AL_EVENT *event = _al_event_source_get_unused_event(es);
         if (event) {
            event->display.type = AL_EVENT_DISPLAY_RESIZE;
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
      AL_EVENT *event = _al_event_source_get_unused_event(es);
      if (event) {
         event->display.type = AL_EVENT_DISPLAY_EXPOSE;
         event->display.timestamp = al_current_time();
         event->display.x = xevent->xconfigure.x;
         event->display.y = xevent->xconfigure.y;
         event->display.width = xevent->xconfigure.width;
         event->display.height = xevent->xconfigure.height;
         _al_event_source_emit_event(es, event);
      }
   }
   _al_event_source_unlock(es);
}

/* Handle an X11 close button event. [X11 thread]
 * Only called from the event handler with the system locked.
 */
void _al_display_xdummy_closebutton(AL_DISPLAY *d, XEvent *xevent)
{
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;

   AL_EVENT_SOURCE *es = &glx->display.es;
   _al_event_source_lock(es);

   if (_al_event_source_needs_to_generate_event(es)) {
      AL_EVENT *event = _al_event_source_get_unused_event(es);
      if (event) {
         event->display.type = AL_EVENT_DISPLAY_CLOSE;
         event->display.timestamp = al_current_time();
         _al_event_source_emit_event(es, event);
      }
   }
   _al_event_source_unlock(es);
}

/* Dummy implementation. */
// FIXME: think about moving this to xbitmap.c instead..
AL_BITMAP *_al_xdummy_create_bitmap(AL_DISPLAY *d, int w, int h)
{
   int format = al_get_new_bitmap_format();
   int flags = al_get_new_bitmap_flags();

   //FIXME
   format = d->format;

   AL_BITMAP_XDUMMY *bitmap = _AL_MALLOC(sizeof *bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->bitmap.vt = _al_bitmap_xdummy_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;

   return &bitmap->bitmap;
}

static AL_BITMAP *get_backbuffer(AL_DISPLAY *d)
{
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;
   return glx->backbuffer;
}

static AL_BITMAP *get_frontbuffer(AL_DISPLAY *d)
{
   return NULL;
}

static void set_target_bitmap(AL_DISPLAY *display, AL_BITMAP *bitmap)
{
   //FIXME: change to offscreen targets and so on
}

static bool is_compatible_bitmap(AL_DISPLAY *display, AL_BITMAP *bitmap)
{
   return true;
}

/* Obtain a reference to this driver. */
AL_DISPLAY_INTERFACE *_al_display_xdummy_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = create_display;
   vt->destroy_display = destroy_display;
   vt->set_current_display = set_current_display;
   vt->flip_display = flip_display;
   vt->acknowledge_resize = acknowledge_resize;
   vt->create_bitmap = _al_xdummy_create_bitmap;
   vt->get_backbuffer = get_backbuffer;
   vt->get_frontbuffer = get_backbuffer;
   vt->set_target_bitmap = set_target_bitmap;
   vt->is_compatible_bitmap = is_compatible_bitmap;
   vt->resize_display = resize_display;
   _xdummy_add_drawing_functions(vt);

   return vt;
}
