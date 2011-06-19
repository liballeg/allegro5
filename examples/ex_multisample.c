#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

static void draw(void)
{
   al_clear_to_color(al_map_rgb_f(1, 1, 1));
   al_draw_line(10, 10, 190, 190, al_map_rgb_f(0, 0, 0), 0);
   al_draw_line(100, 10, 100, 190, al_map_rgb_f(0, 0, 0), 0);
   al_draw_line(10, 100, 190, 100, al_map_rgb_f(0, 0, 0), 0);
   al_draw_line(190, 10, 10, 190, al_map_rgb_f(0, 0, 0), 0);
}

int main(void)
{
   ALLEGRO_DISPLAY *display, *ms_display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_TIMER *timer;
   bool quit = false;
   bool redraw = true;

   if (!al_init()) {
      printf("Could not init Allegro.\n");
      return 1;
   }
   al_init_primitives_addon();

   al_install_keyboard();

   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);
   al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);
   ms_display = al_create_display(200, 200);
   if (!ms_display) {
      printf("Multisampling not available.\n");
      return 1;
   }
   al_set_window_title(ms_display, "Multisampling");

   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_REQUIRE);
   al_set_new_display_option(ALLEGRO_SAMPLES, 0, ALLEGRO_SUGGEST);
   display = al_create_display(200, 200);
   al_set_window_title(display, "Normal");
   
   timer = al_create_timer(1.0 / 30.0);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_display_event_source(ms_display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   
   al_start_timer(timer);
   while (!quit) {
      ALLEGRO_EVENT event;
      /* Check for ESC key or close button event and quit in either case. */
      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            quit = true;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               quit = true;
            break;
         
         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;
      }
      
      if (redraw && al_is_event_queue_empty(queue)) {
         al_set_target_backbuffer(ms_display);
         draw();
         al_flip_display();
         
         al_set_target_backbuffer(display);
         draw();
         al_flip_display();
         
         redraw = false;
      }
   }

   return 0;
}
