#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60

struct Example {
   A5O_DISPLAY *display;
   A5O_BITMAP *glyphs[2];
   A5O_BITMAP *sub;
   int i;
   int p;
} example;


/* Draw the parent bitmaps. */
static void draw_glyphs(void)
{
   int x = 0, y = 0;
   al_clear_to_color(al_map_rgb_f(0, 0, 0));
   al_draw_bitmap(example.glyphs[0], 0, 0, 0);
   al_draw_bitmap(example.glyphs[1], 0, 32 * 6, 0);
   if (example.p == 1)
      y = 32 * 6;
   x += al_get_bitmap_x(example.sub);
   y += al_get_bitmap_y(example.sub);
   al_draw_filled_rectangle(x, y,
      x + al_get_bitmap_width(example.sub),
      y + al_get_bitmap_height(example.sub),
      al_map_rgba_f(0.5, 0, 0, 0.5));
}

static void redraw(void)
{
   int i;

   draw_glyphs();

   /* Here we draw example.sub, which is never re-created, just
    * re-parented.
    */
   for (i = 0; i < 8; i++) {
      al_draw_scaled_rotated_bitmap(example.sub, 0, 0,
         i * 100, 32 * 6 + 33 * 5 + 4,
         2.0, 2.0, A5O_PI / 4 * i / 8, 0);
   }
}

static void update(void)
{
   /* Re-parent out sub bitmap, jumping from glyph to glyph. */
   int i;
   example.i++;
   i = (example.i / 4) % (16 * 6 + 20 * 5);
   if (i < 16 * 6) {
      example.p = 0;
      al_reparent_bitmap(example.sub, example.glyphs[0],
         (i % 16) * 32, (i / 16) * 32, 32, 32);
   }
   else {
      example.p = 1;
      i -= 16 * 6;
      al_reparent_bitmap(example.sub, example.glyphs[1],
         (i % 20) * 37, (i / 20) * 33, 37, 33);
   }
}

static void init(void)
{
   /* We create out sub bitmap once then use it throughout the
    * program, re-parenting as needed.
    */
   example.sub = al_create_sub_bitmap(example.glyphs[0], 0, 0, 32, 32);
}

int main(int argc, char **argv)
{
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   int w = 800, h = 600;
   bool done = false;
   bool need_redraw = true;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   al_init_primitives_addon();

   init_platform_specific();

   al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR);
   al_set_new_display_flags(A5O_RESIZABLE);
   example.display = al_create_display(w, h);
   if (!example.display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   example.glyphs[0] = al_load_bitmap("data/font.tga");
   if (!example.glyphs[0]) {
      abort_example("Error loading data/font.tga\n");
   }

   example.glyphs[1] = al_load_bitmap("data/bmpfont.tga");
   if (!example.glyphs[1]) {
      abort_example("Error loading data/bmpfont.tga\n");
   }

   init();

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());

   al_register_event_source(queue, al_get_timer_event_source(timer));
   
   al_register_event_source(queue, al_get_display_event_source(
      example.display));

   al_start_timer(timer);

   while (!done) {
      A5O_EVENT event;
      w = al_get_display_width(example.display);
      h = al_get_display_height(example.display);

      if (need_redraw && al_is_event_queue_empty(queue)) {
         redraw();
         al_flip_display();
         need_redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_KEY_CHAR:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE)
               done = true;
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            done = true;
            break;
         
         case A5O_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(event.display.source);
            break;
              
         case A5O_EVENT_TIMER:
            update();
            need_redraw = true;
            break;
      }
   }

   return 0;
}
