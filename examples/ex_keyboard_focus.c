/*
 *    Example program for the Allegro library.
 *
 *    This program tests if the A5O_KEYBOARD_STATE `display' field
 *    is set correctly to the focused display.
 */

#include "allegro5/allegro.h"

#include "common.c"

static A5O_DISPLAY *display1;
static A5O_DISPLAY *display2;

static void redraw(A5O_COLOR color1, A5O_COLOR color2)
{
   al_set_target_backbuffer(display1);
   al_clear_to_color(color1);
   al_flip_display();

   al_set_target_backbuffer(display2);
   al_clear_to_color(color2);
   al_flip_display();
}

int main(int argc, char **argv)
{
   A5O_COLOR black;
   A5O_COLOR red;
   A5O_KEYBOARD_STATE kbdstate;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Error initialising Allegro.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   display1 = al_create_display(300, 300);
   display2 = al_create_display(300, 300);
   if (!display1 || !display2) {
      abort_example("Error creating displays.\n");
   }

   black = al_map_rgb(0, 0, 0);
   red = al_map_rgb(255, 0, 0);

   while (1) {
      al_get_keyboard_state(&kbdstate);
      if (al_key_down(&kbdstate, A5O_KEY_ESCAPE)) {
         break;
      }

      if (kbdstate.display == display1) {
         redraw(red, black);
      }
      else if (kbdstate.display == display2) {
         redraw(black, red);
      }
      else {
         redraw(black, black);
      }

      al_rest(0.1);
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
