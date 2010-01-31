/* Test retrieving and settings possible modes.
 * 
 * FIXME: Need a way to query the actually used settings and display
 *        them somewhere.
 * FIXME: Handle case when a mode can't be set with the given
 *        options, like keep the current display and print an error
 *        message.
 * FIXME: I can't test fullscreen support - I only get a single mode
 *        listed which does not work. Most likely the format parameter
 *        should be used somehow.
 * 
 */
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <stdio.h>

#include "common.c"

ALLEGRO_FONT *font;
int font_h;
int modes_count;
int options_count;

int selected_column;
int selected_mode;
int selected_option;

#define X(x, m) {#x, ALLEGRO_##x, 0, m, 0}
struct {
    char const *name;
    int option;
    int value, max_value;
    int required;
} options[] = {
    X(COLOR_SIZE, 32),
    X(RED_SIZE, 8),
    X(GREEN_SIZE, 8),
    X(BLUE_SIZE, 8),
    X(ALPHA_SIZE, 8),
    X(RED_SHIFT, 32),
    X(GREEN_SHIFT, 32),
    X(BLUE_SHIFT, 32),
    X(ALPHA_SHIFT, 32),
    X(DEPTH_SIZE, 32),
    X(FLOAT_COLOR, 1),
    X(FLOAT_DEPTH, 1),
    X(STENCIL_SIZE, 32),
    X(SAMPLE_BUFFERS, 1),
    X(SAMPLES, 8),
    X(RENDER_METHOD, 1),
    X(SINGLE_BUFFER, 1),
    X(SWAP_METHOD, 1),
    X(VSYNC, 1),
    X(COMPATIBLE_DISPLAY, 1),
};

static void display_options(void)
{
   int i, y = 10;
   int x = 10;
   int n = options_count;

   ALLEGRO_COLOR c;
   c = al_map_rgb_f(0.8, 0.8, 1);
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, c);
   al_draw_textf(font, x, y, 0, "Video modes");
   y += font_h;
   for (i = 0; i < modes_count + 1; i++) {
      ALLEGRO_DISPLAY_MODE mode;
      if (i < modes_count) {
         al_get_display_mode(i, &mode);
      }
      else {
         mode.width = 800;
         mode.height = 600;
         mode.format = 0;
         mode.refresh_rate = 0;
      }
      if (selected_column == 0 && selected_mode == i)
         c = al_map_rgb_f(1, 0, 0);
      else
         c = al_map_rgb_f(0, 0, 0);
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, c);
      al_draw_textf(font, x, y, 0, "%s %d x %d (%d, %d)",
         i < modes_count ? "Fullscreen" : "Windowed",
         mode.width,
         mode.height, mode.format, mode.refresh_rate);
      y += font_h;
   }
   
   x = al_get_display_width() / 2 + 10;
   y = 10;
   c = al_map_rgb_f(0.8, 0.8, 1);
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, c);
   al_draw_textf(font, x, y, 0, "Display options");
   y += font_h;
   for (i = 0; i < n; i++) {
      if (selected_column == 1 && selected_option == i)
         c = al_map_rgb_f(1, 0, 0);
      else
         c = al_map_rgb_f(0, 0, 0);
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, c);
      al_draw_textf(font, x, y, 0, "%s: %d (%s)", options[i].name,
         options[i].value,
            options[i].required == ALLEGRO_REQUIRE ? "required" :
            options[i].required == ALLEGRO_SUGGEST ? "suggested" :
            "ignored");
      y += font_h;
   }
   
   c = al_map_rgb_f(0, 0, 0.8);
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, c);
   x = 10;
   y = al_get_display_height() - 10;
   y -= font_h;
   al_draw_textf(font, x, y, 0, "PageUp/Down: modify values");
   y -= font_h;
   al_draw_textf(font, x, y, 0, "Return: set mode or require option");
   y -= font_h;
   al_draw_textf(font, x, y, 0, "Cursor keys: change selection");
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue;
   int n;
   bool redraw = false;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_init_font_addon();

   display = al_create_display(800, 600);

   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      abort_example("data/fixed_font.tga not found\n");
      return 1;
   }

   modes_count = al_get_num_display_modes();
   options_count = sizeof(options) / sizeof(options[0]);
   n = 0;
   font_h = al_get_font_line_height(font);
   al_clear_to_color(al_map_rgb_f(1, 1, 1));
   display_options();
   al_flip_display();

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == ALLEGRO_EVENT_KEY_DOWN ||
         event.type == ALLEGRO_EVENT_KEY_REPEAT) {
         int change;
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
         if (event.keyboard.keycode == ALLEGRO_KEY_LEFT) {
            selected_column = 0;
            redraw = true;
         }
         if (event.keyboard.keycode == ALLEGRO_KEY_RIGHT) {
            selected_column = 1;
            redraw = true;
         }
         if (event.keyboard.keycode == ALLEGRO_KEY_UP) {
            if (selected_column == 0) selected_mode -= 1;
            if (selected_column == 1) selected_option -= 1;
            redraw = true;
         }
         if (event.keyboard.keycode == ALLEGRO_KEY_DOWN) {
            if (selected_column == 0) selected_mode += 1;
            if (selected_column == 1) selected_option += 1;
            redraw = true;
         }
         if (event.keyboard.keycode == ALLEGRO_KEY_ENTER) {
             if (selected_column == 0) {
                ALLEGRO_DISPLAY_MODE mode;
                ALLEGRO_DISPLAY *new_display;
                if (selected_mode < modes_count)
                    al_get_display_mode(selected_mode, &mode);
                else {
                    mode.width = 800;
                    mode.height = 600;
                }
                new_display = al_create_display(
                   mode.width, mode.height);
                al_destroy_display(display);
                display = new_display;
                al_register_event_source(queue, al_get_display_event_source(display));
             }
             if (selected_column == 1) {
                 options[selected_option].required += 1;
                 options[selected_option].required %= 3;
                 al_set_new_display_option(selected_option,
                    options[selected_option].value,
                    options[selected_option].required);
             }
             redraw = true;
         }
         change = 0;
         if (event.keyboard.keycode == ALLEGRO_KEY_PGUP) change = 1;
         if (event.keyboard.keycode == ALLEGRO_KEY_PGDN) change = -1;
         if (change && selected_column == 1) {
             options[selected_option].value += change;
             if (options[selected_option].value < 0)
                options[selected_option].value = 0;
            if (options[selected_option].value > options[selected_option].max_value)
                options[selected_option].value = options[selected_option].max_value;
             redraw = true;
         }
      }
      
      if (selected_mode < 0) selected_mode = 0;
      if (selected_mode > modes_count) selected_mode = modes_count;
      if (selected_option < 0) selected_option = 0;
      if (selected_option >= options_count) selected_option = options_count - 1;

      if (redraw && al_event_queue_is_empty(queue)) {
         redraw = false;
         al_clear_to_color(al_map_rgb_f(1, 1, 1));
         display_options();
         al_flip_display();
      }
   }

   al_destroy_font(font);

   return 0;
}
