#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include <stdlib.h>
#include <time.h>

#include "common.c"

const int W = 100;
const int H = 100;


int main(void)
{
   ALLEGRO_DISPLAY *displays[2];
   ALLEGRO_MONITOR_INFO *info;
   int adapter_count;
   int x, y;
   ALLEGRO_FONT *myfont;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;
   int i;

   srand(time(NULL));

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_init_font_addon();
   al_init_image_addon();

   adapter_count = al_get_num_video_adapters();

   info = malloc(adapter_count * sizeof(ALLEGRO_MONITOR_INFO));

   for (i = 0; i < adapter_count; i++) {
      al_get_monitor_info(i, &info[i]);
   }

   x = ((info[0].x2 - info[0].x1) / 3) - (W / 2);
   y = ((info[0].y2 - info[0].y1) / 2) - (H / 2);

   al_set_new_window_position(x, y);

   displays[0] = al_create_display(W, H);

   x *= 2;
   al_set_new_window_position(x, y);

   displays[1] = al_create_display(W, H);

   if (!displays[0] || !displays[1]) {
      abort_example("Could not create displays.\n");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   myfont = al_load_font("data/fixed_font.tga", 0, 0);
   if (!myfont) {
      abort_example("Could not load font.\n");
   }

   events = al_create_event_queue();
   al_register_event_source(events, al_get_mouse_event_source());
   al_register_event_source(events, al_get_display_event_source(displays[0]));
   al_register_event_source(events, al_get_display_event_source(displays[1]));

   for (;;) {
      for (i = 0; i < 2; i++) {
        al_set_target_backbuffer(displays[i]);
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
        if (i == 0)
           al_clear_to_color(al_map_rgb(255, 0, 255));
        else
           al_clear_to_color(al_map_rgb(155, 255, 0));
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
        al_draw_textf(myfont, al_map_rgb(0, 0, 0), 50, 50, ALLEGRO_ALIGN_CENTRE, "Click me..");
        al_flip_display();
      }

      if (al_wait_for_event_timed(events, &event, 1)) {
         if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
         }
         else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            int a = rand() % adapter_count;
            int w = info[a].x2 - info[a].x1;
            int h = info[a].y2 - info[a].y1;
            int margin = 20;
            x = margin + info[a].x1 + (rand() % (w - W - margin));
            y = margin + info[a].y1 + (rand() % (h - H - margin));
            al_set_window_position(event.mouse.display, x, y);
         }
      }
   }

   al_destroy_event_queue(events);

   al_destroy_display(displays[0]);
   al_destroy_display(displays[1]);

   free(info);

   return 0;
}

