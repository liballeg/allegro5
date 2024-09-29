/*
 * Ryan Roden-Corrent
 * Example that injects regular (non-user-type) allegro events into a queue.
 * This could be useful for 'faking' certain event sources.
 * For example, you could imitate joystick events without a * joystick.
 *
 * Based on the ex_user_events.c example.
 */

#include <stdio.h>
#include "allegro5/allegro.h"

#include "common.c"

int main(int argc, char **argv)
{
   A5O_EVENT_SOURCE fake_src;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT fake_keydown_event, fake_joystick_event;
   A5O_EVENT event;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   /* register our 'fake' event source with the queue */
   al_init_user_event_source(&fake_src);
   queue = al_create_event_queue();
   al_register_event_source(queue, &fake_src);

   /* fake a joystick event */
   fake_joystick_event.any.type = A5O_EVENT_JOYSTICK_AXIS;
   fake_joystick_event.joystick.stick = 1;
   fake_joystick_event.joystick.axis = 0;
   fake_joystick_event.joystick.pos = 0.5;
   al_emit_user_event(&fake_src, &fake_joystick_event, NULL);

   /* fake a keyboard event */
   fake_keydown_event.any.type = A5O_EVENT_KEY_DOWN;
   fake_keydown_event.keyboard.keycode = A5O_KEY_ENTER;
   al_emit_user_event(&fake_src, &fake_keydown_event, NULL);

   /* poll for the events we injected */
   while (!al_is_event_queue_empty(queue)) {
      al_wait_for_event(queue, &event);

      switch (event.type) {
         case A5O_EVENT_KEY_DOWN:
            A5O_ASSERT(event.user.source == &fake_src);
            log_printf("Got keydown: %d\n", event.keyboard.keycode);
            break;
         case A5O_EVENT_JOYSTICK_AXIS:
            A5O_ASSERT(event.user.source == &fake_src);
            log_printf("Got joystick axis: stick=%d axis=%d pos=%f\n",
               event.joystick.stick, event.joystick.axis, event.joystick.pos);
            break;
      }
   }

   al_destroy_user_event_source(&fake_src);
   al_destroy_event_queue(queue);

   log_printf("Done.\n");
   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
