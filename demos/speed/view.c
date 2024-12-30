/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Viewport functions (3d projection, wireframe guide rendering, etc).
 */

#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "speed.h"

#ifndef M_PI
#define M_PI         3.14159265358979323846
#endif



#define NUM_VIEWS    4



/* desired position of a viewport window */
typedef struct
{
   float pos[4];           /* left, top, right, bottom */
} VIEWPOS[NUM_VIEWS];



/* current status of a viewport window */
typedef struct
{
   float pos[4];           /* left, top, right, bottom */
   float vel[4];           /* rate of change of the above */
} VIEWINFO[NUM_VIEWS];



/* viewport positioning macros */
#define OFF_TL    {{ -0.1, -0.1, -0.1, -0.1 }}
#define OFF_TR    {{  1.1, -0.1,  1.1, -0.1 }}
#define OFF_BL    {{ -0.1,  1.1, -0.1,  1.1 }}
#define OFF_BR    {{  1.1,  1.1,  1.1,  1.1 }}

#define QTR_TL    {{  0,    0,    0.5,  0.5 }}
#define QTR_TR    {{  0.5,  0,    1.0,  0.5 }}
#define QTR_BL    {{  0,    0.5,  0.5,  1.0 }}
#define QTR_BR    {{  0.5,  0.5,  1.0,  1.0 }}

#define BIG_TL    {{  0,    0,    0.7,  0.7 }}
#define BIG_TR    {{  0.3,  0,    1.0,  0.7 }}
#define BIG_BL    {{  0,    0.3,  0.7,  1.0 }}
#define BIG_BR    {{  0.3,  0.3,  1.0,  1.0 }}

#define FULL      {{  0,    0,    1.0,  1.0 }}



/* list of viewport window positions */
static VIEWPOS viewpos[] =
{
   { FULL,   OFF_TR, OFF_BL, OFF_BR },    /* 1    single */
   { OFF_TL, FULL,   OFF_BL, OFF_BR },    /* 2    single */
   { BIG_TL, BIG_BR, OFF_BL, OFF_BR },    /* 12   multiple */
   { OFF_TL, OFF_TR, FULL,   OFF_BR },    /* 3    single */
   { BIG_TL, OFF_TR, BIG_BR, OFF_BR },    /* 13   multiple */
   { OFF_TL, BIG_TR, BIG_BL, OFF_BR },    /* 23   multiple */
   { FULL,   FULL,   OFF_BL, OFF_BR },    /* 12   superimpose */
   { OFF_TL, OFF_TR, OFF_BL, FULL,  },    /* 4    single */
   { BIG_TL, OFF_TR, OFF_BL, BIG_BR },    /* 14   multiple */
   { OFF_TL, FULL,   FULL,   OFF_BR },    /* 23   superimpose */
   { OFF_TL, BIG_TL, OFF_BL, BIG_BR },    /* 24   multiple */
   { OFF_TL, FULL,   OFF_BL, FULL   },    /* 24   superimpose */
   { QTR_TL, QTR_TR, QTR_BL, OFF_BR },    /* 123  multiple */
   { BIG_TL, OFF_TR, OFF_BL, BIG_BR },    /* 14   superimpose */
   { QTR_TL, OFF_TR, QTR_BL, QTR_BR },    /* 134  multiple */
   { FULL,   OFF_TR, FULL,   OFF_BR },    /* 13   superimpose */
   { OFF_TL, OFF_TR, BIG_TL, BIG_BR },    /* 34   multiple */
   { OFF_TL, QTR_TR, BIG_BL, QTR_BR },    /* 234  multiple */
   { OFF_TL, OFF_TR, FULL,   FULL   },    /* 34   superimpose */
   { FULL,   QTR_TR, OFF_BL, QTR_BR },    /* 124  multiple */
   { FULL,   FULL,   OFF_BL, FULL   },    /* 124  superimpose */
   { QTR_TL, QTR_TR, QTR_BL, QTR_BR },    /* 1234 multiple */
   { FULL,   FULL,   FULL,   OFF_BR },    /* 123  superimpose */
   { FULL,   OFF_TR, FULL,   FULL   },    /* 134  superimpose */
   { OFF_TL, FULL,   FULL,   FULL   },    /* 234  superimpose */
   { FULL,   FULL,   FULL,   FULL   },    /* 1234 superimpose */
};



/* current viewport state */
static VIEWINFO viewinfo;

static int viewnum;

static float view_left, view_top, view_right, view_bottom;



/* returns a scaling factor for 2d graphics effects */
float view_size()
{
   return ((view_right - view_left) + (view_bottom - view_top)) / 2;
}



/* initialises the view functions */
void init_view()
{
   int i, j;

   viewnum = 0;

   for (i=0; i<4; i++) {
      for (j=0; j<4; j++) {
         viewinfo[i].pos[j] = 0;
         viewinfo[i].vel[j] = 0;
      }
   }
}



/* closes down the view module */
void shutdown_view()
{
}



/* advances to the next view position */
int advance_view()
{
   int cycled = FALSE;

   viewnum++;

   if (viewnum >= (int)(sizeof(viewpos)/sizeof(VIEWPOS))) {
      viewnum = 0;
      cycled = TRUE;
   }

   return cycled;
}



/* updates the view position */
void update_view()
{
   float delta, vel;
   int i, j;

   for (i=0; i<4; i++) {
      for (j=0; j<4; j++) {
         delta = viewpos[viewnum][i].pos[j] - viewinfo[i].pos[j];
         vel = viewinfo[i].vel[j];

         vel *= 0.9;
         delta = log(ABS(delta)+1.0) * SGN(delta) / 64.0;
         vel += delta;

         if ((ABS(delta) < 0.00001) && (ABS(vel) < 0.00001)) {
            viewinfo[i].pos[j] = viewpos[viewnum][i].pos[j];
            viewinfo[i].vel[j] = 0;
         }
         else {
            viewinfo[i].pos[j] += vel;
            viewinfo[i].vel[j] = vel;
         }
      }
   }
}



/* flat projection function */
static int project_flat(float *f, int *i, int c)
{
   while (c > 0) {
      i[0] = view_left + f[0] * (view_right - view_left);
      i[1] = view_top  + f[1] * (view_bottom - view_top);

      f += 2;
      i += 2;
      c -= 2;
   }

   return TRUE;
}



/* spherical coordinate projection function */
static int project_spherical(float *f, int *i, int c)
{
   while (c > 0) {
      float ang = f[0] * M_PI * 2.0;

      float xsize = view_right - view_left;
      float ysize = view_bottom - view_top;
      float size = MIN(xsize, ysize) / 2.0;

      float ff = (f[1] > 0.99) ? 0 : (1.0 - f[1] * 0.9);

      float dx = cos(ang) * ff * size;
      float dy = sin(ang) * ff * size;

      i[0] = dx + (view_left + view_right) / 2.0;
      i[1] = dy + (view_top + view_bottom) / 2.0;

      f += 2;
      i += 2;
      c -= 2;
   }

   return TRUE;
}



/* inside of tube projection function */
static int project_tube(float *f, int *i, int c)
{
   while (c > 0) {
      float ang = f[0] * M_PI * 2.0 + M_PI / 2.0;

      float xsize = view_right - view_left;
      float ysize = view_bottom - view_top;
      float size = MIN(xsize, ysize) / 2.0;

      float x = cos(ang);
      float y = sin(ang);

      float z = 1.0 + (1.0 - f[1]) * 8.0;

      i[0] = x/z * size + (view_left + view_right) / 2.0;
      i[1] = y/z * size + (view_top + view_bottom) / 2.0;

      f += 2;
      i += 2;
      c -= 2;
   }

   return TRUE;
}



/* outside of cylinder projection function */
static int project_cylinder(float *f, int *i, int c)
{
   static MATRIX_f mtx;
   static int virgin = TRUE;

   if (virgin) {
      MATRIX_f m1, m2;

      get_z_rotate_matrix_f(&m1, -64);
      qtranslate_matrix_f(&m1, 0, 1.75, 0);
      get_scaling_matrix_f(&m2, 2.0, 1.0, 1.0);
      matrix_mul_f(&m1, &m2, &mtx);

      virgin = FALSE;
   }

   while (c > 0) {
      float ang = (f[0] - player_pos()) * M_PI * 2.0;

      float xsize = view_right - view_left;
      float ysize = view_bottom - view_top;
      float size = MIN(xsize, ysize) / 2.0;

      float x = cos(ang);
      float y = sin(ang);
      float z = 1.0 + (1.0 - f[1]) * 4.0;

      float xout, yout, zout;

      apply_matrix_f(&mtx, x, y, z, &xout, &yout, &zout);

      if (yout > 1.5)
         return FALSE;

      i[0] = xout/zout * size + (view_left + view_right) / 2.0;
      i[1] = (yout/zout * 2 - 1) * size + (view_top + view_bottom) / 2.0;

      f += 2;
      i += 2;
      c -= 2;
   }

   return TRUE;
}



/* draws the entire view */
void draw_view()
{
   int SCREEN_W = al_get_display_width(screen);
   int SCREEN_H = al_get_display_height(screen);
   int (*project)(float *f, int *i, int c);
   int r, g, b;
   ALLEGRO_COLOR c;
   int i, n, x, y;
   float point[6];
   int ipoint[6];

   al_clear_to_color(makecol(0, 0, 0));

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);

   for (i=0; i<4; i++) {

      view_left   = viewinfo[i].pos[0] * SCREEN_W;
      view_top    = viewinfo[i].pos[1] * SCREEN_H;
      view_right  = viewinfo[i].pos[2] * SCREEN_W;
      view_bottom = viewinfo[i].pos[3] * SCREEN_H;

      if ((view_right > view_left) && (view_bottom > view_top) &&
          (view_right > 0) && (view_bottom > 0) &&
          (view_left < SCREEN_W) && (view_top < SCREEN_H)) {

         switch (i) {

            case 0:
               /* flat projection, green */
               project = project_flat;

               r = 0;
               g = 255;
               b = 0;
               break;

            case 1:
               /* spherical coordinates, yellow */
               project = project_spherical;

               r = 255;
               g = 255;
               b = 0;
               break;

            case 2:
               /* inside a tube, blue */
               project = project_tube;

               r = 0;
               g = 0;
               b = 255;
               break;

            case 3:
               /* surface of cylinder, red */
               project = project_cylinder;

               r = 255;
               g = 0;
               b = 0;
               break;

            default:
               /* oops! */
               assert(FALSE);
               return;
         }

         if (!no_grid) {
            c = makecol(r/5, g/5, b/5);

            n = (low_detail) ? 8 : 16;

            for (x=0; x<=n; x++) {
               for (y=0; y<=n; y++) {
                  point[0] = (float)x / n;
                  point[1] = (float)y / n;
                  point[2] = (float)(x+1) / n;
                  point[3] = (float)y / n;
                  point[4] = (float)x / n;
                  point[5] = (float)(y+1) / n;

                  if (project(point, ipoint, 6)) {
                     if (x < n)
                        line(ipoint[0], ipoint[1], ipoint[2], ipoint[3], c);

                     if ((y < n) && ((x < n) || (i == 0)))
                        line(ipoint[0], ipoint[1], ipoint[4], ipoint[5], c);
                  }
               }
            }
         }

         draw_player(r, g, b, project);
         draw_badguys(r, g, b, project);
         draw_bullets(r, g, b, project);
         draw_explode(r, g, b, project);
      }
   }

   solid_mode();

   draw_message();

   textprintf(font_video, 4, 4, makecol(128, 128, 128), "Lives: %d", lives);
   textprintf(font_video, 4, 16, makecol(128, 128, 128), "Score: %d", score);
   textprintf(font_video, 4, 28, makecol(128, 128, 128), "Hiscore: %d", get_hiscore());

   al_flip_display();
}


