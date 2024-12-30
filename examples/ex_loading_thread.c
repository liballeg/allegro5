#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

static const int load_total = 100;
static int load_count = 0;
static ALLEGRO_BITMAP *bitmaps[100];
static ALLEGRO_MUTEX *mutex;

static void *loading_thread(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_FONT *font;
   ALLEGRO_COLOR text;

   font = al_load_font("data/fixed_font.tga", 0, 0);
   text = al_map_rgb_f(255, 255, 255);

   /* In this example we load mysha.pcx 100 times to simulate loading
    * many bitmaps.
    */
   load_count = 0;
   while (load_count < load_total) {
      ALLEGRO_COLOR color;
      ALLEGRO_BITMAP *bmp;

      color = al_map_rgb(rand() % 256, rand() % 256, rand() % 256);
      color.r /= 4;
      color.g /= 4;
      color.b /= 4;
      color.a /= 4;

      if (al_get_thread_should_stop(thread))
         break;

      bmp = al_load_bitmap("data/mysha.pcx");

      /* Simulate different contents. */
      al_set_target_bitmap(bmp);
      al_draw_filled_rectangle(0, 0, 320, 200, color);
      al_draw_textf(font, text, 0, 0, 0, "bitmap %d", 1 + load_count);
      al_set_target_bitmap(NULL);

      /* Allow the main thread to see the completed bitmap. */
      al_lock_mutex(mutex);
      bitmaps[load_count] = bmp;
      load_count++;
      al_unlock_mutex(mutex);

      /* Simulate that it's slow. */
      al_rest(0.05);
   }

   al_destroy_font(font);
   return arg;
}

static void print_bitmap_flags(ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_USTR *ustr = al_ustr_new("");
   if (al_get_bitmap_flags(bitmap) & ALLEGRO_VIDEO_BITMAP)
      al_ustr_append_cstr(ustr, " VIDEO");
   if (al_get_bitmap_flags(bitmap) & ALLEGRO_MEMORY_BITMAP)
      al_ustr_append_cstr(ustr, " MEMORY");
   if (al_get_bitmap_flags(bitmap) & ALLEGRO_CONVERT_BITMAP)
      al_ustr_append_cstr(ustr, " CONVERT");

   al_ustr_trim_ws(ustr);
   al_ustr_find_replace_cstr(ustr, 0, " ", " | ");

   log_printf("%s", al_cstr(ustr));
   al_ustr_free(ustr);
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   bool redraw = true;
   ALLEGRO_FONT *font;
   ALLEGRO_BITMAP *spin, *spin2;
   int current_bitmap = 0;
   int loaded_bitmap = 0;
   ALLEGRO_THREAD *thread;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_image_addon();
   al_init_font_addon();
   al_init_primitives_addon();
   init_platform_specific();

   open_log();

   al_install_mouse();
   al_install_keyboard();

   spin = al_load_bitmap("data/cursor.tga");
   log_printf("default bitmap without display: %p\n", spin);

   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   spin2 = al_load_bitmap("data/cursor.tga");
   log_printf("video bitmap without display: %p\n", spin2);

   log_printf("%p before create_display: ", spin);
   print_bitmap_flags(spin);
   log_printf("\n");

   display = al_create_display(64, 64);
   if (!display) {
      abort_example("Error creating display\n");
   }

   spin2 = al_load_bitmap("data/cursor.tga");
   log_printf("video bitmap with display: %p\n", spin2);

   log_printf("%p after create_display: ", spin);
   print_bitmap_flags(spin);
   log_printf("\n");

   log_printf("%p after create_display: ", spin2);
   print_bitmap_flags(spin2);
   log_printf("\n");

   al_destroy_display(display);

   log_printf("%p after destroy_display: ", spin);
   print_bitmap_flags(spin);
   log_printf("\n");

   log_printf("%p after destroy_display: ", spin2);
   print_bitmap_flags(spin2);
   log_printf("\n");

   display = al_create_display(640, 480);

   log_printf("%p after create_display: ", spin);
   print_bitmap_flags(spin);
   log_printf("\n");

   log_printf("%p after create_display: ", spin2);
   print_bitmap_flags(spin2);
   log_printf("\n");

   font = al_load_font("data/fixed_font.tga", 0, 0);

   mutex = al_create_mutex();
   thread = al_create_thread(loading_thread, NULL);
   al_start_thread(thread);

   timer = al_create_timer(1.0 / 30);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
             break;
         }
      if (event.type == ALLEGRO_EVENT_TIMER)
         redraw = true;

      if (redraw && al_is_event_queue_empty(queue)) {
         float x = 20, y = 320;
         int i;
         ALLEGRO_COLOR color = al_map_rgb_f(0, 0, 0);
         float t = al_current_time();

         redraw = false;
         al_clear_to_color(al_map_rgb_f(0.5, 0.6, 1));

         al_draw_textf(font, color, x + 40, y, 0, "Loading %d%%",
            100 * load_count / load_total);

         al_lock_mutex(mutex);
         if (loaded_bitmap < load_count) {
            /* This will convert any video bitmaps without a display
             * (all the bitmaps being loaded in the loading_thread) to
             * video bitmaps we can use in the main thread.
             */
            al_convert_bitmap(bitmaps[loaded_bitmap]);
            loaded_bitmap++;
         }
         al_unlock_mutex(mutex);

         if (current_bitmap < loaded_bitmap) {
            int bw;
            al_draw_bitmap(bitmaps[current_bitmap], 0, 0, 0);
            if (current_bitmap + 1 < loaded_bitmap)
               current_bitmap++;

            for (i = 0; i <= current_bitmap; i++) {
               bw = al_get_bitmap_width(bitmaps[i]);
               al_draw_scaled_rotated_bitmap(bitmaps[i],
                  0, 0, (i % 20) * 640 / 20, 360 + (i / 20) * 24,
                  32.0 / bw, 32.0 / bw, 0, 0);
            }
         }

         if (loaded_bitmap < load_total) {
            al_draw_scaled_rotated_bitmap(spin,
               16, 16, x, y, 1.0, 1.0, t * ALLEGRO_PI * 2, 0);
         }

         al_flip_display();
      }
   }

   al_join_thread(thread, NULL);
   al_destroy_mutex(mutex);
   al_destroy_font(font);
   al_destroy_display(display);

   close_log(true);

   return 0;
}
