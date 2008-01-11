#include <allegro.h>
#include "../include/global.h"
#include "../include/transitn.h"


TRANSITION *create_transition(GAMESTATE * from, GAMESTATE * to,
                              float duration)
{
   TRANSITION *t = malloc(sizeof(TRANSITION));

   t->from = from;
   t->to = to;
   t->duration = duration;
   t->progress = 0.0f;

#ifdef DEMO_USE_ALLEGRO_GL
   t->from_bmp = create_video_bitmap(SCREEN_W, SCREEN_H);
#else
   t->from_bmp = create_bitmap(SCREEN_W, SCREEN_H);
#endif
   if (from) {
      from->draw(t->from_bmp);
   } else {
      clear_to_color(t->from_bmp, makecol(0, 0, 0));
   }

#ifdef DEMO_USE_ALLEGRO_GL
   t->to_bmp = create_video_bitmap(SCREEN_W, SCREEN_H);
#else
   t->to_bmp = create_bitmap(SCREEN_W, SCREEN_H);
#endif
   if (to) {
      to->draw(t->to_bmp);
   } else {
      clear_to_color(t->to_bmp, makecol(0, 0, 0));
   }

   return t;
}


void destroy_transition(TRANSITION * t)
{
   if (t->from_bmp) {
      destroy_bitmap(t->from_bmp);
      t->from_bmp = NULL;
   }

   if (t->to_bmp) {
      destroy_bitmap(t->to_bmp);
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


void draw_transition(BITMAP *canvas, TRANSITION * t)
{
   int y = (int)(t->progress * SCREEN_H / t->duration);

   blit(t->to_bmp, canvas, 0, SCREEN_H - y, 0, 0, SCREEN_W, y);
   blit(t->from_bmp, canvas, 0, 0, 0, y, SCREEN_W, SCREEN_H - y);
}
