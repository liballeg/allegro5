#include <allegro5/allegro.h>
#include <math.h>
#include "demodata.h"
#include "global.h"
#include "background_scroller.h"

static int offx;
static int offy;

static double current_time;
static double dt;

static A5O_BITMAP *tile;

void draw_background(void)
{
   int dx = al_get_bitmap_width(tile);
   int dy = al_get_bitmap_height(tile);
   int x, y;

   while (offx > 0)
      offx -= dx;
   while (offy > 0)
      offy -= dy;

   for (y = offy; y < screen_height; y += dy) {
      for (x = offx; x < screen_width; x += dx) {
         al_draw_bitmap(tile, x, y, 0);
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
      (int)(1.4f * al_get_bitmap_width(tile) * sin(0.9f * current_time + 0.2f) +
            0.3f * al_get_bitmap_width(tile) * cos(1.5f * current_time - 0.4f));
   offy =
      (int)(0.6f * al_get_bitmap_height(tile) * sin(1.2f * current_time - 0.7f) -
            1.2f * al_get_bitmap_height(tile) * cos(0.2f * current_time + 1.1f));
}
