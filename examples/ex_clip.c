/* Test performance of al_draw_bitmap_region, al_create_sub_bitmap and
 * al_set_clipping_rectangle when clipping a bitmap.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "common.c"

struct Example
{
   A5O_BITMAP *pattern;
   A5O_FONT *font;
   A5O_EVENT_QUEUE *queue;
   A5O_COLOR background, text, white;

   double timer[4], counter[4];
   int FPS;
   float text_x, text_y;
} ex;

static A5O_BITMAP *example_bitmap(int w, int h)
{
   int i, j;
   float mx = w * 0.5;
   float my = h * 0.5;
   A5O_STATE state;
   A5O_BITMAP *pattern = al_create_bitmap(w, h);
   al_store_state(&state, A5O_STATE_TARGET_BITMAP);
   al_set_target_bitmap(pattern);
   al_lock_bitmap(pattern, A5O_PIXEL_FORMAT_ANY, A5O_LOCK_WRITEONLY);
   for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
         float a = atan2(i - mx, j - my);
         float d = sqrt(pow(i - mx, 2) + pow(j - my, 2));
         float l = 1 - pow(1.0 - 1 / (1 + d * 0.1), 5);
         float hue = a * 180 / A5O_PI;
         float sat = 1;
         if (i == 0 || j == 0 || i == w - 1 || j == h - 1) {
            hue += 180;
         }
         else if (i == 1 || j == 1 || i == w - 2 || j == h - 2) {
            hue += 180;
            sat = 0.5;
         }
         al_put_pixel(i, j, al_color_hsl(hue, sat, l));
      }
   }
   al_unlock_bitmap(pattern);
   al_restore_state(&state);
   return pattern;
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
   int th = al_get_font_line_height(ex.font);

   va_start(list, format);
   vsnprintf(message, sizeof message, format, list);
   va_end(list);

   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
   al_draw_textf(ex.font, ex.text, ex.text_x, ex.text_y, 0, "%s", message);

   ex.text_y += th;   
}

static void start_timer(int i)
{
   ex.timer[i] -= al_get_time();
   ex.counter[i]++;
}

static void stop_timer(int i)
{
    ex.timer[i] += al_get_time();
}

static double get_fps(int i)
{
   if (ex.timer[i] == 0)
      return 0;
   return ex.counter[i] / ex.timer[i];
}

static void draw(void)
{
   float x, y;
   int iw = al_get_bitmap_width(ex.pattern);
   int ih = al_get_bitmap_height(ex.pattern);
   A5O_BITMAP *temp;
   int cx, cy, cw, ch, gap = 8;
   
   al_get_clipping_rectangle(&cx, &cy, &cw, &ch);
   
   al_set_blender(A5O_ADD, A5O_ONE, A5O_ZERO);

   al_clear_to_color(ex.background);

   /* Test 1. */
   set_xy(8, 8);
   print("al_draw_bitmap_region (%.1f fps)", get_fps(0));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   start_timer(0);
   al_draw_bitmap_region(ex.pattern, 1, 1, iw - 2, ih - 2,
      x + 8 + iw + 1, y + 1, 0);
   stop_timer(0);
   set_xy(x, y + ih + gap);

   /* Test 2. */
   print("al_create_sub_bitmap (%.1f fps)", get_fps(1));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   start_timer(1);
   temp = al_create_sub_bitmap(ex.pattern, 1, 1, iw - 2, ih - 2);
   al_draw_bitmap(temp, x + 8 + iw + 1, y + 1, 0);
   al_destroy_bitmap(temp);
   stop_timer(1);
   set_xy(x, y + ih + gap);

   /* Test 3. */
   print("al_set_clipping_rectangle (%.1f fps)", get_fps(2));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   start_timer(2);
   al_set_clipping_rectangle(x + 8 + iw + 1, y + 1, iw - 2, ih - 2);
   al_draw_bitmap(ex.pattern, x + 8 + iw, y, 0);
   al_set_clipping_rectangle(cx, cy, cw, ch);
   stop_timer(2);
   set_xy(x, y + ih + gap);
}

static void tick(void)
{
   draw();
   al_flip_display();
}

static void run(void)
{
   A5O_EVENT event;
   bool need_draw = true;

   while (1) {
      if (need_draw && al_is_event_queue_empty(ex.queue)) {
         tick();
         need_draw = false;
      }

      al_wait_for_event(ex.queue, &event);

      switch (event.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            return;

         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE)
               return;
            break;

         case A5O_EVENT_TIMER:
            need_draw = true;
            break;
      }
   }
}

static void init(void)
{
   ex.FPS = 60;

   ex.font = al_create_builtin_font();
   if (!ex.font) {
      abort_example("Error creating builtin font.\n");
   }
   ex.background = al_color_name("beige");
   ex.text = al_color_name("blue");
   ex.white = al_color_name("white");
   ex.pattern = example_bitmap(100, 100);
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();
   al_install_mouse();
   al_init_font_addon();
   init_platform_specific();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
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
