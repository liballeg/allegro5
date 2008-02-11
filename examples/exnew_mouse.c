#include <allegro5/allegro5.h>

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_MSESTATE msestate;
   ALLEGRO_KBDSTATE kbdstate;

   al_init();

   al_install_mouse();
   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      allegro_message("Error creating display");
      return 1;
   }

   al_hide_mouse_cursor();

   cursor = al_load_bitmap("cursor.tga");
   if (!cursor) {
      allegro_message("Error loading cursor.tga");
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

