#include <allegro5/allegro5.h>

int main(void)
{
   int i;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *icon1, *icon2;
   al_init();
   al_install_mouse();
   display = al_create_display(320, 200);

   /* First icon: Read from file. */
   icon1 = al_load_bitmap("icon.tga");

   /* Second icon: Create it. */
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   icon2 = al_create_bitmap(16, 16);
   al_set_target_bitmap(icon2);
   ALLEGRO_COLOR *color = malloc(sizeof *color);
   for (i = 0; i < 256; i++)
   {
      al_map_rgb_f(color, (i % 16) / 15.0, (i / 16) / 15.0, 1);
      al_put_pixel(i % 16, i / 16, color);
   }
   al_set_target_bitmap(al_get_backbuffer());

   for (i = 0; i < 8; i++)
   {
      al_set_display_icon(i & 1 ? icon2 : icon1);
      al_flip_display();
      al_rest(500);
   }
   return 0;
}
END_OF_MAIN()
