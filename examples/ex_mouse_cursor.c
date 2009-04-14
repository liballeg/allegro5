/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Press '1', '2', '3', '4' or 'c' to select a mouse cursor.
 *    Press 's' or 'h' to show or hide the cursor.
 */


#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include "allegro5/a5_iio.h"


static void draw_display(ALLEGRO_FONT *font)
{
   al_set_target_bitmap(al_get_backbuffer());
   al_clear_to_color(al_map_rgb(128, 128, 128));
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgba_f(0, 0, 0, 1));
   al_draw_textf(font, 50, 20, 0, "Instructions:");
   al_draw_textf(font, 50, 50, 0, "<s> - show cursor");
   al_draw_textf(font, 50, 70, 0, "<h> - hide cursor");
   al_draw_textf(font, 50, 90, 0, "<1> - show cursor 1");
   al_draw_textf(font, 50, 110, 0, "<2> - show cursor 2");
   al_draw_textf(font, 50, 130, 0, "<3> - show cursor 3");
   al_draw_textf(font, 50, 150, 0, "<4> - show cursor 4");
   al_draw_textf(font, 50, 170, 0, "<c> - show custom cursor");
   al_draw_textf(font, 50, 190, 0, "<esc> - exit program");
   al_flip_display();
}

static void hide_cursor(void)
{
   if (!al_hide_mouse_cursor()) {
      TRACE("Error hiding mouse cursor\n");
   }
}

static void show_cursor(void)
{
   if (!al_show_mouse_cursor()) {
      TRACE("Error showing mouse cursor\n");
   }
}

static void test_set_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR cursor_id)
{
   if (!al_set_system_mouse_cursor(cursor_id)) {
      TRACE("Error setting system mouse cursor\n");
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display1;
   ALLEGRO_DISPLAY *display2;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_BITMAP *shrunk_bmp;
   ALLEGRO_MOUSE_CURSOR *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   ALLEGRO_EVENT event;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_init_iio_addon();

   if (!al_install_mouse()) {
      TRACE("Error installing mouse\n");
      return 1;
   }

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display1 = al_create_display(400, 300);
   if (!display1) {
      TRACE("Error creating display1\n");
      return 1;
   }

   display2 = al_create_display(400, 300);
   if (!display2) {
      TRACE("Error creating display2\n");
      return 1;
   }

   bmp = al_load_bitmap("data/allegro.pcx");
   if (!bmp) {
      TRACE("Error loading data/allegro.pcx\n");
      return 1;
   }

   font = al_load_bitmap_font("data/font.tga");
   if (!font) {
      TRACE("Error loading data/font.tga\n");
      return 1;
   }

   shrunk_bmp = al_create_bitmap(32, 32);
   if (!shrunk_bmp) {
      TRACE("Error creating shrunk_bmp\n");
      return 1;
   }

   al_set_target_bitmap(shrunk_bmp);
   al_draw_scaled_bitmap(bmp,
      0, 0, al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
      0, 0, 32, 32,
      0);

   cursor = al_create_mouse_cursor(shrunk_bmp, 0, 0);
   if (!cursor) {
      TRACE("Error creating mouse cursor\n");
      return 1;
   }

   al_destroy_bitmap(shrunk_bmp);
   al_destroy_bitmap(bmp);
   shrunk_bmp = NULL;
   bmp = NULL;

   if (!al_install_keyboard()) {
      TRACE("Error installing keyboard\n");
      return 1;
   }

   queue = al_create_event_queue();
   if (!queue) {
      TRACE("Error creating event queue\n");
      return 1;
   }

   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display1);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display2);

   al_set_current_display(display1);
   al_set_target_bitmap(al_get_backbuffer());
   draw_display(font);
   al_set_current_display(display2);
   al_set_target_bitmap(al_get_backbuffer());
   draw_display(font);

   al_show_mouse_cursor();

   while (1) {
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
         al_set_current_display(event.display.source);
         draw_display(font);
         continue;
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         al_set_current_display(event.keyboard.display);
         switch (event.keyboard.unichar) {
            case 27: /* escape */
               goto Quit;
            case 'h':
               hide_cursor();
               break;
            case 's':
               show_cursor();
               break;
            /* Is this part of the API?
            case '0':
               test_set_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_NONE);
               break;
            */
            case '1':
               test_set_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_ARROW);
               break;
            case '2':
               test_set_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_BUSY);
               break;
            case '3':
               test_set_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_QUESTION);
               break;
            case '4':
               test_set_cursor(ALLEGRO_SYSTEM_MOUSE_CURSOR_EDIT);
               break;
            case 'c':
               if (!al_set_mouse_cursor(cursor)) {
                  TRACE("Error setting custom mouse cursor\n");
               }
               break;
            default:
               break;
         }
      }
   }

Quit:

   al_destroy_mouse_cursor(cursor);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
