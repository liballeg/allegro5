#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"

#include "common.c"

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *tex1, *tex2;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   bool redraw = true;
   A5O_LOCKED_REGION *lock;
   A5O_FONT *font;
   unsigned char *p;
   int x, y;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_font_addon();

   al_install_mouse();
   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   font = al_create_builtin_font();

   tex1 = al_create_bitmap(8, 8);
   lock = al_lock_bitmap(tex1, A5O_PIXEL_FORMAT_ABGR_8888_LE, A5O_LOCK_WRITEONLY);
   p = lock->data;
   for (y = 0; y < 8; y++) {
      unsigned char *lp = p;
      for (x = 0; x < 8; x++) {
         if (x == 0 || y == 0 || x == 7 || y == 7) {
             p[0] = 0;
             p[1] = 0;
             p[2] = 0;
             p[3] = 0;
         }
         else {
             p[0] = 0;
             p[1] = 255;
             p[2] = 0;
             p[3] = 255;
         }
         p += 4;
      }
      p = lp + lock->pitch;
   }
   al_unlock_bitmap(tex1);

   al_set_new_bitmap_flags(A5O_MAG_LINEAR);
   tex2 = al_clone_bitmap(tex1);

   timer = al_create_timer(1.0 / 30);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);

      if (event.type == A5O_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == A5O_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE)
             break;
         }
      if (event.type == A5O_EVENT_TIMER)
         redraw = true;

      if (redraw && al_is_event_queue_empty(queue)) {
         float x = 8, y = 60;
         float a, t = al_get_time();
         float th = al_get_font_line_height(font);
         A5O_COLOR color = al_map_rgb_f(0, 0, 0);
         A5O_COLOR color2 = al_map_rgb_f(1, 0, 0);
         A5O_COLOR color3 = al_map_rgb_f(0, 0.5, 0);

         t /= 10;
         a = t - floor(t);
         a *= 2 * A5O_PI;

         redraw = false;
         al_clear_to_color(al_map_rgb_f(0.5, 0.6, 1));
         
         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_textf(font, color, x, y, 0, "not premultiplied");
         al_draw_textf(font, color, x, y + th, 0, "no filtering");
         al_set_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA);
         al_draw_scaled_rotated_bitmap(tex1, 4, 4, x + 320, y, 8, 8, a, 0);
         
         y += 120;

         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_textf(font, color, x, y, 0, "not premultiplied");
         al_draw_textf(font, color, x, y + th, 0, "mag linear filtering");
         al_draw_textf(font, color2, x + 400, y, 0, "wrong dark border");
         al_set_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA);
         al_draw_scaled_rotated_bitmap(tex2, 4, 4, x + 320, y, 8, 8, a, 0);

         y += 120;

         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_textf(font, color, x, y, 0, "premultiplied alpha");
         al_draw_textf(font, color, x, y + th, 0, "no filtering");
         al_draw_textf(font, color, x + 400, y, 0, "no difference");
         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_scaled_rotated_bitmap(tex1, 4, 4, x + 320, y, 8, 8, a, 0);
         
         y += 120;

         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_textf(font, color, x, y, 0, "premultiplied alpha");
         al_draw_textf(font, color, x, y + th, 0, "mag linear filtering");
         al_draw_textf(font, color3, x + 400, y, 0, "correct color");
         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_scaled_rotated_bitmap(tex2, 4, 4, x + 320, y, 8, 8, a, 0);

         al_flip_display();
      }
   }

   al_destroy_font(font); 
   al_destroy_bitmap(tex1);
   al_destroy_bitmap(tex2);

   return 0;
}
