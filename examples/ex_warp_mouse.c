#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>

int width = 640;
int height = 480;

int main()
{
   ALLEGRO_FONT *font;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *event_queue;

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
      al_wait_for_event(event_queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         al_clear(al_map_rgb_f(0, 0, 0));
         al_font_textprintf(font, 0, 0, "x: %i y: %i dx: %i dy %i",
            event.mouse.x, event.mouse.y,
            event.mouse.dx, event.mouse.dy);
         al_font_textprintf_centre(font, width / 2,
            (height - al_font_text_height(font)) / 2,
            "Click to warp pointer to the middle.");
         al_flip_display();
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         al_set_mouse_xy(width / 2, height / 2);
      }
   }

   al_destroy_event_queue(event_queue);
   al_destroy_display(display);

   return 0;
}
