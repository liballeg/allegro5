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
ALLEGRO_BITMAP *target;
ALLEGRO_BITMAP *target_bmp;

class Prog {
private:
   Dialog d;
   Button source_title;
   Button destination_title;
   List source_image;
   List destination_image;
   List slst;
   List dlst;
   VSlider r;
   VSlider g;
   VSlider b;
   VSlider a;
   
   void blending_test(bool memory);

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();

private:
   void draw_samples();
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 20, 20)),
   source_title(Button("Source")),
   destination_title(Button("Destination")),
   destination_image(List(1)),
   r(VSlider(255, 255)),
   g(VSlider(255, 255)),
   b(VSlider(255, 255)),
   a(VSlider(255, 255))
{
   d.add(source_title, 1, 9, 6, 1);
   d.add(destination_title, 7, 9, 6, 1);

   List *images[] = {&source_image, &destination_image};
   for (int i = 0; i < 2; i++) {
      List &image = *images[i];
      image.append_item("Mysha");
      image.append_item("Allegro");
      image.append_item("Opaque Black");
      image.append_item("Opaque White");
      image.append_item("Transparent Black");
      image.append_item("Transparent White");
      d.add(image, 1 + i * 6, 10, 6, 4);
   }

   slst.append_item("ONE");
   slst.append_item("ZERO");
   slst.append_item("ALPHA");
   slst.append_item("INVERSE_ALPHA");
   d.add(slst, 1, 15, 6, 4);

   dlst.append_item("ONE");
   dlst.append_item("ZERO");
   dlst.append_item("ALPHA");
   dlst.append_item("INVERSE_ALPHA");
   d.add(dlst, 7, 15, 6, 4);

   d.add(r, 15, 15, 1, 4);
   d.add(g, 16, 15, 1, 4);
   d.add(b, 17, 15, 1, 4);
   d.add(a, 18, 15, 1, 4);
}

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear(al_map_rgb(128, 128, 128));
         draw_samples();
         d.draw();
         al_flip_display();
      }

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

void draw_background()
{
   ALLEGRO_COLOR c[] = {al_map_rgba(0x66, 0x66, 0x66, 0xff),
   	  al_map_rgba(0x99, 0x99, 0x99, 0xff)};
   for (int i = 0; i < 640 / 16; i++) {
   	  for (int j = 0; j < 200 / 16; j++) {
   	  	  al_draw_rectangle(i * 16, j * 16, i * 16 + 16, j * 16 + 16,
   	  	  	 c[(i + j) & 1], ALLEGRO_FILLED);
      }
   }
}

void draw_bitmap(const std::string & str, bool memory)
{
   ALLEGRO_COLOR opaque_black, opaque_white, transparent_black,
      transparent_white;
   opaque_black = al_map_rgba_f(0, 0, 0, 1);
   opaque_white = al_map_rgba_f(1, 1, 1, 1);
   transparent_black = al_map_rgba_f(0, 0, 0, 0.5);
   transparent_white = al_map_rgba_f(1, 1, 1, 0.5);
   
   if (str == "Mysha")
      al_draw_bitmap(memory ? mysha_bmp : mysha, 0, 0, 0);
   if (str == "Allegro")
      al_draw_bitmap(memory ? allegro_bmp : allegro, 0, 0, 0);
   if (str == "Opaque Black")
      al_draw_rectangle(0, 0, 320, 200, opaque_black, ALLEGRO_FILLED);
   if (str == "Opaque White")
      al_draw_rectangle(0, 0, 320, 200, opaque_white, ALLEGRO_FILLED);
   if (str == "Transparent Black")
      al_draw_rectangle(0, 0, 320, 200, transparent_black, ALLEGRO_FILLED);
   if (str == "Transparent White")
      al_draw_rectangle(0, 0, 320, 200, transparent_white, ALLEGRO_FILLED);
}

void Prog::blending_test(bool memory)
{
   ALLEGRO_COLOR opaque_white = al_map_rgba_f(1, 1, 1, 1);
   int src = str_to_blend_mode(slst.get_selected_item_text());
   int dst = str_to_blend_mode(dlst.get_selected_item_text());
   int rv = r.get_cur_value();
   int gv = g.get_cur_value();
   int bv = b.get_cur_value();
   int av = a.get_cur_value();

   /* Initialize with destination. */
   al_clear(opaque_white); // Just in case.
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, opaque_white);
   draw_bitmap(destination_image.get_selected_item_text(), memory);

   /* Now draw the blended source over it. */
   al_set_blender(src, dst, al_map_rgba(rv, gv, bv, av));
   draw_bitmap(source_image.get_selected_item_text(), memory);
}

void Prog::draw_samples()
{
   ALLEGRO_STATE state;
   al_store_state(&state, ALLEGRO_STATE_ALL);
      
   /* Draw a background, in case our target bitmap will end up with
    * alpha in it.
    */
   draw_background();
   
   /* Test standard blending. */
   al_set_target_bitmap(target);
   blending_test(false);

   /* Test memory blending. */
   al_set_target_bitmap(target_bmp);
   blending_test(true);

   /* Display results. */
   al_set_target_bitmap(al_get_backbuffer());
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgba(255, 255, 255, 255));
   al_draw_bitmap(target, 0, 0, 0);
   al_draw_bitmap(target_bmp, 320, 0, 0);
 
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

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Unable to create display\n");
      return 1;
   }
   font = al_font_load_font("data/fixed_font.tga", 0);
   if (!font) {
      TRACE("Failed to load data/fixed_font.tga\n");
      return 1;
   }
   allegro = al_iio_load("data/allegro.pcx");
   if (!allegro) {
      TRACE("Failed to load data/allegro.pcx\n");
      return 1;
   }
   mysha = al_iio_load("data/mysha.pcx");
   if (!mysha) {
      TRACE("Failed to load data/mysha.pcx\n");
      return 1;
   }
   
   target = al_create_bitmap(320, 200);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   allegro_bmp = al_clone_bitmap(allegro);
   mysha_bmp = al_clone_bitmap(mysha);
   target_bmp = al_clone_bitmap(target);

   al_show_mouse_cursor();

   /* Don't remove these braces. */
   {
      Theme theme(font);
      Prog prog(theme, display);
      prog.run();
   }

   al_destroy_bitmap(allegro);
   al_destroy_bitmap(allegro_bmp);
   al_destroy_bitmap(mysha);
   al_destroy_bitmap(mysha_bmp);
   al_destroy_bitmap(target);
   al_destroy_bitmap(target_bmp);

   al_font_destroy_font(font);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
