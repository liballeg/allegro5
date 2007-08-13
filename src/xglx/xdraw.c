#include "xglx.h"

static void set_opengl_color(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
    unsigned char r, g, b, a;
    _al_unmap_rgba(d->format, color, &r, &g, &b, &a);
    glColor4b(r, g, b, a);
}

/* Dummy implementation of clear. */
static void clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   unsigned char r, g, b, a;
   _al_unmap_rgba(d->format, color, &r, &g, &b, &a);
   glClearColor(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
   glClear(GL_COLOR_BUFFER_BIT);
}

/* Dummy implementation of line. */
static void draw_line(ALLEGRO_DISPLAY *d, float fx, float fy, float tx, float ty,
   ALLEGRO_COLOR *color, int flags)
{
   set_opengl_color(d, color);
   glBegin(GL_LINES);
   glVertex2d(fx, fy);
   glVertex2d(tx, ty);
   glEnd();
}

/* Dummy implementation of a filled rectangle. */
static void draw_rectangle(ALLEGRO_DISPLAY *d, float tlx, float tly,
   float brx, float bry, ALLEGRO_COLOR *color, int flags)
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
void _xglx_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->clear = clear;
   vt->draw_line = draw_line;
   vt->draw_rectangle = draw_rectangle;
}
