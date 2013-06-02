#include <allegro5/allegro.h>
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define NUM_BUTTONS  3

static void draw_mouse_button(int but, bool down)
{
   const int offset[NUM_BUTTONS] = {0, 70, 35};
   ALLEGRO_COLOR grey;
   ALLEGRO_COLOR black;
   int x;
   int y;

   x = 400 + offset[but-1];
   y = 130;

   grey = al_map_rgb(0xe0, 0xe0, 0xe0);
   black = al_map_rgb(0, 0, 0);

   al_draw_filled_rectangle(x, y, x + 27, y + 42, grey);
   al_draw_rectangle(x + 0.5, y + 0.5, x + 26.5, y + 41.5, black, 0);
   if (down) {
      al_draw_filled_rectangle(x + 2, y + 2, x + 25, y + 40, black);
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_MOUSE_STATE msestate;
   ALLEGRO_KEYBOARD_STATE kbdstate;
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   al_hide_mouse_cursor(display);

   cursor = al_load_bitmap("data/cursor.tga");
   if (!cursor) {
      abort_example("Error loading cursor.tga\n");
   }

   do {
      al_get_mouse_state(&msestate);
      al_get_keyboard_state(&kbdstate);

      al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));
      for (i = 1; i <= NUM_BUTTONS; i++) {
         draw_mouse_button(i, al_mouse_button_down(&msestate, i));
      }
      al_draw_bitmap(cursor, msestate.x, msestate.y, 0);
      al_flip_display();

      al_rest(0.005);
   } while (!al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE));

   return 0;
}

