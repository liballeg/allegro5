#include <allegro5/allegro5.h>
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

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

   al_draw_filled_rectangle(x, y, x + 26.5, y + 41.5, grey);
   al_draw_rectangle(x, y, x + 26.5, y + 41.5, black, 0);
   if (down) {
      al_draw_filled_rectangle(x + 2, y + 2, x + 24.5, y + 39.5, black);
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
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_hide_mouse_cursor();

   cursor = al_load_bitmap("data/cursor.tga");
   if (!cursor) {
      TRACE("Error loading cursor.tga\n");
      return 1;
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
END_OF_MAIN()

