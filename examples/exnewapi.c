/* This is a temporary example to get a feeling for the new API, which was very
 * much in flux yet when this example was created.
 *
 * Right now, it moves a red rectangle across the screen with a speed of
 * 100 pixels / second, limits the FPS to 100 Hz to save CPU, and prints the
 * current FPS in the top right corner.
 */
#include <allegro.h>

int main(void)
{
   AL_DISPLAY *display;
   AL_KEYBOARD *keyboard;
   AL_EVENT event;
   AL_EVENT_QUEUE *events;
   BITMAP *buffer;
   int quit = 0;
   int ticks = 0, last_rendered = 0, start_ticks;
   int fps_accumulator = 0, fps_time = 0;
   double fps = 0;
   int FPS = 100;
   int x = 0, y = 0;
   int dx = 1;

   allegro_init();
   
   events = al_create_event_queue();
   
   al_install_keyboard();
   al_register_event_source(events, (AL_EVENT_SOURCE *)al_get_keyboard());

   display = al_create_display(GFX_AUTODETECT, AL_UPDATE_DOUBLE_BUFFER,
      AL_DEPTH_32, 640, 480);

   start_ticks = al_current_time();
   while (!quit) {
      /* read input */
      while (!al_event_queue_is_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == AL_EVENT_KEY_DOWN)
         {
            AL_KEYBOARD_EVENT *key = &event.keyboard;
            if (key->keycode == AL_KEY_ESCAPE)
               quit = 1;
         }
      }
      
      /* handle game ticks */
      while (ticks * 1000 < (al_current_time() - start_ticks) * FPS) {
          x += dx;
          if (x == 0) dx = 1;
          if (x == 640 - 40) dx = -1;
          ticks++;
      }
      
      /* render */
      if (ticks > last_rendered) {

         buffer = al_get_buffer(display);
         clear_to_color(buffer, makecol(0, 0, 0));
         rectfill(buffer, x, y, x + 40, y + 40, makecol(255, 0, 0));
         textprintf_right_ex(buffer, font, 640, 0, makecol(255, 255, 255), -1,
            "%.1f", fps);
         al_flip_display(display);
         last_rendered = ticks;
         
         {
            int d = al_current_time() - fps_time;
            fps_accumulator++;
            if (d >= 1000) {
               fps_time += d;
               fps = 1000.0 * fps_accumulator / d;
               fps_accumulator = 0;
            }
         }
      }
      else {
         al_rest(start_ticks + 1000 * ticks / FPS - al_current_time());
      }
   }
   
   return 0;
}
END_OF_MAIN()
