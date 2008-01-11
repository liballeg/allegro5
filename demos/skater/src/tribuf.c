#include <allegro.h>
#include "../include/defines.h"
#include "../include/tribuf.h"


/*****************************************************************************
 * Triple buffering module                                                   *
 *****************************************************************************/

static BITMAP *tb_page1 = NULL;
static BITMAP *tb_page2 = NULL;
static BITMAP *tb_page3 = NULL;
static BITMAP *tb_active_page = NULL;
static int tb_page = 0;

static void destroy(void)
{
   if (tb_page1) {
      destroy_bitmap(tb_page1);
      tb_page1 = NULL;
   }

   if (tb_page2) {
      destroy_bitmap(tb_page2);
      tb_page2 = NULL;
   }

   if (tb_page3) {
      destroy_bitmap(tb_page3);
      tb_page3 = NULL;
   }

   tb_active_page = NULL;
   tb_page = 0;
}


static int create(void)
{
   destroy();

   if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
      enable_triple_buffer();
   }

   if (!(gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)) {
      return DEMO_ERROR_TRIPLEBUFFER;
   }

   tb_page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
   tb_page2 = create_video_bitmap(SCREEN_W, SCREEN_H);
   tb_page3 = create_video_bitmap(SCREEN_W, SCREEN_H);

   if ((!tb_page1) || (!tb_page2) || (!tb_page3)) {
      destroy();
      return DEMO_ERROR_VIDEOMEMORY;
   }

   tb_active_page = tb_page1;

   return DEMO_OK;
}


static void draw(void)
{
   do {
   } while (poll_scroll());

   request_video_bitmap(tb_active_page);

   switch (tb_page) {
      case 0:
         tb_page = 1;
         tb_active_page = tb_page2;
         break;

      case 1:
         tb_page = 2;
         tb_active_page = tb_page3;
         break;

      case 2:
         tb_page = 0;
         tb_active_page = tb_page1;
         break;
   }
}


static BITMAP *get_canvas(void)
{
   return tb_active_page;
}


void select_triple_buffer(DEMO_SCREEN_UPDATE_DRIVER * driver)
{
   driver->create = create;
   driver->destroy = destroy;
   driver->draw = draw;
   driver->get_canvas = get_canvas;
}
