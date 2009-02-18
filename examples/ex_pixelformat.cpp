/*
 *    Simple (incomplete) test of pixel format conversions.
 *
 *    This should be made comprehensive.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_iio.h"
#include "allegro5/a5_primitives.h"
#include "nihgui.hpp"


typedef struct FORMAT
{
   int format;
   char const *name;
} FORMAT;

const FORMAT formats[] = {
   {ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA, "any"},
   {ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA, "alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA, "15"},
   {ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA, "15 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA, "16"},
   {ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA, "16 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA, "24"},
   {ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA, "24 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA, "32"},
   {ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA, "32 alpha"},
   {ALLEGRO_PIXEL_FORMAT_ARGB_8888, "ARGB8888"},
   {ALLEGRO_PIXEL_FORMAT_RGBA_8888, "RGBA8888"},
   {ALLEGRO_PIXEL_FORMAT_ARGB_4444, "ARGB4444"},
   {ALLEGRO_PIXEL_FORMAT_RGB_888, "RGB888"},
   {ALLEGRO_PIXEL_FORMAT_RGB_565, "RGB565"},
   {ALLEGRO_PIXEL_FORMAT_RGB_555, "RGB555"},
   {ALLEGRO_PIXEL_FORMAT_RGBA_5551, "RGBA5551"},
   {ALLEGRO_PIXEL_FORMAT_ARGB_1555, "ARGB1555"},
   {ALLEGRO_PIXEL_FORMAT_ABGR_8888, "ABGR8888"},
   {ALLEGRO_PIXEL_FORMAT_XBGR_8888, "XBGR8888"},
   {ALLEGRO_PIXEL_FORMAT_BGR_888, "BGR888"},
   {ALLEGRO_PIXEL_FORMAT_BGR_565, "BGR565"},
   {ALLEGRO_PIXEL_FORMAT_BGR_555, "BGR555"},
   {ALLEGRO_PIXEL_FORMAT_RGBX_8888, "RGBX8888"},
   {ALLEGRO_PIXEL_FORMAT_XRGB_8888, "XRGB8888"},
};

#define NUM_FORMATS  (sizeof(formats) / sizeof(formats[0]))


const char *get_format_name(ALLEGRO_BITMAP *bmp)
{
   if (!bmp)
      return "none";

   int format = al_get_bitmap_format(bmp);
   for (unsigned i = 0; i < NUM_FORMATS; i++) {
      if (formats[i].format == format)
         return formats[i].name;
   }
   return "unknown";
}


class Prog {
private:
   Dialog d;
   Label source_label;
   Label dest_label;
   List source_list;
   List dest_list;
   Label true_formats;

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();

private:
   void draw_sample();
};


Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 20, 30)),
   source_label(Label("Source")),
   dest_label(Label("Destination")),
   source_list(List()),
   dest_list(List()),
   true_formats(Label(""))
{
   d.add(source_label, 11, 0, 4,  1);
   d.add(source_list,  11, 1, 4, 25);
   d.add(dest_label,   15, 0, 4,  1);
   d.add(dest_list,    15, 1, 4, 25);
   d.add(true_formats, 0, 15, 10, 1);

   for (unsigned i = 0; i < NUM_FORMATS; i++) {
      source_list.append_item(formats[i].name);
      dest_list.append_item(formats[i].name);
   }
}


void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear(al_map_rgb(128, 128, 128));
         draw_sample();
         d.draw();
         al_flip_display();
      }

      d.run_step(true);
   }
}


void Prog::draw_sample()
{
   const int i = source_list.get_cur_value();
   const int j = dest_list.get_cur_value();
   ALLEGRO_BITMAP *bitmap1;
   ALLEGRO_BITMAP *bitmap2;

   al_set_new_bitmap_format(formats[i].format);

   bitmap1 = al_iio_load("data/allegro.pcx");
   if (!bitmap1) {
      TRACE("Could not load image, bitmap format = %d\n", formats[i].format);
      printf("Could not load image, bitmap format = %d\n", formats[i].format);
   }

   al_set_new_bitmap_format(formats[j].format);

   bitmap2 = al_create_bitmap(320, 200);
   if (!bitmap2) {
      TRACE("Could not create bitmap, format = %d\n", formats[j].format);
      printf("Could not create bitmap, format = %d\n", formats[j].format);
   }

   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb(255, 255, 255));

   if (bitmap1 && bitmap2) {
      al_set_target_bitmap(bitmap2);
      al_draw_bitmap(bitmap1, 0, 0, 0);
      al_set_target_bitmap(al_get_backbuffer());
      al_draw_bitmap(bitmap2, 0, 0, 0);
   }
   else {
      al_draw_line(0, 0, 320, 200, al_map_rgb_f(1, 0, 0), 0);
      al_draw_line(0, 200, 320, 0, al_map_rgb_f(1, 0, 0), 0);
   }

   std::string s = get_format_name(bitmap1);
   s += " -> ";
   s += get_format_name(bitmap2);
   true_formats.set_text(s);

   al_destroy_bitmap(bitmap1);
   al_destroy_bitmap(bitmap2);
}


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_iio_init();
   al_font_init();

   al_install_keyboard();
   al_install_mouse();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   printf("Display format = %d\n", al_get_display_format());

   font = al_font_load_font("data/fixed_font.tga", NULL);
   if (!font) {
      TRACE("Failed to load data/fixed_font.tga\n");
      return 1;
   }

   /* Don't remove these braces. */
   {
      Theme theme(font);
      Prog prog(theme, display);
      prog.run();
   }

   al_font_destroy_font(font);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
