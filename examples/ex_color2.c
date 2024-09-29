#define A5O_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60

typedef struct {
   A5O_COLOR rgb;
   float l, a, b;
} Color;

struct Example {
   A5O_FONT *font;
   A5O_BITMAP *lab[512];
   A5O_COLOR black;
   A5O_COLOR white;
   Color color[2];
   int mx, my;
   int mb; // 1 = clicked, 2 = released, 3 = pressed
   int slider;
   int top_y;
   int left_x;
   int slider_x;
   int half_x;
} example;

static void draw_lab(int l)
{
   if (example.lab[l]) return;

   example.lab[l] = al_create_bitmap(512, 512);
   A5O_LOCKED_REGION *rg = al_lock_bitmap(example.lab[l],
         A5O_PIXEL_FORMAT_ABGR_8888_LE, A5O_LOCK_WRITEONLY);
   int x, y;
   for (y = 0; y < 512; y++) {
      for (x = 0; x < 512; x++) {
         float a = (x - 511.0 / 2) / (511.0 / 2);
         float b = (y - 511.0 / 2) / (511.0 / 2);
         b = -b;
         A5O_COLOR rgb = al_color_lab(l / 511.0, a, b);
         if (!al_is_color_valid(rgb)) {
            rgb = al_map_rgb_f(0, 0, 0);
         }
         int red = 255 * rgb.r;
         int green = 255 * rgb.g;
         int blue = 255 * rgb.b;

         uint8_t *p = rg->data;
         p += rg->pitch * y;
         p += x * 4;
         p[0] = red;
         p[1] = green;
         p[2] = blue;
         p[3] = 255;
      }
   }
   al_unlock_bitmap(example.lab[l]);
}

static void draw_range(int ci)
{
   float l = example.color[ci].l;
   int cx = (example.color[ci].a * 511.0 / 2) + 511.0 / 2;
   int cy = (-example.color[ci].b * 511.0 / 2) + 511.0 / 2;
   int r = 2;
   /* Loop around the center in outward circles, starting with a 3 x 3
    * rectangle, then 5 x 5, then 7 x 7, and so on.
    * Each loop has four sides, top, right, bottom, left. For example
    * these are the four loops for the 5 x 5 case:
    * 1 2 3 4 .
    * .       .
    * .       .
    * .       .
    * . . . . .
    * 
    * o o o o 1
    * .       2
    * .       3
    * .       4
    * . . . . .
    *
    * o o o o o
    * .       o
    * .       o
    * . 3 2 1 1
    *
    * o o o o o
    * 4       o
    * 3       o
    * 2       o
    * 1 o o o o
    * 
    * 1654321
    */
   while (true) {
      bool found = false;
      int x = cx - r / 2;
      int y = cy - r / 2;
      int i;
      for (i = 0; i < r * 4; i++) {
         if (i < r) x++;
         else if (i < r * 2) y++;
         else if (i < r * 3) x--;
         else y--;

         float a = (x - 511.0 / 2) / (511.0 / 2);
         float b = (y - 511.0 / 2) / (511.0 / 2);
         float rf, gf, bf;
         b = -b;
         al_color_lab_to_rgb(l, a, b, &rf, &gf, &bf);
         if (rf < 0 || rf > 1 || gf < 0 || gf > 1 || bf < 0 || bf > 1) {
            continue;
         }
         A5O_COLOR rgb = {rf, gf, bf, 1};
         float d = al_color_distance_ciede2000(rgb, example.color[ci].rgb);
         if (d <= 0.05) {
            if (d > 0.04) {
               al_draw_pixel(example.half_x * ci + example.left_x + x,
                  example.top_y + y, example.white);
            }
            found = true;
         }
      }
      if (!found) break;
      r += 2;
      if (r > 128) break;
   }
}

static void init(void)
{
  example.black = al_map_rgb_f(0, 0, 0);
  example.white = al_map_rgb_f(1, 1, 1);
  example.left_x = 120;
  example.top_y = 48;
  example.slider_x = 48;
  example.color[0].l = 0.5;
  example.color[1].l = 0.5;
  example.color[0].a = 0.2;
  example.half_x = al_get_display_width(al_get_current_display()) / 2;
}

static void draw_axis(float x, float y, float a, float a2, float l,
      float tl, char const *label, int ticks,
      char const *numformat, float num1, float num2, float num) {
   al_draw_line(x, y, x + l * cos(a), y - l * sin(a), example.white, 1);
   int i;
   for (i = 0; i < ticks; i++) {
      float x2 = x + l * cos(a) * i / (ticks - 1);
      float y2 = y - l * sin(a) * i / (ticks - 1);
      al_draw_line(x2, y2, x2 + tl * cos(a2), y2 - tl * sin(a2), example.white, 1);
      al_draw_textf(example.font, example.white,
         (int)(x2 + (tl + 2) * cos(a2)), (int)(y2 - (tl + 2) * sin(a2)), A5O_ALIGN_RIGHT, numformat,
         num1 + i * (num2 - num1) / (ticks - 1));
   }
   float lv = (num - num1) * l / (num2 - num1);
   al_draw_filled_circle(x + lv * cos(a), y - lv * sin(a), 8, example.white);
   al_draw_textf(example.font, example.white,
      (int)(x + (l + 4) * cos(a)) - 24, (int)(y - (l + 4) * sin(a)) - 12, 0, "%s", label);
}

static void redraw(void)
{
   al_clear_to_color(example.black);
   int w = al_get_display_width(al_get_current_display());
   int h = al_get_display_height(al_get_current_display());

   int ci;
   for (ci = 0; ci < 2; ci++) {
      int cx = w / 2 * ci;
   
      float l = example.color[ci].l;
      float a = example.color[ci].a;
      float b = example.color[ci].b;

      A5O_COLOR rgb = example.color[ci].rgb;
      
      draw_lab((int)(l * 511));

      al_draw_bitmap(example.lab[(int)(l * 511)], cx + example.left_x, example.top_y, 0);

      draw_axis(cx + example.left_x, example.top_y + 512.5, 0, A5O_PI / -2, 512, 16,
         "a*", 11, "%.2f", -1, 1, a);
      draw_axis(cx + example.left_x - 0.5, example.top_y + 512, A5O_PI / 2, A5O_PI,
         512, 16, "b*", 11, "%.2f", -1, 1, b);
      
      al_draw_textf(example.font, example.white, cx + example.left_x + 36, 8, 0,
         "L*a*b* = %.2f/%.2f/%.2f sRGB = %.2f/%.2f/%.2f",
         l, a, b, rgb.r, rgb.g, rgb.b);
      draw_axis(cx + example.slider_x - 0.5, example.top_y + 512, A5O_PI / 2, A5O_PI,
         512, 4, "L*", 3, "%.1f", 0, 1, l);

      A5O_COLOR c = al_map_rgb_f(rgb.r, rgb.g, rgb.b);
      al_draw_filled_rectangle(cx, h - 128, cx + w / 2, h, c);

      draw_range(ci);
   }

   al_draw_textf(example.font, example.white, example.left_x + 36, 28, 0,
      "%s", "Lab colors visible in sRGB");
   al_draw_textf(example.font, example.white, example.half_x + example.left_x + 36, 28, 0,
      "%s", "ellipse shows CIEDE2000 between 0.4 and 0.5");

   al_draw_line(w / 2, 0, w / 2, h - 128, example.white, 4);

   float dr = example.color[0].rgb.r - example.color[1].rgb.r;
   float dg = example.color[0].rgb.g - example.color[1].rgb.g;
   float db = example.color[0].rgb.b - example.color[1].rgb.b;
   float drgb = sqrt(dr * dr + dg * dg + db * db);
   float dl = example.color[0].l - example.color[1].l;
   float da = example.color[0].a - example.color[1].a;
   db = example.color[0].b - example.color[1].b;
   float dlab = sqrt(da * da + db * db + dl * dl);
   float d2000 = al_color_distance_ciede2000(example.color[0].rgb,
      example.color[1].rgb);
   al_draw_textf(example.font, example.white, w / 2, h - 64,
      A5O_ALIGN_CENTER, "dRGB = %.2f", drgb);
   al_draw_textf(example.font, example.white, w / 2, h - 64 + 12,
      A5O_ALIGN_CENTER, "dLab = %.2f", dlab);
   al_draw_textf(example.font, example.white, w / 2, h - 64 + 24,
      A5O_ALIGN_CENTER, "CIEDE2000 = %.2f", d2000);

   al_draw_rectangle(w / 2 - 80, h - 70, w / 2 + 80, h - 24,
      example.white, 4);
}

static void update(void)
{
   if (example.mb == 1 || example.mb == 3) {
      if (example.mb == 1) {
         int s = 0;
         int mx = example.mx;
         if (example.mx >= example.half_x) {
            mx -= example.half_x;
            s += 2;
         }
         if (mx > example.left_x) {
            s += 1;
         }
         example.slider = s;
      }

      if (example.slider == 0 || example.slider == 2) {
         int l = example.my - example.top_y;
         if (l < 0) l = 0;
         if (l > 511) l = 511;
         example.color[example.slider / 2].l = 1 - l / 511.0;
      }
      else {
         int ci = example.slider / 2;
         int a = example.mx - example.left_x - ci * example.half_x;
         int b = example.my - example.top_y;
         b = 511 - b;
         if (a < 0) a = 0;
         if (b < 0) b = 0;
         if (a > 511) a = 511;
         if (b > 511) b = 511;
         example.color[ci].a = 2 * a / 511.0 - 1;
         example.color[ci].b = 2 * b / 511.0 - 1;
      }
   }
   
   if (example.mb == 1) example.mb = 3;
   if (example.mb == 2) example.mb = 0;

   int ci;
   for (ci = 0; ci < 2; ci++) {
      example.color[ci].rgb = al_color_lab(example.color[ci].l,
         example.color[ci].a, example.color[ci].b);
      }
}

int main(int argc, char **argv)
{
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_DISPLAY *display;
   int w = 1280, h = 720;
   bool done = false;
   bool need_redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   al_init_font_addon();
   example.font = al_create_builtin_font();

   init_platform_specific();

   display = al_create_display(w, h);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   al_install_keyboard();
   al_install_mouse();
   al_init_primitives_addon();

   init();

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   while (!done) {
      A5O_EVENT event;

      if (need_redraw) {
         redraw();
         al_flip_display();
         need_redraw = false;
      }

      while (true) {
         al_wait_for_event(queue, &event);
         switch (event.type) {
            case A5O_EVENT_KEY_CHAR:
               if (event.keyboard.keycode == A5O_KEY_ESCAPE)
                  done = true;
               break;

            case A5O_EVENT_MOUSE_AXES:
               example.mx = event.mouse.x;
               example.my = event.mouse.y;
               break;

            case A5O_EVENT_MOUSE_BUTTON_DOWN:
               example.mb = 1;
               break;

            case A5O_EVENT_MOUSE_BUTTON_UP:
               example.mb = 2;
               break;

            case A5O_EVENT_DISPLAY_CLOSE:
               done = true;
               break;

            case A5O_EVENT_TIMER:
               update();
               need_redraw = true;
               break;
         }
         if (al_is_event_queue_empty(queue))
            break;
      }
   }

   return 0;
}
