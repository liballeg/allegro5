/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test timed version of al_wait_for_event().
 */


#include <allegro5/allegro5.h>



int main(int argc, char *argv[])
{
   ALLEGRO_DISPLAY *dpy;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   float shade = 0.1;

   if (!al_init())
      return 1;

   al_install_keyboard();

   dpy = al_create_display(640, 480);
   if (!dpy) {
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());

   while (true) {
      if (al_wait_for_event(queue, &event, 100)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto Quit;
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

      al_clear(al_map_rgba_f(0.5 * shade, 0.25 * shade, shade, 0));
      al_flip_display();
   }

Quit:

   return 0;
}

/* vi: set sts=3 sw=3 et: */
