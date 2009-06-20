/* Test retrieving and settings possible modes.
 */
#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <stdio.h>

#define SCREEN_W 800
#define SCREEN_H 600

ALLEGRO_FONT *font;
int font_h;
int count;
int fpp;

static void display_formats(int n)
{
   int c = 0;
   int fppc =  fpp;

   if (n + fpp >= count)
      fppc = count - n - 1;

   while ((c++) != fppc) {
      char buffer[2048];
      memset(buffer, 0, 2048);

      al_draw_text(font, 10, SCREEN_H * 0.2 - font_h * 4, 0, "USE KEYS UP/DOWN TO LIST");
      al_draw_text(font, 10, SCREEN_H * 0.2 - font_h*2, 0,
                      "   color     color         color       depth stencil        accele- double         compa-");
      al_draw_text(font, 10, SCREEN_H * 0.2 - font_h*1, 0,
                      "   depth     sizes         shifts      size   size  samples  rated  buff    swap   tible");
      al_draw_text(font, 10, SCREEN_H * 0.2, 0, "----------------------------------------------------------------------------------------------");
      sprintf(buffer, "%3i: %02i | R%i G%i B%i A%i | R%02i G%02i B%02i A%02i |  %2i  |  %i  |  %2i  |  %3s  |  %3s  |   %i   |  %3s |",
                      n + c,
                      al_get_display_format_option(n + c, ALLEGRO_COLOR_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_RED_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_GREEN_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_BLUE_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_ALPHA_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_RED_SHIFT),
                      al_get_display_format_option(n + c, ALLEGRO_GREEN_SHIFT),
                      al_get_display_format_option(n + c, ALLEGRO_BLUE_SHIFT),
                      al_get_display_format_option(n + c, ALLEGRO_ALPHA_SHIFT),
                      al_get_display_format_option(n + c, ALLEGRO_DEPTH_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_STENCIL_SIZE),
                      al_get_display_format_option(n + c, ALLEGRO_SAMPLES),
                      al_get_display_format_option(n + c, ALLEGRO_RENDER_METHOD) ? "yes" : "no",
                      al_get_display_format_option(n + c, ALLEGRO_SINGLE_BUFFER) ? "yes" : "no",
                      al_get_display_format_option(n + c, ALLEGRO_SWAP_METHOD),
                      al_get_display_format_option(n + c, ALLEGRO_COMPATIBLE_DISPLAY) ? "yes" : "no");
      al_draw_text(font, 10, c * font_h + SCREEN_H * 0.2, 0, buffer);
   }
}


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   int n;
   bool redraw = false;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_init_font_addon();

   display = al_create_display(SCREEN_W, SCREEN_H);

   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      TRACE("data/fixed_font.tga not found\n");
      return 1;
   }

   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb_f(0, 0, 0));

   count = al_get_num_display_formats();
   n = 0;
   font_h = al_get_font_line_height(font);
   fpp = SCREEN_H / font_h * 0.8 - 1;
   al_clear_to_color(al_map_rgb_f(1, 1, 1));
   display_formats(0);
   al_flip_display();

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
         if (event.keyboard.keycode == ALLEGRO_KEY_UP
          || event.keyboard.keycode == ALLEGRO_KEY_PGUP) {
            if (n - fpp > 0)
               n -= fpp;
            else
               n = 0;
            redraw = true;
         }
         if (event.keyboard.keycode == ALLEGRO_KEY_DOWN
          || event.keyboard.keycode == ALLEGRO_KEY_PGDN) {
            if (n + fpp < count)
               n += fpp;
            redraw = true;
         }
      }

      if (redraw && al_event_queue_is_empty(queue)) {
         redraw = false;
         al_clear_to_color(al_map_rgb_f(1, 1, 1));
         display_formats(n);
         al_flip_display();
      }
   }

   al_destroy_font(font);

   return 0;
}
END_OF_MAIN()
