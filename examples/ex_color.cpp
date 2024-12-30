/*
 *    Example program for the Allegro library, by Elias Pschernig.
 *
 *    Demonstrates some of the conversion functions in the color addon.
 */

#include <string>
#include <cstdio>
#include "allegro5/allegro.h"
#include "allegro5/allegro_ttf.h"
#include "allegro5/allegro_color.h"
#include <allegro5/allegro_primitives.h>

#include "nihgui.hpp"

#include "common.c"

#define SLIDERS_COUNT 19
char const *names[] = {"R", "G", "B", "H", "S", "V", "H", "S", "L",
    "Y", "U", "V", "C", "M", "Y", "K", "L", "C", "H"};

float clamp(float x)
{
    if (x < 0)
       return 0;
    if (x > 1)
       return 1;
    return x;
}

class Prog {
private:
   Dialog d;
   VSlider sliders[SLIDERS_COUNT];
   Label labels[SLIDERS_COUNT];
   Label labels2[SLIDERS_COUNT];
   int previous[SLIDERS_COUNT];

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();

private:
   void draw_swatch(float x, float y, float w, float h, float v[3]);
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 640, 480))
{
   for (int i = 0; i < SLIDERS_COUNT; i++) {
      int j = i < 12 ? i / 3 : i < 16 ? 4 : 5;
      sliders[i] = VSlider(1000, 1000);
      d.add(sliders[i], 8 + i * 32 + j * 16, 8, 15, 256);
      labels[i].set_text(names[i]);
      d.add(labels[i], i * 32 + j * 16, 8 + 256, 32, 20);
      d.add(labels2[i], i * 32 + j * 16, 8 + 276, 32, 20);
      previous[i] = 0;
   }
}

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear_to_color(al_map_rgb(128, 128, 128));
         float v[SLIDERS_COUNT];
         int keep = -1;
         for (int i = 0; i < SLIDERS_COUNT; i++) {
            int x = sliders[i].get_cur_value();
            v[i] = x / 1000.0;
            if (previous[i] != x) {
               keep = i;
            }
         }

         if (keep != -1) {
            int space = keep < 12 ? keep / 3 : keep < 16 ? 4 : 5;
            switch (space) {
               case 0:
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_lch(v[0], v[1], v[2], v + 16, v + 17, v + 18);
                  v[3] /= 360;
                  v[6] /= 360;
                  v[18] /= ALLEGRO_PI * 2;
                  break;
               case 1:
                  al_color_hsv_to_rgb(v[3] * 360, v[4], v[5], v + 0, v + 1, v + 2);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_lch(v[0], v[1], v[2], v + 16, v + 17, v + 18);
                  v[6] /= 360;
                  v[18] /= ALLEGRO_PI * 2;
                  break;
               case 2:
                  al_color_hsl_to_rgb(v[6] * 360, v[7], v[8], v + 0, v + 1, v + 2);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_lch(v[0], v[1], v[2], v + 16, v + 17, v + 18);
                  v[3] /= 360;
                  v[18] /= ALLEGRO_PI * 2;
                  break;
               case 3:
                  al_color_yuv_to_rgb(v[9], v[10], v[11], v + 0, v + 1, v + 2);
                  v[0] = clamp(v[0]);
                  v[1] = clamp(v[1]);
                  v[2] = clamp(v[2]);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_lch(v[0], v[1], v[2], v + 16, v + 17, v + 18);
                  v[3] /= 360;
                  v[6] /= 360;
                  v[18] /= ALLEGRO_PI * 2;
                  break;
               case 4:
                  al_color_cmyk_to_rgb(v[12], v[13], v[14], v[15], v + 0, v + 1, v + 2);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_lch(v[0], v[1], v[2], v + 16, v + 17, v + 18);
                  v[3] /= 360;
                  v[6] /= 360;
                  v[18] /= ALLEGRO_PI * 2;
                  break;
               case 5:
                  al_color_lch_to_rgb(v[16], v[17], v[18] * 2 * ALLEGRO_PI, v + 0, v + 1, v + 2);
                  v[0] = clamp(v[0]);
                  v[1] = clamp(v[1]);
                  v[2] = clamp(v[2]);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_lch(v[0], v[1], v[2], v + 16, v + 17, v + 18);
                  v[3] /= 360;
                  v[6] /= 360;
                  v[18] /= ALLEGRO_PI * 2;
                  break;
            }
         }

         for (int i = 0; i < SLIDERS_COUNT; i++) {
            sliders[i].set_cur_value((int)(v[i] * 1000));
            previous[i] = sliders[i].get_cur_value();
            char c[100];
            sprintf(c, "%d", (int)(v[i] * 100));
            labels2[i].set_text(c);
         }

         d.draw();

         ALLEGRO_BITMAP *target = al_get_target_bitmap();
         int w = al_get_bitmap_width(target);
         int h = al_get_bitmap_height(target);
         draw_swatch(0, h - 80, w, h, v);

         al_flip_display();
      }

      d.run_step(true);
   }
}

void Prog::draw_swatch(float x1, float y1, float x2, float y2, float v[3])
{
   al_draw_filled_rectangle(x1, y1, x2, y2, al_map_rgb_f(v[0], v[1], v[2]));

   char const *name = al_color_rgb_to_name(v[0], v[1], v[2]);
   char html[8];
   al_color_rgb_to_html(v[0], v[1], v[2], html);
   al_draw_text(d.get_theme().font, al_map_rgb(0, 0, 0), x1, y1 - 20, 0, name);
   al_draw_text(d.get_theme().font, al_map_rgb(0, 0, 0), x1, y1 - 40, 0, html);
}

int main(int argc, char *argv[])
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro\n");
   }
   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();

   al_init_font_addon();
   al_init_ttf_addon();
   init_platform_specific();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(720, 480);
   if (!display) {
      abort_example("Unable to create display\n");
   }
   font = al_load_font("data/DejaVuSans.ttf", 12, 0);
   if (!font) {
      abort_example("Failed to load data/DejaVuSans.ttf\n");
   }

   /* Prog is destroyed at the end of this scope. */
   {
      Theme theme(font);
      Prog prog(theme, display);
      prog.run();
   }

   al_destroy_font(font);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
