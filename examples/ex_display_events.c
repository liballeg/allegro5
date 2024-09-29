#include <stdarg.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

#define MAX_EVENTS 23

static char events[MAX_EVENTS][1024];

static void add_event(char const *f, ...)
{
   va_list args;
   memmove(events[1], events[0], (MAX_EVENTS - 1) * 1024);
   va_start(args, f);
   vsnprintf(events[0], 1024, f, args);
   va_end(args);
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   A5O_FONT *font;
   A5O_COLOR color, black, red, blue;
   int i;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_mouse();
   al_install_keyboard();
   al_init_font_addon();

   al_set_new_display_flags(A5O_RESIZABLE);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   font = al_create_builtin_font();
   if (!font) {
      abort_example("Error creating builtin font\n");
   }
   
   black = al_map_rgb_f(0, 0, 0);
   red = al_map_rgb_f(1, 0, 0);
   blue = al_map_rgb_f(0, 0, 1);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   while (1) {
      if (al_is_event_queue_empty(queue)) {
         float x = 8, y = 28;
         al_clear_to_color(al_map_rgb(0xff, 0xff, 0xc0));
         
         al_draw_textf(font, blue, 8, 8, 0, "Display events (newest on top)");

         color = red;
         for (i = 0; i < MAX_EVENTS; i++) {
            if (!events[i]) continue;
            al_draw_textf(font, color, x, y, 0, "%s", events[i]);
            color = black;
            y += 20;
         }
         al_flip_display();
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_MOUSE_ENTER_DISPLAY:
            add_event("A5O_EVENT_MOUSE_ENTER_DISPLAY");
            break;

         case A5O_EVENT_MOUSE_LEAVE_DISPLAY:
            add_event("A5O_EVENT_MOUSE_LEAVE_DISPLAY");
            break;

         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
               goto done;
            }
            break;

         case A5O_EVENT_DISPLAY_RESIZE:
            add_event("A5O_EVENT_DISPLAY_RESIZE x=%d, y=%d, "
               "width=%d, height=%d",
               event.display.x, event.display.y, event.display.width,
               event.display.height);
            al_acknowledge_resize(event.display.source);
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            add_event("A5O_EVENT_DISPLAY_CLOSE");
            break;
         
         case A5O_EVENT_DISPLAY_LOST:
            add_event("A5O_EVENT_DISPLAY_LOST");
            break;
         
         case A5O_EVENT_DISPLAY_FOUND:
            add_event("A5O_EVENT_DISPLAY_FOUND");
            break;
         
         case A5O_EVENT_DISPLAY_SWITCH_OUT:
            add_event("A5O_EVENT_DISPLAY_SWITCH_OUT");
            break;
         
         case A5O_EVENT_DISPLAY_SWITCH_IN:
            add_event("A5O_EVENT_DISPLAY_SWITCH_IN");
            break;
      }
   }

done:

   al_destroy_event_queue(queue);

   return 0;
}


/* vim: set sw=3 sts=3 et: */
