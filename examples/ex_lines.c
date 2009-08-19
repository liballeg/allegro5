/*
 * This example exercises line drawing, and single buffer mode.
 */

#include <math.h>
#include "allegro5/allegro5.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

/* Define this to test drawing to memory bitmaps.
 * XXX the software line drawer currently doesn't perform clipping properly
 *
 * Once event `display' fields are working we can just open up two windows,
 * one to test drawing to the backbuffer and one to test drawing to a memory
 * buffer.
 */
/* #define TEST_MEMBMP */

const int W = 640;
const int H = 480;

ALLEGRO_DISPLAY *display;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_COLOR black;
ALLEGRO_COLOR white;
ALLEGRO_COLOR background;
#ifdef TEST_MEMBMP
   ALLEGRO_BITMAP *memory_bitmap;
#endif

int last_x = -1;
int last_y = -1;


static void fade(void)
{
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, background);
   al_draw_filled_rectangle(0, 0, W, H, al_map_rgba_f(0.5, 0.5, 0.6, 0.2));
}

static void red_dot(int x, int y)
{
   al_draw_filled_rectangle(x - 2, y - 2, x + 2, y + 2, al_map_rgb_f(1, 0, 0));
}

static void draw_clip_rect(void)
{
   al_draw_rectangle(100.5, 100.5, W - 100.5, H - 100.5, black, 0);
}

static void my_set_clip_rect(void)
{
   al_set_clipping_rectangle(100, 100, W - 200, H - 200);
}

static void reset_clip_rect(void)
{
   al_set_clipping_rectangle(0, 0, W, H);
}

static void plonk(const int x, const int y, bool blend)
{
#ifdef TEST_MEMBMP
   al_set_target_bitmap(memory_bitmap);
#else
   al_set_target_bitmap(al_get_backbuffer());
#endif

   fade();
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, white);
   draw_clip_rect();
   red_dot(x, y);

   if (last_x == -1 && last_y == -1) {
      last_x = x;
      last_y = y;
   }
   else {
      my_set_clip_rect();
      if (blend) {
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
            al_map_rgb_f(0.5, 1, 1));
      }
      al_draw_line(last_x, last_y, x, y, white, 0);
      last_x = last_y = -1;
      reset_clip_rect();
   }

#ifdef TEST_MEMBMP
   al_set_target_bitmap(al_get_backbuffer());
   al_draw_bitmap(memory_bitmap, 0.0, 0.0, 0);
#endif

   al_flip_display();
}

static void splat(const int x, const int y, bool blend)
{
   double theta;

#ifdef TEST_MEMBMP
   al_set_target_bitmap(memory_bitmap);
#else
   al_set_target_bitmap(al_get_backbuffer());
#endif

   fade();
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, white);
   draw_clip_rect();
   red_dot(x, y);

   my_set_clip_rect();
   if (blend) {
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
         al_map_rgb_f(0.5, 1, 1));
   }
   for (theta = 0.0; theta < 2.0 * ALLEGRO_PI; theta += ALLEGRO_PI/16.0) {
      al_draw_line(x, y, x + 40.0 * cos(theta), y + 40.0 * sin(theta), white, 0);
   }
   reset_clip_rect();

#ifdef TEST_MEMBMP
   al_set_target_bitmap(al_get_backbuffer());
   al_draw_bitmap(memory_bitmap, 0.0, 0.0, 0);
#endif

   al_flip_display();
}

int main(void)
{
   ALLEGRO_EVENT event;
   ALLEGRO_KEYBOARD_STATE kst;
   bool blend;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();

   al_set_new_display_option(ALLEGRO_SINGLE_BUFFER, true, ALLEGRO_SUGGEST);
   display = al_create_display(W, H);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   black = al_map_rgb_f(0.0, 0.0, 0.0);
   white = al_map_rgb_f(1.0, 1.0, 1.0);
   background = al_map_rgb_f(0.5, 0.5, 0.6);

#ifdef TEST_MEMBMP
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   memory_bitmap = al_create_bitmap(W, H);
   if (!memory_bitmap) {
      abort_example("Error creating memory bitmap\n");
      return 1;
   }
   al_set_target_bitmap(memory_bitmap);
   al_clear_to_color(background);
#endif

   al_set_target_bitmap(al_get_backbuffer());
   al_clear_to_color(background);
   draw_clip_rect();
   al_flip_display();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());

   while (true) {
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         al_get_keyboard_state(&kst);
         blend = al_key_down(&kst, ALLEGRO_KEY_LSHIFT) ||
            al_key_down(&kst, ALLEGRO_KEY_RSHIFT);
         if (event.mouse.button == 1) {
            plonk(event.mouse.x, event.mouse.y, blend);
         } else {
            splat(event.mouse.x, event.mouse.y, blend);
         }
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_OUT) {
         last_x = last_y = -1;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
         event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
      {
         break;
      }
   }

   al_destroy_event_queue(queue);
#ifdef TEST_MEMBMP
   al_destroy_bitmap(memory_bitmap);
#endif

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
