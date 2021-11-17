/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Color gradient example program.
 * 
 *      This example draws several color gradients, each one linearly
 *      interpolating between two colors. For each version interpolating
 *      the raw R/G/B values there is also versions interpolating the
 *      components in other color spaces (e.g. Oklab L/a/b).
 *
 *      See readme.txt for copyright information.
 */
#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>

#include "common.c"

const int SCREEN_W = 1280;
const int SCREEN_H = 720;

typedef enum {
   RGB, LINEAR, HSV, OKLAB, CIELAB
} ColorSpace;
static char const *colorspace_name[] = {
   "RGB", "Linear RGB", "HSV", "Oklab", "CIE Lab"
};

struct Example {
   ALLEGRO_FONT *font;
   bool resized;
   int ox, oy;
   float scale;
   int count;
   char const *colors[1 + 8];
} example;


static void print(int x, int y, char const *format, ...)
{
   va_list args;
   va_start(args, format);
   char s[1000];
   vsnprintf(s, 1000, format, args);
   va_end(args);
   float tw = al_get_text_width(example.font, s) / example.scale;
   float ox = 0;
   if (x - tw / 2 < 0) ox = tw / 2 - x;
   if (x + tw / 2 > SCREEN_W) ox = SCREEN_W - x - tw / 2;
   ALLEGRO_TRANSFORM backup = *al_get_current_transform();
   ALLEGRO_TRANSFORM t;
   // Since we re-load the font when the window is resized we need to
   // un-scale it for drawing (but keep the scaled position).
   al_identity_transform(&t);
   al_scale_transform(&t, 1 / example.scale, 1 / example.scale);
   al_translate_transform(&t, x, y);
   al_compose_transform(&t, &backup);
   al_use_transform(&t);
   // black "outline" (not really, just offset by 1 pixel but good enough)
   al_draw_text(example.font, al_map_rgba_f(0, 0, 0, .5), (ox - tw / 2) * example.scale + 1, 1, 0, s);
   al_draw_text(example.font, al_map_rgb_f(1, 1, 1), (ox - tw / 2) * example.scale, 0, 0, s);
   al_use_transform(&backup);
}


static ALLEGRO_COLOR get_colorspace_rgb(ColorSpace cs, float *v)
{
   if (cs == RGB) {
      return al_map_rgb_f(v[0], v[1], v[2]);
   }
   if (cs == OKLAB) {
      return al_color_oklab(v[0], v[1], v[2]);
   }
   if (cs == CIELAB) {
      return al_color_lab(v[0], v[1], v[2]);
   }
   if (cs == LINEAR) {
      return al_color_linear(v[0], v[1], v[2]);
   }
   if (cs == HSV) {
      return al_color_hsv(v[0], v[1], v[2]);
   }
   return al_map_rgb(0, 0, 0);
}


static void draw_gradient(int x, int y, ColorSpace cs,
      int color1, int color2)
{
   ALLEGRO_COLOR rgb1 = al_color_name(example.colors[color1]);
   ALLEGRO_COLOR rgb2 = al_color_name(example.colors[color2]);
   float v1[3], v2[3];
   if (cs == RGB) {
      al_unmap_rgb_f(rgb1, v1 + 0, v1 + 1, v1 + 2);
      al_unmap_rgb_f(rgb2, v2 + 0, v2 + 1, v2 + 2);
   }
   if (cs == OKLAB) {
      al_color_rgb_to_oklab(rgb1.r, rgb1.g, rgb1.b, v1 + 0, v1 + 1, v1 + 2);
      al_color_rgb_to_oklab(rgb2.r, rgb2.g, rgb2.b, v2 + 0, v2 + 1, v2 + 2);
   }
   if (cs == CIELAB) {
      al_color_rgb_to_lab(rgb1.r, rgb1.g, rgb1.b, v1 + 0, v1 + 1, v1 + 2);
      al_color_rgb_to_lab(rgb2.r, rgb2.g, rgb2.b, v2 + 0, v2 + 1, v2 + 2);
   }
   if (cs == LINEAR) {
      al_color_rgb_to_linear(rgb1.r, rgb1.g, rgb1.b, v1 + 0, v1 + 1, v1 + 2);
      al_color_rgb_to_linear(rgb2.r, rgb2.g, rgb2.b, v2 + 0, v2 + 1, v2 + 2);
   }
   if (cs == HSV) {
      al_color_rgb_to_hsv(rgb1.r, rgb1.g, rgb1.b, v1 + 0, v1 + 1, v1 + 2);
      al_color_rgb_to_hsv(rgb2.r, rgb2.g, rgb2.b, v2 + 0, v2 + 1, v2 + 2);
   }
   int w = ceil(300 * example.scale);
   float ly;
   for (int i = 0; i < w; i++) {
      float p = 1.0 * i / (w - 1);
      float v[3];
      v[0] = v1[0] + p * (v2[0] - v1[0]);
      v[1] = v1[1] + p * (v2[1] - v1[1]);
      v[2] = v1[2] + p * (v2[2] - v1[2]);
      ALLEGRO_COLOR rgb = get_colorspace_rgb(cs, v);
      float colx = x + 10 + i / example.scale;
      al_draw_filled_rectangle(colx, y + 10, colx + 1 / example.scale, y + 80, rgb);
      float l, a, b;
      al_color_rgb_to_oklab(rgb.r, rgb.g, rgb.b, &l, &a, &b);
      if (i > 0) {
         al_draw_line(colx, y + 6 - ly * 50, colx + 1, y + 6 - l * 50, al_map_rgb_f(1, 1, 1), 0);
      }
      ly = l;
   }
   al_draw_rectangle(x + 11, y + 11, x + 311, y + 81, al_map_rgb_f(0, 0, 0), 1);
   al_draw_rectangle(x + 10, y + 10, x + 310, y + 80, al_map_rgb_f(1, 1, 1), 1);
}


static void redraw(void)
{
   if (example.resized) {
      // We maintain a transformation to scale and offset everything so
      // a logical resolution of SCREEN_W x SCREEN_H fits into the
      // actual window (centered if it doesn't completely fit).
      example.resized = false;
      float dw = al_get_display_width(al_get_current_display());
      float dh = al_get_display_height(al_get_current_display());
      if (SCREEN_W * dh / dw < SCREEN_H) {
         example.ox = (dw - SCREEN_W * dh / SCREEN_H) / 2;
         example.oy = 0;
         example.scale = dh / SCREEN_H;
      }
      else {
         example.ox = 0;
         example.oy = (dh - SCREEN_H * dw / SCREEN_W) / 2;
         example.scale = dw / SCREEN_W;
      }
      // For best text quality we reload the font for the specific pixel
      // size whenever the transformation changes.
      example.font = al_load_font("data/DejaVuSans.ttf", 20 * example.scale, 0);
   }
   ALLEGRO_TRANSFORM transform;
   al_build_transform(&transform, example.ox, example.oy,
      example.scale, example.scale, 0);
   al_use_transform(&transform);
   al_clear_to_color(al_map_rgb_f(0.5, 0.5, 0.5));

   // Here is the actual example. Draw interpolated gradients for
   // various color spaces.
   ColorSpace cs[] = {RGB, OKLAB, CIELAB, LINEAR, HSV};
   example.count = 0;
   print(640, 10, "color gradients interpolated in different color spaces, with luminance curves");
   float y = 20;
   for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 4; j++) {
         print(160 + j * 320, y + 30, colorspace_name[cs[i]]);
      }
      draw_gradient(0, y + 50, cs[i], 1, 2);
      draw_gradient(320, y + 50, cs[i], 3, 4);
      draw_gradient(640, y + 50, cs[i], 5, 6);
      draw_gradient(960, y + 50, cs[i], 7, 8);
      y += 140;
   }
}


static void init(void)
{
   example.colors[1] = "blue";
   example.colors[2] = "white";
   example.colors[3] = "blue";
   example.colors[4] = "gold";
   example.colors[5] = "red";
   example.colors[6] = "lime";
   example.colors[7] = "black";
   example.colors[8] = "pink";
}


int main(int argc, char **argv)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_DISPLAY *display;
   bool done = false;
   bool need_redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   al_init_font_addon();
   al_init_ttf_addon();
   init_platform_specific();
   example.resized = true;

   al_set_new_display_flags(ALLEGRO_RESIZABLE);
   al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
   al_set_new_display_option(ALLEGRO_SAMPLES, 16, ALLEGRO_SUGGEST);
   display = al_create_display(SCREEN_W, SCREEN_H);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   al_install_keyboard();
   al_install_mouse();
   al_init_primitives_addon();

   init();

   timer = al_create_timer(1.0 / 5);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   while (!done) {
      ALLEGRO_EVENT event;

      if (need_redraw) {
         redraw();
         al_flip_display();
         need_redraw = false;
      }

      while (true) {
         al_wait_for_event(queue, &event);
         switch (event.type) {
            case ALLEGRO_EVENT_KEY_CHAR:
               if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
                  done = true;
               break;

            case ALLEGRO_EVENT_DISPLAY_CLOSE:
               done = true;
               break;

            case ALLEGRO_EVENT_DISPLAY_RESIZE:
               al_acknowledge_resize(display);
               example.resized = true;
               break;

            case ALLEGRO_EVENT_TIMER:
               need_redraw = true;
               break;
         }
         if (al_is_event_queue_empty(queue))
            break;
      }
   }

   return 0;
}
