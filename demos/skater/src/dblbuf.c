#include <allegro.h>
#include "../include/dblbuf.h"             /* double_buffer.h */
#include "../include/defines.h"
#include "../include/global.h"

/*****************************************************************************
 * Double buffering module                                                   *
 *****************************************************************************/

static BITMAP *double_buffer = NULL;


static void destroy(void)
{
   if (double_buffer) {
      destroy_bitmap(double_buffer);
      double_buffer = NULL;
   }
}


static int create(void)
{
   destroy();
   double_buffer = create_bitmap(SCREEN_W, SCREEN_H);

   if (!double_buffer) {
      return DEMO_ERROR_MEMORY;
   } else {
      clear_to_color(double_buffer, makecol(0, 0, 0));
   }

   return DEMO_OK;
}


static void draw(void)
{
   if (use_vsync) {
      vsync();
   }
   blit(double_buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}


static BITMAP *get_canvas(void)
{
   return double_buffer;
}


void select_double_buffer(DEMO_SCREEN_UPDATE_DRIVER * driver)
{
   driver->create = create;
   driver->destroy = destroy;
   driver->draw = draw;
   driver->get_canvas = get_canvas;
}
