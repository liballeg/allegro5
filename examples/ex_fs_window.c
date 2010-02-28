#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

static ALLEGRO_DISPLAY *display;
static ALLEGRO_BITMAP *picture;
static ALLEGRO_EVENT_QUEUE *key_queue;

static void redraw(void)
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

static void wait_a_while(void)
{
   ALLEGRO_EVENT event;
   double start_time = al_current_time();
   while (start_time+60 > al_current_time()) {
      if (al_get_next_event(key_queue, &event)) {
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               break;
            else if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
	       al_toggle_display_flag(ALLEGRO_FULLSCREEN_WINDOW,
	          !(al_get_display_flags() & ALLEGRO_FULLSCREEN_WINDOW));
	       redraw();
	    }
	 }
      }
      /* FIXME: Lazy timing */
      al_rest(0.001);
   }
}

int main(void)
{
   bool acknowledged = false;
   ALLEGRO_EVENT_QUEUE *display_queue;
   double timeout;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   key_queue = al_create_event_queue();
   al_register_event_source(key_queue, al_get_keyboard_event_source());
   display_queue = al_create_event_queue();
   al_register_event_source(display_queue, al_get_display_event_source(display));

   picture = al_load_bitmap("data/mysha.pcx");
   if (!picture) {
      abort_example("mysha.pcx not found\n");
      return 1;
   }

   redraw();
   wait_a_while();

   al_resize_display(800, 600);
   timeout = al_current_time();
   while (timeout+3 > al_current_time()) {
      ALLEGRO_EVENT event;
      if (al_get_next_event(display_queue, &event)) {
         if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
	 	al_acknowledge_resize(display);
		acknowledged = true;
		break;
	 }
      }
   }
   if (!acknowledged) {
      return 1;
   }

   redraw();
   wait_a_while();

   /* Destroying the fullscreen display restores the original screen
    * resolution.  Shutting down Allegro would automatically destroy the
    * display, too.
    */
   al_destroy_display(display);

   al_destroy_event_queue(key_queue);
   al_destroy_event_queue(display_queue);

   return 0;
}

