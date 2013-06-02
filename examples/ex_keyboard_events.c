/*
 *    Example program for the Allegro library, by Peter Wang.
 *    Updated by Ryan Dickie.
 *
 *    This program tests keyboard events.
 */

#include <stdio.h>
#include <allegro5/allegro.h>

#include "common.c"

#define WIDTH        640
#define HEIGHT       480
#define SIZE_LOG     50


/* globals */
ALLEGRO_EVENT_QUEUE *event_queue;
ALLEGRO_DISPLAY     *display;



static void log_key(char const *how, int keycode, int unichar, int modifiers)
{
   char multibyte[5] = {0, 0, 0, 0, 0};
   const char* key_name;

   al_utf8_encode(multibyte, unichar <= 32 ? ' ' : unichar);
   key_name = al_keycode_to_name(keycode);
   log_printf("%-8s  code=%03d, char='%s' (%4d), modifiers=%08x, [%s]\n",
      how, keycode, multibyte, unichar, modifiers, key_name);
}



/* main_loop:
 *  The main loop of the program.  Here we wait for events to come in from
 *  any one of the event sources and react to each one accordingly.  While
 *  there are no events to react to the program sleeps and consumes very
 *  little CPU time.  See main() to see how the event sources and event queue
 *  are set up.
 */
static void main_loop(void)
{
   ALLEGRO_EVENT event;

   log_printf("Focus on the main window (black) and press keys to see events. ");
   log_printf("Escape quits.\n\n");

   while (true) {
      /* Take the next event out of the event queue, and store it in `event'. */
      al_wait_for_event(event_queue, &event);

      /* Check what type of event we got and act accordingly.  ALLEGRO_EVENT
       * is a union type and interpretation of its contents is dependent on
       * the event type, which is given by the 'type' field.
       *
       * Each event also comes from an event source and has a timestamp.
       * These are accessible through the 'any.source' and 'any.timestamp'
       * fields respectively, e.g. 'event.any.timestamp'
       */
      switch (event.type) {

         /* ALLEGRO_EVENT_KEY_DOWN - a keyboard key was pressed.
          */
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               return;
            }
            log_key("KEY_DOWN", event.keyboard.keycode, 0, 0);
            break;

         /* ALLEGRO_EVENT_KEY_UP - a keyboard key was released.
          */
         case ALLEGRO_EVENT_KEY_UP:
            log_key("KEY_UP", event.keyboard.keycode, 0, 0);
            break;

         /* ALLEGRO_EVENT_KEY_CHAR - a character was typed or repeated.
          */
         case ALLEGRO_EVENT_KEY_CHAR: {
            char const *label = (event.keyboard.repeat ? "repeat" : "KEY_CHAR");
            log_key(label,
               event.keyboard.keycode,
               event.keyboard.unichar,
               event.keyboard.modifiers);
            break;
         }

         /* ALLEGRO_EVENT_DISPLAY_CLOSE - the window close button was pressed.
          */
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return;

         /* We received an event of some type we don't know about.
          * Just ignore it.
          */
         default:
            break;
      }
   }
}



int main(void)
{
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log_monospace();

   display = al_create_display(WIDTH, HEIGHT);
   if (!display) {
      abort_example("al_create_display failed\n");
   }
   al_clear_to_color(al_map_rgb_f(0, 0, 0));
   al_flip_display();

   if (!al_install_keyboard()) {
      abort_example("al_install_keyboard failed\n");
   }

   event_queue = al_create_event_queue();
   if (!event_queue) {
      abort_example("al_create_event_queue failed\n");
   }

   al_register_event_source(event_queue, al_get_keyboard_event_source());
   al_register_event_source(event_queue, al_get_display_event_source(display));

   main_loop();

   close_log(false);

   return 0;
}

/* vim: set ts=8 sts=3 sw=3 et: */
