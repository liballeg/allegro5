/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test multi line text routines.
 */

#include <string>
#include <cmath>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_ttf.h"
#include "allegro5/allegro_color.h"
#include "allegro5/allegro_primitives.h"
#include "nihgui.hpp"

#include "common.c"

#define TEST_TEXT "This is utf-8 €€€€€ multi line text output with a\nhard break,\n\ntwice even!"

/* This is a custom mult line output function that demonstrates
 * al_do_multiline_text. See below for the implementation. */
static void draw_custom_multiline(ALLEGRO_FONT * font, int x, int y,
   int max_width, int line_height, int tick, const char * text);

ALLEGRO_TIMER *timer;
ALLEGRO_FONT  *font;
ALLEGRO_FONT  *font_ttf;
ALLEGRO_FONT  *font_bmp;
ALLEGRO_FONT  *font_gui;
ALLEGRO_FONT  *font_bin;

class Prog : public EventHandler {
private:
   Dialog d;
   Label text_label;
   Label width_label;
   Label height_label;
   Label align_label;
   Label font_label;

   TextEntry text_entry;
   HSlider width_slider;
   VSlider height_slider;
   List text_align;
   List text_font;
   int tick;

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   virtual ~Prog() {}
   void run();
   void draw_text();
   void handle_event(const ALLEGRO_EVENT & event);
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 14, 20)),
   text_label(Label("Text")),
   width_label(Label("Width")),
   height_label(Label("Line height")),
   align_label(Label("Align")),
   font_label(Label("Font")),
   text_entry(TextEntry(TEST_TEXT)),
   width_slider(HSlider(200, al_get_display_width(display))),
   height_slider(VSlider(0, 50)),
   text_align(List(0)),
   text_font(List(0))
{
   text_align.append_item("Left");
   text_align.append_item("Center");
   text_align.append_item("Right");

   text_font.append_item("Truetype");
   text_font.append_item("Bitmap");
   text_font.append_item("Builtin");

   d.add(text_label, 0, 14, 1, 1);
   d.add(text_entry, 1, 14, 12, 1);

   d.add(width_label,  0, 15, 1, 1);
   d.add(width_slider, 1, 15, 12, 1);


   d.add(align_label,  0, 17, 2, 1);
   d.add(text_align ,  2, 17, 2, 3);

   d.add(font_label,  4, 17, 2, 1);
   d.add(text_font ,  6, 17, 2, 3);

   d.add(height_label,  8, 17, 2, 1);
   d.add(height_slider, 10, 17, 2, 3);
}

void Prog::run()
{
   d.prepare();

   d.register_event_source(al_get_timer_event_source(timer));
   d.set_event_handler(this);

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear_to_color(al_map_rgb(128, 128, 128));
         draw_text();
         d.draw();
         al_flip_display();
      }

      d.run_step(true);
   }
}

void Prog::handle_event(const ALLEGRO_EVENT & event)
{
   if (event.type == ALLEGRO_EVENT_TIMER) {
      tick = (int)event.timer.count;
      d.request_draw();
   }
}

void Prog::draw_text()
{
   int x  = 10, y  = 10;
   int sx = 10, sy = 10;
   int w = width_slider.get_cur_value();
   int h = height_slider.get_cur_value();
   int flags = 0;
   const char * text = text_entry.get_text();

   if (text_font.get_selected_item_text() == "Truetype") {
      font = font_ttf;
   } else if (text_font.get_selected_item_text() == "Bitmap") {
      font = font_bmp;
   } else if (text_font.get_selected_item_text() == "Builtin") {
      font = font_bin;
   }

   if (text_align.get_selected_item_text() == "Left") {
      flags = ALLEGRO_ALIGN_LEFT | ALLEGRO_ALIGN_INTEGER;
   } else if (text_align.get_selected_item_text() == "Center") {
      flags = ALLEGRO_ALIGN_CENTER | ALLEGRO_ALIGN_INTEGER;
      x = 10 + w / 2;
   } else if (text_align.get_selected_item_text() == "Right") {
      flags = ALLEGRO_ALIGN_RIGHT | ALLEGRO_ALIGN_INTEGER;
      x  = 10 + w;
   }


   /* Draw a red rectangle on the top with the requested width,
    * a blue rectangle around the real bounds of the text,
    * a green line for the X axis location of drawing the text
    * and the line height, and finally the text itself.
    */

   al_draw_rectangle(sx, sy-2 + 30, sx + w, sy - 1 + 30, al_map_rgb(255, 0, 0), 0);
   al_draw_line(x, y + 30, x, y + h + 30, al_map_rgb(0, 255, 0), 0);
   al_draw_multiline_text(font, al_map_rgb_f(1, 1, 1), x, y + 30, w, h, flags, text);

   /* also do some custom bultiline drawing */
   al_draw_text(font, al_map_rgb_f(1, 1, 1), w + 10, y, 0, "Custom multiline text:" );
   draw_custom_multiline(font, w + 10 , y + 30, w, h, tick, text);
}

int main(int argc, char *argv[])
{
   ALLEGRO_DISPLAY *display;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro\n");
   }
   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();

   al_init_image_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   init_platform_specific();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to create display\n");
   }

   /* Test TTF fonts and bitmap fonts. */
   font_ttf = al_load_font("data/DejaVuSans.ttf", 24, 0);
   if (!font_ttf) {
      abort_example("Failed to load data/DejaVuSans.ttf\n");
   }

   font_bmp = al_load_font("data/font.tga", 0, 0);
   if (!font_bmp) {
      abort_example("Failed to load data/font.tga\n");
   }

   font_bin = al_create_builtin_font();

   font = font_ttf;

   font_gui = al_load_font("data/DejaVuSans.ttf", 14, 0);
   if (!font_gui) {
      abort_example("Failed to load data/DejaVuSans.ttf\n");
   }

   timer = al_create_timer(1.0 / 60);
   al_start_timer(timer);

   /* Don't remove these braces. */
   {
      Theme theme(font_gui);
      Prog prog(theme, display);
      prog.run();
   }

   al_destroy_font(font_bmp);
   al_destroy_font(font_ttf);
   al_destroy_font(font_bin);
   al_destroy_font(font_gui);
   al_destroy_timer(timer);

   return 0;
}


/* Helper struct for draw_custom_multiline. */
typedef struct DRAW_CUSTOM_LINE_EXTRA {
   const ALLEGRO_FONT *font;
   float x;
   float y;
   int tick;
   float line_height;
   int flags;
} DRAW_CUSTOM_LINE_EXTRA;


/* This function is the helper callback that implements the actual drawing
 * for draw_custom_multiline.
 */
static bool draw_custom_multiline_cb(int line_num, const char *line, int size,
   void *extra) {
   DRAW_CUSTOM_LINE_EXTRA *s = (DRAW_CUSTOM_LINE_EXTRA *) extra;
   float x, y;
   ALLEGRO_USTR_INFO info;
   ALLEGRO_COLOR c =
      al_color_hsv(fmod(360.0 * (float)line_num / 5.0 + s->tick, 360.0),
                   1.0, 1.0);
   x  = s->x + 10 + sin(line_num + s->tick * 0.05) * 10;
   y  = s->y + (s->line_height * line_num);
   al_draw_ustr(s->font, c, x, y, 0, al_ref_buffer(&info, line, size));
   return (line_num < 5);
}

/* This is a custom mult line output function that demonstrates
 * al_do_multiline_text. */
static void draw_custom_multiline(ALLEGRO_FONT * font, int x, int y,
   int max_width, int line_height, int tick, const char * text) {
   DRAW_CUSTOM_LINE_EXTRA extra;

   extra.font = font;
   extra.x = x;
   extra.y = y;
   extra.line_height = line_height + al_get_font_line_height(font);
   extra.tick = tick;

   al_do_multiline_text(font, max_width, text,
      draw_custom_multiline_cb, (void *)&extra);
}


/* vim: set sts=3 sw=3 et: */
