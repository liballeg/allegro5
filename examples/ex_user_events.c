/*
 *    Example program for the Allegro library.
 */

#include <stdio.h>
#include "allegro5/allegro.h"

#include "common.c"

#define MY_SIMPLE_EVENT_TYPE     A5O_GET_EVENT_TYPE('m', 's', 'e', 't')
#define MY_COMPLEX_EVENT_TYPE    A5O_GET_EVENT_TYPE('m', 'c', 'e', 't')


/* Just some fantasy event, supposedly used in an RPG - it's just to show that
 * in practice, the 4 user fields we have now never will be enough. */
typedef struct MY_EVENT
{
   int id;
   int type; /* For example "attack" or "buy". */
   int x, y, z; /* Position in the game world the event takes place. */
   int server_time; /* Game time in ticks the event takes place. */
   int source_unit_id; /* E.g. attacker or seller. */
   int destination_unit_id; /* E.g. defender of buyer. */
   int item_id; /* E.g. weapon used or item sold. */
   int amount; /* Gold the item is sold for. */
} MY_EVENT;


static MY_EVENT *new_event(int id)
{
    MY_EVENT *event = calloc(1, sizeof *event);
    event->id = id;
    return event;
}


static void my_event_dtor(A5O_USER_EVENT *event)
{
   log_printf("my_event_dtor: %p\n", (void *) event->data1);
   free((void *) event->data1);
}


int main(int argc, char **argv)
{
   A5O_TIMER *timer;
   A5O_EVENT_SOURCE user_src;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT user_event;
   A5O_EVENT event;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   timer = al_create_timer(0.5);
   if (!timer) {
      abort_example("Could not install timer.\n");
   }

   open_log();

   al_init_user_event_source(&user_src);

   queue = al_create_event_queue();
   al_register_event_source(queue, &user_src);
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_start_timer(timer);

   while (true) {
      al_wait_for_event(queue, &event);

      if (event.type == A5O_EVENT_TIMER) {
         int n = event.timer.count;

         log_printf("Got timer event %d\n", n);

         user_event.user.type = MY_SIMPLE_EVENT_TYPE;
         user_event.user.data1 = n;
         al_emit_user_event(&user_src, &user_event, NULL);

         user_event.user.type = MY_COMPLEX_EVENT_TYPE;
         user_event.user.data1 = (intptr_t)new_event(n);
         al_emit_user_event(&user_src, &user_event, my_event_dtor);
      }
      else if (event.type == MY_SIMPLE_EVENT_TYPE) {
         int n = (int) event.user.data1;
         A5O_ASSERT(event.user.source == &user_src);

         al_unref_user_event(&event.user);

         log_printf("Got simple user event %d\n", n);
         if (n == 5) {
            break;
         }
      }
      else if (event.type == MY_COMPLEX_EVENT_TYPE) {
         MY_EVENT *my_event = (void *)event.user.data1;
         A5O_ASSERT(event.user.source == &user_src);

         log_printf("Got complex user event %d\n", my_event->id);
         al_unref_user_event(&event.user);
      }
   }

   al_destroy_user_event_source(&user_src);
   al_destroy_event_queue(queue);
   al_destroy_timer(timer);

   log_printf("Done.\n");
   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
