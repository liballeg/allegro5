/* Module for taking screenshots for Allegro games - implementation.
   Written by Miran Amon.
*/

#include <allegro.h>
#include "../include/scrshot.h"

void next_screenshot(SCREENSHOT * ss)
{
   char buf[64];

   ss->counter++;
   if (ss->counter >= 10000) {
      ss->counter = 0;
      return;
   }

   uszprintf(buf, sizeof(buf), "%s%04d.%s", ss->name, ss->counter, ss->ext);
   if (exists(buf)) {
      next_screenshot(ss);
   }
}


SCREENSHOT *create_screenshot(char *name, char *ext)
{
   SCREENSHOT *ret = (SCREENSHOT *) malloc(sizeof(SCREENSHOT));

   ret->name = (char *)malloc(ustrsizez(name));
   ustrcpy(ret->name, name);
   ret->ext = (char *)malloc(ustrsizez(ext));
   ustrcpy(ret->ext, ext);
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


void take_screenshot(SCREENSHOT * ss, BITMAP *bmp)
{
   char buf[64];
   BITMAP *bmp2 = NULL;
   PALETTE pal;

   uszprintf(buf, sizeof(buf), "%s%04d.%s", ss->name, ss->counter, ss->ext);
   get_palette(pal);
   if (is_video_bitmap(bmp)) {
      bmp2 = create_bitmap(SCREEN_W, SCREEN_H);
      blit(bmp, bmp2, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      save_bitmap(buf, bmp2, pal);
      destroy_bitmap(bmp2);
   } else {
      save_bitmap(buf, bmp, pal);
   }
   next_screenshot(ss);
}
