/*
 *    Example program for the Allegro library.
 *
 *    This program blitting to/from sub-bitmaps.
 *
 *    Highlight sub-bitmap regions with left mouse button.
 *    Press '1' to perform plain blits.
 *    Press 's' to perform scaled blits.
 */

/* TODO: test non-memory bitmaps */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define CLAMP(x,y,z) MAX((x), MIN((y), (z)))

typedef enum {
   PLAIN_BLIT,
   SCALED_BLIT
} Mode;

enum {
   SRC_WIDTH  = 640,
   SRC_HEIGHT = 480,
   SRC_X = 160,
   SRC_Y = 120,
   DST_WIDTH  = 640,
   DST_HEIGHT = 480
};


ALLEGRO_DISPLAY *src_display;
ALLEGRO_DISPLAY *dst_display;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_BITMAP *mem_bmp;

int src_x1 = SRC_X;
int src_y1 = SRC_Y;
int src_x2 = SRC_X + 319;
int src_y2 = SRC_Y + 199;
int dst_x1 = 0;
int dst_y1 = 0;
int dst_x2 = DST_WIDTH-1;
int dst_y2 = DST_HEIGHT-1;

Mode mode = PLAIN_BLIT;
int draw_flags = 0;

int main(void)
{
   ALLEGRO_BITMAP *src_subbmp[2] = {NULL, NULL};
   ALLEGRO_BITMAP *dst_subbmp[2] = {NULL, NULL};
   ALLEGRO_EVENT event;
   bool mouse_down;
   bool recreate_subbitmaps;
   bool redraw;

   if (!al_init()) {
      return 1;
   }
   al_init_primitives_addon();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   src_display = al_create_display(SRC_WIDTH, SRC_HEIGHT);
   if (!src_display) {
      return 1;
   }
   al_set_window_title(src_display, "Source");

   dst_display = al_create_display(DST_WIDTH, DST_HEIGHT);
   if (!dst_display) {
      return 1;
   }
   al_set_window_title(dst_display, "Destination");

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   mem_bmp = al_load_bitmap("data/mysha.pcx");
   if (!mem_bmp) {
      abort_example("Could not load data/mysha.pcx\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(src_display));
   al_register_event_source(queue, al_get_display_event_source(dst_display));

   mouse_down = false;
   recreate_subbitmaps = true;
   redraw = true;

   while (true) {
      if (recreate_subbitmaps) {
         int l, r, t, b, sw, sh;

         al_destroy_bitmap(src_subbmp[0]);
         al_destroy_bitmap(dst_subbmp[0]);
         al_destroy_bitmap(src_subbmp[1]);
         al_destroy_bitmap(dst_subbmp[1]);

         l = MIN(src_x1, src_x2);
         r = MAX(src_x1, src_x2);
         t = MIN(src_y1, src_y2);
         b = MAX(src_y1, src_y2);
         
         l -= SRC_X;
         t -= SRC_Y;
         r -= SRC_X;
         b -= SRC_Y;

         src_subbmp[0] = al_create_sub_bitmap(mem_bmp, l, t, r - l + 1,
            b - t + 1);
         sw = al_get_bitmap_width(src_subbmp[0]);
         sh = al_get_bitmap_height(src_subbmp[0]);
         src_subbmp[1] = al_create_sub_bitmap(src_subbmp[0], 2, 2,
            sw - 4, sh - 4);

         l = MIN(dst_x1, dst_x2);
         r = MAX(dst_x1, dst_x2);
         t = MIN(dst_y1, dst_y2);
         b = MAX(dst_y1, dst_y2);

         al_set_target_backbuffer(dst_display);
         dst_subbmp[0] = al_create_sub_bitmap(al_get_backbuffer(dst_display),
            l, t, r - l + 1, b - t + 1);
         dst_subbmp[1] = al_create_sub_bitmap(dst_subbmp[0],
            2, 2, r - l - 3, b - t - 3);

         recreate_subbitmaps = false;
      }

      if (redraw && al_event_queue_is_empty(queue)) {
         al_set_target_backbuffer(dst_display);
         al_clear_to_color(al_map_rgb(0, 0, 0));

         al_set_target_bitmap(dst_subbmp[1]);
         switch (mode) {
            case PLAIN_BLIT:
            {
               al_draw_bitmap(src_subbmp[1], 0, 0, draw_flags);
               break;
            }
            case SCALED_BLIT:
            {
               al_draw_scaled_bitmap(src_subbmp[1],
                  0, 0, al_get_bitmap_width(src_subbmp[1]),
                  al_get_bitmap_height(src_subbmp[1]),
                  0, 0, al_get_bitmap_width(dst_subbmp[1]),
                  al_get_bitmap_height(dst_subbmp[1]),
                  draw_flags);
               break;
            }
         }

         #define SWAP_GREATER(f1, f2) { \
            if (f1 > f2) { \
               float tmp = f1; \
               f1 = f2; \
               f2 = tmp; \
            } \
         }

         {
            /* pixel center is at 0.5/0.5 */
            float x = dst_x1 + 0.5;
            float y = dst_y1 + 0.5;
            float x_ = dst_x2 + 0.5;
            float y_ = dst_y2 + 0.5;
            SWAP_GREATER(x, x_)
            SWAP_GREATER(y, y_)
            al_set_target_backbuffer(dst_display);
            al_draw_rectangle(x, y, x_, y_,
               al_map_rgb(0, 255, 255), 0);
            al_draw_rectangle(x + 2, y + 2, x_ - 2, y_ - 2,
               al_map_rgb(255, 255, 0), 0);
            al_flip_display();
         }

         {
            /* pixel center is at 0.5/0.5 */
            float x = src_x1 + 0.5;
            float y = src_y1 + 0.5;
            float x_ = src_x2 + 0.5;
            float y_ = src_y2 + 0.5;
            SWAP_GREATER(x, x_)
            SWAP_GREATER(y, y_)
            al_set_target_backbuffer(src_display);
            al_clear_to_color(al_map_rgb(0, 0, 0));
            al_draw_bitmap(mem_bmp, SRC_X, SRC_Y, 0);
            al_draw_rectangle(x, y, x_, y_,
               al_map_rgb(0, 255, 255), 0);
            al_draw_rectangle(x + 2, y + 2, x_ - 2, y_ - 2,
               al_map_rgb(255, 255, 0), 0);
            al_flip_display();
         }

         #undef SWAP_GREATER

         redraw = false;
      }

      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
         }
         if (event.keyboard.unichar == '1') {
            mode = PLAIN_BLIT;
            redraw = true;
         }
         else if (event.keyboard.unichar == 's') {
            mode = SCALED_BLIT;
            redraw = true;
         }
         else if (event.keyboard.unichar == 'h') {
            draw_flags ^= ALLEGRO_FLIP_HORIZONTAL;
            redraw = true;
         }
         else if (event.keyboard.unichar == 'v') {
            draw_flags ^= ALLEGRO_FLIP_VERTICAL;
            redraw = true;
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN &&
            event.mouse.button == 1) {
         if (event.mouse.display == src_display) {
            src_x1 = src_x2 = event.mouse.x;
            src_y1 = src_y2 = event.mouse.y;
         }
         else if (event.mouse.display == dst_display) {
            dst_x1 = dst_x2 = event.mouse.x;
            dst_y1 = dst_y2 = event.mouse.y;
         }
         mouse_down = true;
         redraw = true;
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         if (mouse_down) {
            if (event.mouse.display == src_display) {
               src_x2 = event.mouse.x;
               src_y2 = event.mouse.y;
            }
            else if (event.mouse.display == dst_display) {
               dst_x2 = event.mouse.x;
               dst_y2 = event.mouse.y;
            }
            redraw = true;
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP &&
            event.mouse.button == 1) {
         mouse_down = false;
         recreate_subbitmaps = true;
         redraw = true;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
         redraw = true;
      }
   }

   al_destroy_event_queue(queue);

   return 0;
}


/* vim: set sts=3 sw=3 et: */
