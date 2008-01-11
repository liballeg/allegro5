#include <allegro.h>
#include "../include/defines.h"
#include "../include/pageflip.h"

/*****************************************************************************
 * Page flipping module                                                      *
 *****************************************************************************/

static BITMAP *pf_page1 = NULL;
static BITMAP *pf_page2 = NULL;
static BITMAP *pf_active_page = NULL;

static void destroy(void)
{
   if (pf_page1) {
      destroy_bitmap(pf_page1);
      pf_page1 = NULL;
   }

   if (pf_page2) {
      destroy_bitmap(pf_page2);
      pf_page2 = NULL;
   }

   pf_active_page = NULL;
}


static int create(void)
{
   destroy();

   pf_page1 = create_video_bitmap(SCREEN_W, SCREEN_H);
   pf_page2 = create_video_bitmap(SCREEN_W, SCREEN_H);

   if ((!pf_page1) || (!pf_page2)) {
      destroy();
      return DEMO_ERROR_VIDEOMEMORY;
   }

   pf_active_page = pf_page1;

   return DEMO_OK;
}


static void draw(void)
{
   show_video_bitmap(pf_active_page);
   if (pf_active_page == pf_page1) {
      pf_active_page = pf_page2;
   } else {
      pf_active_page = pf_page1;
   }
}


static BITMAP *get_canvas(void)
{
   return pf_active_page;
}


void select_page_flipping(DEMO_SCREEN_UPDATE_DRIVER * driver)
{
   driver->create = create;
   driver->destroy = destroy;
   driver->draw = draw;
   driver->get_canvas = get_canvas;
}
