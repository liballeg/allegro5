/* An example demonstrating different blending modes.
 */
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_color.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "common.c"

struct Example
{
   ALLEGRO_BITMAP *pattern;
   ALLEGRO_FONT *font;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_COLOR background, text, white;

   double timer[4], counter[4];
   int FPS;
   float text_x, text_y;
} ex;

static ALLEGRO_BITMAP *example_bitmap(int w, int h)
{
   int i, j;
   float mx = w * 0.5;
   float my = h * 0.5;
   ALLEGRO_STATE state;
   ALLEGRO_BITMAP *pattern = al_create_bitmap(w, h);
   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(pattern);
   al_lock_bitmap(pattern, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
   for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
         float a = atan2(i - mx, j - my);
         float d = sqrt(pow(i - mx, 2) + pow(j - my, 2));
         float sat = pow(1.0 - 1 / (1 + d * 0.1), 5);
         float hue = 3 * a * 180 / ALLEGRO_PI;
         hue = (hue / 360 - floorf(hue / 360)) * 360;
         al_put_pixel(i, j, al_color_hsv(hue, sat, 1));
      }
   }
   al_put_pixel(0, 0, al_map_rgb(0, 0, 0));
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
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
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
   ALLEGRO_BITMAP *screen, *temp;
   ALLEGRO_LOCKED_REGION *lock;
   void *data;
   int size, i, format;
   
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

   al_clear_to_color(ex.background);

   screen = al_get_target_bitmap();

   set_xy(8, 8);

   /* Test 1. */
   /* Disabled: drawing to same bitmap is not supported. */
   /*
   print("Screen -> Screen (%.1f fps)", get_fps(0));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   start_timer(0);
   al_draw_bitmap_region(screen, x, y, iw, ih, x + 8 + iw, y, 0);
   stop_timer(0);
   set_xy(x, y + ih);
   */

   /* Test 2. */
   print("Screen -> Bitmap -> Screen (%.1f fps)", get_fps(1));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   temp = al_create_bitmap(iw, ih);
   al_set_target_bitmap(temp);
   al_clear_to_color(al_map_rgba_f(1, 0, 0, 1));
   start_timer(1);
   al_draw_bitmap_region(screen, x, y, iw, ih, 0, 0, 0);

   al_set_target_bitmap(screen);
   al_draw_bitmap(temp, x + 8 + iw, y, 0);
   stop_timer(1);
   set_xy(x, y + ih);
   
   al_destroy_bitmap(temp);

   /* Test 3. */
   print("Screen -> Memory -> Screen (%.1f fps)", get_fps(2));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   temp = al_create_bitmap(iw, ih);
   al_set_target_bitmap(temp);
   al_clear_to_color(al_map_rgba_f(1, 0, 0, 1));
   start_timer(2);
   al_draw_bitmap_region(screen, x, y, iw, ih, 0, 0, 0);

   al_set_target_bitmap(screen);
   al_draw_bitmap(temp, x + 8 + iw, y, 0);
   stop_timer(2);
   set_xy(x, y + ih);
   
   al_destroy_bitmap(temp);
   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);

   /* Test 4. */
   print("Screen -> Locked -> Screen (%.1f fps)", get_fps(3));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   start_timer(3);
   lock = al_lock_bitmap_region(screen, x, y, iw, ih,
      ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);
   format = lock->format;
   size = lock->pixel_size;
   data = malloc(size * iw * ih);
   for (i = 0; i < ih; i++)
      memcpy((char*)data + i * size * iw,
         (char*)lock->data + i * lock->pitch, size * iw);
   al_unlock_bitmap(screen);
   
   lock = al_lock_bitmap_region(screen, x + 8 + iw, y, iw, ih, format,
      ALLEGRO_LOCK_WRITEONLY);
   for (i = 0; i < ih; i++)
      memcpy((char*)lock->data + i * lock->pitch,
         (char*)data + i * size * iw, size * iw);
   al_unlock_bitmap(screen);
   free(data);
   stop_timer(3);
   set_xy(x, y + ih);

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
      abort_example("data/fixed_font.tga not found\n");
   }
   ex.background = al_color_name("beige");
   ex.text = al_color_name("black");
   ex.white = al_color_name("white");
   ex.pattern = example_bitmap(100, 100);
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();
   al_install_mouse();
   al_init_image_addon();
   al_init_font_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
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
