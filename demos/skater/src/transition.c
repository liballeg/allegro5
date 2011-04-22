#include <allegro5/allegro.h>
#include "global.h"
#include "transition.h"


TRANSITION *create_transition(GAMESTATE * from, GAMESTATE * to,
                              double duration)
{
   TRANSITION *t = malloc(sizeof(TRANSITION));

   t->from = from;
   t->to = to;
   t->duration = duration;
   t->progress = 0.0f;

   t->from_bmp = al_create_bitmap(screen_width, screen_height);
   al_set_target_bitmap(t->from_bmp);
   if (from) {
      from->draw();
   } else {
      al_clear_to_color(al_map_rgb(0, 0, 0));
   }
   al_set_target_backbuffer(screen);

   t->to_bmp = al_create_bitmap(screen_width, screen_height);
   al_set_target_bitmap(t->to_bmp);

   if (to) {
      to->draw();
   } else {
      al_clear_to_color(al_map_rgb(0, 0, 0));
   }
   al_set_target_backbuffer(screen);

   return t;
}


void destroy_transition(TRANSITION * t)
{
   if (t->from_bmp) {
      al_destroy_bitmap(t->from_bmp);
      t->from_bmp = NULL;
   }

   if (t->to_bmp) {
      al_destroy_bitmap(t->to_bmp);
      t->from_bmp = NULL;
   }

   free(t);
   t = NULL;
}


int update_transition(TRANSITION * t)
{
   t->progress += 1.0f / logic_framerate;

   if (t->progress >= t->duration) {
      return 0;
   } else {
      return 1;
   }
}


void draw_transition(TRANSITION * t)
{
   int y = (int)(t->progress * screen_height / t->duration);

   al_draw_bitmap_region(t->to_bmp, 0, screen_height - y, screen_width, y, 0, 0, 0);
   al_draw_bitmap_region(t->from_bmp, 0, 0, screen_width, screen_height - y, 0, y, 0);
}
