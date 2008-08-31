#include <allegro5/allegro5.h>
#include "allegro5/a5_iio.h"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_MOUSE_STATE msestate;
   ALLEGRO_KEYBOARD_STATE kbdstate;

   al_init();
   al_install_mouse();
   al_install_keyboard();
   al_iio_init();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_hide_mouse_cursor();

   cursor = al_iio_load("cursor.tga");
   if (!cursor) {
      TRACE("Error loading cursor.tga\n");
      return 1;
   }

   do {
      al_get_mouse_state(&msestate);
      al_get_keyboard_state(&kbdstate);
      al_clear(al_map_rgb(0, 0, 0));
      al_draw_bitmap(cursor, msestate.x, msestate.y, 0);
      al_flip_display();
      al_rest(0.005);
   } while (!al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE)
      && !msestate.buttons);

   return 0;
}
END_OF_MAIN()

