#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_primitives.h>

int width = 640;
int height = 480;

int main(void)
{
   ALLEGRO_FONT *font;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *event_queue;
   bool right_button_down = false;
   bool redraw = true;
   int fake_x = 0, fake_y = 0;

   al_init();
   al_font_init();
   al_install_mouse();
   al_install_keyboard();

   al_set_new_display_flags(ALLEGRO_WINDOWED);
   display = al_create_display(width, height);
   al_show_mouse_cursor();

   event_queue = al_create_event_queue();
   al_register_event_source(event_queue, (void *)display);
   al_register_event_source(event_queue, (void *)al_get_mouse());
   al_register_event_source(event_queue, (void *)al_get_keyboard());

   font = al_font_load_font("data/fixed_font.tga", NULL);

   while (1) {
      ALLEGRO_EVENT event;
      
      if (redraw && al_event_queue_is_empty(event_queue)) {
         int th = al_font_text_height(font);
         
         al_clear(al_map_rgb_f(0, 0, 0));
         
         if (right_button_down) {
            al_draw_line(width / 2, height / 2, fake_x, fake_y,
               al_map_rgb_f(1, 0, 0), 1);
            al_draw_line(fake_x - 5, fake_y, fake_x + 5, fake_y,
               al_map_rgb_f(1, 1, 1), 2);
            al_draw_line(fake_x, fake_y - 5, fake_x, fake_y + 5,
               al_map_rgb_f(1, 1, 1), 2);
         }
         
         al_font_textprintf(font, 0, 0, "x: %i y: %i dx: %i dy %i",
            event.mouse.x, event.mouse.y,
            event.mouse.dx, event.mouse.dy);
         al_font_textprintf_centre(font, width / 2, height / 2 - th,
            "Left-Click to warp pointer to the middle once.");
         al_font_textprintf_centre(font, width / 2, height / 2,
            "Hold right mouse button to constantly move pointer to the middle.");
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(event_queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_WARPED) {
         printf("Warp\n");

      }
      if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         if (right_button_down) {
            al_set_mouse_xy(width / 2, height / 2);
            fake_x += event.mouse.dx;
            fake_y += event.mouse.dy;
         }
         redraw = true;
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1)
            al_set_mouse_xy(width / 2, height / 2);
         if (event.mouse.button == 2) {
            right_button_down = true;
            fake_x = width / 2;
            fake_y = height / 2;
         }
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if (event.mouse.button == 2) {
            right_button_down = false;
         }
      }
   }

   al_destroy_event_queue(event_queue);
   al_destroy_display(display);

   return 0;
}
END_OF_MAIN()
