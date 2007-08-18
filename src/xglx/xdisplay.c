/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * display driver.
 */

#include "xglx.h"

static ALLEGRO_DISPLAY_INTERFACE *vt;

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
      XSetWMNormalHints(system->xdisplay, glx->window, hints);

      XFree(hints);
   }
}

/* Create a new X11 dummy display, which maps directly to a GLX window. */
static ALLEGRO_DISPLAY *create_display(int w, int h)
{
   ALLEGRO_DISPLAY_XGLX *d = _AL_MALLOC(sizeof *d);
   memset(d, 0, sizeof *d);

   ALLEGRO_SYSTEM_XGLX *system = (void *)al_system_driver();

   _al_mutex_lock(&system->lock);

   d->display.w = w;
   d->display.h = h;
   d->display.vt = vt;
   //FIXME
   //d->display.format = al_get_new_display_format();
   d->display.format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;

   d->display.refresh_rate = al_get_new_display_refresh_rate();
   d->display.flags = al_get_new_display_flags();

   // TODO: What is this?
   d->xscreen = DefaultScreen(system->xdisplay);

   if (d->display.flags & ALLEGRO_FULLSCREEN)
      _al_xglx_fullscreen_set_mode(system, w, h, 0, 0);

   //FIXME
   //d->display.flags |= ALLEGRO_WINDOWED;

   d->backbuffer = _al_xglx_create_bitmap(&d->display, w, h);
   ALLEGRO_BITMAP_XGLX *backbuffer = (void *)d->backbuffer;
   backbuffer->is_backbuffer = 1;
   /* Create a memory cache for the whole screen. */
   //TODO: Maybe we should do this lazily and defer to lock_bitmap_region
   //FIXME: need to resize this on resizing
   if (!d->backbuffer->memory) {
      int n = w * h * al_get_pixel_size(d->backbuffer->format);
      d->backbuffer->memory = _AL_MALLOC(n);
      memset(d->backbuffer->memory, 0, n);
   }

   /* Add ourself to the list of displays. */
   ALLEGRO_DISPLAY_XGLX **add = _al_vector_alloc_back(&system->system.displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&d->display.es);

   /* Let GLX choose an appropriate visual. */
   int double_buffer = True;
   int red_bits = 0;
   int green_bits = 0;
   int blue_bits = 0;
   int alpha_bits = 0;
   if (d->display.flags & ALLEGRO_SINGLEBUFFER)
      double_buffer = False;
   int attributes[]  = {
      GLX_DOUBLEBUFFER, double_buffer,
      GLX_RED_SIZE, red_bits,
      GLX_GREEN_SIZE, green_bits,
      GLX_BLUE_SIZE, blue_bits,
      GLX_ALPHA_SIZE, alpha_bits, 
      None};
   int nelements;
   GLXFBConfig *fbc = glXChooseFBConfig(system->xdisplay,
      d->xscreen, attributes, &nelements);
   XVisualInfo *vi = glXGetVisualFromFBConfig(system->xdisplay, fbc[0]);

   TRACE("xdisplay: Selected visual %lx.\n", vi->visualid);

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

   if (d->display.flags & ALLEGRO_FULLSCREEN) {
      _al_xglx_fullscreen_to_display(system, d);
   }

   /* Create a GLX context. */
   d->context = glXCreateNewContext(system->xdisplay, fbc[0], GLX_RGBA_TYPE,
      NULL, True);

   TRACE("xdisplay: Got GLX context.\n");

   _al_mutex_unlock(&system->lock);

   return &d->display;
}

static void destroy_display(ALLEGRO_DISPLAY *d)
{
   int i;
   ALLEGRO_SYSTEM_XGLX *s = (void *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   _al_mutex_lock(&s->lock);
   for (i = 0; i < s->system.displays._size; i++) {
      ALLEGRO_DISPLAY_XGLX **dptr = _al_vector_ref(&s->system.displays, i);
      if (glx == *dptr) {
         _al_vector_delete_at(&s->system.displays, i);
         break;
      }
   }
   XDestroyWindow(s->xdisplay, glx->window);

   if (d->flags & ALLEGRO_FULLSCREEN)
      _al_xglx_restore_video_mode(s);

   // FIXME: deallocate ourselves?

   _al_mutex_unlock(&s->lock);
}

static void set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   /* Make our GLX context current for reading and writing in the current
    * thread.
    */
   glXMakeContextCurrent(system->xdisplay, glx->glxwindow, glx->glxwindow,
      glx->context);

   if (!glx->opengl_initialized) {
      setup_gl(d);
   }
}

/* Dummy implementation of flip. */
static void flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   glFlush();
   glXSwapBuffers(system->xdisplay, glx->glxwindow);
}

static bool acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   unsigned int w, h;
   glXQueryDrawable(system->xdisplay, glx->glxwindow, GLX_WIDTH, &w);
   glXQueryDrawable(system->xdisplay, glx->glxwindow, GLX_HEIGHT, &h);

   d->w = w;
   d->h = h;

   setup_gl(d);

   return true;
}

static bool resize_display(ALLEGRO_DISPLAY *d, int w, int h)
{
   ALLEGRO_SYSTEM_XGLX *system = (ALLEGRO_SYSTEM_XGLX *)al_system_driver();
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

   _al_mutex_lock(&system->lock);

   set_size_hints(d, w, h);
   XResizeWindow(system->xdisplay, glx->window, w, h);
   XSync(system->xdisplay, False);

   /* Wait until we are actually resized. There might be a better way.. */
   _al_cond_wait(&system->resized, &system->lock);

   /* The user can expect the display to already be resized when resize_display
    * returns.
    * TODO: Right now, we still generate a resize event - maybe we should not.
    */
   acknowledge_resize(d);

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
void _al_display_xglx_closebutton(ALLEGRO_DISPLAY *d, XEvent *xevent)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;

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
ALLEGRO_BITMAP *_al_xglx_create_bitmap(ALLEGRO_DISPLAY *d, int w, int h)
{
   int format = al_get_new_bitmap_format();
   int flags = al_get_new_bitmap_flags();

   //FIXME
   format = d->format;

   ALLEGRO_BITMAP_XGLX *bitmap = _AL_MALLOC(sizeof *bitmap);
   memset(bitmap, 0, sizeof *bitmap);
   bitmap->bitmap.vt = _al_bitmap_xglx_driver();
   bitmap->bitmap.w = w;
   bitmap->bitmap.h = h;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;
   bitmap->bitmap.cl = 0;
   bitmap->bitmap.ct = 0;
   bitmap->bitmap.cr = w - 1;
   bitmap->bitmap.cb = h - 1;

   return &bitmap->bitmap;
}

static ALLEGRO_BITMAP *get_backbuffer(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)d;
   return glx->backbuffer;
}

static ALLEGRO_BITMAP *get_frontbuffer(ALLEGRO_DISPLAY *d)
{
   return NULL;
}

static void set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY_XGLX *glx = (ALLEGRO_DISPLAY_XGLX *)display;
   int x_1, y_1, x_2, y_2;
   //FIXME: change to offscreen targets and so on

   if (bitmap->cl == 0 && bitmap->cr == bitmap->w - 1 &&
      bitmap->ct == 0 && bitmap->cb == bitmap->h - 1) {
      glDisable(GL_SCISSOR_TEST);
   }
   else {
      glEnable(GL_SCISSOR_TEST);

      x_1 = bitmap->cl;
      y_1 = bitmap->ct;
      /* In OpenGL, coordinates are the top-left corner of pixels, so we need to
       * add one to the right abd bottom edge.
       */
      x_2 = bitmap->cr + 1;
      y_2 = bitmap->cb + 1;

      /* OpenGL is upside down, so must adjust y_2 to the height. */
      glScissor(x_1, bitmap->h - y_2, x_2 - x_1, y_2 - y_1);
   }
}

static bool is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   return true;
}

/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_xglx_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = create_display;
   vt->destroy_display = destroy_display;
   vt->set_current_display = set_current_display;
   vt->flip_display = flip_display;
   vt->acknowledge_resize = acknowledge_resize;
   vt->create_bitmap = _al_xglx_create_bitmap;
   vt->get_backbuffer = get_backbuffer;
   vt->get_frontbuffer = get_backbuffer;
   vt->set_target_bitmap = set_target_bitmap;
   vt->is_compatible_bitmap = is_compatible_bitmap;
   vt->resize_display = resize_display;
   vt->upload_compat_screen = _al_xglx_display_upload_compat_screen;
   _xglx_add_drawing_functions(vt);

   return vt;
}
