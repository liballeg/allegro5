/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test timed version of al_wait_for_event().
 */


#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#include "common.c"

static bool test_relative_timeout(ALLEGRO_FONT *font, ALLEGRO_EVENT_QUEUE *queue);
static bool test_absolute_timeout(ALLEGRO_FONT *font, ALLEGRO_EVENT_QUEUE *queue);



int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *dpy;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   if (!al_init_font_addon()) {
      abort_example("Could not init Font addon.\n");
   }

   al_install_keyboard();

   dpy = al_create_display(640, 480);
   if (!dpy) {
      abort_example("Could not create a display.\n");
   }

   font = al_create_builtin_font();
   if (!font) {
      abort_example("Could not create builtin font.\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(dpy));

   while (true) {
      if (!test_relative_timeout(font, queue))
         break;
      if (!test_absolute_timeout(font, queue))
         break;
   }

   return 0;
}



static bool test_relative_timeout(ALLEGRO_FONT *font, ALLEGRO_EVENT_QUEUE *queue)
{
   ALLEGRO_EVENT event;
   float shade = 0.1;

   while (true) {
      if (al_wait_for_event_timed(queue, &event, 0.1)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               return true;
            }
            else {
               shade = 0.0;
            }
         }
         else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            return false;
         }
      }
      else {
         /* timed out */
         shade += 0.1;
         if (shade > 1.0)
            shade = 1.0;
      }

      al_clear_to_color(al_map_rgba_f(0.5 * shade, 0.25 * shade, shade, 0));
      al_draw_textf(font, al_map_rgb_f(1., 1., 1.), 320, 240, ALLEGRO_ALIGN_CENTRE, "Relative timeout (%.2f)", shade);
      al_draw_text(font, al_map_rgb_f(1., 1., 1.), 320, 260, ALLEGRO_ALIGN_CENTRE, "Esc to move on, Space to restart.");
      al_flip_display();
   }
}



static bool test_absolute_timeout(ALLEGRO_FONT *font, ALLEGRO_EVENT_QUEUE *queue)
{
   ALLEGRO_TIMEOUT timeout;
   ALLEGRO_EVENT event;
   float shade = 0.1;
   bool ret;

   while (true) {
      al_init_timeout(&timeout, 0.1);
      while ((ret = al_wait_for_event_until(queue, &event, &timeout))) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               return true;
            } else {
               shade = 0.;
            }
         }
         else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            return false;
         }
      }

      if (!ret) {
         /* timed out */
         shade += 0.1;
         if (shade > 1.0)
            shade = 1.0;
      }

      al_clear_to_color(al_map_rgba_f(shade, 0.5 * shade, 0.25 * shade, 0));
      al_draw_textf(font, al_map_rgb_f(1., 1., 1.), 320, 240, ALLEGRO_ALIGN_CENTRE, "Absolute timeout (%.2f)", shade);
      al_draw_text(font, al_map_rgb_f(1., 1., 1.), 320, 260, ALLEGRO_ALIGN_CENTRE, "Esc to move on, Space to restart.");
      al_flip_display();
   }
}


/* vi: set sts=3 sw=3 et: */
