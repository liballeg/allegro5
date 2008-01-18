#include "xglx.h"

static void set_opengl_color(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
    unsigned char r, g, b, a;
    al_unmap_rgba(color, &r, &g, &b, &a);
    glColor4b(r, g, b, a);
}

static void set_opengl_blending(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };
   ALLEGRO_COLOR *bc;
   int src_mode, dst_mode;
   float r, g, b, a;
   al_unmap_rgba_f(color, &r, &g, &b, &a);

   al_get_blender(&src_mode, &dst_mode, NULL);
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_mode], blend_modes[dst_mode]);
   bc = _al_get_blend_color();
   glColor4f(r * bc->r, g * bc->g, b * bc->b, a * bc->a);
}

/* Dummy implementation of clear. */
static void clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_XGLX *xglx_target = (ALLEGRO_BITMAP_XGLX *)target;
   if (!xglx_target->is_backbuffer && glx->opengl_target != xglx_target) {
      _al_clear_memory(color);
      return;
   }

   unsigned char r, g, b, a;
   al_unmap_rgba(color, &r, &g, &b, &a);

   glClearColor(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
   glClear(GL_COLOR_BUFFER_BIT);
}

/* Dummy implementation of line. */
static void draw_line(ALLEGRO_DISPLAY *d, float fx, float fy, float tx, float ty,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_XGLX *xglx_target = (ALLEGRO_BITMAP_XGLX *)target;
   if (!xglx_target->is_backbuffer && glx->opengl_target != xglx_target) {
      _al_draw_line_memory(fx, fy, tx, ty, color);
   }

   set_opengl_blending(d, color);
   glBegin(GL_LINES);
   glVertex2d(fx, fy);
   glVertex2d(tx, ty);
   glEnd();
}

/* Dummy implementation of a filled rectangle. */
static void draw_rectangle(ALLEGRO_DISPLAY *d, float tlx, float tly,
   float brx, float bry, ALLEGRO_COLOR *color, int flags)
{
   ALLEGRO_DISPLAY_XGLX *glx = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_XGLX *xglx_target = (ALLEGRO_BITMAP_XGLX *)target;
   if (!xglx_target->is_backbuffer && glx->opengl_target != xglx_target) {
      _al_draw_rectangle_memory(tlx, tly, brx, bry, color, flags);
      return;
   }

   set_opengl_blending(d, color);
   if (flags & ALLEGRO_FILLED)
      glBegin(GL_QUADS);
   else
      glBegin(GL_LINE_LOOP);
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
