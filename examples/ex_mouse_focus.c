/*
 *    Example program for the Allegro library.
 *
 *    This program tests if the ALLEGRO_MOUSE_STATE `display' field
 *    is set correctly.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"

#include "common.c"

static ALLEGRO_DISPLAY *display1;
static ALLEGRO_DISPLAY *display2;

static void redraw(ALLEGRO_COLOR color1, ALLEGRO_COLOR color2)
{
   al_set_target_backbuffer(display1);
   al_clear_to_color(color1);
   al_flip_display();

   al_set_target_backbuffer(display2);
   al_clear_to_color(color2);
   al_flip_display();
}

int main(void)
{
   ALLEGRO_COLOR black;
   ALLEGRO_COLOR red;
   ALLEGRO_MOUSE_STATE mst;
   ALLEGRO_KEYBOARD_STATE kst;

   if (!al_init()) {
      return 1;
   }
   if (!al_install_mouse()) {
      return 1;
   }
   if (!al_install_keyboard()) {
      return 1;
   }

   display1 = al_create_display(300, 300);
   display2 = al_create_display(300, 300);
   if (!display1 || !display2) {
      al_destroy_display(display1);
      al_destroy_display(display2);
      return 1;
   }

   black = al_map_rgb(0, 0, 0);
   red = al_map_rgb(255, 0, 0);

   while (1) {
      al_get_mouse_state(&mst);
      printf("display = %p, x = %d, y = %d\n", mst.display, mst.x, mst.y);

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
      if (al_key_down(&kst, ALLEGRO_KEY_ESCAPE)) {
         break;
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
