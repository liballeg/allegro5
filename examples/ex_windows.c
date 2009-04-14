#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include <stdlib.h>
#include <time.h>


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
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_mouse();
   al_init_font_addon();

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
   al_show_mouse_cursor();

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   myfont = al_load_font("data/fixed_font.tga", 0, 0);

   events = al_create_event_queue();
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)displays[0]);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)displays[1]);

   for (;;) {
      for (i = 0; i < 2; i++) {
        al_set_current_display(displays[i]);
        al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(255, 255, 255));
        if (i == 0)
           al_clear_to_color(al_map_rgb(255, 0, 255));
        else
           al_clear_to_color(al_map_rgb(155, 255, 0));
        al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(0, 0, 0));
        al_draw_textf(myfont, 50, 50, ALLEGRO_ALIGN_CENTRE, "Click me..");
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
END_OF_MAIN()

