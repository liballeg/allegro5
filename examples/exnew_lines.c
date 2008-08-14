/*
 * This example exercises line drawing, and single buffer mode.
 */

#include "allegro5/allegro5.h"

/* Define this to test drawing to memory bitmaps.
 * XXX currently al_draw_line() crashes when drawing to memory bitmaps
 *
 * Once event `display' fields are working we can just open up two windows,
 * one to test drawing to the backbuffer and one to test drawing to a memory
 * buffer.
 */
/* #define TEST_MEMBMP */

const int W = 640;
const int H = 480;

ALLEGRO_DISPLAY *display;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_COLOR background;
#ifdef TEST_MEMBMP
   ALLEGRO_BITMAP *memory_bitmap;
#endif

int last_x = -1;
int last_y = -1;


void plonk(const int x, const int y)
{
#ifdef TEST_MEMBMP
   al_set_target_bitmap(memory_bitmap);
#else
   al_set_target_bitmap(al_get_backbuffer());
#endif

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, background);
   al_draw_rectangle(0, 0, W, H, al_map_rgba_f(1, 1, 1, 0.2), ALLEGRO_FILLED);

   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb_f(1, 1, 1));
   al_draw_rectangle(x - 2, y - 2, x + 2, y + 2, al_map_rgb_f(1, 0, 0),
      ALLEGRO_FILLED);
   if (last_x != -1 && last_y != -1) {
      al_draw_line(last_x, last_y, x, y, al_map_rgb_f(1, 1, 1));
   }

#ifdef TEST_MEMBMP
   al_set_target_bitmap(al_get_backbuffer());
   al_draw_bitmap(memory_bitmap, 0.0, 0.0, 0);
#endif

   al_flip_display();

   last_x = x;
   last_y = y;
}

int main(void)
{
   ALLEGRO_EVENT event;

   al_init();
   al_install_keyboard();
   al_install_mouse();

   al_set_new_display_flags(ALLEGRO_SINGLEBUFFER);
   display = al_create_display(W, H);
   if (!display) {
      allegro_message("Error creating display");
      return 1;
   }

   background = al_map_rgb_f(0.5, 0.5, 0.6);

#ifdef TEST_MEMBMP
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   memory_bitmap = al_create_bitmap(W, H);
   if (!memory_bitmap) {
      allegro_message("Error creating memory bitmap");
      return 1;
   }
   al_set_target_bitmap(memory_bitmap);
   al_clear(background);
#endif

   al_set_target_bitmap(al_get_backbuffer());
   al_clear(background);
   al_flip_display();

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_mouse());

   while (true) {
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         plonk(event.mouse.x, event.mouse.y);
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
         last_x = last_y = -1;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
         event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
      {
         break;
      }
   }

   al_destroy_event_queue(queue);
#ifdef TEST_MEMBMP
   al_destroy_bitmap(memory_bitmap);
#endif

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
