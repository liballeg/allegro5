#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

static void redraw(ALLEGRO_BITMAP *picture)
{
   ALLEGRO_COLOR color;
   int w = al_get_display_width();
   int h = al_get_display_height();

   color = al_map_rgb(rand() % 255, rand() % 255, rand() % 255);
   al_clear_to_color(color);

   color = al_map_rgb(255, 0, 0);
   al_draw_line(0, 0, w, h, color, 0);
   al_draw_line(0, h, w, 0, color, 0);

   al_draw_bitmap(picture, 0, 0, 0);
   al_flip_display();
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *picture;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   picture = al_load_bitmap("data/mysha.pcx");
   if (!picture) {
      TRACE("mysha.pcx not found\n");
      return 1;
   }

   redraw(picture);
   al_rest(2.5);

   if (al_resize_display(800, 600)) {
      redraw(picture);
      al_rest(2.5);
   }
   else {
      return 1;
   }

   /* Destroying the fullscreen display restores the original screen
    * resolution.  Shutting down Allegro would automatically destroy the
    * display, too.
    */
   al_destroy_display(display);

   return 0;
}
END_OF_MAIN()

