/* This is a temporary example to get a feeling for the new API, which was very
 * much in flux yet when this example was created.
 *
 * Right now, it moves a red rectangle across the screen with a speed of
 * 100 pixels / second, limits the FPS to 100 Hz to save CPU, and prints the
 * current FPS in the top right corner.
 */
#include "allegro.h"
#include "allegro/internal/aintern_bitmap.h"
#include <winalleg.h>
#include <math.h>

#include "d3d.h"

int main(void)
{
   AL_DISPLAY *display[3];
   AL_KEYBOARD *keyboard;
   AL_EVENT event;
   AL_EVENT_QUEUE *events;
   int quit = 0;
   int ticks = 0, last_rendered = 0, start_ticks;
   int fps_accumulator = 0, fps_time = 0;
   double fps = 0;
   int FPS = 500;
   int x = 0, y = 0;
   int dx = 1;
   int w = 640, h = 480;
   AL_BITMAP *picture;
   AL_BITMAP *mask;
   AL_BITMAP *mem_bmp;
   AL_COLOR colors[3];
   AL_COLOR white;
   AL_COLOR mask_color;
   int i;

   al_init();

   events = al_create_event_queue();

   al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_RGB_565);
   al_set_new_display_flags(AL_WINDOWED|AL_RESIZABLE);

   /* Create three windows. */
   display[0] = al_create_display(w, h);
   display[1] = al_create_display(w, h);

   al_set_new_display_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   al_set_new_display_flags(AL_WINDOWED);

   display[2] = al_create_display(w, h);

   /* This is only needed since we want to receive resize events. */
   al_register_event_source(events, (AL_EVENT_SOURCE *)display[0]);
   al_register_event_source(events, (AL_EVENT_SOURCE *)display[1]);

   al_register_event_source(events, (AL_EVENT_SOURCE *)display[2]);

   /* Apparently, need to think a bit more about memory/display bitmaps.. should
    * only need to load it once (as memory bitmap), then make available on all
    * displays.
    */
   al_set_current_display(display[2]);

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   al_set_new_bitmap_flags(AL_SYNC_MEMORY_COPY|AL_USE_ALPHA);

   picture = al_load_bitmap("mysha.tga");

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_4444);
   mask = al_load_bitmap("mask.pcx");
   
   al_set_new_bitmap_flags(AL_MEMORY_BITMAP);
   mem_bmp = al_load_bitmap("mysha.tga");

   AL_COLOR color;
   AL_LOCKED_REGION lr;
   al_lock_bitmap(picture, &lr, 0);
   al_set_target_bitmap(picture);
   for (y = 0; y < 100; y++) {
	for (x = 0; x < 160; x++) {
		al_put_pixel(x+160, y+100, al_get_pixel(picture, x, y, &color)) ;
	}
   }
   al_unlock_bitmap(picture);

   al_install_keyboard();
   al_register_event_source(events, (AL_EVENT_SOURCE *)al_get_keyboard());

   start_ticks = al_current_time();

   al_set_mask_color(al_map_rgb(picture, &colors[0], 255, 0, 255));
   al_set_target_bitmap(mask);
   al_draw_bitmap(picture, 0, 0, AL_MASK_SOURCE);
   al_set_target_bitmap(mask);
   al_convert_mask_to_alpha(picture, al_map_rgb(picture, &colors[0], 255, 0, 255));

   for (i = 0; i < 3; i++) {
   	al_set_current_display(display[i]);
	if (i == 0)
		al_map_rgba_f(al_get_backbuffer(), &colors[0], 1, 0, 0, 0.5f);
	else if (i == 1)
		al_map_rgba_f(al_get_backbuffer(), &colors[1], 0, 1, 0, 0.5f);
	else
		al_map_rgba_f(al_get_backbuffer(), &colors[2], 0, 0, 1, 0.5f);
   }
	
   long start = al_current_time();
   long last_move = al_current_time();
   int frames = 0;

   while (!quit) {
      /* read input */
      while (!al_event_queue_is_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == AL_EVENT_KEY_DOWN)
         {
            AL_KEYBOARD_EVENT *key = &event.keyboard;
            if (key->keycode == AL_KEY_ESCAPE) {
               quit = 1;
	    }
         }
         if (event.type == AL_EVENT_DISPLAY_RESIZE) {
            AL_DISPLAY_EVENT *display = &event.display;
            w = display->width;
            h = display->height;
            al_set_current_display(display->source);
            al_notify_resize();
         }
         if (event.type == AL_EVENT_DISPLAY_CLOSE)
         {
	   int i;
	    for (i = 0; i < 3; i++) {
	      if (display[i] == event.display.source)
	      	display[i] = 0;
	    }
	    al_destroy_display(event.display.source);
	    for (i = 0; i < 3; i++)
	    	if (display[i])
			goto not_done;
            quit = 1;
	    not_done:;
         }
      }

      while (last_move < al_current_time()) {
         last_move++;
	 x += dx;
         if (x == 0) dx = 1;
         if (x == w - 40) dx = -1;
      }

	#if 0
      /* handle game ticks */
      while (ticks * 1000 < (al_current_time() - start_ticks) * FPS) {
          x += dx;
          if (x == 0) dx = 1;
          if (x == w - 40) dx = -1;
          ticks++;
      }
      #endif

      /* render */
      //if (ticks > last_rendered) {
         frames++;
         for (i = 0; i < 3; i++) {
	    if (!display[i])
	       continue;
            al_set_current_display(display[i]);
	    al_map_rgb_f(al_get_backbuffer(), &white, 1, 1, 1);
	    al_set_target_bitmap(al_get_backbuffer());
            al_clear(&white);
	    if (i == 1) {
	    	al_draw_line(50, 50, 150, 150, &colors[0]);
	    }
            else if (i == 2) {
	    	al_draw_bitmap(picture, 0, 0, 0);
	    	al_draw_scaled_bitmap(picture, 0, 0, picture->w, picture->h,
			0, 0, 640, 480, 0);
		al_draw_bitmap_region(picture, 20, 20, 150, 150, 0, 0, 0);
		al_set_mask_color(al_map_rgb(mem_bmp, &mask_color, 255, 0, 255));
		al_draw_rotated_scaled_bitmap(mem_bmp, 160, 100, 320, 240, 1.5f, 1.5f, M_PI/4, AL_MASK_SOURCE|AL_FLIP_VERTICAL);
		al_draw_rotated_bitmap(mem_bmp, 160, 100, 320, 240, M_PI/4, 0);
	    }
            al_draw_filled_rectangle(x, y, x + 40, y + 40, &colors[i]);
            al_flip_display();
         }
         /*last_rendered = ticks;
         {
            int d = al_current_time() - fps_time;
            fps_accumulator++;
            if (d >= 1000) {
               fps_time += d;
               fps = 1000.0 * fps_accumulator / d;
               fps_accumulator = 0;
            }
         }
	 */
      //}
      /*
      else {
      	int r = start_ticks + 1000 * ticks / FPS - al_current_time();
	al_rest(r < 0 ? 0 : r);
      }
      */
   }

   printf("frames=%d start=%ld now=%ld\n", frames, start, al_current_time());
   printf("fps=%f\n", (float)(frames * 1000) / (float)(al_current_time()-start));

   return 0;
}
END_OF_MAIN();

