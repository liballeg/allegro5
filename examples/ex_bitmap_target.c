/* An example comparing FPS when drawing to a bitmap with the
 * ALLEGRO_FORCE_LOCKING flag and without. Mainly meant as a test how much
 * speedup direct drawing can give over the slow locking.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "common.c"

const int W = 300, H = 300; /* Size of target bitmap. */
const int RW = 50, RH = 50; /* Size of rectangle we draw to it. */
ALLEGRO_DISPLAY *display;
ALLEGRO_BITMAP *target; /* The target bitmap. */
float x, y, dx, dy; /* Position and velocity of moving rectangle. */
double last_time; /* For controling speed. */
bool quit; /* Flag to record Esc key or X button. */
ALLEGRO_FONT *myfont; /* Our font. */
ALLEGRO_EVENT_QUEUE *queue; /* Our events queue. */

/* Print some text with a shadow. */
static void print(int x, int y, char const *format, ...)
{
   va_list list;
   char message[1024];

   va_start(list, format);
   vsnprintf(message, sizeof message, format, list);
   va_end(list);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

   al_draw_text(myfont, al_map_rgb(0, 0, 0), x + 2, y + 2, 0, message);
   al_draw_text(myfont, al_map_rgb(255, 255, 255), x, y, 0, message);
}

/* Draw our example scene. */
static void draw(void)
{
   float xs, ys, a;
   double dt = 0;
   double t = al_get_time();
   if (last_time > 0) {
      dt = t - last_time;
   }
   last_time = t;

   al_set_target_bitmap(target);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

   al_draw_filled_rectangle(x, y, x + RW, y + RH, al_map_rgba_f(1, 0, 0, 1));
   al_draw_filled_rectangle(0, 0, W, H, al_map_rgba_f(1, 1, 0, 0.1));

   x += dx * dt;
   if (x < 0) {
      x = 0;
      dx = -dx;
   }
   if (x + RW > W) {
      x = W - RW;
      dx = -dx;
   }
   y += dy * dt;
   if (y < 0) {
      y = 0;
      dy = -dy;
   }
   if (y + RH > H) {
      y = H - RH;
      dy = -dy;
   }

   al_set_target_backbuffer(display);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_clear_to_color(al_map_rgba_f(0, 0, 1, 1));
   xs = 1 + 0.2 * sin(t * ALLEGRO_PI * 2);
   ys = 1 + 0.2 * sin(t * ALLEGRO_PI * 2);
   a = t * ALLEGRO_PI * 2 / 3;
   al_draw_scaled_rotated_bitmap(target, W / 2, H / 2, 320, 240, xs, ys, a, 0);
}

/* Run the FPS test. */
static void run(void)
{
   ALLEGRO_EVENT event;
   int frames = 0;
   double start;

   target = al_create_bitmap(W, H);
   al_set_target_bitmap(target);
   al_clear_to_color(al_map_rgba_f(1, 1, 0, 1));

   al_set_target_backbuffer(display);

   dx = 81;
   dy = 63;

   start = al_get_time();
   while (true) {
      /* Check for ESC key or close button event and quit in either case. */
      if (!al_is_event_queue_empty(queue)) {
         while (al_get_next_event(queue, &event)) {
            switch (event.type) {
               case ALLEGRO_EVENT_DISPLAY_CLOSE:
                  quit = true;
                  goto done;

               case ALLEGRO_EVENT_KEY_DOWN:
                  if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                     quit = true;
                     goto done;
                  }
                  if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                      goto done;
                  }
                  break;
            }
         }
      }
      draw();
      print(0, 0, "FPS: %.1f", frames / (al_get_time() - start));
      if (al_get_new_bitmap_flags() & ALLEGRO_FORCE_LOCKING) {
         print(0, al_get_font_line_height(myfont), "using forced bitmap locking");
      }
      else {
         print(0, al_get_font_line_height(myfont), "drawing directly to bitmap");
      }
      print(0, al_get_font_line_height(myfont) * 2,
         "Press SPACE to toggle drawing method.");
      al_flip_display();
      frames++;
   }

done:

   al_destroy_bitmap(target);
}

int main(void)
{
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_keyboard();
   al_init_image_addon();
   al_init_font_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   myfont = al_load_font("data/font.tga", 0, 0);
   if (!myfont) {
      abort_example("font.tga not found\n");
   }

   while (!quit) {
      if (al_get_new_bitmap_flags() & ALLEGRO_FORCE_LOCKING)
         al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
      else
         al_set_new_bitmap_flags(ALLEGRO_FORCE_LOCKING);
      run();
   }

   al_destroy_event_queue(queue);  
   
   return 0;
}
