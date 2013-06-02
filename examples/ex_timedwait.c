/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test timed version of al_wait_for_event().
 */


#include <allegro5/allegro.h>

#include "common.c"

static void test_relative_timeout(ALLEGRO_EVENT_QUEUE *queue);
static void test_absolute_timeout(ALLEGRO_EVENT_QUEUE *queue);



int main(void)
{
   ALLEGRO_DISPLAY *dpy;
   ALLEGRO_EVENT_QUEUE *queue;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();

   dpy = al_create_display(640, 480);
   if (!dpy) {
      abort_example("Unable to set any graphic mode\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   test_relative_timeout(queue);
   test_absolute_timeout(queue);

   return 0;
}



static void test_relative_timeout(ALLEGRO_EVENT_QUEUE *queue)
{
   ALLEGRO_EVENT event;
   float shade = 0.1;

   while (true) {
      if (al_wait_for_event_timed(queue, &event, 0.1)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               return;
            }
            else {
               shade = 0.1;
            }
         }
      }
      else {
         /* timed out */
         shade += 0.1;
         if (shade > 1.0)
            shade = 1.0;
      }

      al_clear_to_color(al_map_rgba_f(0.5 * shade, 0.25 * shade, shade, 0));
      al_flip_display();
   }
}



static void test_absolute_timeout(ALLEGRO_EVENT_QUEUE *queue)
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
               return;
            } else {
               shade = 0.1;
            }
         }
      }

      if (!ret) {
         /* timed out */
         shade += 0.1;
         if (shade > 1.0)
            shade = 1.0;
      }

      al_clear_to_color(al_map_rgba_f(shade, 0.5 * shade, 0.25 * shade, 0));
      al_flip_display();
   }
}


/* vi: set sts=3 sw=3 et: */
