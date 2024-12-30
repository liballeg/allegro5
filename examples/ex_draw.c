/* Tests some drawing primitives.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.c"

struct Example
{
   ALLEGRO_FONT *font;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_COLOR background, text, white, foreground;
   ALLEGRO_COLOR outline;
   ALLEGRO_BITMAP *pattern;
   ALLEGRO_BITMAP *zoom;

   double timer[4], counter[4];
   int FPS;
   float text_x, text_y;

   bool software;
   int samples;
   int what;
   int thickness;
} ex;

char const *names[] = {
   "filled rectangles",
   "rectangles",
   "filled circles",
   "circles",
   "lines"
};

static void draw_pattern(ALLEGRO_BITMAP *b)
{
   int w = al_get_bitmap_width(b);
   int h = al_get_bitmap_height(b);
   int x, y;
   int format = ALLEGRO_PIXEL_FORMAT_BGR_888;
   ALLEGRO_COLOR light = al_map_rgb_f(1, 1, 1);
   ALLEGRO_COLOR dark = al_map_rgb_f(1, 0.9, 0.8);
   ALLEGRO_LOCKED_REGION *lock;
   lock = al_lock_bitmap(b, format, ALLEGRO_LOCK_WRITEONLY);
   for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
         ALLEGRO_COLOR c = (x + y) & 1 ? light : dark;
         unsigned char r, g, b;
         unsigned char *data = lock->data;
         al_unmap_rgb(c, &r, &g, &b);
         data += y * lock->pitch;
         data += x * 3;
         data[0] = r;
         data[1] = g;
         data[2] = b;
      }
   }
   al_unlock_bitmap(b);
}

static void set_xy(float x, float y)
{
   ex.text_x = x;
   ex.text_y = y;
}

static void print(char const *format, ...)
{
   va_list list;
   char message[1024];
   ALLEGRO_STATE state;
   int th = al_get_font_line_height(ex.font);
   al_store_state(&state, ALLEGRO_STATE_BLENDER);

   va_start(list, format);
   vsnprintf(message, sizeof message, format, list);
   va_end(list);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   al_draw_textf(ex.font, ex.text, ex.text_x, ex.text_y, 0, "%s", message);
   al_restore_state(&state);

   ex.text_y += th;
}

static void primitive(float l, float t, float r, float b,
   ALLEGRO_COLOR color, bool never_fill)
{
   float cx = (l + r) / 2;
   float cy = (t + b) / 2;
   float rx = (r - l) / 2;
   float ry = (b - t) / 2;
   int tk = never_fill ? 0 : ex.thickness;
   int w = ex.what;
   if (w == 0 && never_fill) w = 1;
   if (w == 2 && never_fill) w = 3;
   if (w == 0) al_draw_filled_rectangle(l, t, r, b, color);
   if (w == 1) al_draw_rectangle(l, t, r, b, color, tk);
   if (w == 2) al_draw_filled_ellipse(cx, cy, rx, ry, color);
   if (w == 3) al_draw_ellipse(cx, cy, rx, ry, color, tk);
   if (w == 4) al_draw_line(l, t, r, b, color, tk);
}

static void draw(void)
{
   float x, y;
   int cx, cy, cw, ch;
   int w = al_get_bitmap_width(ex.zoom);
   int h = al_get_bitmap_height(ex.zoom);
   ALLEGRO_BITMAP *screen = al_get_target_bitmap();
   ALLEGRO_BITMAP *mem;
   int rects_num = 16, i, j;
   float rects[16 * 4];
   for (j = 0; j < 4; j++) {
      for (i = 0; i < 4; i++) {
         rects[(j * 4 + i) * 4 + 0] = 2 + i * 0.25 + i * 7;
         rects[(j * 4 + i) * 4 + 1] = 2 + j * 0.25 + j * 7;
         rects[(j * 4 + i) * 4 + 2] = 2 + i * 0.25 + i * 7 + 5;
         rects[(j * 4 + i) * 4 + 3] = 2 + j * 0.25 + j * 7 + 5;
      }
   }

   al_get_clipping_rectangle(&cx, &cy, &cw, &ch);
   al_clear_to_color(ex.background);

   set_xy(8, 0);
   print("Drawing %s (press SPACE to change)", names[ex.what]);

   set_xy(8, 16);
   print("Original");

   set_xy(80, 16);
   print("Enlarged x 16");

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

   if (ex.software) {
      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      al_set_new_bitmap_format(al_get_bitmap_format(al_get_target_bitmap()));
      mem = al_create_bitmap(w, h);
      al_set_target_bitmap(mem);
      x = 0;
      y = 0;
   }
   else {
      mem = NULL;
      x = 8;
      y = 40;
   }
   al_draw_bitmap(ex.pattern, x, y, 0);

   /* Draw the test scene. */

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   for (i = 0; i < rects_num; i++) {
      ALLEGRO_COLOR rgba = ex.foreground;
      rgba.a *= 0.5;
      primitive(
         x + rects[i * 4 + 0],
         y + rects[i * 4 + 1],
         x + rects[i * 4 + 2],
         y + rects[i * 4 + 3],
         rgba, false);
   }

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

   if (ex.software) {
      al_set_target_bitmap(screen);
      x = 8;
      y = 40;
      al_draw_bitmap(mem, x, y, 0);
      al_destroy_bitmap(mem);
   }

   /* Grab screen contents into our bitmap. */
   al_set_target_bitmap(ex.zoom);
   al_draw_bitmap_region(screen, x, y, w, h, 0, 0, 0);
   al_set_target_bitmap(screen);

   /* Draw it enlarged. */
   x = 80;
   y = 40;
   al_draw_scaled_bitmap(ex.zoom, 0, 0, w, h, x, y, w * 16, h * 16, 0);

   /* Draw outlines. */
   for (i = 0; i < rects_num; i++) {
      primitive(
         x + rects[i * 4 + 0] * 16,
         y + rects[i * 4 + 1] * 16,
         x + rects[i * 4 + 2] * 16,
         y + rects[i * 4 + 3] * 16,
         ex.outline, true);
   }

   set_xy(8, 640 - 48);
   print("Thickness: %d (press T to change)", ex.thickness);
   print("Drawing with: %s (press S to change)",
      ex.software ? "software" : "hardware");
   print("Supersampling: %dx (edit ex_draw.ini to change)", ex.samples);

// FIXME: doesn't work
//      al_get_display_option(ALLEGRO_SAMPLE_BUFFERS));
}

static void tick(void)
{
   draw();
   al_flip_display();
}

static void run(void)
{
   ALLEGRO_EVENT event;
   bool need_draw = true;

   while (1) {
      if (need_draw && al_is_event_queue_empty(ex.queue)) {
         tick();
         need_draw = false;
      }

      al_wait_for_event(ex.queue, &event);

      switch (event.type) {
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               return;
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
               ex.what++;
               if (ex.what == 5) ex.what = 0;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_S) {
               ex.software = !ex.software;
            }
            if (event.keyboard.keycode == ALLEGRO_KEY_T) {
               ex.thickness++;
               if (ex.thickness == 2) ex.thickness = 0;
            }
            break;

         case ALLEGRO_EVENT_TIMER:
            need_draw = true;
            break;
      }
   }
}

static void init(void)
{
   ex.FPS = 60;

   ex.font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!ex.font) {
      abort_example("data/fixed_font.tga not found.\n");
   }
   ex.background = al_color_name("beige");
   ex.foreground = al_color_name("black");
   ex.outline = al_color_name("red");
   ex.text = al_color_name("blue");
   ex.white = al_color_name("white");
   ex.pattern = al_create_bitmap(32, 32);
   ex.zoom = al_create_bitmap(32, 32);
   draw_pattern(ex.pattern);
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_CONFIG *config;
   char const *value;
   char str[256];

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();
   al_init_image_addon();
   al_init_font_addon();
   init_platform_specific();

   /* Read supersampling info from ex_draw.ini. */
   ex.samples = 0;
   config = al_load_config_file("ex_draw.ini");
   if (!config)
      config = al_create_config();
   value = al_get_config_value(config, "settings", "samples");
   if (value)
      ex.samples = strtol(value, NULL, 0);
   sprintf(str, "%d", ex.samples);
   al_set_config_value(config, "settings", "samples", str);
   al_save_config_file("ex_draw.ini", config);
   al_destroy_config(config);

   if (ex.samples) {
      al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);
      al_set_new_display_option(ALLEGRO_SAMPLES, ex.samples, ALLEGRO_SUGGEST);
   }
   display = al_create_display(640, 640);
   if (!display) {
      abort_example("Unable to create display.\n");
   }

   init();

   timer = al_create_timer(1.0 / ex.FPS);

   ex.queue = al_create_event_queue();
   al_register_event_source(ex.queue, al_get_keyboard_event_source());
   al_register_event_source(ex.queue, al_get_mouse_event_source());
   al_register_event_source(ex.queue, al_get_display_event_source(display));
   al_register_event_source(ex.queue, al_get_timer_event_source(timer));

   al_start_timer(timer);
   run();

   al_destroy_event_queue(ex.queue);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
