/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * display driver.
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
static AL_DISPLAY *create_display(int w, int h, int flags)
{
   AL_DISPLAY_XDUMMY *d = _AL_MALLOC(sizeof *d);
   memset(d, 0, sizeof *d);

   d->display.w = w;
   d->display.h = h;
   d->display.vt = vt;

   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();

   /* Add ourself to the list of displays. */
   AL_DISPLAY_XDUMMY **add = _al_vector_alloc_back(&system->displays);
   *add = d;

   /* Each display is an event source. */
   _al_event_source_init(&d->display.es, _AL_ALL_DISPLAY_EVENTS);

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
   
   if (!(flags & AL_RESIZABLE)) {
      XSizeHints *hints = XAllocSizeHints();;

      hints->flags = PMinSize | PMaxSize | PBaseSize;
      hints->min_width  = hints->max_width  = hints->base_width  = w;
      hints->min_height = hints->max_height = hints->base_height = h;
      XSetWMNormalHints(system->xdisplay, d->window, hints);

      XFree(hints);
   }

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

static void make_display_current(AL_DISPLAY *d)
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

/* Dummy implementation of clear. */
static void clear(AL_DISPLAY *d, AL_COLOR color)
{
   glClearColor(color.r, color.g, color.b, color.a);
   glClear(GL_COLOR_BUFFER_BIT);
}

/* Dummy implementation of line. */
static void draw_line(AL_DISPLAY *d, float fx, float fy, float tx, float ty,
   AL_COLOR color)
{
   glColor4f(color.r, color.g, color.b, color.a);
   glBegin(GL_LINES);
   glVertex2d(fx, fy);
   glVertex2d(tx, ty);
   glEnd();
}

/* Dummy implementation of a filled rectangle. */
static void filled_rectangle(AL_DISPLAY *d, float tlx, float tly,
   float brx, float bry, AL_COLOR color)
{
   glColor4f(color.r, color.g, color.b, color.a);
   glBegin(GL_QUADS);
   glVertex2d(tlx, tly);
   glVertex2d(brx, tly);
   glVertex2d(brx, bry);
   glVertex2d(tlx, bry);
   glEnd();
}

/* Dummy implementation of flip. */
static void flip(AL_DISPLAY *d)
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
   AL_SYSTEM_XDUMMY *system = (AL_SYSTEM_XDUMMY *)al_system_driver();
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
      if (_al_event_source_needs_to_generate_event(es,
         AL_EVENT_DISPLAY_RESIZE)) {
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
   if (_al_event_source_needs_to_generate_event(es, AL_EVENT_DISPLAY_EXPOSE)) {
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

/* Obtain a reference to this driver. */
AL_DISPLAY_INTERFACE *_al_display_xdummy_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = create_display;
   vt->make_display_current = make_display_current;
   vt->clear = clear;
   vt->line = draw_line;
   vt->filled_rectangle = filled_rectangle;
   vt->flip = flip;
   vt->acknowledge_resize = acknowledge_resize;

   return vt;
}
