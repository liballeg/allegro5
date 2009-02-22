/* An example demonstrating different blending modes.
 */
#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_color.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

struct Example
{
   ALLEGRO_BITMAP *pattern;
   ALLEGRO_FONT *font;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_COLOR background, text;

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
   ALLEGRO_LOCKED_REGION lock;
   ALLEGRO_BITMAP *pattern = al_create_bitmap(w, h);
   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(pattern);
   al_lock_bitmap(pattern, &lock, ALLEGRO_LOCK_WRITEONLY);
   for (i = 0; i < w; i++) {
      for (j = 0; j < h; j++) {
         float a = atan2(i - mx, j - my);
         float d = sqrt(pow(i - mx, 2) + pow(j - my, 2));
         float sat = pow(1.0 - 1 / (1 + d * 0.1), 5);
         float hue = 3 * a * 180 / AL_PI;
         hue = (hue / 360 - floor(hue / 360)) * 360;
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

static void start_timer(int i)
{
   ex.timer[i] -= al_current_time();
   ex.counter[i]++;
}

static void stop_timer(int i)
{
    ex.timer[i] += al_current_time();
}

static double get_fps(int i)
{
   if (ex.timer[i] == 0) return 0;
   return ex.counter[i] / ex.timer[i];
}

static void draw(void)
{
   float x, y;
   int iw = al_get_bitmap_width(ex.pattern);
   int ih = al_get_bitmap_height(ex.pattern);
   ALLEGRO_BITMAP *screen, *temp;
   ALLEGRO_LOCKED_REGION lock;
   void *data;
   int size, i;
   
   al_clear(ex.background);

   screen = al_get_target_bitmap();

   /* Test 1. */
   set_xy(8, 8);
   print("Screen -> Screen (%.1f fps)", get_fps(0));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   start_timer(0);
   al_draw_bitmap_region(screen, x, y, iw, ih, x + 8 + iw, y, 0);
   stop_timer(0);
   set_xy(x, y + ih);

   /* Test 2. */
   print("Screen -> Bitmap -> Screen (%.1f fps)", get_fps(1));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   temp = al_create_bitmap(iw, ih);
   al_set_target_bitmap(temp);
   al_clear(al_map_rgba_f(1, 0, 0, 1));
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
   al_clear(al_map_rgba_f(1, 0, 0, 1));
   start_timer(2);
   al_draw_bitmap_region(screen, x, y, iw, ih, 0, 0, 0);

   al_set_target_bitmap(screen);
   al_draw_bitmap(temp, x + 8 + iw, y, 0);
   stop_timer(2);
   set_xy(x, y + ih);
   
   al_destroy_bitmap(temp);
   al_set_new_bitmap_flags(0);
   
   /* Test 4. */
   print("Screen -> Locked -> Screen (%.1f fps)", get_fps(3));
   get_xy(&x, &y);
   al_draw_bitmap(ex.pattern, x, y, 0);

   size = al_get_pixel_size(al_get_display_format());
   data = malloc(size * iw * ih);
   start_timer(3);
   al_lock_bitmap_region(screen, x, y, iw, ih, &lock, ALLEGRO_LOCK_READONLY);
   for (i = 0; i < ih; i++)
      memcpy((char*)data + i * size * iw, (char*)lock.data + i * lock.pitch, size * iw);
   al_unlock_bitmap(screen);
   
   al_lock_bitmap_region(screen, x + 8 + iw, y,  iw, ih, &lock, ALLEGRO_LOCK_WRITEONLY);
   for (i = 0; i < ih; i++)
      memcpy((char*)lock.data + i * lock.pitch, (char*)data + i * size * iw, size * iw);
   al_unlock_bitmap(screen);
   stop_timer(3);
   free(data);
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
      printf("data/fixed_font.tga not found\n");
      exit(1);
   }
   ex.background = al_color_name("beige");
   ex.text = al_color_name("black");
   ex.pattern = example_bitmap(100, 100);
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();
   al_font_init();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
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
