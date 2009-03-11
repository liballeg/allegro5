/* Tests some drawing primitives.
 */

#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_color.h>
#include <allegro5/a5_primitives.h>
#include <stdio.h>
#include <stdarg.h>

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

   int what;
} ex;

char const *names[] = {
   "filled rectangles",
   "rectangles",
   "circles",
   "filled circles",
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

static void get_xy(float *x, float *y)
{
    *x = ex.text_x;
    *y = ex.text_y;
}

static void print(char const *format, ...)
{
   va_list list;
   char message[1024];
   ALLEGRO_STATE state;
   int th = al_font_text_height(ex.font);
   al_store_state(&state, ALLEGRO_STATE_BLENDER);

   va_start(list, format);
   uvszprintf(message, sizeof message, format, list);
   va_end(list);
   
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, ex.text);
   al_font_textprintf(ex.font, ex.text_x, ex.text_y, "%s", message);
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
   int w = ex.what;
   if (w == 0 && never_fill) w = 1;
   if (w == 2 && never_fill) w = 3;
   if (w == 0) al_draw_filled_rectangle(l, t, r, b, color);
   if (w == 1) al_draw_rectangle(l, t, r, b, color, 0);
   if (w == 2) al_draw_filled_ellipse(cx, cy, rx, ry, color);
   if (w == 3) al_draw_ellipse(cx, cy, rx, ry, color, 0);
   if (w == 4) al_draw_line(l, t, r, b, color, 0);
}

static void draw(void)
{
   float x, y;
   int cx, cy, cw, ch;
   int w = al_get_bitmap_width(ex.zoom);
   int h = al_get_bitmap_height(ex.zoom);
   ALLEGRO_BITMAP *screen = al_get_target_bitmap();
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
   al_clear(ex.background);

   set_xy(8, 0);
   print("Drawing %s (press SPACE to change)", names[ex.what]);

   set_xy(8, 16);
   print("Original");

   set_xy(80, 16);
   print("Enlarged x 16");

   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, ex.white);

   x = 8;
   y = 40;
   al_draw_bitmap(ex.pattern, x, y, 0);
   
   /* Draw the test scene. */
   for (i = 0; i < rects_num; i++)
      primitive(
         x + rects[i * 4 + 0],
         y + rects[i * 4 + 1],
         x + rects[i * 4 + 2],
         y + rects[i * 4 + 3],
         ex.foreground, false);

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
      if (need_draw && al_event_queue_is_empty(ex.queue)) {
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

   ex.font = al_font_load_font("data/fixed_font.tga", 0);
   if (!ex.font) {
      printf("data/fixed_font.tga not found.\n");
      exit(1);
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

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;

   if (!al_init()) {
      printf("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();
   al_font_init();

   display = al_create_display(640, 640);
   if (!display) {
      printf("Error creating display.\n");
      return 1;
   }

   init();

   timer = al_install_timer(1.0 / ex.FPS);

   ex.queue = al_create_event_queue();
   al_register_event_source(ex.queue, (void *)al_get_keyboard());
   al_register_event_source(ex.queue, (void *)al_get_mouse());
   al_register_event_source(ex.queue, (void *)display);
   al_register_event_source(ex.queue, (void *)timer);

   al_start_timer(timer);
   run();

   al_destroy_event_queue(ex.queue);  

   return 0;
}
END_OF_MAIN()
