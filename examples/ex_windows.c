#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include <stdlib.h>
#include <time.h>

#include "common.c"

const int W = 400;
const int H = 200;


int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *displays[2];
   ALLEGRO_MONITOR_INFO *info;
   int adapter_count;
   int x, y;
   int jump_x[2], jump_y[2], jump_adapter[2];
   ALLEGRO_FONT *myfont;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_EVENT event;
   ALLEGRO_TIMER *timer;
   int i;

   (void)argc;
   (void)argv;

   srand(time(NULL));

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_mouse();
   al_init_font_addon();
   open_log();

   adapter_count = al_get_num_video_adapters();

   if (adapter_count == 0) {
      abort_example("No adapters found!\n");
   }

   info = malloc(adapter_count * sizeof(ALLEGRO_MONITOR_INFO));

   for (i = 0; i < adapter_count; i++) {
      al_get_monitor_info(i, &info[i]);
      log_printf("Monitor %d: %d %d - %d %d\n", i, info[i].x1, info[i].y1, info[i].x2, info[i].y2);
   }

   ALLEGRO_MONITOR_INFO init_info = info[0];
   x = init_info.x1 + ((init_info.x2 - init_info.x1) / 3) - (W / 2);
   y = init_info.y1 + ((init_info.y2 - init_info.y1) / 2) - (H / 2);
   jump_x[0] = x;
   jump_y[0] = y;
   jump_adapter[0] = 0;

   al_set_new_window_position(x, y);

   al_set_new_display_flags(ALLEGRO_RESIZABLE);
   displays[0] = al_create_display(W, H);

   x = init_info.x1 + (2 * (init_info.x2 - init_info.x1) / 3) - (W / 2);
   jump_x[1] = x;
   jump_y[1] = y;
   jump_adapter[1] = 0;
   al_set_new_window_position(x, y);

   displays[1] = al_create_display(W, H);

   if (!displays[0] || !displays[1]) {
      abort_example("Could not create displays.\n");
   }

   myfont = al_create_builtin_font();
   timer = al_create_timer(1 / 60.);

   events = al_create_event_queue();
   al_register_event_source(events, al_get_mouse_event_source());
   al_register_event_source(events, al_get_display_event_source(displays[0]));
   al_register_event_source(events, al_get_display_event_source(displays[1]));
   al_register_event_source(events, al_get_timer_event_source(timer));

   bool redraw = true;
   al_start_timer(timer);
   while (true) {
      if (al_is_event_queue_empty(events) && redraw) {
         for (i = 0; i < 2; i++) {
            int dx, dy, dw, dh;
            al_set_target_backbuffer(displays[i]);
            al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
            if (i == 0)
              al_clear_to_color(al_map_rgb(255, 0, 255));
            else
              al_clear_to_color(al_map_rgb(155, 255, 0));
            al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
            al_get_window_position(displays[i], &dx, &dy);
            dw = al_get_display_width(displays[i]);
            dh = al_get_display_height(displays[i]);
            al_draw_textf(myfont, al_map_rgb(0, 0, 0), dw / 2, dh / 2 - 15, ALLEGRO_ALIGN_CENTRE, "Location: %d %d", dx, dy);
            al_draw_textf(myfont, al_map_rgb(0, 0, 0), dw / 2, dh / 2, ALLEGRO_ALIGN_CENTRE, "Last jumped to: %d %d (adapter %d)", jump_x[i], jump_y[i], jump_adapter[i]);
            al_draw_textf(myfont, al_map_rgb(0, 0, 0), dw / 2, dh / 2 + 15, ALLEGRO_ALIGN_CENTRE, "Size: %dx%d", dw, dh);
            al_draw_textf(myfont, al_map_rgb(0, 0, 0), dw / 2, dh / 2 + 30, ALLEGRO_ALIGN_CENTRE, "Click me to jump!");

            al_flip_display();
         }
         redraw = false;
      }

      al_wait_for_event(events, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         int a = rand() % adapter_count;
         int w = info[a].x2 - info[a].x1;
         int h = info[a].y2 - info[a].y1;
         int margin = 20;
         int i = event.mouse.display == displays[0] ? 0 : 1;
         x = margin + info[a].x1 + (rand() % (w - W - 2 * margin));
         y = margin + info[a].y1 + (rand() % (h - H - 2 * margin));
         jump_x[i] = x;
         jump_y[i] = y;
         jump_adapter[i] = a;
         al_set_window_position(event.mouse.display, x, y);
      }
      else if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
      }
   }

   al_destroy_event_queue(events);

   al_destroy_display(displays[0]);
   al_destroy_display(displays[1]);

   free(info);

   close_log(true);

   return 0;
}

