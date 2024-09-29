/*
 *    Example program for the Allegro library, by Peter Wang.
 */


#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include "allegro5/allegro_image.h"

#include "common.c"


typedef struct {
   int system_cursor;
   const char *label;
} CursorList;


#define MARGIN_LEFT  20
#define MARGIN_TOP   20
#define NUM_CURSORS  20

CursorList cursor_list[NUM_CURSORS] =
{
   { A5O_SYSTEM_MOUSE_CURSOR_DEFAULT, "DEFAULT" },
   { A5O_SYSTEM_MOUSE_CURSOR_ARROW, "ARROW" },
   { A5O_SYSTEM_MOUSE_CURSOR_BUSY, "BUSY" },
   { A5O_SYSTEM_MOUSE_CURSOR_QUESTION, "QUESTION" },
   { A5O_SYSTEM_MOUSE_CURSOR_EDIT, "EDIT" },
   { A5O_SYSTEM_MOUSE_CURSOR_MOVE, "MOVE" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_N, "RESIZE_N" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_W, "RESIZE_W" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_S, "RESIZE_S" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_E, "RESIZE_E" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_NW, "RESIZE_NW" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_SW, "RESIZE_SW" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_SE, "RESIZE_SE" },
   { A5O_SYSTEM_MOUSE_CURSOR_RESIZE_NE, "RESIZE_NE" },
   { A5O_SYSTEM_MOUSE_CURSOR_PROGRESS, "PROGRESS" },
   { A5O_SYSTEM_MOUSE_CURSOR_PRECISION, "PRECISION" },
   { A5O_SYSTEM_MOUSE_CURSOR_LINK, "LINK" },
   { A5O_SYSTEM_MOUSE_CURSOR_ALT_SELECT, "ALT_SELECT" },
   { A5O_SYSTEM_MOUSE_CURSOR_UNAVAILABLE, "UNAVAILABLE" },
   { -1, "CUSTOM" }
};

int current_cursor[2] = { 0, 0 };


static void draw_display(A5O_FONT *font)
{
   int th;
   int i;

   al_clear_to_color(al_map_rgb(128, 128, 128));

   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
   th = al_get_font_line_height(font);
   for (i = 0; i < NUM_CURSORS; i++) {
      al_draw_text(font, al_map_rgba_f(0, 0, 0, 1),
         MARGIN_LEFT, MARGIN_TOP + i * th, 0, cursor_list[i].label);
   }

   i++;
   al_draw_text(font, al_map_rgba_f(0, 0, 0, 1),
      MARGIN_LEFT, MARGIN_TOP + i * th, 0,
      "Press S/H to show/hide cursor");

   al_flip_display();
}

static int hover(A5O_FONT *font, int y)
{
   int th;
   int i;

   if (y < MARGIN_TOP)
      return -1;

   th = al_get_font_line_height(font);
   i = (y - MARGIN_TOP) / th;
   if (i < NUM_CURSORS)
      return i;

   return -1;
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display1;
   A5O_DISPLAY *display2;
   A5O_BITMAP *bmp;
   A5O_BITMAP *shrunk_bmp;
   A5O_MOUSE_CURSOR *custom_cursor;
   A5O_EVENT_QUEUE *queue;
   A5O_FONT *font;
   A5O_EVENT event;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_image_addon();
   init_platform_specific();

   if (!al_install_mouse()) {
      abort_example("Error installing mouse\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard\n");
   }

   al_set_new_display_flags(A5O_GENERATE_EXPOSE_EVENTS);
   display1 = al_create_display(400, 400);
   if (!display1) {
      abort_example("Error creating display1\n");
   }

   display2 = al_create_display(400, 400);
   if (!display2) {
      abort_example("Error creating display2\n");
   }

   bmp = al_load_bitmap("data/allegro.pcx");
   if (!bmp) {
      abort_example("Error loading data/allegro.pcx\n");
   }

   font = al_load_bitmap_font("data/fixed_font.tga");
   if (!font) {
      abort_example("Error loading data/fixed_font.tga\n");
   }

   shrunk_bmp = al_create_bitmap(32, 32);
   if (!shrunk_bmp) {
      abort_example("Error creating shrunk_bmp\n");
   }

   al_set_target_bitmap(shrunk_bmp);
   al_draw_scaled_bitmap(bmp,
      0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
      0, 0, 32, 32,
      0);

   custom_cursor = al_create_mouse_cursor(shrunk_bmp, 0, 0);
   if (!custom_cursor) {
      abort_example("Error creating mouse cursor\n");
   }

   al_set_target_bitmap(NULL);
   al_destroy_bitmap(shrunk_bmp);
   al_destroy_bitmap(bmp);
   shrunk_bmp = NULL;
   bmp = NULL;

   queue = al_create_event_queue();
   if (!queue) {
      abort_example("Error creating event queue\n");
   }

   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display1));
   al_register_event_source(queue, al_get_display_event_source(display2));

   al_set_target_backbuffer(display1);
   draw_display(font);

   al_set_target_backbuffer(display2);
   draw_display(font);

   while (1) {
      al_wait_for_event(queue, &event);
      if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == A5O_EVENT_DISPLAY_EXPOSE) {
         al_set_target_backbuffer(event.display.source);
         draw_display(font);
         continue;
      }
      if (event.type == A5O_EVENT_KEY_CHAR) {
         switch (event.keyboard.unichar) {
            case 27: /* escape */
               goto Quit;
            case 'h':
               al_hide_mouse_cursor(event.keyboard.display);
               break;
            case 's':
               al_show_mouse_cursor(event.keyboard.display);
               break;
            default:
               break;
         }
      }
      if (event.type == A5O_EVENT_MOUSE_AXES) {
         int dpy = (event.mouse.display == display1) ? 0 : 1;
         int i = hover(font, event.mouse.y);

         if (i >= 0 && current_cursor[dpy] != i) {
            if (cursor_list[i].system_cursor != -1) {
               al_set_system_mouse_cursor(event.mouse.display,
                  cursor_list[i].system_cursor);
            }
            else {
               al_set_mouse_cursor(event.mouse.display, custom_cursor);
            }
            current_cursor[dpy] = i;
         }
      }
   }

Quit:

   al_destroy_mouse_cursor(custom_cursor);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
