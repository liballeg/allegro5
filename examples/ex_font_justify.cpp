/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test text justification routines.
 */

#include <string>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_ttf.h"
#include <allegro5/allegro_primitives.h>
#include "nihgui.hpp"

#include "common.c"

ALLEGRO_FONT *font;
ALLEGRO_FONT *font_gui;

class Prog {
private:
   Dialog d;
   Label text_label;
   Label width_label;
   Label diff_label;
   TextEntry text_entry;
   HSlider width_slider;
   HSlider diff_slider;

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();
   void draw_text();
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 10, 20)),
   text_label(Label("Text")),
   width_label(Label("Width")),
   diff_label(Label("Diff")),
   text_entry(TextEntry("Lorem ipsum dolor sit amet")),
   width_slider(HSlider(400, al_get_display_width(display))),
   diff_slider(HSlider(100, al_get_display_width(display)))
{
   d.add(text_label, 0, 10, 1, 1);
   d.add(text_entry, 1, 10, 8, 1);

   d.add(width_label,  0, 12, 1, 1);
   d.add(width_slider, 1, 12, 8, 1);

   d.add(diff_label,  0, 14, 1, 1);
   d.add(diff_slider, 1, 14, 8, 1);
}

void Prog::run()
{
   d.prepare();

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

void Prog::draw_text()
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   const int cx = al_get_bitmap_width(target) / 2;
   const int x1 = cx - width_slider.get_cur_value() / 2;
   const int x2 = cx + width_slider.get_cur_value() / 2;
   const int diff = diff_slider.get_cur_value();
   const int th = al_get_font_line_height(font);

   al_draw_justified_text(font, al_map_rgb_f(1, 1, 1), x1, x2, 50, diff,
      ALLEGRO_ALIGN_INTEGER, text_entry.get_text());

   al_draw_rectangle(x1, 50, x2, 50 + th, al_map_rgb(0, 0, 255), 0);

   al_draw_line(cx - diff / 2, 53 + th, cx + diff / 2, 53 + th,
      al_map_rgb(0, 255, 0), 0);
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

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to create display\n");
   }

   /* Test TTF fonts or bitmap fonts. */
#if 1
   font = al_load_font("data/DejaVuSans.ttf", 24, 0);
   if (!font) {
      abort_example("Failed to load data/DejaVuSans.ttf\n");
   }
#else
   font = al_load_font("data/font.tga", 0, 0);
   if (!font) {
      abort_example("Failed to load data/font.tga\n");
   }
#endif

   font_gui = al_load_font("data/DejaVuSans.ttf", 14, 0);
   if (!font_gui) {
      abort_example("Failed to load data/DejaVuSans.ttf\n");
   }

   /* Don't remove these braces. */
   {
      Theme theme(font_gui);
      Prog prog(theme, display);
      prog.run();
   }

   al_destroy_font(font);
   al_destroy_font(font_gui);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
