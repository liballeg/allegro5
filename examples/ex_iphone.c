#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

const int MAX_TOUCHES = 5;

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *cursor;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   ALLEGRO_FONT *font;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT touch_events[MAX_TOUCHES];
   int touch = 0;
   bool in = true;
   bool down = false;
   int i;
   
   memset(touch_events, 0, sizeof(touch_events));
   
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_primitives_addon();
   al_install_mouse();
   al_init_image_addon();
   al_init_font_addon();

   display = al_create_display(480, 320);
   if (!display) {
      abort_example("Error creating display\n");
   }

   al_hide_mouse_cursor();

   cursor = al_load_bitmap("data/cursor.tga");
   if (!cursor) {
      abort_example("Error loading cursor.tga\n");
   }

   font = al_load_font("data/fixed_font.tga", 1, 0);
   if (!font) {
      abort_example("data/fixed_font.tga not found\n");
   }

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   
   timer = al_create_timer(1/10.0);
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      al_wait_for_event(queue, &event);
         switch (event.type) {
         case ALLEGRO_EVENT_MOUSE_AXES:
            touch_events[touch] = event;
            touch++;
            touch %= MAX_TOUCHES;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            down = true;
            break;

         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            down = false;
            break;

         case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            in = true;
            break;

         case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            in = false;
            break;

         case ALLEGRO_EVENT_TIMER:
            al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));
            if (down) {
               for (i = 0; i < MAX_TOUCHES; i++) {
                  al_draw_bitmap(cursor, touch_events[i].mouse.x, touch_events[i].mouse.y, 0);
               }
            }
            al_flip_display();
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
   
