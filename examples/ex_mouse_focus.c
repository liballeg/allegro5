/*
 *    Example program for the Allegro library.
 *
 *    This program tests if the A5O_MOUSE_STATE `display' field
 *    is set correctly.
 */

#include <stdio.h>
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
   A5O_MOUSE_STATE mst0;
   A5O_MOUSE_STATE mst;
   A5O_KEYBOARD_STATE kst;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Couldn't initialise Allegro.\n");
   }
   if (!al_install_mouse()) {
      abort_example("Couldn't install mouse.\n");
   }
   if (!al_install_keyboard()) {
      abort_example("Couldn't install keyboard.\n");
   }

   display1 = al_create_display(300, 300);
   display2 = al_create_display(300, 300);
   if (!display1 || !display2) {
      al_destroy_display(display1);
      al_destroy_display(display2);
      abort_example("Couldn't open displays.\n");
   }

   open_log();
   log_printf("Move the mouse cursor over the displays\n"); 

   black = al_map_rgb(0, 0, 0);
   red = al_map_rgb(255, 0, 0);

   memset(&mst0, 0, sizeof(mst0));

   while (1) {
      al_get_mouse_state(&mst);
      if (mst.display != mst0.display ||
            mst.x != mst0.x ||
            mst.y != mst0.y) {
         if (mst.display == NULL)
            log_printf("Outside either display\n");
         else if (mst.display == display1)
            log_printf("In display 1, x = %d, y = %d\n", mst.x, mst.y);
         else if (mst.display == display2)
            log_printf("In display 2, x = %d, y = %d\n", mst.x, mst.y);
         else {
            log_printf("Unknown display = %p, x = %d, y = %d\n", mst.display,
               mst.x, mst.y);
         }
         mst0 = mst;
      }

      if (mst.display == display1) {
         redraw(red, black);
      }
      else if (mst.display == display2) {
         redraw(black, red);
      }
      else {
         redraw(black, black);
      }

      al_rest(0.1);

      al_get_keyboard_state(&kst);
      if (al_key_down(&kst, A5O_KEY_ESCAPE)) {
         break;
      }
   }

   close_log(false);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
