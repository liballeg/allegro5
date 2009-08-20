#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60
#define MAX_SPRITES 1024

typedef struct Sprite {
   float x, y, dx, dy;
} Sprite;

char const *text[] = {
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
   ALLEGRO_BITMAP *mysha, *bitmap;
   int bitmap_size;
   int sprite_count;
   bool show_help;
   ALLEGRO_FONT *font;

   ALLEGRO_COLOR white;
   ALLEGRO_COLOR half_white;
   ALLEGRO_COLOR dark;
   ALLEGRO_COLOR red;

   int ftpos;
   double frame_times[FPS];
} example;

static void add_time(void)
{
   example.frame_times[example.ftpos++] = al_current_time();
   if (example.ftpos >= FPS) example.ftpos = 0;
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
         if (dt < min_dt) min_dt = dt;
         if (dt > max_dt) max_dt = dt;
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
   int w = al_get_display_width();
   int h = al_get_display_height();
   int i = example.sprite_count++;
   if (example.sprite_count >= MAX_SPRITES)
      example.sprite_count = MAX_SPRITES - 1;
   Sprite *s = example.sprites + i;
   float a = rand() % 360;
   s->x = rand() % (w - example.bitmap_size);
   s->y = rand() % (h - example.bitmap_size);
   s->dx = cos(a) * FPS * 2;
   s->dy = sin(a) * FPS * 2;
}

static void remove_sprite(void)
{
   if (example.sprite_count > 0) example.sprite_count--;
}

static void change_size(int size)
{
   if (example.bitmap) al_destroy_bitmap(example.bitmap);
   al_set_new_bitmap_flags(example.use_memory_bitmaps ? ALLEGRO_MEMORY_BITMAP : 0);
   example.bitmap = al_create_bitmap(size, size);
   example.bitmap_size = size;
   al_set_target_bitmap(example.bitmap);
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, example.white);
   al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
   al_draw_bitmap(example.mysha, size / 2 - 160, size / 2 - 100, 0);
   al_set_target_bitmap(al_get_backbuffer());
}

static void sprite_update(Sprite *s)
{
   int w = al_get_display_width();
   int h = al_get_display_height();

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
}

static void update(void)
{
   int i;
   for (i = 0; i < example.sprite_count; i++)
      sprite_update(example.sprites + i);
}

static void redraw(void)
{
   int w = al_get_display_width();
   int i;
   int f1, f2;
   int fh = al_get_font_line_height(example.font);
   char const *info[] = {"textures", "memory buffers"};
   char const *binfo[] = {"alpha", "additive", "tinted", "solid"};

   if (example.blending == 0)
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, example.half_white);
   else if (example.blending == 1)
      al_set_blender(ALLEGRO_ONE, ALLEGRO_ONE, example.dark);
   else if (example.blending == 2)
      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, example.red);
   else if (example.blending == 3)
      al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, example.white);

   for (i = 0; i < example.sprite_count; i++) {
      Sprite *s = example.sprites + i;
      al_draw_bitmap(example.bitmap, s->x, s->y, 0);
   }

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, example.white);
   if (example.show_help) {
      for (i = 0; i < 5; i++)
         al_draw_text(example.font, 0, i * fh, 0, text[i]);
   }

   al_draw_textf(example.font, w / 2, 0, ALLEGRO_ALIGN_CENTRE, "count: %d",
      example.sprite_count);
   al_draw_textf(example.font, w / 2, fh, ALLEGRO_ALIGN_CENTRE, "size: %d",
      example.bitmap_size);
   al_draw_textf(example.font, w / 2, fh * 2, ALLEGRO_ALIGN_CENTRE, "%s",
      info[example.use_memory_bitmaps]);
   al_draw_textf(example.font, w / 2, fh * 3, ALLEGRO_ALIGN_CENTRE, "%s",
      binfo[example.blending]);

   get_fps(&f1, &f2);
   al_draw_textf(example.font, w, 0, ALLEGRO_ALIGN_RIGHT, "FPS: %4d +- %-4d",
      f1, f2);
   
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   bool done = false;
   bool need_redraw = true;
   example.show_help = true;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
      return 1;
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
      return 1;
   }

   al_init_font_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
      return 1;
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
      return 1;
   }

   example.font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!example.font) {
      abort_example("Error loading data/fixed_font.tga\n");
      return 1;
   }

   example.mysha = al_load_bitmap("data/mysha.pcx");
   if (!example.mysha) {
      abort_example("Error loading data/mysha.pcx\n");
      return 1;
   }

   example.white = al_map_rgb_f(1, 1, 1);
   example.half_white = al_map_rgba_f(1, 1, 1, 0.5);
   example.dark = al_map_rgb(15, 15, 15);
   example.red = al_map_rgb_f(1, 0.2, 0.1);
   change_size(256);
   add_sprite();

   timer = al_install_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   while (!done) {
      ALLEGRO_EVENT event;

      if (need_redraw && al_event_queue_is_empty(queue)) {
         add_time();
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         redraw();
         al_flip_display();
         need_redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
         case ALLEGRO_EVENT_KEY_REPEAT:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            else if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
               add_sprite();
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
               remove_sprite();
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
               if (example.bitmap_size > 1) change_size(example.bitmap_size - 1);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
               if (example.bitmap_size < 1024) change_size(example.bitmap_size + 1);
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
               if (example.blending == 4)
                  example.blending = 0;
            }
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            update();
            need_redraw = true;
            break;
      }
   }

   al_destroy_bitmap(example.bitmap);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
