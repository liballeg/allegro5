#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>

#include "common.c"

#define NUM_RESOLUTIONS 4

static struct {
   int w, h;
} res[NUM_RESOLUTIONS] = {
   { 640, 480 },
   { 800, 600 },
   { 1024, 768 },
   { 1280, 1024 }
};

static int cur_res = 0;

static void redraw(ALLEGRO_BITMAP *picture)
{
   ALLEGRO_COLOR color;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   int w = al_get_bitmap_width(target);
   int h = al_get_bitmap_height(target);

   color = al_map_rgb(
       128 + rand() % 128,
       128 + rand() % 128,
       128 + rand() % 128);
   al_clear_to_color(color);

   color = al_map_rgb(255, 0, 0);
   al_draw_line(0, 0, w, h, color, 0);
   al_draw_line(0, h, w, 0, color, 0);

   al_draw_bitmap(picture, 0, 0, 0);
   al_flip_display();
}

static void main_loop(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *picture)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   bool can_draw;
   int new_res;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   can_draw = true;

   while (1) {
      if (al_is_event_queue_empty(queue) && can_draw) {
         redraw(picture);
      }
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_LOST) {
         log_printf("Display lost\n");
         can_draw = false;
         continue;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_FOUND) {
         log_printf("Display found\n");
         can_draw = true;
         continue;
      }
      if (event.type != ALLEGRO_EVENT_KEY_CHAR) {
         continue;
      }

      if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }

      new_res = cur_res;

      if (event.keyboard.unichar == '+' ||
            event.keyboard.unichar == ' ' ||
            event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
         new_res++;
         if (new_res >= NUM_RESOLUTIONS)
            new_res = 0;
      }
      else if (event.keyboard.unichar == '-') {
         new_res--;
         if (new_res < 0)
            new_res = NUM_RESOLUTIONS - 1;
      }

      if (new_res != cur_res) {
         cur_res = new_res;
         log_printf("Switching to %dx%d... ", res[cur_res].w, res[cur_res].h);
         if (al_resize_display(display, res[cur_res].w, res[cur_res].h)) {
            log_printf("Succeeded.\n");
         }
         else {
            log_printf("Failed. current resolution: %dx%d\n",
               al_get_display_width(display), al_get_display_height(display));
         }
      }
   }

   al_destroy_event_queue(queue);
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *picture;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_primitives_addon();
   al_install_keyboard();
   al_init_image_addon();
   init_platform_specific();

   open_log_monospace();

   if (argc == 2) {
           al_set_new_display_adapter(atoi(argv[1]));
   }

   al_set_new_display_flags(ALLEGRO_FULLSCREEN |
       ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(res[cur_res].w, res[cur_res].h);
   if (!display) {
      abort_example("Error creating display\n");
   }

   picture = al_load_bitmap("data/mysha.pcx");
   if (!picture) {
      abort_example("mysha.pcx not found\n");
   }

   main_loop(display, picture);

   al_destroy_bitmap(picture);

   /* Destroying the fullscreen display restores the original screen
    * resolution.  Shutting down Allegro would automatically destroy the
    * display, too.
    */
   al_destroy_display(display);

   return 0;
}

