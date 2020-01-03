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
   ALLEGRO_COLOR grey;
   ALLEGRO_COLOR black;
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
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   ALLEGRO_FONT *font;
   float mx = 0;
   float my = 0;
   float mz = 0;
   float mw = 0;
   float mmx = 0;
   float mmy = 0;
   float mmz = 0;
   float mmw = 0;
   int precision = 1;
   bool in = true;
   bool buttons[NUM_BUTTONS] = {false};
   int i;
   float p = 0.0;
   ALLEGRO_COLOR black;

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

   al_set_new_display_flags(ALLEGRO_RESIZABLE);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

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
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
         al_draw_textf(font, black, 5, 5, 0, "dx %g, dy %g, dz %g, dw %g", mmx, mmy, mmz, mmw);
         al_draw_textf(font, black, 5, 25, 0, "x %g, y %g, z %g, w %g", mx, my, mz, mw);
         al_draw_textf(font, black, 5, 45, 0, "p = %g", p);
         al_draw_textf(font, black, 5, 65, 0, "%s", in ? "in" : "out");
         al_draw_textf(font, black, 5, 85, 0, "wheel precision (PgUp/PgDn) %d", precision);
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
         mmx = mmy = mmz = 0;
         al_flip_display();
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_MOUSE_AXES_FLOAT:
            mx = event.mouse_float.x;
            my = event.mouse_float.y;
            mz = event.mouse_float.z;
            mw = event.mouse_float.w;
            mmx = event.mouse_float.dx;
            mmy = event.mouse_float.dy;
            mmz = event.mouse_float.dz;
            mmw = event.mouse_float.dw;
            p = event.mouse_float.pressure;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN_FLOAT:
            if (event.mouse_float.button-1 < NUM_BUTTONS) {
               buttons[event.mouse_float.button-1] = true;
            }
            p = event.mouse.pressure;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP_FLOAT:
            if (event.mouse_float.button-1 < NUM_BUTTONS) {
               buttons[event.mouse_float.button-1] = false;
            }
            p = event.mouse_float.pressure;
            break;

         case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY_FLOAT:
            in = true;
            break;

         case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY_FLOAT:
            in = false;
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto done;
            }
            break;

         case ALLEGRO_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == ALLEGRO_KEY_PGUP) {
               precision++;
               al_set_mouse_wheel_precision(precision);
            }
            else if (event.keyboard.keycode == ALLEGRO_KEY_PGDN) {
               precision--;
               if (precision < 1)
                  precision = 1;
               al_set_mouse_wheel_precision(precision);
            }
            break;

         case ALLEGRO_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event.display.source);
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            goto done;
      }
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}


/* vim: set sw=3 sts=3 et: */
