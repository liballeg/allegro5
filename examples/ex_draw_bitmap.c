#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

ALLEGRO_DEBUG_CHANNEL("main")

#define FPS 60
#define MAX_SPRITES 1024

typedef struct Sprite {
   float x, y, dx, dy;
} Sprite;

char const *text[] = {
   "H - toggle held drawing",
   "Space - toggle use of textures",
   "B - toggle alpha blending",
   "Left/Right - change bitmap size",
   "Up/Down - change bitmap count",
   "F1 - toggle help text"
};

struct Example {
   Sprite sprites[MAX_SPRITES];
   bool use_memory_bitmaps;
   int blending;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *mysha, *bitmap;
   bool hold_bitmap_drawing;
   int bitmap_size;
   int sprite_count;
   bool show_help;
   ALLEGRO_FONT *font;

   int t;

   ALLEGRO_COLOR white;
   ALLEGRO_COLOR half_white;
   ALLEGRO_COLOR dark;
   ALLEGRO_COLOR red;

   double direct_speed_measure;

   int ftpos;
   double frame_times[FPS];
} example;

static void add_time(void)
{
   example.frame_times[example.ftpos++] = al_get_time();
   if (example.ftpos >= FPS)
      example.ftpos = 0;
}

static void get_fps(int *average, int *minmax)
{
   int i;
   int prev = FPS - 1;
   double min_dt = 1;
   double max_dt = 1 / 1000000.0;
   double av = 0;
   double d;
   for (i = 0; i < FPS; i++) {
      if (i != example.ftpos) {
         double dt = example.frame_times[i] - example.frame_times[prev];
         if (dt < min_dt)
            min_dt = dt;
         if (dt > max_dt)
            max_dt = dt;
         av += dt;
      }
      prev = i;
   }
   av /= (FPS - 1);
   *average = ceil(1 / av);
   d = 1 / min_dt - 1 / max_dt;
   *minmax = floor(d / 2);
}

static void add_sprite(void)
{
   if (example.sprite_count < MAX_SPRITES) {
      int w = al_get_display_width(example.display);
      int h = al_get_display_height(example.display);
      int i = example.sprite_count++;
      Sprite *s = example.sprites + i;
      float a = rand() % 360;
      s->x = rand() % (w - example.bitmap_size);
      s->y = rand() % (h - example.bitmap_size);
      s->dx = cos(a) * FPS * 2;
      s->dy = sin(a) * FPS * 2;
   }
}

static void add_sprites(int n)
{
    int i;
    for (i = 0; i < n; i++)
       add_sprite();
}

static void remove_sprites(int n)
{
   example.sprite_count -= n;
   if (example.sprite_count < 0)
      example.sprite_count = 0;
}

static void change_size(int size)
{
   int bw, bh;
   if (size < 1)
      size = 1;
   if (size > 1024)
      size = 1024;

   if (example.bitmap)
      al_destroy_bitmap(example.bitmap);
   al_set_new_bitmap_flags(
      example.use_memory_bitmaps ? ALLEGRO_MEMORY_BITMAP : ALLEGRO_VIDEO_BITMAP);
   example.bitmap = al_create_bitmap(size, size);
   example.bitmap_size = size;
   al_set_target_bitmap(example.bitmap);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
   bw = al_get_bitmap_width(example.mysha);
   bh = al_get_bitmap_height(example.mysha);
   al_draw_scaled_bitmap(example.mysha, 0, 0, bw, bh, 0, 0,
      size, size * bh / bw, 0);
   al_set_target_backbuffer(example.display);
}

static void sprite_update(Sprite *s)
{
   int w = al_get_display_width(example.display);
   int h = al_get_display_height(example.display);

   s->x += s->dx / FPS;
   s->y += s->dy / FPS;

   if (s->x < 0) {
      s->x = -s->x;
      s->dx = -s->dx;
   }
   if (s->x + example.bitmap_size > w) {
      s->x = -s->x + 2 * (w - example.bitmap_size);
      s->dx = -s->dx;
   }
   if (s->y < 0) {
      s->y = -s->y;
      s->dy = -s->dy;
   }
   if (s->y + example.bitmap_size > h) {
      s->y = -s->y + 2 * (h - example.bitmap_size);
      s->dy = -s->dy;
   }

   if (example.bitmap_size > w) s->x = w / 2 - example.bitmap_size / 2;
   if (example.bitmap_size > h) s->y = h / 2 - example.bitmap_size / 2;
}

static void update(void)
{
   int i;
   for (i = 0; i < example.sprite_count; i++)
      sprite_update(example.sprites + i);

   example.t++;
   if (example.t == 60) {
      ALLEGRO_DEBUG("tick");
      example.t = 0;
   }
}

static void redraw(void)
{
   int w = al_get_display_width(example.display);
   int h = al_get_display_height(example.display);
   int i;
   int f1, f2;
   int fh = al_get_font_line_height(example.font);
   char const *info[] = {"textures", "memory buffers"};
   char const *binfo[] = {"alpha", "additive", "tinted", "solid", "alpha test"};
   ALLEGRO_COLOR tint = example.white;

   if (example.blending == 0) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
      tint = example.half_white;
   }
   else if (example.blending == 1) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ONE);
      tint = example.dark;
   }
   else if (example.blending == 2) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
      tint = example.red;
   }
   else if (example.blending == 3) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   }

   if (example.blending == 4) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
      al_set_render_state(ALLEGRO_ALPHA_TEST, true);
      al_set_render_state(ALLEGRO_ALPHA_FUNCTION, ALLEGRO_RENDER_GREATER);
      al_set_render_state(ALLEGRO_ALPHA_TEST_VALUE, 128);
   }
   else {
      al_set_render_state(ALLEGRO_ALPHA_TEST, false);
   }

   if (example.hold_bitmap_drawing) {
      al_hold_bitmap_drawing(true);
   }
   for (i = 0; i < example.sprite_count; i++) {
      Sprite *s = example.sprites + i;
      al_draw_tinted_bitmap(example.bitmap, tint, s->x, s->y, 0);
   }
   if (example.hold_bitmap_drawing) {
      al_hold_bitmap_drawing(false);
   }

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   if (example.show_help) {
      int dh = fh * 3.5;
      for (i = 5; i >= 0; i--) {
         al_draw_text(example.font, example.white, 0, h - dh, 0, text[i]);
         dh += fh * 6;
      }
   }

   al_draw_textf(example.font, example.white, 0, 0, 0, "count: %d",
      example.sprite_count);
   al_draw_textf(example.font, example.white, 0, fh, 0, "size: %d",
      example.bitmap_size);
   al_draw_textf(example.font, example.white, 0, fh * 2, 0, "%s",
      info[example.use_memory_bitmaps]);
   al_draw_textf(example.font, example.white, 0, fh * 3, 0, "%s",
      binfo[example.blending]);

   get_fps(&f1, &f2);
   al_draw_textf(example.font, example.white, w, 0, ALLEGRO_ALIGN_RIGHT, "FPS: %4d +- %-4d",
      f1, f2);
   al_draw_textf(example.font, example.white, w, fh, ALLEGRO_ALIGN_RIGHT, "%4d / sec",
      (int)(1.0 / example.direct_speed_measure));

}

int main(int argc, char **argv)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_MONITOR_INFO info;
   const char* bitmap_filename;
   int w = 640, h = 480;
   bool done = false;
   bool need_redraw = true;
   bool background = false;
   example.show_help = true;
   example.hold_bitmap_drawing = false;

   if (argc > 1) {
      bitmap_filename = argv[1];
   }
   else {
      bitmap_filename = "data/mysha256x256.png";
   }

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   al_init_font_addon();
   init_platform_specific();

   al_get_num_video_adapters();

   al_get_monitor_info(0, &info);

   al_set_new_display_option(ALLEGRO_SUPPORTED_ORIENTATIONS,
                             ALLEGRO_DISPLAY_ORIENTATION_ALL, ALLEGRO_SUGGEST);
   example.display = al_create_display(w, h);
   if (!example.display) {
      abort_example("Error creating display.\n");
   }

   w = al_get_display_width(example.display);
   h = al_get_display_height(example.display);

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   if (!al_install_mouse()) {
      abort_example("Error installing mouse.\n");
   }

   al_install_touch_input();

   example.font = al_create_builtin_font();
   if (!example.font) {
      abort_example("Error creating builtin font\n");
   }

   example.mysha = al_load_bitmap(bitmap_filename);
   if (!example.mysha) {
      abort_example("Error loading %s\n", bitmap_filename);
   }

   example.white = al_map_rgb_f(1, 1, 1);
   example.half_white = al_map_rgba_f(1, 1, 1, 0.5);
   example.dark = al_map_rgb(15, 15, 15);
   example.red = al_map_rgb_f(1, 0.2, 0.1);
   change_size(256);
   add_sprite();
   add_sprite();

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   if (al_install_touch_input())
      al_register_event_source(queue, al_get_touch_input_event_source());
   al_register_event_source(queue, al_get_display_event_source(example.display));

   al_start_timer(timer);

   while (!done) {
      float x, y;
      ALLEGRO_EVENT event;
      w = al_get_display_width(example.display);
      h = al_get_display_height(example.display);

      if (!background && need_redraw && al_is_event_queue_empty(queue)) {
         double t = -al_get_time();
         add_time();
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         redraw();
         t += al_get_time();
         example.direct_speed_measure  = t;
         al_flip_display();
         need_redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_CHAR: /* includes repeats */
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            else if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
               add_sprites(1);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
               remove_sprites(1);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
               change_size(example.bitmap_size - 1);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
               change_size(example.bitmap_size + 1);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_F1) {
               example.show_help ^= 1;
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
               example.use_memory_bitmaps ^= 1;
               change_size(example.bitmap_size);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_B) {
               example.blending++;
               if (example.blending == 5)
                  example.blending = 0;
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_H) {
               example.hold_bitmap_drawing ^= 1;
            }
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
            al_acknowledge_drawing_resume(event.display.source);
            break;

         case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event.display.source);
            break;

         case ALLEGRO_EVENT_TIMER:
            update();
            need_redraw = true;
            break;

         case ALLEGRO_EVENT_TOUCH_BEGIN:
            x = event.touch.x;
            y = event.touch.y;
            goto click;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            x = event.mouse.x;
            y = event.mouse.y;
            goto click;

         click:
         {
            int fh = al_get_font_line_height(example.font);

            if (x < fh * 12 && y >= h - fh * 30) {
               int button = (y - (h - fh * 30)) / (fh * 6);
               if (button == 0) {
                  example.use_memory_bitmaps ^= 1;
                  change_size(example.bitmap_size);
               }
               if (button == 1) {
                  example.blending++;
                  if (example.blending == 5)
                     example.blending = 0;
               }
               if (button == 3) {
                  if (x < fh * 6)
                     remove_sprites(example.sprite_count / 2);
                  else
                     add_sprites(example.sprite_count);
               }
               if (button == 2) {
                  int s = example.bitmap_size * 2;
                  if (x < fh * 6)
                     s = example.bitmap_size / 2;
                  change_size(s);
               }
               if (button == 4) {
                  example.show_help ^= 1;
               }

            }
            break;
         }
      }
   }

   al_destroy_bitmap(example.bitmap);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
