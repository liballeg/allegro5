/*
 *    Example program for the Allegro library.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"


/* XXX Currently we have no formal way to allocate event type numbers. */
#define MY_EVENT_TYPE   1025


int main(void)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_SOURCE *user_src;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT user_event;
   ALLEGRO_EVENT event;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   timer = al_install_timer(0.5);
   if (!timer) {
      TRACE("Could not install timer.\n");
      return 1;
   }

   user_src = al_create_user_event_source();
   if (!user_src) {
      TRACE("Could not create user event source.\n");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, user_src);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) timer);

   al_start_timer(timer);

   while (true) {
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_TIMER) {
         int n = event.timer.count;

         printf("Got timer event %d\n", n);

         user_event.user.type = MY_EVENT_TYPE;
         user_event.user.data1 = n;
         al_emit_user_event(user_src, &user_event);
      }
      else if (event.type == MY_EVENT_TYPE) {
         int n = (int) event.user.data1;
         ASSERT(event.user.source == user_src);

         printf("Got user event %d\n", n);
         if (n == 5) {
            break;
         }
      }
   }

   al_destroy_user_event_source(user_src);
   al_uninstall_timer(timer);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
