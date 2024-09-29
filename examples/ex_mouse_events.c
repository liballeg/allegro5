#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

#define NUM_BUTTONS  5
static int actual_buttons;

static void draw_mouse_button(int but, bool down)
{
   const int offset[NUM_BUTTONS] = {0, 70, 35, 105, 140};
   A5O_COLOR grey;
   A5O_COLOR black;
   int x;
   int y;
   
   x = 400 + offset[but];
   y = 130;

   grey = al_map_rgb(0xe0, 0xe0, 0xe0);
   black = al_map_rgb(0, 0, 0);

   al_draw_filled_rectangle(x, y, x + 27, y + 42, grey);
   al_draw_rectangle(x + 0.5, y + 0.5, x + 26.5, y + 41.5, black, 0);
   if (down) {
      al_draw_filled_rectangle(x + 2, y + 2, x + 25, y + 40, black);
   }
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_BITMAP *cursor;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   A5O_FONT *font;
   int mx = 0;
   int my = 0;
   int mz = 0;
   int mw = 0;
   int mmx = 0;
   int mmy = 0;
   int mmz = 0;
   int mmw = 0;
   int precision = 1;
   bool in = true;
   bool buttons[NUM_BUTTONS] = {false};
   int i;
   float p = 0.0;
   A5O_COLOR black;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_mouse();
   al_install_keyboard();
   al_init_image_addon();
   al_init_font_addon();
   init_platform_specific();
   
   actual_buttons = al_get_mouse_num_buttons();
   if (actual_buttons > NUM_BUTTONS)
      actual_buttons = NUM_BUTTONS;

   al_set_new_display_flags(A5O_RESIZABLE);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   // Resize the display - this is to excercise the resizing code wrt.
   // the cursor display boundary, which requires some special care on some
   // platforms (such as OSX).
   al_resize_display(display, 640*1.5, 480*1.5);

   al_hide_mouse_cursor(display);

   cursor = al_load_bitmap("data/cursor.tga");
   if (!cursor) {
      abort_example("Error loading cursor.tga\n");
   }

   font = al_load_font("data/fixed_font.tga", 1, 0);
   if (!font) {
      abort_example("data/fixed_font.tga not found\n");
   }
   
   black = al_map_rgb_f(0, 0, 0);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   while (1) {
      if (al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));
         for (i = 0; i < actual_buttons; i++) {
            draw_mouse_button(i, buttons[i]);
         }
         al_draw_bitmap(cursor, mx, my, 0);
         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         al_draw_textf(font, black, 5, 5, 0, "dx %i, dy %i, dz %i, dw %i", mmx, mmy, mmz, mmw);
         al_draw_textf(font, black, 5, 25, 0, "x %i, y %i, z %i, w %i", mx, my, mz, mw);
         al_draw_textf(font, black, 5, 45, 0, "p = %g", p);
         al_draw_textf(font, black, 5, 65, 0, "%s", in ? "in" : "out");
         al_draw_textf(font, black, 5, 85, 0, "wheel precision (PgUp/PgDn) %d", precision);
         al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
         mmx = mmy = mmz = 0;
         al_flip_display();
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            mz = event.mouse.z;
            mw = event.mouse.w;
            mmx = event.mouse.dx;
            mmy = event.mouse.dy;
            mmz = event.mouse.dz;
            mmw = event.mouse.dw;
            p = event.mouse.pressure;
            break;

         case A5O_EVENT_MOUSE_BUTTON_DOWN:
            if (event.mouse.button-1 < NUM_BUTTONS) {
               buttons[event.mouse.button-1] = true;
            }
            p = event.mouse.pressure;
            break;

         case A5O_EVENT_MOUSE_BUTTON_UP:
            if (event.mouse.button-1 < NUM_BUTTONS) {
               buttons[event.mouse.button-1] = false;
            }
            p = event.mouse.pressure;
            break;

         case A5O_EVENT_MOUSE_ENTER_DISPLAY:
            in = true;
            break;

         case A5O_EVENT_MOUSE_LEAVE_DISPLAY:
            in = false;
            break;

         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
               goto done;
            }
            break;

         case A5O_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == A5O_KEY_PGUP) {
               precision++;
               al_set_mouse_wheel_precision(precision);
            }
            else if (event.keyboard.keycode == A5O_KEY_PGDN) {
               precision--;
               if (precision < 1)
                  precision = 1;
               al_set_mouse_wheel_precision(precision);
            }
            break;

         case A5O_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event.display.source);
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            goto done;
      }
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}


/* vim: set sw=3 sts=3 et: */
