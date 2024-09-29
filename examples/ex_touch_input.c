#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define MAX_TOUCHES 16

typedef struct TOUCH TOUCH;
struct TOUCH {
   int id;
   int x;
   int y;
};

static void draw_touches(int num, TOUCH touches[])
{
   int i;

   for (i = 0; i < num; i++) {
      int x = touches[i].x;
      int y = touches[i].y;
      al_draw_circle(x, y, 50, al_map_rgb(255, 0, 0), 4);
   }
}

static int find_index(int id, int num, TOUCH touches[])
{
   int i;

   for (i = 0; i < num; i++) {
      if (touches[i].id == id) {
         return i;
      }
   }

   return -1;
}

int main(int argc, char **argv)
{
   int num_touches = 0;
   bool quit = false;
   bool background = false;
   TOUCH touches[MAX_TOUCHES];
   A5O_DISPLAY *display;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   al_init_primitives_addon();
   if (!al_install_touch_input()) {
      abort_example("Could not init touch input.\n");
   }

   display = al_create_display(800, 600);
   if (!display) {
       abort_example("Error creating display\n");
   }
   queue = al_create_event_queue();

   al_register_event_source(queue, al_get_touch_input_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   while (!quit) {
      if (!background && al_is_event_queue_empty(queue)) {
         al_clear_to_color(al_map_rgb(255, 255, 255));
         draw_touches(num_touches, touches);
         al_flip_display();
      }
      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            quit = true;
            break;
         case A5O_EVENT_TOUCH_BEGIN: {
            int i = num_touches;
            if (num_touches < MAX_TOUCHES) {
               touches[i].id = event.touch.id;
               touches[i].x = event.touch.x;
               touches[i].y = event.touch.y;
               num_touches++;
            }
            break;
         }
         case A5O_EVENT_TOUCH_END: {
            int i = find_index(event.touch.id, num_touches, touches);
            if (i >= 0 && i < num_touches) {
               touches[i] = touches[num_touches - 1];
               num_touches--;
            }
            break;
         }
         case A5O_EVENT_TOUCH_MOVE: {
            int i = find_index(event.touch.id, num_touches, touches);
            if (i >= 0) {
               touches[i].x = event.touch.x;
               touches[i].y = event.touch.y;
            }
            break;
         }
         case A5O_EVENT_DISPLAY_HALT_DRAWING:
            background = true;
            al_acknowledge_drawing_halt(event.display.source);
            break;
         case A5O_EVENT_DISPLAY_RESUME_DRAWING:
            background = false;
            al_acknowledge_drawing_resume(event.display.source);
            break;
         case A5O_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event.display.source);
            break;
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
