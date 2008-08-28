#include <allegro5/allegro5.h>
#include <allegro5/a5_iio.h>

const int W = 320;
const int H = 200;

int main(void)
{
   al_init();
   al_iio_init();
   al_install_keyboard();
   al_install_mouse();

   al_set_new_display_flags(ALLEGRO_SINGLEBUFFER | ALLEGRO_RESIZABLE |
      ALLEGRO_GENERATE_EXPOSE_EVENTS);
   al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA);
   ALLEGRO_DISPLAY *display = al_create_display(W, H);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_show_mouse_cursor();

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA);
   ALLEGRO_BITMAP *bitmap = al_iio_load("mysha.pcx");
   if (!bitmap) {
      TRACE("%s not found or failed to load", "mysha.pcx");
      return 1;
   }
   al_draw_bitmap(bitmap, 0, 0, 0);
   al_flip_display();

   ALLEGRO_TIMER *timer = al_install_timer(0.5);

   ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) display);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);
   al_start_timer(timer);

   while (true) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
      if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
         event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
         int x = event.display.x,
            y = event.display.y, 
            w = event.display.width,
            h = event.display.height;
         /* Draw a red rectangle over the damaged area. */
         al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
         al_draw_rectangle(x, y, x + w, y + h, al_map_rgba_f(1, 0, 0, 1),
            ALLEGRO_FILLED);
         al_flip_display();
      }
      if (event.type == ALLEGRO_EVENT_TIMER) {
         /* Slowly restore the original bitmap. */
         int x, y;
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
            al_map_rgba_f(1, 1, 1, 0.1));
         for (y = 0; y < al_get_display_height(); y += 200) {
            for (x = 0; x < al_get_display_width(); x += 320) {
               al_draw_bitmap(bitmap, x, y, 0);
               //al_draw_rectangle(x,  y, 320, 200, al_map_rgb(255, 255, 255), ALLEGRO_FILLED);
            }
         }
         al_flip_display();
      }
   }

   al_destroy_event_queue(queue);
   al_destroy_bitmap(bitmap);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
