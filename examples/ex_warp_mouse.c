#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>

#include "common.c"

int width = 640;
int height = 480;

int main(void)
{
   ALLEGRO_FONT *font;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *event_queue;
   ALLEGRO_EVENT event;
   bool right_button_down = false;
   bool redraw = true;
   int fake_x = 0, fake_y = 0;
   ALLEGRO_COLOR white;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_init_primitives_addon();
   al_init_font_addon();
   al_init_image_addon();
   al_install_mouse();
   al_install_keyboard();

   al_set_new_display_flags(ALLEGRO_WINDOWED);
   display = al_create_display(width, height);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   memset(&event, 0, sizeof(event));

   event_queue = al_create_event_queue();
   al_register_event_source(event_queue, al_get_display_event_source(display));
   al_register_event_source(event_queue, al_get_mouse_event_source());
   al_register_event_source(event_queue, al_get_keyboard_event_source());

   font = al_load_font("data/fixed_font.tga", 0, 0);
   white = al_map_rgb_f(1, 1, 1);

   while (1) {      
      if (redraw && al_is_event_queue_empty(event_queue)) {
         int th = al_get_font_line_height(font);
         
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         
         if (right_button_down) {
            al_draw_line(width / 2, height / 2, fake_x, fake_y,
               al_map_rgb_f(1, 0, 0), 1);
            al_draw_line(fake_x - 5, fake_y, fake_x + 5, fake_y,
               al_map_rgb_f(1, 1, 1), 2);
            al_draw_line(fake_x, fake_y - 5, fake_x, fake_y + 5,
               al_map_rgb_f(1, 1, 1), 2);
         }
         
         al_draw_textf(font, white, 0, 0, 0, "x: %i y: %i dx: %i dy %i",
            event.mouse.x, event.mouse.y,
            event.mouse.dx, event.mouse.dy);
         al_draw_textf(font, white, width / 2, height / 2 - th, ALLEGRO_ALIGN_CENTRE,
            "Left-Click to warp pointer to the middle once.");
         al_draw_textf(font, white, width / 2, height / 2, ALLEGRO_ALIGN_CENTRE,
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
         log_printf("Warp\n");
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         if (right_button_down) {
            al_set_mouse_xy(display, width / 2, height / 2);
            fake_x += event.mouse.dx;
            fake_y += event.mouse.dy;
         }
         redraw = true;
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1)
            al_set_mouse_xy(display, width / 2, height / 2);
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

   close_log(false);

   return 0;
}
