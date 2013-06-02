/* This example demonstrates the effect of multi-sampling on primitives and
 * bitmaps.
 *
 * 
 * In the window without multi-sampling, the edge of the colored lines will not
 * be anti-aliased - each pixel is either filled completely with the primitive
 * color or not at all.
 *
 * The same is true for bitmaps. They will always be drawn at the nearest
 * full-pixel position. However this is only true for the bitmap outline -
 * when texture filtering is enabled the texture itself will honor the sub-pixel
 * position.
 *
 * Therefore in the case with no multi-sampling but texture-filtering the
 * outer black edge will stutter in one-pixel steps while the inside edge will
 * be filtered and appear smooth.
 *
 * 
 * In the multi-sampled version the colored lines will be anti-aliased.
 *
 * Same with the bitmaps. This means the bitmap outlines will always move in
 * smooth sub-pixel steps. However if texture filtering is turned off the
 * texels still are rounded to the next integer position.
 *
 * Therefore in the case where multi-sampling is enabled but texture-filtering
 * is not the outer bitmap edges will move smoothly but the inner will not.
 */

#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>

#include "common.c"

static ALLEGRO_FONT *font, *font_ms;
static ALLEGRO_BITMAP *bitmap_normal;
static ALLEGRO_BITMAP *bitmap_filter;
static ALLEGRO_BITMAP *bitmap_normal_ms;
static ALLEGRO_BITMAP *bitmap_filter_ms;
static float bitmap_x[8], bitmap_y[8];
static float bitmap_t;


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

static void bitmap_move(void)
{
   int i;

   bitmap_t++;
   for (i = 0; i < 8; i++) {
      float a = 2 * ALLEGRO_PI * i / 16;
      float s = sin((bitmap_t + i * 40) / 180 * ALLEGRO_PI);
      s *= 90;
      bitmap_x[i] = 100 + s * cos(a);
      bitmap_y[i] = 100 + s * sin(a);
   }
}

static void draw(ALLEGRO_BITMAP *bitmap, int y, char const *text)
{
   int i;

   al_draw_text(font_ms, al_map_rgb(0, 0, 0), 0, y, 0, text);

   for (i = 0; i < 16; i++) {
      float a = 2 * ALLEGRO_PI * i / 16;
      ALLEGRO_COLOR c = al_color_hsv(i * 360 / 16, 1, 1);
      al_draw_line(150 + cos(a) * 10, y + 100 + sin(a) * 10,
         150 + cos(a) * 90, y + 100 + sin(a) * 90, c, 3);
   }

   for (i = 0; i < 8; i++) {
      float a = 2 * ALLEGRO_PI * i / 16;
      int s = al_get_bitmap_width(bitmap);
      al_draw_rotated_bitmap(bitmap, s / 2, s / 2,
         50 + bitmap_x[i], y + bitmap_y[i], a, 0);
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display, *ms_display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_TIMER *timer;
   ALLEGRO_BITMAP *memory;
   char title[1024];
   bool quit = false;
   bool redraw = true;
   int wx, wy;

   if (!al_init()) {
      abort_example("Couldn't initialise Allegro.\n");
   }
   al_init_primitives_addon();

   al_install_keyboard();

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   memory = create_bitmap();

   /* Create the normal display. */
   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_REQUIRE);
   al_set_new_display_option(ALLEGRO_SAMPLES, 0, ALLEGRO_SUGGEST);
   display = al_create_display(300, 450);
   if (!display) {
      abort_example("Error creating display\n");
   }
   al_set_window_title(display, "Normal");

   /* Create bitmaps for the normal display. */
   al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
   bitmap_filter = al_clone_bitmap(memory);
   al_set_new_bitmap_flags(0);
   bitmap_normal = al_clone_bitmap(memory);

   font = al_create_builtin_font();

   al_get_window_position(display, &wx, &wy);
   if (wx < 160)
      wx = 160;

   /* Create the multi-sampling display. */
   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_REQUIRE);
   al_set_new_display_option(ALLEGRO_SAMPLES, 4, ALLEGRO_SUGGEST);
   ms_display = al_create_display(300, 450);
   if (!ms_display) {
      abort_example("Multisampling not available.\n");
   }
   sprintf(title, "Multisampling (%dx)", al_get_display_option(
      ms_display, ALLEGRO_SAMPLES));
   al_set_window_title(ms_display, title);

   /* Create bitmaps for the multi-sampling display. */
   al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);
   bitmap_filter_ms = al_clone_bitmap(memory);
   al_set_new_bitmap_flags(0);
   bitmap_normal_ms = al_clone_bitmap(memory);

   font_ms = al_create_builtin_font();

   /* Move the windows next to each other, because some window manager
    * would put them on top of each other otherwise.
    */
   al_set_window_position(display, wx - 160, wy);
   al_set_window_position(ms_display, wx + 160, wy);

   timer = al_create_timer(1.0 / 30.0);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_display_event_source(ms_display));
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_start_timer(timer);
   while (!quit) {
      ALLEGRO_EVENT event;

      /* Check for ESC key or close button event and quit in either case. */
      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            quit = true;
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               quit = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            bitmap_move();
            redraw = true;
            break;
      }

      if (redraw && al_is_event_queue_empty(queue)) {
         /* Draw the multi-sampled version into the first window. */
         al_set_target_backbuffer(ms_display);

         al_clear_to_color(al_map_rgb_f(1, 1, 1));

         draw(bitmap_filter_ms, 0, "filtered, multi-sample");
         draw(bitmap_normal_ms, 250, "no filter, multi-sample");

         al_flip_display();

         /* Draw the normal version into the second window. */
         al_set_target_backbuffer(display);

         al_clear_to_color(al_map_rgb_f(1, 1, 1));

         draw(bitmap_filter, 0, "filtered");
         draw(bitmap_normal, 250, "no filter");

         al_flip_display();

         redraw = false;
      }
   }

   return 0;
}
