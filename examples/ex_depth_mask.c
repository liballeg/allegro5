#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60
#define COUNT 80

struct Example {
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *mysha, *obp;
   ALLEGRO_FONT *font, *font2;
   double direct_speed_measure;

   struct Sprite {
      double x, y, angle;
   } sprites[COUNT];

} example;


static void redraw(void)
{
   ALLEGRO_TRANSFORM t;
   int i;

   /* We first draw the Obp background and clear the depth buffer to 1. */

   al_set_render_state(ALLEGRO_ALPHA_TEST, true);
   al_set_render_state(ALLEGRO_ALPHA_FUNCTION, ALLEGRO_RENDER_GREATER);
   al_set_render_state(ALLEGRO_ALPHA_TEST_VALUE, 0);

   al_set_render_state(ALLEGRO_DEPTH_TEST, false);

   al_set_render_state(ALLEGRO_WRITE_MASK, ALLEGRO_MASK_DEPTH | ALLEGRO_MASK_RGBA);

   al_clear_depth_buffer(1);
   al_clear_to_color(al_map_rgb_f(0, 0, 0));

   al_draw_scaled_bitmap(example.obp, 0, 0, 532, 416, 0, 0, 640, 416 * 640 / 532, 0);

   /* Next we draw all sprites but only to the depth buffer (with a depth value
    * of 0).
    */

   al_set_render_state(ALLEGRO_DEPTH_TEST, true);
   al_set_render_state(ALLEGRO_DEPTH_FUNCTION, ALLEGRO_RENDER_ALWAYS);
   al_set_render_state(ALLEGRO_WRITE_MASK, ALLEGRO_MASK_DEPTH);

   for (i = 0; i < COUNT; i++) {
      struct Sprite *s = example.sprites + i;
      int x, y;
      al_hold_bitmap_drawing(true);
      for (y = -480; y <= 0; y += 480) {
         for (x = -640; x <= 0; x += 640) {
            al_identity_transform(&t);
            al_rotate_transform(&t, s->angle);
            al_translate_transform(&t, s->x + x, s->y + y);
            al_use_transform(&t);
            al_draw_text(example.font, al_map_rgb(0, 0, 0), 0, 0,
               ALLEGRO_ALIGN_CENTER, "Allegro 5");
         }
       }
       al_hold_bitmap_drawing(false);
   }
   al_identity_transform(&t);
   al_use_transform(&t);

   /* Finally we draw Mysha, with depth testing so she only appears where
    * sprites have been drawn before.
    */

   al_set_render_state(ALLEGRO_DEPTH_FUNCTION, ALLEGRO_RENDER_EQUAL);
   al_set_render_state(ALLEGRO_WRITE_MASK, ALLEGRO_MASK_RGBA);
   al_draw_scaled_bitmap(example.mysha, 0, 0, 320, 200, 0, 0, 320 * 480 / 200, 480, 0);

   /* Finally we draw an FPS counter. */
   al_set_render_state(ALLEGRO_DEPTH_TEST, false);

   al_draw_textf(example.font2, al_map_rgb_f(1, 1, 1), 640, 0,
      ALLEGRO_ALIGN_RIGHT, "%.1f FPS", 1.0 / example.direct_speed_measure);
}

static void update(void)
{
   int i;
   for (i = 0; i < COUNT; i++) {
      struct Sprite *s = example.sprites + i;
      s->x -= 4;
      if (s->x < 80)
         s->x += 640;
      s->angle += i * ALLEGRO_PI / 180 / COUNT;
   }
}

static void init(void)
{
   int i;
   for (i = 0; i < COUNT; i++) {
      struct Sprite *s = example.sprites + i;
      s->x = (i % 4) * 160;
      s->y = (i / 4) * 24;
   }
}

int main(int argc, char **argv)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_MONITOR_INFO info;
   int w = 640, h = 480;
   bool done = false;
   bool need_redraw = true;
   bool background = false;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   al_init_font_addon();

   if (!al_init_ttf_addon()) {
      abort_example("Failed to init TTF addon.\n");
   }

   init_platform_specific();

   al_get_num_video_adapters();

   al_get_monitor_info(0, &info);

   int flags = 0;
   #ifdef ALLEGRO_IPHONE
   flags |= ALLEGRO_FULLSCREEN_WINDOW;
   #endif

   if (argc > 1 && strcmp(argv[1], "shader") == 0) {
      flags |= ALLEGRO_PROGRAMMABLE_PIPELINE;
   }
   al_set_new_display_flags(flags);

   al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS,
      ALLEGRO_DISPLAY_ORIENTATION_ALL, ALLEGRO_SUGGEST);

   al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 8, ALLEGRO_SUGGEST);

   al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

   example.display = al_create_display(w, h);
   if (!example.display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   example.font = al_load_font("data/DejaVuSans.ttf", 40, 0);
   if (!example.font) {
      abort_example("Error loading data/DejaVuSans.ttf\n");
   }

   example.font2 = al_load_font("data/DejaVuSans.ttf", 12, 0);
   if (!example.font2) {
      abort_example("Error loading data/DejaVuSans.ttf\n");
   }

   example.mysha = al_load_bitmap("data/mysha.pcx");
   if (!example.mysha) {
      abort_example("Error loading data/mysha.pcx\n");
   }

   example.obp = al_load_bitmap("data/obp.jpg");
   if (!example.obp) {
      abort_example("Error loading data/obp.jpg\n");
   }

   init();

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_register_event_source(queue, al_get_display_event_source(example.display));

   al_start_timer(timer);

   while (!done) {
      ALLEGRO_EVENT event;
      w = al_get_display_width(example.display);
      h = al_get_display_height(example.display);

      if (!background && need_redraw && al_is_event_queue_empty(queue)) {
         double t = -al_get_time();

         redraw();

         t += al_get_time();
         example.direct_speed_measure  = t;
         al_flip_display();
         need_redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:

            background = true;
            al_acknowledge_drawing_halt(event.display.source);

            break;

         case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            background = false;
            break;

         case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event.display.source);
            break;

         case ALLEGRO_EVENT_TIMER:
            update();
            need_redraw = true;
            break;
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
