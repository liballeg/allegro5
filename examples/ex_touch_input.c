#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

void draw_touches(int num, int *touches)
{
   int i;

   for (i = 0; i < num; i++) {
      int x = touches[i*3+1];
      int y = touches[i*3+2];
      al_draw_circle(x, y, 50, al_map_rgb(255, 0, 0), 4);
   }
}

int find_index(int id, int num, int *touches)
{
   int i;

   for (i = 0; i < num; i++) {
      if (touches[i*3] == id) {
         return i;
      }
   }

   return -1;
}

int main(int argc, char **argv)
{
   int num_touches = 0;
   int *touches = NULL;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;

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

   while (true) {
      al_clear_to_color(al_map_rgb(255, 255, 255));
      draw_touches(num_touches, touches);
      al_flip_display();

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_TOUCH_BEGIN) {
         int current;
         if (touches == NULL) {
            touches = (int *)malloc(3*sizeof(int));
            current = 0;
            num_touches = 1;
         }
         else {
            touches = (int *)realloc(touches, (num_touches+1)*3*sizeof(int));
            current = num_touches++;
         }
         touches[current*3+0] = event.touch.id;
         touches[current*3+1] = event.touch.x;
         touches[current*3+2] = event.touch.y;
      }
      else if (event.type == ALLEGRO_EVENT_TOUCH_END) {
         if (num_touches == 1) {
            free(touches);
            touches = NULL;
            num_touches--;
         }
         else {
            int index = find_index(event.touch.id, num_touches, touches);
            if (index >= 0) {
               memcpy(touches+index*3, touches+(index+1)*3, ((num_touches-1)-index)*3*sizeof(int));
               touches = (int *)realloc(touches, (--num_touches)*3*sizeof(int));
            }
         }
      }
      else if (event.type == ALLEGRO_EVENT_TOUCH_MOVE) {
         int index = find_index(event.touch.id, num_touches, touches);
         if (index >= 0) {
            touches[index*3+1] = event.touch.x;
            touches[index*3+2] = event.touch.y;
         }
      }
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
