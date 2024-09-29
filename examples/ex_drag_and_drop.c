/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      System drag&drop example.
 * 
 *      This example shows how an Allegro window can receive pictures
 *      (and text) from other applications.
 *
 *      See readme.txt for copyright information.
 */
#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

// We only accept up to 25 lines of text or files at a time.
typedef struct Cell {
   char *rows[25];
   A5O_BITMAP *bitmap;
} Cell;

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_FONT *font;
   bool done = false;
   bool redraw = true;
   int xdiv = 3;
   int ydiv = 4;
   Cell grid[xdiv * ydiv];
   int drop_x = -1;
   int drop_y = -1;

   (void)argc;
   (void)argv;

   // Initialize everything to NULL
   for (int i = 0; i < xdiv * ydiv; i++) {
      grid[i].rows[0] = strdup("drop file or text here");
      for (int r = 1; r < 25; r++) grid[i].rows[r] = NULL;
      grid[i].bitmap = NULL;
   }

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }
   al_init_image_addon();
   al_init_primitives_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   init_platform_specific();
   A5O_MONITOR_INFO info;
   al_get_monitor_info(0, &info);
   // Make the window half as wide as the desktop and 4:3 aspect
   int w = (info.x2 - info.x1) / 2;
   int h = w * 3 / 4;

   // Turn on drag/drop support
   al_set_new_display_flags(A5O_DRAG_AND_DROP);
   display = al_create_display(w, h);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   font = al_load_font("data/DejaVuSans.ttf", 20, 0);
   if (!font) {
      abort_example("Could not load font.\n");
   }

   timer = al_create_timer(1 / 60.0);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   // drag&drop events will come from the display, if enabled
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   while (!done) {
      A5O_EVENT event;

      int fh = al_get_font_line_height(font);

      if (redraw && al_is_event_queue_empty(queue)) {
         // draw a grid with the dropped text rows or files
         A5O_COLOR c1 = al_color_name("gainsboro");
         A5O_COLOR c2 = al_color_name("orange");
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         for (int y = 0; y < ydiv; y++) {
            for (int x = 0; x < xdiv; x++) {
               int gx = x * w / xdiv;
               int gy = y * h / ydiv;
               if (grid[x + y * xdiv].bitmap) {
                  A5O_BITMAP *bmp = grid[x + y * xdiv].bitmap;
                  float bw = al_get_bitmap_width(bmp);
                  float bh = al_get_bitmap_height(bmp);
                  float s = w / xdiv / bw;
                  if (s > (h / ydiv - fh) / bh) s = (h / ydiv - fh) / bh;
                  al_draw_scaled_bitmap(bmp, 0, 0, bw, bh, gx, gy + fh, bw * s, bh * s, 0);
               }
               for (int r = 0; r < 25; r++) {
                  if (!grid[x + y * xdiv].rows[r]) break;
                  al_draw_textf(font, c1, gx, gy + r * fh,
                     0, "%s", grid[x + y * xdiv].rows[r]);
               }
               if (drop_x >= gx && drop_x < gx + w / xdiv &&
                  drop_y >= gy && drop_y < gy + h / ydiv) {
                     al_draw_rectangle(gx, gy, gx + w / xdiv, gy + h / ydiv, c2, 2);
               }
               else {
                  al_draw_rectangle(gx, gy, gx + w / xdiv, gy + h / ydiv, c1, 0);
               }
            }
         }
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
               done = true;
            }
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case A5O_EVENT_TIMER:
            redraw = true;
            break;

         case A5O_EVENT_DROP:
            drop_x = event.drop.x;
            drop_y = event.drop.y;
            int col = drop_x * xdiv / w;
            int row = drop_y * ydiv / h;
            int i = col + row * xdiv;
            if (event.drop.text) {
               if (event.drop.row == 0) {
                  // clear the previous contents of the cell
                  for (int r = 0; r < 25; r++) {
                     if (grid[i].rows[r]) {
                        al_free(grid[i].rows[r]);
                        grid[i].rows[r] = NULL;
                     }
                  }
                  // insert the first row/file
                  grid[i].rows[0] = event.drop.text;
                  if (event.drop.is_file) {
                     // if a file, try and load it as a bitmap to display
                     if (grid[i].bitmap) al_destroy_bitmap(grid[i].bitmap);
                     grid[i].bitmap = al_load_bitmap(grid[i].rows[0]);
                  }
               }
               else {
                  if (event.drop.row < 25) {
                     grid[i].rows[event.drop.row] = event.drop.text;
                  }
                  else {
                     al_free(event.drop.text);
                  }
               }
            }
            if (event.drop.is_complete) {
               // stop highlighting a cell when the drop is completed
               drop_x = -1;
               drop_y = -1;
            }
            break;
      }
   }

   al_destroy_font(font);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
