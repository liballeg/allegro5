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
 *      Emoji example program.
 *
 *      This example draws some emoji characters.
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

char *emoji = "🐃🐅🐆🐈🐊🐏🐐🐑🐒🐖🐕🐘🐫🐠🐢🐧🐟🐙🦖🕊️🦒🦌";

struct Example {
   ALLEGRO_FONT *font1;
   ALLEGRO_FONT *font2;
   bool resized;
   int ox, oy;
   float scale;
} example;

struct Emoji {
   char *text;
   int font;
   float x, y;
   int ox, oy;
   int w, h;
   float dx, dy;
   struct Emoji *next;
};
struct Emoji *first;

static void print(int f, int x, int y, char const *format, ...)
{
   float scale = 1.0;
   ALLEGRO_FONT *font = example.font1;
   if (f == 2) {
      font = example.font2;
      scale = example.scale;
   }
   va_list args;
   va_start(args, format);
   char s[1000];
   vsnprintf(s, 1000, format, args);
   va_end(args);
   ALLEGRO_TRANSFORM backup = *al_get_current_transform();
   ALLEGRO_TRANSFORM t;
   // Since we re-load the font when the window is resized we need to
   // un-scale it for drawing (but keep the scaled position).
   al_identity_transform(&t);
   al_scale_transform(&t, scale / example.scale, scale / example.scale);
   al_translate_transform(&t, x, y);
   al_compose_transform(&t, &backup);
   al_use_transform(&t);
   // black "outline" (not really, just offset by 1/1 pixel but good enough)
   al_draw_text(font, al_map_rgba_f(0, 0, 0, .5), example.scale, example.scale, ALLEGRO_ALIGN_INTEGER, s);
   al_draw_text(font, al_map_rgb_f(1, 1, 1), 0, 0, ALLEGRO_ALIGN_INTEGER, s);
   al_use_transform(&backup);
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
      al_destroy_font(example.font1);
      example.font1 = al_load_font("data/DejaVuSans.ttf", 140 * example.scale, 0);
   }
   ALLEGRO_TRANSFORM transform;
   al_build_transform(&transform, example.ox, example.oy,
      example.scale, example.scale, 0);
   al_use_transform(&transform);
   al_clear_to_color(al_map_rgb_f(0.5, 0.5, 0.5));

   // Here is the actual example.

   struct Emoji *e = first;
   while (e) {
      print(e->font, e->x - e->ox, e->y - e->oy, e->text);
      e = e->next;
   }

   al_draw_rectangle(0, 0, SCREEN_W, SCREEN_H, al_color_name("silver"), 2);
}


static bool collides(struct Emoji *e)
{
   if (e->font == 1) return false;
   struct Emoji *e2 = first;
   while (e2) {
      if (e2 != e && e2->font != 1) {
         if (e->x < e2->x + e2->w &&
             e->y < e2->y + e2->h &&
             e->x + e->w > e2->x &&
             e->y + e->h > e2->y) {
            return true;
            break;
         }
      }
      e2 = e2->next;
   }
   return false;
}


static void add_emoji(int font, int border, char *text)
{
   struct Emoji *e;
   e = calloc(1, sizeof *e);
   e->font = font;
   e->text = text;
   ALLEGRO_FONT *f = example.font1;
   if (font == 2) f = example.font2;
   al_get_text_dimensions(f, e->text, &e->ox, &e->oy, &e->w, &e->h);
   e->w -= border * 2;
   e->h -= border * 2;
   e->ox += border;
   e->oy += border;
   for (int t = 0; t < 1000; t++) {
      e->x = rand() % (SCREEN_W - e->w);
      e->y = rand() % (SCREEN_H - e->h);
      if (!collides(e)) break;
      if (e->font == 1) break;
   }
   float a = rand() % 360;
   e->dx = -2 - rand() % 3;
   e->dy = sin(a * ALLEGRO_PI / 180);
   e->next = first;
   first = e;
}


static void init(void)
{
   example.font2 = al_load_font("data/NotoColorEmoji_Animals.ttf", 0, 0);

   ALLEGRO_USTR *u = al_ustr_new(emoji);
   int n = al_ustr_length(u);
   for (int i = 0; i < n; i++) {
      int offset1 = al_ustr_offset(u, i);
      int offset2 = al_ustr_offset(u, i + 1);
      ALLEGRO_USTR *sub = al_ustr_dup_substr(u, offset1, offset2);
      add_emoji(2, 50, al_cstr_dup(sub));
      al_ustr_free(sub);
   }
   al_ustr_free(u);

   add_emoji(1, 0, "Allegro");
}


static void tick(void)
{
   struct Emoji *e = first;
   float s = 5;
   while (e) {
      float oy = e->y;
      e->x += e->dx * s;
      e->y += e->dy * s;
      if (collides(e)) {
         e->dy = -e->dy;
         e->y = oy;
      }
      if (e->x < 0 -e->w) e->x = SCREEN_W + e->w;
      if (e->y < 0 && e->dy < 0) e->dy = -e->dy;
      if (e->y > SCREEN_H - e->h && e->dy > 0) e->dy = -e->dy;
      e = e->next;
   }
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

   timer = al_create_timer(1.0 / 60);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_register_event_source(queue, al_get_display_event_source(display));

   redraw();
   init();

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
               tick();
               need_redraw = true;
               break;
         }
         if (al_is_event_queue_empty(queue))
            break;
      }
   }

   return 0;
}
