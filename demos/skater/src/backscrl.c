#include <allegro.h>
#include <math.h>
#include "../include/demodata.h"
#include "../include/global.h"

static int offx;
static int offy;

static float current_time;
static float dt;

static BITMAP *tile;

void draw_background(BITMAP *canvas)
{
   int dx = tile->w;
   int dy = tile->h;
   int x, y;

   while (offx > 0)
      offx -= dx;
   while (offy > 0)
      offy -= dy;

   for (y = offy; y < canvas->h; y += dy) {
      for (x = offx; x < canvas->w; x += dx) {
         blit(tile, canvas, 0, 0, x, y, dx, dy);
      }
   }
}


void init_background(void)
{
   offx = 0;
   offy = 0;

   current_time = 0.0f;
   dt = 1.0f / (float)logic_framerate;

   tile = demo_data[DEMO_BMP_BACK].dat;
}


void update_background(void)
{
   /* increase time */
   current_time += dt;

   /* calculate new offset from current current_time with some weird trig functions
      change the constants arbitrarily and/or add sin/cos components to
      get more complex animations */
   offx =
      (int)(1.4f * tile->w * sin(0.9f * current_time + 0.2f) +
            0.3f * tile->w * cos(1.5f * current_time - 0.4f));
   offy =
      (int)(0.6f * tile->h * sin(1.2f * current_time - 0.7f) -
            1.2f * tile->h * cos(0.2f * current_time + 1.1f));
}
