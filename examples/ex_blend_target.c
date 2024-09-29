#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60

static A5O_BITMAP *load_bitmap(char const *filename)
{
   A5O_BITMAP *bitmap = al_load_bitmap(filename);
   if (!bitmap)
      abort_example("%s not found or failed to load\n", filename);
   return bitmap;
}

int main(int argc, char **argv)
{
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_DISPLAY *display;
   A5O_BITMAP* mysha;
   A5O_BITMAP* parrot;
   A5O_BITMAP* targets[4];
   A5O_BITMAP* backbuffer;
   int w;
   int h;

   bool done = false;
   bool need_redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init image addon.\n");
   }

   init_platform_specific();

   mysha = load_bitmap("data/mysha.pcx");
   parrot = load_bitmap("data/obp.jpg");

   w = al_get_bitmap_width(mysha);
   h = al_get_bitmap_height(mysha);

   display = al_create_display(2 * w, 2 * h);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   /* Set up blend modes once. */
   backbuffer = al_get_backbuffer(display);
   targets[0] = al_create_sub_bitmap(backbuffer, 0, 0, w, h );
   al_set_target_bitmap(targets[0]);
   al_set_bitmap_blender(A5O_DEST_MINUS_SRC, A5O_SRC_COLOR, A5O_DST_COLOR);
   targets[1] = al_create_sub_bitmap(backbuffer, w, 0, w, h );
   al_set_target_bitmap(targets[1]);
   al_set_bitmap_blender(A5O_ADD, A5O_SRC_COLOR, A5O_DST_COLOR);
   al_set_bitmap_blend_color(al_map_rgb_f(0.5, 0.5, 1.0));
   targets[2] = al_create_sub_bitmap(backbuffer, 0, h, w, h );
   al_set_target_bitmap(targets[2]);
   al_set_bitmap_blender(A5O_SRC_MINUS_DEST, A5O_SRC_COLOR, A5O_DST_COLOR);
   targets[3] = al_create_sub_bitmap(backbuffer, w, h, w, h );
   al_set_target_bitmap(targets[3]);
   al_set_bitmap_blender(A5O_DEST_MINUS_SRC, A5O_INVERSE_SRC_COLOR, A5O_DST_COLOR);

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   while (!done) {
      A5O_EVENT event;

      if (need_redraw) {
         int i;
         al_set_target_bitmap(backbuffer);
         al_draw_bitmap(parrot, 0, 0, 0);

         for (i = 0; i < 4; i++) {
            /* Simply setting the target also sets the blend mode. */
            al_set_target_bitmap(targets[i]);
            al_draw_bitmap(mysha, 0, 0, 0);
         }

         al_flip_display();
         need_redraw = false;
      }

      while (true) {
         al_wait_for_event(queue, &event);
         switch (event.type) {
            case A5O_EVENT_KEY_CHAR:
               if (event.keyboard.keycode == A5O_KEY_ESCAPE)
                  done = true;
               break;

            case A5O_EVENT_DISPLAY_CLOSE:
               done = true;
               break;

            case A5O_EVENT_TIMER:
               need_redraw = true;
               break;
         }
         if (al_is_event_queue_empty(queue))
            break;
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
