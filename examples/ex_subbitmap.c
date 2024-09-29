/*
 *    Example program for the Allegro library.
 *
 *    This program blitting to/from sub-bitmaps.
 *
 */

#include "allegro5/allegro.h"
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


A5O_DISPLAY *src_display;
A5O_DISPLAY *dst_display;
A5O_EVENT_QUEUE *queue;
A5O_BITMAP *src_bmp;

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

int main(int argc, char **argv)
{
   A5O_BITMAP *src_subbmp[2] = {NULL, NULL};
   A5O_BITMAP *dst_subbmp[2] = {NULL, NULL};
   A5O_EVENT event;
   bool mouse_down;
   bool recreate_subbitmaps;
   bool redraw;
   bool use_memory;
   const char *image_filename = NULL;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_primitives_addon();
   al_init_image_addon();
   init_platform_specific();

   open_log();

   al_set_new_display_flags(A5O_GENERATE_EXPOSE_EVENTS);
   src_display = al_create_display(SRC_WIDTH, SRC_HEIGHT);
   if (!src_display) {
      abort_example("Error creating display\n");
   }
   al_set_window_title(src_display, "Source");

   dst_display = al_create_display(DST_WIDTH, DST_HEIGHT);
   if (!dst_display) {
      abort_example("Error creating display\n");
   }
   al_set_window_title(dst_display, "Destination");

   {
      int i;
      for (i = 1; i < argc; ++i) {
         if (!image_filename)
            image_filename = argv[i];
         else
            abort_example("Unknown argument: %s\n", argv[i]);
      }
      
      if (!image_filename) {
         image_filename = "data/mysha.pcx";
      }
   }

   src_bmp = al_load_bitmap(image_filename);
   if (!src_bmp) {
      abort_example("Could not load image file\n");
   }
   
   src_x2 = src_x1 + al_get_bitmap_width(src_bmp);
   src_y2 = src_y1 + al_get_bitmap_height(src_bmp);

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
   use_memory = false;

   log_printf("Highlight sub-bitmap regions with left mouse button.\n");
   log_printf("Press 'm' to toggle memory bitmaps.\n");
   log_printf("Press '1' to perform plain blits.\n");
   log_printf("Press 's' to perform scaled blits.\n");
   log_printf("Press 'h' to flip horizontally.\n");
   log_printf("Press 'v' to flip vertically.\n");

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

         src_subbmp[0] = al_create_sub_bitmap(src_bmp, l, t, r - l + 1,
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

      if (redraw && al_is_event_queue_empty(queue)) {
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
            al_draw_bitmap(src_bmp, SRC_X, SRC_Y, 0);
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
      if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == A5O_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
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
            draw_flags ^= A5O_FLIP_HORIZONTAL;
            redraw = true;
         }
         else if (event.keyboard.unichar == 'v') {
            draw_flags ^= A5O_FLIP_VERTICAL;
            redraw = true;
         }
         else if (event.keyboard.unichar == 'm') {
            A5O_BITMAP *temp = src_bmp;
            use_memory = !use_memory;
            log_printf("Using a %s bitmap.\n", use_memory ? "memory" : "video");
            al_set_new_bitmap_flags(use_memory ?
               A5O_MEMORY_BITMAP : A5O_VIDEO_BITMAP);
            src_bmp = al_clone_bitmap(temp);
            al_destroy_bitmap(temp);
            redraw = true;
            recreate_subbitmaps = true;
         }
      }
      else if (event.type == A5O_EVENT_MOUSE_BUTTON_DOWN &&
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
      else if (event.type == A5O_EVENT_MOUSE_AXES) {
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
      else if (event.type == A5O_EVENT_MOUSE_BUTTON_UP &&
            event.mouse.button == 1) {
         mouse_down = false;
         recreate_subbitmaps = true;
         redraw = true;
      }
      else if (event.type == A5O_EVENT_DISPLAY_EXPOSE) {
         redraw = true;
      }
   }

   al_destroy_event_queue(queue);

   close_log(false);
   return 0;
}


/* vim: set sts=3 sw=3 et: */
