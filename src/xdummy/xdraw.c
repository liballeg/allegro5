#include "xdummy.h"

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
   AL_COLOR *color, int flags)
{
   set_opengl_color(d, color);
   glBegin(GL_LINES);
   glVertex2d(fx, fy);
   glVertex2d(tx, ty);
   glEnd();
}

/* Dummy implementation of a filled rectangle. */
static void draw_rectangle(AL_DISPLAY *d, float tlx, float tly,
   float brx, float bry, AL_COLOR *color, int flags)
{
   set_opengl_color(d, color);
   glBegin(GL_QUADS);
   glVertex2d(tlx, tly);
   glVertex2d(brx, tly);
   glVertex2d(brx, bry);
   glVertex2d(tlx, bry);
   glEnd();
}

/* Add drawing commands to the vtable. */
void _xdummy_add_drawing_functions(AL_DISPLAY_INTERFACE *vt)
{
   vt->clear = clear;
   vt->draw_line = draw_line;
   vt->draw_rectangle = draw_rectangle;
}
