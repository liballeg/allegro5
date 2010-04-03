#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

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

static void wait_for_key(void)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   while (1) {
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
	  break;
      }
   }
   al_destroy_event_queue(queue);
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *picture;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   picture = al_load_bitmap("data/mysha.pcx");
   if (!picture) {
      abort_example("mysha.pcx not found\n");
      return 1;
   }

   redraw(picture);
   wait_for_key();

   if (al_resize_display(1024, 768)) {
      redraw(picture);
      wait_for_key();
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

