/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Press '1', '2', '3', '4' or 'c' to select a mouse cursor.
 *    Press 's' or 'h' to show or hide the cursor.
 */


#include <allegro5/allegro5.h>
#include "allegro5/a5_iio.h"


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
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_BITMAP *shrunk_bmp;
   ALLEGRO_MOUSE_CURSOR *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;

   al_init();
   iio_init();

   display = al_create_display(400, 300);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   /* XXX work around blitting problem */
   /* this problem no longer appears in X, but did in Wine */
   /*al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);*/
   bmp = iio_load("allegro.pcx");
   if (!bmp) {
      TRACE("Error loading mysha.pcx\n");
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
      0, 0, 128, 128,
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

   if (!al_install_mouse()) {
      TRACE("Error installing mouse\n");
      return 1;
   }

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
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());

   /* XXX some instructions on this blank screen would be nice */

   while (1) {
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
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

/* vi: set sts=3 sw=3 et: */
