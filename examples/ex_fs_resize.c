#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

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

static void main_loop(ALLEGRO_BITMAP *picture)
{
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   int new_res;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(
            al_get_current_display()));

   while (1) {
      if (al_event_queue_is_empty(queue)) {
         redraw(picture);
      }
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
         continue;
      }

      if (event.type != ALLEGRO_EVENT_KEY_DOWN) {
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
         printf("Switching to %dx%d... ", res[cur_res].w, res[cur_res].h);
         if (al_resize_display(res[cur_res].w, res[cur_res].h)) {
            printf("succeeded.\n");
         }
         else {
            printf("failed.\n");
         }
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
   al_init_primitives_addon();
   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   display = al_create_display(res[cur_res].w, res[cur_res].h);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   picture = al_load_bitmap("data/mysha.pcx");
   if (!picture) {
      abort_example("mysha.pcx not found\n");
      return 1;
   }

   main_loop(picture);

   al_destroy_bitmap(picture);

   /* Destroying the fullscreen display restores the original screen
    * resolution.  Shutting down Allegro would automatically destroy the
    * display, too.
    */
   al_destroy_display(display);

   return 0;
}

