#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"

#ifndef ALLEGRO_WINDOWS
#error FIXME: this example only works on Windows! We either need to add \
a function to the mouse driver to get an absolute mouse position, or \
add a function to get the X11 Display so we can use it in this example
#endif


void get_mouse_xy(int *x, int *y)
{
   POINT p;
   GetCursorPos(&p);
   if (x) *x = p.x;
   if (y) *y = p.y;
}


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;
   int i;
   bool down = false;
   int down_x, down_y;
   ALLEGRO_TIMER *timer;
   ALLEGRO_MONITOR_INFO info;

   al_init();
   al_install_mouse();
   al_install_keyboard();

   display = al_create_display(300, 200);
   
   bitmap = al_load_bitmap("fakeamp.bmp");

   al_remove_window_frame(display);
   
   al_show_mouse_cursor();

   timer = al_install_timer(1.0f/30.0f);

   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)timer);

   al_get_monitor_info(0, &info);

   al_start_timer(timer);

   for (;;) {
      al_wait_for_event(events, &event);
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         if (event.mouse.button == 1 && event.mouse.x >= 0 &&
               event.mouse.y >= 0 &&
               event.mouse.x < al_get_display_width() &&
               event.mouse.y < al_get_display_height()) {
            down = true;
            get_mouse_xy(&down_x, &down_y);
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
         if (event.mouse.button == 1) {
            down = false;
         }
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {
         if (down) {
            int cx, cy;
            int dx, dy;
            int wx, wy;
            get_mouse_xy(&cx, &cy);
            dx = cx - down_x;
            dy = cy - down_y;
            down_x = cx;
            down_y = cy;
            al_get_window_position(display, &wx, &wy);
            wx += dx;
            wy += dy;
            //if (wx >= info.x1 && wy >= info.y1 &&
              //    wx+al_get_display_width() < info.x2 &&
                //  wy+al_get_display_height() < info.y2) {
               al_set_window_position(display, wx, wy);
           // }
         }
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_TIMER) {
         al_draw_bitmap(bitmap, 0, 0, 0);
         al_flip_display();
      }
   }

   al_uninstall_timer(timer);
   al_destroy_event_queue(events);
   al_destroy_display(display);

   return 0;
}
END_OF_MAIN()

