/* An example comparing FPS when drawing to a bitmap with the
 * ALLEGRO_FORCE_LOCKING flag and without. Mainly meant as a test how much
 * speedup direct drawing can give over the slow locking.
 */

#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

const int W = 300, H = 300; /* Size of target bitmap. */
const int RW = 50, RH = 50; /* Size of rectangle we draw to it. */
ALLEGRO_BITMAP *target; /* The target bitmap. */
float x, y, dx, dy; /* Position and velocity of moving rectangle. */
double last_time; /* For controling speed. */
bool quit; /* Flag to record Esc key or X button. */
A5FONT_FONT *myfont; /* Our font. */
ALLEGRO_EVENT_QUEUE *queue; /* Our events queue. */

/* Print some text with a shadow. */
static void print(int x, int y, char const *format, ...)
{
   va_list list;
   char message[1024];

   va_start(list, format);
   uvszprintf(message, sizeof message, format, list);
   va_end(list);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(0, 0, 0));
   a5font_textout(myfont, message, x + 2, y + 2);

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgb(255, 255, 255));
   a5font_textout(myfont, message, x, y);
}

/* Draw our example scene. */
static void draw(void)
{
   float xs, ys, a;
   double dt = 0;
   double t = al_current_time();
   if (last_time > 0) {
      dt = t - last_time;
   }
   last_time = t;

   al_set_target_bitmap(target);
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgba_f(1, 1, 1, 1));

   al_draw_rectangle(x, y, x + RW, y + RH, al_map_rgba_f(1, 0, 0, 1),
      ALLEGRO_FILLED);
   al_draw_rectangle(0, 0, W, H, al_map_rgba_f(1, 1, 0, 0.1), ALLEGRO_FILLED);

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

   al_set_target_bitmap(al_get_backbuffer());
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba_f(1, 1, 1, 1));
   al_clear(al_map_rgba_f(0, 0, 1, 1));
   xs = 1 + 0.2 * sin(t * AL_PI * 2);
   ys = 1 + 0.2 * sin(t * AL_PI * 2);
   a = t * AL_PI * 2 / 3;
   al_draw_rotated_scaled_bitmap(target, W / 2, H / 2, 320, 240, xs, ys, a, 0);
}

/* Run the FPS test. */
void run(void)
{
   ALLEGRO_EVENT event;
   int frames = 0;
   double start;

   target = al_create_bitmap(W, H);
   al_set_target_bitmap(target);
   al_clear(al_map_rgba_f(1, 1, 0, 1));

   al_set_target_bitmap(al_get_backbuffer());

   dx = 81;
   dy = 63;

   start = al_current_time();
   while (true) {
      /* Check for ESC key or close button event and quit in either case. */
      if (!al_event_queue_is_empty(queue)) {
         while (al_get_next_event(queue, &event)) {
            switch (event.type) {
               case ALLEGRO_EVENT_DISPLAY_CLOSE:
                  quit = true;
                  goto done;

               case ALLEGRO_EVENT_KEY_DOWN:
                  if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                     quit = true;
                     goto done;
                  if (event.keyboard.keycode == ALLEGRO_KEY_SPACE)
                     goto done;
                  break;
            }
         }
      }
      draw();
      print(0, 0, "FPS: %.1f", frames / (al_current_time() - start));
      if (al_get_new_bitmap_flags() & ALLEGRO_FORCE_LOCKING) {
         print(0, a5font_text_height(myfont), "using forced bitmap locking");
      }
      else {
         print(0, a5font_text_height(myfont), "drawing directly to bitmap");
      }
      print(0, a5font_text_height(myfont) * 2,
         "Press SPACE to toggle drawing method.");
      al_flip_display();
      frames++;
   }

done:

   al_destroy_bitmap(target);
}

int main(void)
{
   ALLEGRO_DISPLAY *display;

   al_init();
   al_install_keyboard();
   a5font_init();

   display = al_create_display(640, 480);
   if (!display) {
      allegro_message("Error creating display");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);

   myfont = a5font_load_font("font.tga", 0);
   if (!myfont) {
      allegro_message("font.tga not found");
      return 1;
   }

   while (!quit) {
      if (al_get_new_bitmap_flags() & ALLEGRO_FORCE_LOCKING)
         al_set_new_bitmap_flags(0);
      else
         al_set_new_bitmap_flags(ALLEGRO_FORCE_LOCKING);
      run();
   }

   al_destroy_event_queue(queue);  
   
   return 0;
}
END_OF_MAIN()
