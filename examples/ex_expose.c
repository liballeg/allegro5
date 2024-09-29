#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *bitmap;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_init_image_addon();
   al_install_keyboard();
   al_install_mouse();
   init_platform_specific();

   al_set_new_display_flags(A5O_RESIZABLE |
      A5O_GENERATE_EXPOSE_EVENTS);
   al_set_new_display_option(A5O_SINGLE_BUFFER, true, A5O_REQUIRE);
   display = al_create_display(320, 200);
   if (!display) {
      abort_example("Error creating display\n");
   }

   bitmap = al_load_bitmap("data/mysha.pcx");
   if (!bitmap) {
      abort_example("mysha.pcx not found or failed to load\n");
   }
   al_draw_bitmap(bitmap, 0, 0, 0);
   al_flip_display();

   timer = al_create_timer(0.1);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (true) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
            break;
      if (event.type == A5O_EVENT_KEY_DOWN &&
         event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
      if (event.type == A5O_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
      }
      if (event.type == A5O_EVENT_DISPLAY_EXPOSE) {
         int x = event.display.x,
            y = event.display.y, 
            w = event.display.width,
            h = event.display.height;
         /* Draw a red rectangle over the damaged area. */
         al_set_blender(A5O_ADD, A5O_ONE, A5O_ZERO);
         al_draw_filled_rectangle(x, y, x + w, y + h, al_map_rgba_f(1, 0, 0, 1));
         al_flip_display();
      }
      if (event.type == A5O_EVENT_TIMER) {
         /* Slowly restore the original bitmap. */
         int x, y;
         al_set_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA);
         for (y = 0; y < al_get_display_height(display); y += 200) {
            for (x = 0; x < al_get_display_width(display); x += 320) {
               al_draw_tinted_bitmap(bitmap, al_map_rgba_f(1, 1, 1, 0.1), x, y, 0);
            }
         }
         al_flip_display();
      }
   }

   al_destroy_event_queue(queue);
   al_destroy_bitmap(bitmap);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
