#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60

static struct Example {
   ALLEGRO_BITMAP *targets[4];
   ALLEGRO_BITMAP *bitmap_normal;
   ALLEGRO_BITMAP *bitmap_filter;
   ALLEGRO_BITMAP *sub;
   ALLEGRO_FONT *font;
   float bitmap_x[8], bitmap_y[8];
   float bitmap_t;
   int step_t, step_x, step_y;
   ALLEGRO_BITMAP *step[4];
   bool update_step;
} example;

static ALLEGRO_BITMAP *create_bitmap(void)
{
   const int checkers_size = 8;
   const int bitmap_size = 24;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_LOCKED_REGION *locked;
   int x, y, p;
   unsigned char *rgba;

   bitmap = al_create_bitmap(bitmap_size, bitmap_size);
   locked = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, 0);
   rgba = locked->data;
   p = locked->pitch;
   for (y = 0; y < bitmap_size; y++) {
      for (x = 0; x < bitmap_size; x++) {
         int c = (((x / checkers_size) + (y / checkers_size)) & 1) * 255;
         rgba[y * p + x * 4 + 0] = 0;
         rgba[y * p + x * 4 + 1] = 0;
         rgba[y * p + x * 4 + 2] = 0;
         rgba[y * p + x * 4 + 3] = c;
      }
   }
   al_unlock_bitmap(bitmap);
   return bitmap;
}

static void init(void)
{
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   ALLEGRO_BITMAP *memory = create_bitmap();
   al_set_new_bitmap_flags(0);

   al_set_new_bitmap_samples(0);
   example.targets[0] = al_create_bitmap(300, 200);
   example.targets[1] = al_create_bitmap(300, 200);
   al_set_new_bitmap_samples(4);
   example.targets[2] = al_create_bitmap(300, 200);
   example.targets[3] = al_create_bitmap(300, 200);
   al_set_new_bitmap_samples(0);

   al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
   example.bitmap_filter = al_clone_bitmap(memory);
   al_set_new_bitmap_flags(0);
   example.bitmap_normal = al_clone_bitmap(memory);

   example.sub = al_create_sub_bitmap(memory, 0, 0, 30, 30);
   example.step[0] = al_create_bitmap(30, 30);
   example.step[1] = al_create_bitmap(30, 30);
   example.step[2] = al_create_bitmap(30, 30);
   example.step[3] = al_create_bitmap(30, 30);
}

static void bitmap_move(void)
{
   int i;

   example.bitmap_t++;
   for (i = 0; i < 8; i++) {
      float a = 2 * ALLEGRO_PI * i / 16;
      float s = sin((example.bitmap_t + i * 40) / 180 * ALLEGRO_PI);
      s *= 90;
      example.bitmap_x[i] = 100 + s * cos(a);
      example.bitmap_y[i] = 100 + s * sin(a);
   }
}

static void draw(char const *text, ALLEGRO_BITMAP *bitmap)
{
   int i;

   al_clear_to_color(al_color_name("white"));

   al_draw_text(example.font, al_map_rgb(0, 0, 0), 0, 0, 0, text);

   for (i = 0; i < 16; i++) {
      float a = 2 * ALLEGRO_PI * i / 16;
      ALLEGRO_COLOR c = al_color_hsv(i * 360 / 16, 1, 1);
      al_draw_line(100 + cos(a) * 10, 100 + sin(a) * 10,
         100 + cos(a) * 90, 100 + sin(a) * 90, c, 3);
   }

   for (i = 0; i < 8; i++) {
      float a = 2 * ALLEGRO_PI * i / 16;
      int s = al_get_bitmap_width(bitmap);
      al_draw_rotated_bitmap(bitmap, s / 2, s / 2,
         example.bitmap_x[i], example.bitmap_y[i], a, 0);
   }
}

static void redraw(void)
{
   al_set_target_bitmap(example.targets[1]);
   draw("filtered", example.bitmap_filter);
   al_set_target_bitmap(example.targets[0]);
   draw("no filter", example.bitmap_normal);
   al_set_target_bitmap(example.targets[3]);
   draw("filtered, multi-sample x4", example.bitmap_filter);
   al_set_target_bitmap(example.targets[2]);
   draw("no filter, multi-sample x4", example.bitmap_normal);

   float x = example.step_x;
   float y = example.step_y;

   if (example.update_step) {
      int i;
      for (i = 0; i < 4; i++) {
         al_set_target_bitmap(example.step[i]);
         al_reparent_bitmap(example.sub, example.targets[i], x, y, 30, 30);
         al_draw_bitmap(example.sub, 0, 0, 0);
      }
      example.update_step = false;
   }

   al_set_target_backbuffer(al_get_current_display());
   al_clear_to_color(al_map_rgb_f(0.5, 0, 0));
   al_draw_bitmap(example.targets[0], 20, 20, 0);
   al_draw_bitmap(example.targets[1], 20, 240, 0);
   al_draw_bitmap(example.targets[2], 340, 20, 0);
   al_draw_bitmap(example.targets[3], 340, 240, 0);

   al_draw_scaled_rotated_bitmap(example.step[0], 15, 15, 320 - 50, 220 - 50, 4, 4, 0, 0);
   al_draw_scaled_rotated_bitmap(example.step[1], 15, 15, 320 - 50, 440 - 50, 4, 4, 0, 0);
   al_draw_scaled_rotated_bitmap(example.step[2], 15, 15, 640 - 50, 220 - 50, 4, 4, 0, 0);
   al_draw_scaled_rotated_bitmap(example.step[3], 15, 15, 640 - 50, 440 - 50, 4, 4, 0, 0);

}

static void update(void)
{
   bitmap_move();
   if (example.step_t == 0) {
      example.step_x = example.bitmap_x[1] - 15;
      example.step_y = example.bitmap_y[1] - 15;
      example.step_t = 60;
      example.update_step = true;
   }
   example.step_t--;
}

int main(int argc, char **argv)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_DISPLAY *display;
   int w = 20 + 300 + 20 + 300 + 20, h = 20 + 200 + 20 + 200 + 20;
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

   while (!done) {
      ALLEGRO_EVENT event;

      if (need_redraw) {
         redraw();
         al_flip_display();
         need_redraw = false;
      }

      while (true) {
         al_wait_for_event(queue, &event);
         switch (event.type) {
            case ALLEGRO_EVENT_KEY_CHAR:
               if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                  done = true;
               break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
               done = true;
               break;

            case ALLEGRO_EVENT_TIMER:
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
