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

/* Create a new X11 dummy display, which maps directly to a GLX window. */
static AL_DISPLAY *create_display(int w, int h)
{
   AL_DISPLAY_XDUMMY *d = _AL_MALLOC(sizeof *d);
   memset(d, 0, sizeof *d);

   d->display.w = w;
   d->display.h = h;
   d->display.vt = vt;
   //FIXME
   //d->display.format = al_get_new_display_format();
   d->display.format = ALLEGRO_PIXEL_FORMAT_RGBA_8888,

   d->display.refresh_rate = al_get_new_display_refresh_rate();
   d->display.flags = al_get_new_display_flags();

   d->backbuffer = xdummy_create_bitmap(&d->display, w, h);
   AL_BITMAP_XDUMMY *backbuffer = (void *)d->backbuffer;
   backbuffer->is_backbuffer = 1;
   /* Create a memory cache for the whole screen. */
   //TODO: Maybe we should do this lazily and defer to lock_bitmap_region
   if (!d->backbuffer->memory) {
      int n = w * h * _al_get_pixel_size(d->backbuffer->format);
      d->backbuffer->memory = _AL_MALLOC(n);
      memset(d->backbuffer->memory, 0, n);
   }

   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();

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
   
   if (!(d->display.flags & AL_RESIZABLE)) {
      XSizeHints *hints = XAllocSizeHints();;

      hints->flags = PMinSize | PMaxSize | PBaseSize;
      hints->min_width  = hints->max_width  = hints->base_width  = w;
      hints->min_height = hints->max_height = hints->base_height = h;
      XSetWMNormalHints(system->xdisplay, d->window, hints);

      XFree(hints);
   }

   d->wm_delete_window_atom = XInternAtom (system->xdisplay,
      "WM_DELETE_WINDOW", False);
   XSetWMProtocols (system->xdisplay, d->window, &d->wm_delete_window_atom, 1);

   XMapWindow(system->xdisplay, d->window);
   // TODO: Do we need to wait here until the window is mapped?
   TRACE("xdisplay: X11 window mapped.\n");

   /* Create a GLX subwindow inside our window. */
   d->glxwindow = glXCreateWindow(system->xdisplay, fbc[0], d->window, 0);

   TRACE("xdisplay: GLX window created.\n");
   
   /* Create a GLX context. */
   d->context = glXCreateNewContext(system->xdisplay, fbc[0], GLX_RGBA_TYPE,
      NULL, True);

   TRACE("xdisplay: Got GLX context.\n");

   return (AL_DISPLAY *)d;
}

static void destroy_display(AL_DISPLAY *d)
{
   //FIXME!
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

   if (!glx->is_initialized) {
      setup_gl(d);
      glx->is_initialized = 1;
   }

   TRACE("xdisplay: GLX context made current.\n");
}

static void set_opengl_color(AL_DISPLAY *d, AL_COLOR *color)
{
    unsigned char r, g, b, a;
    _al_unmap_rgba(d->format, color, &r, &g, &b, &a);
    glColor4b(r, g, b, a);
}

/* Dummy implementation of clear. */
static void clear(AL_DISPLAY *d, AL_COLOR *color)
{
   unsigned char r, g, b, a;
   _al_unmap_rgba(d->format, color, &r, &g, &b, &a);
   glClearColor(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
   glClear(GL_COLOR_BUFFER_BIT);
}

/* Dummy implementation of line. */
static void draw_line(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
   AL_COLOR *color)
{
   set_opengl_color(d, color);
   glBegin(GL_LINES);
   glVertex2d(fx, fy);
   glVertex2d(tx, ty);
   glEnd();
}

/* Dummy implementation of a filled rectangle. */
static void draw_filled_rectangle(AL_DISPLAY *d, float tlx, float tly,
   float brx, float bry, AL_COLOR *color)
{
   set_opengl_color(d, color);
   glBegin(GL_QUADS);
   glVertex2d(tlx, tly);
   glVertex2d(brx, tly);
   glVertex2d(brx, bry);
   glVertex2d(tlx, bry);
   glEnd();
}

/* Dummy implementation of flip. */
static void flip_display(AL_DISPLAY *d)
{
   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;
   glFlush();
   glXSwapBuffers(system->xdisplay, glx->glxwindow);
}

static void acknowledge_resize(AL_DISPLAY *d)
{
   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();
   AL_DISPLAY_XDUMMY *glx = (AL_DISPLAY_XDUMMY *)d;

   unsigned int w, h;
   glXQueryDrawable(system->xdisplay, glx->glxwindow, GLX_WIDTH, &w);
   glXQueryDrawable(system->xdisplay, glx->glxwindow, GLX_HEIGHT, &h);

   d->w = w;
   d->h = h;
   glx->is_initialized = 0;
}

/* Handle an X11 configure event. */
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

/* Handle an X11 close button event. */
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
// TODO: rename to create_bitmap once the A4 function of same name is out of the
// way
// FIXME: think about moving this to xbitmap.c instead..
AL_BITMAP *xdummy_create_bitmap(AL_DISPLAY *d, unsigned int w, unsigned int h)
{
   int format = al_get_new_bitmap_format();
   int flags = al_get_new_bitmap_flags();

   //FIXME
   format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;

   AL_BITMAP_XDUMMY *bitmap = _AL_MALLOC(sizeof *bitmap);
   bitmap->bitmap.vt = _al_bitmap_xdummy_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.memory = NULL;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;

   bitmap->texture = 0;

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
   vt->clear = clear;
   vt->draw_line = draw_line;
   vt->draw_filled_rectangle = draw_filled_rectangle;
   vt->flip_display = flip_display;
   vt->notify_resize = acknowledge_resize;
   vt->create_bitmap = xdummy_create_bitmap;
   vt->get_backbuffer = get_backbuffer;
   vt->get_frontbuffer = get_backbuffer;
   vt->set_target_bitmap = set_target_bitmap;
   vt->is_compatible_bitmap = is_compatible_bitmap;

   return vt;
}
