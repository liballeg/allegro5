/* Module for taking screenshots for Allegro games - implementation.
   Written by Miran Amon.
*/

#include <allegro5/allegro.h>
#include <stdio.h>
#include "global.h"
#include "screenshot.h"

static void next_screenshot(SCREENSHOT * ss)
{
   char buf[64];

   ss->counter++;
   if (ss->counter >= 10000) {
      ss->counter = 0;
      return;
   }

   snprintf(buf, sizeof(buf), "%s%04d.%s", ss->name, ss->counter, ss->ext);
   if (al_filename_exists(buf)) {
      next_screenshot(ss);
   }
}


SCREENSHOT *create_screenshot(char *name, char *ext)
{
   SCREENSHOT *ret = (SCREENSHOT *) malloc(sizeof(SCREENSHOT));

   ret->name = strdup(name);
   ret->ext = strdup(ext);
   ret->counter = -1;
   next_screenshot(ret);

   return ret;
}


void destroy_screenshot(SCREENSHOT * ss)
{
   free(ss->name);
   ss->name = NULL;
   free(ss->ext);
   ss->ext = NULL;
   free(ss);
   ss = NULL;
}


void take_screenshot(SCREENSHOT * ss)
{
   char buf[64];
   A5O_BITMAP *bmp2 = NULL;

   snprintf(buf, sizeof(buf), "%s%04d.%s", ss->name, ss->counter, ss->ext);

   bmp2 = al_create_bitmap(screen_width, screen_height);
   al_set_target_bitmap(bmp2);
   al_draw_bitmap(al_get_backbuffer(screen), 0, 0, 0);
   al_set_target_backbuffer(screen);
   al_save_bitmap(buf, bmp2);
   al_destroy_bitmap(bmp2);

   next_screenshot(ss);
}
