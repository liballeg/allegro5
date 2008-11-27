/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Compare software blending routines with hardware blending.
 */

#include <string>
#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"

#include "nihgui.hpp"

ALLEGRO_BITMAP *allegro;
ALLEGRO_BITMAP *mysha;
ALLEGRO_BITMAP *allegro_bmp;
ALLEGRO_BITMAP *mysha_bmp;

class Prog {
private:
   Dialog d;
   List slst;
   List dlst;
   VSlider r;
   VSlider g;
   VSlider b;
   VSlider a;

public:
   explicit Prog(const Theme & theme);
   void run();

private:
   void draw_samples();
};

Prog::Prog(const Theme & theme) :
   d(Dialog(theme, 20, 20)),
   r(VSlider(255, 255)),
   g(VSlider(255, 255)),
   b(VSlider(255, 255)),
   a(VSlider(255, 255))
{
   slst.append_item("ONE");
   slst.append_item("ZERO");
   slst.append_item("ALPHA");
   slst.append_item("INVERSE_ALPHA");
   d.add(slst, 1, 15, 4, 4);

   dlst.append_item("ONE");
   dlst.append_item("ZERO");
   dlst.append_item("ALPHA");
   dlst.append_item("INVERSE_ALPHA");
   d.add(dlst, 5, 15, 4, 4);

   d.add(r, 10, 15, 1, 4);
   d.add(g, 11, 15, 1, 4);
   d.add(b, 12, 15, 1, 4);
   d.add(a, 13, 15, 1, 4);
}

void Prog::run()
{
   d.prepare(al_get_current_display());

   while (!d.is_quit_requested()) {
      al_clear(al_map_rgb(128, 128, 128));
      draw_samples();
      d.draw();
      al_flip_display();

      d.run_step(true);
   }
}

int str_to_blend_mode(const std::string & str)
{
   if (str == "ZERO")
      return ALLEGRO_ZERO;
   if (str == "ONE")
      return ALLEGRO_ONE;
   if (str == "ALPHA")
      return ALLEGRO_ALPHA;
   if (str == "INVERSE_ALPHA")
      return ALLEGRO_INVERSE_ALPHA;

   ASSERT(false);
   return ALLEGRO_ONE;
}

void Prog::draw_samples()
{
   int src = str_to_blend_mode(slst.get_selected_item_text());
   int dst = str_to_blend_mode(dlst.get_selected_item_text());
   int rv = r.get_cur_value();
   int gv = g.get_cur_value();
   int bv = b.get_cur_value();
   int av = a.get_cur_value();

   ALLEGRO_STATE state;
   al_store_state(&state, ALLEGRO_STATE_ALL);

   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgba(255, 255, 255, 255));
   al_draw_bitmap(allegro, 0, 0, 0);
   al_draw_bitmap(allegro_bmp, 320, 0, 0);

   al_set_blender(src, dst, al_map_rgba(rv, gv, bv, av));
   al_draw_bitmap(mysha, 0, 0, 0);
   al_draw_bitmap(mysha_bmp, 320, 0, 0);

   al_restore_state(&state);
}

int main()
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;

   if (!al_init()) {
      TRACE("Could not init Allegro\n");
      return 1;
   }
   al_install_keyboard();
   al_install_mouse();

   al_font_init();
   al_iio_init();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Unable to create display\n");
      return 1;
   }
   font = al_font_load_font("data/fixed_font.tga", 0);
   if (!font) {
      TRACE("Failed to load data/fixed_font.tga");
      return 1;
   }
   allegro = al_iio_load("data/allegro.pcx");
   if (!allegro) {
      TRACE("Failed to load data/allegro.pcx");
      return 1;
   }
   mysha = al_iio_load("data/mysha.pcx");
   if (!mysha) {
      TRACE("Failed to load data/mysha.pcx");
      return 1;
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   allegro_bmp = al_clone_bitmap(allegro);
   mysha_bmp = al_clone_bitmap(mysha);

   al_show_mouse_cursor();

   /* Don't remove these braces. */
   {
      Theme theme(font);
      Prog prog(theme);
      prog.run();
   }

   al_destroy_bitmap(allegro);
   al_destroy_bitmap(allegro_bmp);
   al_destroy_bitmap(mysha);
   al_destroy_bitmap(mysha_bmp);

   al_font_destroy_font(font);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
