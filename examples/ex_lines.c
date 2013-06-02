/*
 * This example exercises line drawing, and single buffer mode.
 */

#include <math.h>
#include "allegro5/allegro.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

/* XXX the software line drawer currently doesn't perform clipping properly */

const int W = 640;
const int H = 480;

ALLEGRO_DISPLAY *display;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_COLOR black;
ALLEGRO_COLOR white;
ALLEGRO_COLOR background;
ALLEGRO_BITMAP *dbuf;

int last_x = -1;
int last_y = -1;


static void fade(void)
{
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
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

static void flip(void)
{
   al_set_target_backbuffer(display);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_draw_bitmap(dbuf, 0.0, 0.0, 0);
   al_flip_display();
}

static void plonk(const int x, const int y, bool blend)
{
   al_set_target_bitmap(dbuf);

   fade();
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ZERO);
   draw_clip_rect();
   red_dot(x, y);

   if (last_x == -1 && last_y == -1) {
      last_x = x;
      last_y = y;
   }
   else {
      my_set_clip_rect();
      if (blend) {
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
      }
      al_draw_line(last_x, last_y, x, y, white, 0);
      last_x = last_y = -1;
      reset_clip_rect();
   }

   flip();
}

static void splat(const int x, const int y, bool blend)
{
   double theta;

   al_set_target_bitmap(dbuf);

   fade();
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ZERO);
   draw_clip_rect();
   red_dot(x, y);

   my_set_clip_rect();
   if (blend) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
   }
   for (theta = 0.0; theta < 2.0 * ALLEGRO_PI; theta += ALLEGRO_PI/16.0) {
      al_draw_line(x, y, x + 40.0 * cos(theta), y + 40.0 * sin(theta), white, 0);
   }
   reset_clip_rect();

   flip();
}

int main(int argc, const char *argv[])
{
   ALLEGRO_EVENT event;
   ALLEGRO_KEYBOARD_STATE kst;
   bool blend;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();

   display = al_create_display(W, H);
   if (!display) {
      abort_example("Error creating display\n");
   }

   black = al_map_rgb_f(0.0, 0.0, 0.0);
   white = al_map_rgb_f(1.0, 1.0, 1.0);
   background = al_map_rgb_f(0.5, 0.5, 0.6);

   if (argc > 1 && 0 == strcmp(argv[1], "--memory-bitmap")) {
      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   }
   dbuf = al_create_bitmap(W, H);
   if (!dbuf) {
      abort_example("Error creating double buffer\n");
   }

   al_set_target_bitmap(dbuf);
   al_clear_to_color(background);
   draw_clip_rect();
   flip();

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
   al_destroy_bitmap(dbuf);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
