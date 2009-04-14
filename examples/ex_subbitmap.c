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
#include "allegro5/a5_iio.h"
#include <allegro5/a5_primitives.h>

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define CLAMP(x,y,z) MAX((x), MIN((y), (z)))

typedef enum {
   PLAIN_BLIT,
   SCALED_BLIT
} Mode;

enum {
   SRC_WIDTH  = 320,
   SRC_HEIGHT = 200,
   DST_WIDTH  = 640,
   DST_HEIGHT = 480
};


ALLEGRO_DISPLAY *src_display;
ALLEGRO_DISPLAY *dst_display;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_BITMAP *mem_bmp;

int src_x1 = 0;
int src_y1 = 0;
int src_x2 = SRC_WIDTH-1;
int src_y2 = SRC_HEIGHT-1;
int dst_x1 = 0;
int dst_y1 = 0;
int dst_x2 = DST_WIDTH-1;
int dst_y2 = DST_HEIGHT-1;

Mode mode = PLAIN_BLIT;


int main(void)
{
   ALLEGRO_BITMAP *src_subbmp = NULL;
   ALLEGRO_BITMAP *dst_subbmp = NULL;
   ALLEGRO_EVENT event;
   bool mouse_down;
   bool recreate_subbitmaps;
   bool redraw;

   if (!al_init()) {
      return 1;
   }
   al_init_iio_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   src_display = al_create_display(SRC_WIDTH, SRC_HEIGHT);
   if (!src_display) {
      return 1;
   }
   al_set_window_title("Source");

   dst_display = al_create_display(DST_WIDTH, DST_HEIGHT);
   if (!dst_display) {
      return 1;
   }
   al_set_window_title("Destination");

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   mem_bmp = al_load_bitmap("data/mysha.pcx");
   if (!mem_bmp) {
      TRACE("Could not load data/mysha.pcx\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) src_display);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) dst_display);

   mouse_down = false;
   recreate_subbitmaps = true;
   redraw = true;

   while (true) {
      if (recreate_subbitmaps) {
         int l, r, t, b;

         al_destroy_bitmap(src_subbmp);
         al_destroy_bitmap(dst_subbmp);

         l = MIN(src_x1, src_x2);
         r = MAX(src_x1, src_x2);
         t = MIN(src_y1, src_y2);
         b = MAX(src_y1, src_y2);

         l = CLAMP(0, l, SRC_WIDTH-1);
         r = CLAMP(0, r, SRC_WIDTH-1);
         t = CLAMP(0, t, SRC_HEIGHT-1);
         b = CLAMP(0, b, SRC_HEIGHT-1);

         src_subbmp = al_create_sub_bitmap(mem_bmp, l, t, r-l+1, b-t+1);

         l = MIN(dst_x1, dst_x2);
         r = MAX(dst_x1, dst_x2);
         t = MIN(dst_y1, dst_y2);
         b = MAX(dst_y1, dst_y2);

         l = CLAMP(0, l, DST_WIDTH-1);
         r = CLAMP(0, r, DST_WIDTH-1);
         t = CLAMP(0, t, DST_HEIGHT-1);
         b = CLAMP(0, b, DST_HEIGHT-1);

         al_set_current_display(dst_display);
         dst_subbmp = al_create_sub_bitmap(al_get_backbuffer(),
            l, t, r-l+1, b-t+1);

         recreate_subbitmaps = false;
      }

      if (redraw && al_event_queue_is_empty(queue)) {
         al_set_current_display(dst_display);
         al_clear_to_color(al_map_rgb(0, 0, 0));

         al_set_target_bitmap(dst_subbmp);
         switch (mode) {
            case PLAIN_BLIT:
               al_draw_bitmap(src_subbmp, 0, 0, 0);
               break;
            case SCALED_BLIT:
               al_draw_scaled_bitmap(src_subbmp,
                  0, 0, al_get_bitmap_width(src_subbmp),
                  al_get_bitmap_height(src_subbmp),
                  0, 0, al_get_bitmap_width(dst_subbmp),
                  al_get_bitmap_height(dst_subbmp),
                  0);
               break;
         }

         al_set_target_bitmap(al_get_backbuffer());
         al_draw_rectangle(dst_x1, dst_y1, dst_x2, dst_y2,
            al_map_rgb(0, 255, 255), 0);
         al_flip_display();

         al_set_current_display(src_display);
         al_clear_to_color(al_map_rgb(0, 0, 0));
         al_draw_bitmap(mem_bmp, 0, 0, 0);
         al_draw_rectangle(src_x1, src_y1, src_x2, src_y2,
            al_map_rgb(0, 255, 255), 0);
         al_flip_display();

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

END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
