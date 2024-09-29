#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60

struct Example {
   A5O_BITMAP *target_depth;
   A5O_BITMAP *target_no_depth;
   A5O_BITMAP *target_no_multisample;
   A5O_FONT *font;
   double t;
   double direct_speed_measure;
} example;

static void draw_on_target(A5O_BITMAP *target)
{
   A5O_STATE state;
   A5O_TRANSFORM transform;
   
   al_store_state(&state, A5O_STATE_TARGET_BITMAP |
      A5O_STATE_TRANSFORM |
      A5O_STATE_PROJECTION_TRANSFORM);

   al_set_target_bitmap(target);
   al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
   al_clear_depth_buffer(1);

   al_identity_transform(&transform);
   al_translate_transform_3d(&transform, 0, 0, 0);
   al_orthographic_transform(&transform,
      -1, 1, -1, 1, -1, 1);
   al_use_projection_transform(&transform);

   al_draw_filled_rectangle(-0.75, -0.5, 0.75, 0.5, al_color_name("blue"));

   al_set_render_state(A5O_DEPTH_TEST, true);

   int i, j;
   for (i = 0; i < 24; i++) {
      for (j = 0; j < 2; j++) {
         al_identity_transform(&transform);
         al_translate_transform_3d(&transform, 0, 0, j * 0.01);
         al_rotate_transform_3d(&transform, 0, 1, 0,
            A5O_PI * (i / 12.0 + example.t / 2));
         al_rotate_transform_3d(&transform, 1, 0, 0, A5O_PI * 0.25);
         al_use_transform(&transform);
         if (j == 0)
            al_draw_filled_rectangle(0, -.5, .5, .5, i % 2 == 0 ?
               al_color_name("yellow") : al_color_name("red"));
         else
            al_draw_filled_rectangle(0, -.5, .5, .5, i % 2 == 0 ?
               al_color_name("goldenrod") : al_color_name("maroon"));
      }
   }

   al_set_render_state(A5O_DEPTH_TEST, false);

   al_identity_transform(&transform);
   al_use_transform(&transform);
  
   al_draw_line(0.9, 0, 1, 0, al_color_name("black"), 0.05);
   al_draw_line(-0.9, 0, -1, 0, al_color_name("black"), 0.05);
   al_draw_line(0, 0.9, 0, 1, al_color_name("black"), 0.05);
   al_draw_line(0, -0.9, 0, -1, al_color_name("black"), 0.05);

   al_restore_state(&state);
}

static void redraw(void)
{
   double w = 512, h = 512;
   A5O_COLOR black = al_color_name("black");
   draw_on_target(example.target_depth);
   draw_on_target(example.target_no_depth);
   draw_on_target(example.target_no_multisample);

   al_clear_to_color(al_color_name("green"));

   A5O_TRANSFORM transform;

   al_identity_transform(&transform);
   al_use_transform(&transform);
   al_translate_transform(&transform, -128, -128);
   al_rotate_transform(&transform, example.t * A5O_PI / 3);
   al_translate_transform(&transform, 512 + 128, 128);
   al_use_transform(&transform);
   al_draw_bitmap(example.target_no_depth, 0, 0, 0);
   al_draw_text(example.font, black, 0, 0, 0, "no depth");

   al_identity_transform(&transform);
   al_use_transform(&transform);
   al_translate_transform(&transform, -128, -128);
   al_rotate_transform(&transform, example.t * A5O_PI / 3);
   al_translate_transform(&transform, 512 + 128, 256 + 128);
   al_use_transform(&transform);
   al_draw_bitmap(example.target_no_multisample, 0, 0, 0);
   al_draw_textf(example.font, black, 0, 0, 0, "no multisample");

   al_identity_transform(&transform);
   al_use_transform(&transform);
   al_translate_transform(&transform, -256, -256);
   al_rotate_transform(&transform, example.t * A5O_PI / 3);
   al_translate_transform(&transform, 256, 256);
   al_use_transform(&transform);
   al_draw_bitmap(example.target_depth, 0, 0, 0);

   al_draw_line(30, h / 2, 60, h / 2, black, 12);
   al_draw_line(w - 30, h / 2, w - 60, h / 2, black, 12);
   al_draw_line(w / 2, 30, w / 2, 60, black, 12);
   al_draw_line(w / 2, h - 30, w / 2, h - 60, black, 12);
   al_draw_text(example.font, black, 30, h / 2 - 16, 0, "back buffer");
   al_draw_text(example.font, black, 0, h / 2 + 10, 0, "bitmap");

   al_identity_transform(&transform);
   al_use_transform(&transform);
   al_draw_textf(example.font, black, w, 0,
      A5O_ALIGN_RIGHT, "%.1f FPS", 1.0 / example.direct_speed_measure);
}

static void update(void)
{
   example.t += 1.0 / FPS;
}

static void init(void)
{
   al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR);
   al_set_new_bitmap_depth(16);
   al_set_new_bitmap_samples(4);
   example.target_depth = al_create_bitmap(512, 512);
   al_set_new_bitmap_depth(0);
   example.target_no_depth = al_create_bitmap(256, 256);
   al_set_new_bitmap_samples(0);
   example.target_no_multisample = al_create_bitmap(256, 256);
}

int main(int argc, char **argv)
{
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_DISPLAY *display;
   int w = 512 + 256, h = 512;
   bool done = false;
   bool need_redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   al_init_font_addon();
   example.font = al_create_builtin_font();

   al_init_primitives_addon();

   init_platform_specific();

   al_set_new_display_option(A5O_DEPTH_SIZE, 16, A5O_SUGGEST);
   al_set_new_display_flags(A5O_PROGRAMMABLE_PIPELINE);

   display = al_create_display(w, h);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   init();

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   al_register_event_source(queue, al_get_timer_event_source(timer));
   
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   double t = -al_get_time();

   while (!done) {
      A5O_EVENT event;

      if (need_redraw) {
         t += al_get_time();
         example.direct_speed_measure  = t;
         t = -al_get_time();
         redraw();
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
               update();
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
