/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Compare software blending routines with hardware blending.
 */

#include <string>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

#include "common.c"

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
   Label memory_label;
   Label texture_label;
   Label source_label;
   Label destination_label;
   List source_image;
   List destination_image;
   List draw_mode;
   Label operation_label[6];
   List operations[6];
   Label rgba_label[2];
   HSlider r[2];
   HSlider g[2];
   HSlider b[2];
   HSlider a[2];

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();

private:
   void blending_test(bool memory);
   void draw_samples();
   void draw_bitmap(const std::string &, const std::string &, bool, bool);
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 20, 40)),
   memory_label(Label("Memory")),
   texture_label(Label("Texture")),
   source_label(Label("Source", false)),
   destination_label(Label("Destination", false)),
   source_image(List(0)),
   destination_image(List(1)),
   draw_mode(List(0))
{
   d.add(memory_label, 9, 0, 10, 2);
   d.add(texture_label, 0, 0, 10, 2);
   d.add(source_label, 1, 15, 6, 2);
   d.add(destination_label, 7, 15, 6, 2);

   List *images[] = {&source_image, &destination_image, &draw_mode};
   for (int i = 0; i < 3; i++) {
      List & image = *images[i];
      if (i < 2) {
         image.append_item("Mysha");
         image.append_item("Allegro");
         image.append_item("Mysha (tinted)");
         image.append_item("Allegro (tinted)");
         image.append_item("Color");
      }
      else {
         image.append_item("original");
         image.append_item("scaled");
         image.append_item("rotated");
      }
      d.add(image, 1 + i * 6, 17, 4, 6);
   }

   for (int i = 0; i < 4; i++) {
      operation_label[i] = Label(i % 2 == 0 ? "Color" : "Alpha", false);
      d.add(operation_label[i], 1 + i * 3, 24, 3, 2);
      List &l = operations[i];
      l.append_item("ONE");
      l.append_item("ZERO");
      l.append_item("ALPHA");
      l.append_item("INVERSE");
      l.append_item("SRC_COLOR");
      l.append_item("DEST_COLOR");
      l.append_item("INV_SRC_COLOR");
      l.append_item("INV_DEST_COLOR");
      d.add(l, 1 + i * 3, 25, 3, 9);
   }

   for (int i = 4; i < 6; i++) {
      operation_label[i] = Label(i == 4 ? "Blend op" : "Alpha op", false);
      d.add(operation_label[i], 1 + i * 3, 24, 3, 2);
      List &l = operations[i];
      l.append_item("ADD");
      l.append_item("SRC_MINUS_DEST");
      l.append_item("DEST_MINUS_SRC");
      d.add(l, 1 + i * 3, 25, 3, 6);
   }

   rgba_label[0] = Label("Source tint/color RGBA");
   rgba_label[1] = Label("Dest tint/color RGBA");
   d.add(rgba_label[0], 1, 34, 5, 1);
   d.add(rgba_label[1], 7, 34, 5, 1);

   for (int i = 0; i < 2; i++) {
      r[i] = HSlider(255, 255);
      g[i] = HSlider(255, 255);
      b[i] = HSlider(255, 255);
      a[i] = HSlider(255, 255);
      d.add(r[i], 1 + i * 6, 35, 5, 1);
      d.add(g[i], 1 + i * 6, 36, 5, 1);
      d.add(b[i], 1 + i * 6, 37, 5, 1);
      d.add(a[i], 1 + i * 6, 38, 5, 1);
   }
}

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear_to_color(al_map_rgb(128, 128, 128));
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
   if (str == "SRC_COLOR")
      return ALLEGRO_SRC_COLOR;
   if (str == "DEST_COLOR")
      return ALLEGRO_DEST_COLOR;
   if (str == "INV_SRC_COLOR")
      return ALLEGRO_INVERSE_SRC_COLOR;
   if (str == "INV_DEST_COLOR")
      return ALLEGRO_INVERSE_DEST_COLOR;
   if (str == "ALPHA")
      return ALLEGRO_ALPHA;
   if (str == "INVERSE")
      return ALLEGRO_INVERSE_ALPHA;
   if (str == "ADD")
      return ALLEGRO_ADD;
   if (str == "SRC_MINUS_DEST")
      return ALLEGRO_SRC_MINUS_DEST;
   if (str == "DEST_MINUS_SRC")
      return ALLEGRO_DEST_MINUS_SRC;

   ALLEGRO_ASSERT(false);
   return ALLEGRO_ONE;
}

void draw_background(int x, int y)
{
   ALLEGRO_COLOR c[] = {
      al_map_rgba(0x66, 0x66, 0x66, 0xff),
      al_map_rgba(0x99, 0x99, 0x99, 0xff)
   };

   for (int i = 0; i < 320 / 16; i++) {
      for (int j = 0; j < 200 / 16; j++) {
         al_draw_filled_rectangle(x + i * 16, y + j * 16,
            x + i * 16 + 16, y + j * 16 + 16,
            c[(i + j) & 1]);
      }
   }
}

static ALLEGRO_COLOR makecol(int r, int g, int b, int a)
{
   /* Premultiply alpha. */
   float rf = (float)r / 255.0f;
   float gf = (float)g / 255.0f;
   float bf = (float)b / 255.0f;
   float af = (float)a / 255.0f;
   return al_map_rgba_f(rf*af, gf*af, bf*af, af);
}

static bool contains(const std::string & haystack, const std::string & needle)
{
   return haystack.find(needle) != std::string::npos;
}

void Prog::draw_bitmap(const std::string & str,
   const std::string &how,
   bool memory,
   bool destination)
{
   int i = destination ? 1 : 0;
   int rv = r[i].get_cur_value();
   int gv = g[i].get_cur_value();
   int bv = b[i].get_cur_value();
   int av = a[i].get_cur_value();
   ALLEGRO_COLOR color = makecol(rv, gv, bv, av);
   ALLEGRO_BITMAP *bmp;

   if (contains(str, "Mysha"))
      bmp = (memory ? mysha_bmp : mysha);
   else
      bmp = (memory ? allegro_bmp : allegro);

   if (how == "original") {
      if (str == "Color")
         al_draw_filled_rectangle(0, 0, 320, 200, color);
      else if (contains(str, "tint"))
         al_draw_tinted_bitmap(bmp, color, 0, 0, 0);
      else
         al_draw_bitmap(bmp, 0, 0, 0);
   }
   else if (how == "scaled") {
      int w = al_get_bitmap_width(bmp);
      int h = al_get_bitmap_height(bmp);
      float s = 200.0 / h * 0.9;
      if (str == "Color") {
         al_draw_filled_rectangle(10, 10, 300, 180, color);
      }
      else if (contains(str, "tint")) {
         al_draw_tinted_scaled_bitmap(bmp, color, 0, 0, w, h,
            160 - w * s / 2, 100 - h * s / 2, w * s, h * s, 0);
      }
      else {
         al_draw_scaled_bitmap(bmp, 0, 0, w, h,
            160 - w * s / 2, 100 - h * s / 2, w * s, h * s, 0);
      }
   }
   else if (how == "rotated") {
      if (str == "Color") {
         al_draw_filled_circle(160, 100, 100, color);
      }
      else if (contains(str, "tint")) {
         al_draw_tinted_rotated_bitmap(bmp, color, 160, 100,
            160, 100, ALLEGRO_PI / 8, 0);
      }
      else {
         al_draw_rotated_bitmap(bmp, 160, 100,
            160, 100, ALLEGRO_PI / 8, 0);
      }
   }
}

void Prog::blending_test(bool memory)
{
   ALLEGRO_COLOR transparency = al_map_rgba_f(0, 0, 0, 0);
   int op = str_to_blend_mode(operations[4].get_selected_item_text());
   int aop = str_to_blend_mode(operations[5].get_selected_item_text());
   int src = str_to_blend_mode(operations[0].get_selected_item_text());
   int asrc = str_to_blend_mode(operations[1].get_selected_item_text());
   int dst = str_to_blend_mode(operations[2].get_selected_item_text());
   int adst = str_to_blend_mode(operations[3].get_selected_item_text());

   /* Initialize with destination. */
   al_clear_to_color(transparency); // Just in case.
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   draw_bitmap(destination_image.get_selected_item_text(),
      "original", memory, true);

   /* Now draw the blended source over it. */
   al_set_separate_blender(op, src, dst, aop, asrc, adst);
   draw_bitmap(source_image.get_selected_item_text(),
      draw_mode.get_selected_item_text(), memory, false);
}

void Prog::draw_samples()
{
   ALLEGRO_STATE state;
   al_store_state(&state, ALLEGRO_STATE_TARGET_BITMAP | ALLEGRO_STATE_BLENDER);
      
   /* Draw a background, in case our target bitmap will end up with
    * alpha in it.
    */
   draw_background(40, 20);
   draw_background(400, 20);
   
   /* Test standard blending. */
   al_set_target_bitmap(target);
   blending_test(false);

   /* Test memory blending. */
   al_set_target_bitmap(target_bmp);
   blending_test(true);

   /* Display results. */
   al_restore_state(&state);
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   al_draw_bitmap(target, 40, 20, 0);
   al_draw_bitmap(target_bmp, 400, 20, 0);
 
   al_restore_state(&state);
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
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(800, 600);
   if (!display) {
      abort_example("Unable to create display\n");
   }

   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      abort_example("Failed to load data/fixed_font.tga\n");
   }
   allegro = al_load_bitmap("data/allegro.pcx");
   if (!allegro) {
      abort_example("Failed to load data/allegro.pcx\n");
   }
   mysha = al_load_bitmap("data/mysha256x256.png");
   if (!mysha) {
      abort_example("Failed to load data/mysha256x256.png\n");
   }
   
   target = al_create_bitmap(320, 200);

   al_add_new_bitmap_flag(ALLEGRO_MEMORY_BITMAP);
   allegro_bmp = al_clone_bitmap(allegro);
   mysha_bmp = al_clone_bitmap(mysha);
   target_bmp = al_clone_bitmap(target);

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

   al_destroy_font(font);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
