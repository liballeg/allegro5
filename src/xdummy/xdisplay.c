/* This is only a dummy driver, not implementing most required things properly,
 * it's just here to give me some understanding of the base framework of a
 * display driver.
 */

#include <GL/glx.h>
#include <GL/gl.h>
#include <string.h>

#include "allegro.h"
#include "system_new.h"
#include "internal/aintern.h"
#include "platform/aintunix.h"
#include "internal/aintern2.h"
#include "internal/system_new.h"
#include "internal/display_new.h"

#include "xdummy.h"

typedef struct AL_DISPLAY_XDUMMY AL_DISPLAY_XDUMMY;

/* This is our version of AL_DISPLAY with driver specific extra data. */
struct AL_DISPLAY_XDUMMY
{
    AL_DISPLAY display; /* This must be the first member. */

    Window window;
    GLXWindow glxwindow;
    GLXContext context;
};

static AL_DISPLAY_INTERFACE *vt;

static Bool WaitForNotify(Display *d, XEvent *e, char *arg)
{
   return (e->type == MapNotify) && (e->xmap.window == (Window)arg);
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

   /* Let GLX choose an appropriate visual. */
   int nelements;
   GLXFBConfig *fbc = glXChooseFBConfig(system->xdisplay,
      DefaultScreen(system->xdisplay), 0, &nelements);
   XVisualInfo *vi = glXGetVisualFromFBConfig(system->xdisplay, fbc[0]);

   TRACE("xdisplay: Selected visual %lu.\n", vi->visualid);

   /* Create a GLX context. */
   d->context = glXCreateNewContext(system->xdisplay, fbc[0], GLX_RGBA_TYPE, 0,
      GL_FALSE);

    TRACE("xdisplay: Got GLX context.\n");

   /* Create a colormap. */
   Colormap cmap = XCreateColormap(system->xdisplay,
    RootWindow(system->xdisplay, vi->screen), vi->visual, AllocNone);

   /* Create an X11 window */
   XSetWindowAttributes swa;
   swa.colormap = cmap;
   swa.border_pixel = 0;
   swa.event_mask = StructureNotifyMask;
   d->window = XCreateWindow(system->xdisplay, RootWindow(
      system->xdisplay, vi->screen), 0, 0, w, h, 0, vi->depth, InputOutput,
      vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);

    TRACE("xdisplay: X11 window created.\n");

   /* X11 cludge: Wait for the window to get mapped, or nothing will work. */
   XEvent event;
   XMapWindow(system->xdisplay, d->window);
   XIfEvent(system->xdisplay, &event, WaitForNotify, (void *)d->window);

   TRACE("xdisplay: X11 window mapped.\n");

   /* Create a GLX subwindow inside our window. */
   d->glxwindow = glXCreateWindow(system->xdisplay, fbc[0], d->window, 0);

   TRACE("xdisplay: GLX window created.\n");

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

   /* Set up GL state as we want it. */
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, d->w, d->h, 0, -1, 1);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

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

   return vt;
}

