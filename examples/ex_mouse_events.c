#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_primitives.h"

#define NUM_BUTTONS  3

static void draw_mouse_button(int but, bool down)
{
   const int offset[NUM_BUTTONS] = {0, 70, 35};
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

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   ALLEGRO_FONT *font;
   int mx = 0;
   int my = 0;
   int mz = 0;
   int mmx = 0;
   int mmy = 0;
   int mmz = 0;
   bool in = true;
   bool buttons[NUM_BUTTONS] = {false};
   int i;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_mouse();
   al_install_keyboard();
   al_init_iio_addon();
   al_init_font_addon();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   al_hide_mouse_cursor();

   cursor = al_load_bitmap("data/cursor.tga");
   if (!cursor) {
      TRACE("Error loading cursor.tga\n");
      return 1;
   }

   font = al_load_font("data/fixed_font.tga", 1, 0);
   if (!font) {
      TRACE("data/fixed_font.tga not found\n");
      return 1;
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source(al_get_mouse()));
   al_register_event_source(queue, al_get_keyboard_event_source(al_get_keyboard()));
   al_register_event_source(queue, al_get_display_event_source(display));

   while (1) {
      al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));
      for (i = 0; i < NUM_BUTTONS; i++) {
         draw_mouse_button(i, buttons[i]);
      }
      al_draw_bitmap(cursor, mx, my, 0);
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
         al_map_rgb_f(0, 0, 0));
      al_draw_textf(font, 5, 5, 0, "dx %i, dy %i, dz %i", mmx, mmy, mmz);
      al_draw_textf(font, 5, 15, 0, "x %i, y %i, z %i", mx, my, mz);
      al_draw_textf(font, 5, 25, 0, "%s", in ? "in" : "out");
      al_set_blender(ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA,
         al_map_rgb_f(1, 1, 1));
      mmx = mmy = mmz = 0;
      al_flip_display();

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_MOUSE_AXES:
            mx = event.mouse.x;
            my = event.mouse.y;
            mz = event.mouse.z;
            mmx = event.mouse.dx;
            mmy = event.mouse.dy;
            mmz = event.mouse.dz;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            if (event.mouse.button-1 < NUM_BUTTONS) {
               buttons[event.mouse.button-1] = true;
            }
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            if (event.mouse.button-1 < NUM_BUTTONS) {
               buttons[event.mouse.button-1] = false;
            }
            break;

         case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            in = true;
            break;

         case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            in = false;
            break;

         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
               goto done;
            }
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            goto done;
      }
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}

END_OF_MAIN()

/* vim: set sw=3 sts=3 et: */
