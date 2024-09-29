/* An example showing bitmap flipping flags, by Steven Wallace. */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "common.c"

/* Fire the update every 10 milliseconds. */
#define INTERVAL 0.01


float bmp_x = 200;
float bmp_y = 200;
float bmp_dx = 96;
float bmp_dy = 96;
int bmp_flag = 0;

/* Updates the bitmap velocity, orientation and position. */
static void update(A5O_BITMAP *bmp)
{
   A5O_BITMAP *target = al_get_target_bitmap();
   int display_w = al_get_bitmap_width(target);
   int display_h = al_get_bitmap_height(target);
   int bitmap_w = al_get_bitmap_width(bmp);
   int bitmap_h = al_get_bitmap_height(bmp);

   bmp_x += bmp_dx * INTERVAL;
   bmp_y += bmp_dy * INTERVAL;

   /* Make sure bitmap is still on the screen. */
   if (bmp_y < 0) {
      bmp_y = 0;
      bmp_dy *= -1;
      bmp_flag &= ~A5O_FLIP_VERTICAL;
   }

   if (bmp_x < 0) {
      bmp_x = 0;
      bmp_dx *= -1;
      bmp_flag &= ~A5O_FLIP_HORIZONTAL;
   }

   if (bmp_y > display_h - bitmap_h) {
      bmp_y = display_h - bitmap_h;
      bmp_dy *= -1;
      bmp_flag |= A5O_FLIP_VERTICAL;
   }

   if (bmp_x > display_w - bitmap_w) {
      bmp_x = display_w - bitmap_w;
      bmp_dx *= -1;
      bmp_flag |= A5O_FLIP_HORIZONTAL;
   }
}


int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_BITMAP *bmp;
   A5O_BITMAP *mem_bmp;
   A5O_BITMAP *disp_bmp;
   A5O_FONT *font;
   char *text;
   bool done = false;
   bool redraw = true;

   /* Silence unused arguments warnings. */
   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   /* Initialize the image addon. Requires the allegro_image addon
    * library.
    */
   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   /* Initialize the image font. Requires the allegro_font addon
    * library.
    */
   al_init_font_addon();
   init_platform_specific(); /* Helper functions from common.c. */

   /* Create a new display that we can render the image to. */
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   /* Allegro requires installing drivers for all input devices before
    * they can be used.
    */
   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   /* Loads a font from disk. This will use al_load_bitmap_font if you
    * pass the name of a known bitmap format, or else al_load_ttf_font.
    */
   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      abort_example("Error loading data/fixed_font.tga\n");
   }

   bmp = disp_bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Error loading data/mysha.pcx\n");
   }
   text = "Display bitmap (space to change)";

   al_set_new_bitmap_flags(A5O_MEMORY_BITMAP);
   mem_bmp = al_load_bitmap("data/mysha.pcx");
   if (!mem_bmp) {
      abort_example("Error loading data/mysha.pcx\n");
   }


   timer = al_create_timer(INTERVAL);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(display));

   al_start_timer(timer);

   /* Default premultiplied aplha blending. */
   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);

   /* Primary 'game' loop. */
   while (!done) {
      A5O_EVENT event;

      /* If the timer has since been fired and the queue is empty, draw.*/
      if (redraw && al_is_event_queue_empty(queue)) {
         update(bmp);
         /* Clear so we don't get trippy artifacts left after zoom. */
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         al_draw_tinted_bitmap(bmp, al_map_rgba_f(1, 1, 1, 0.5),
            bmp_x, bmp_y, bmp_flag);
         al_draw_text(font, al_map_rgba_f(1, 1, 1, 0.5), 0, 0, 0, text);
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == A5O_KEY_ESCAPE)
               done = true;
            else if (event.keyboard.keycode == A5O_KEY_SPACE) {
               /* Spacebar toggles whether render from a memory bitmap
                * or display bitamp.
                */
               if (bmp == mem_bmp) {
                  bmp = disp_bmp;
                  text = "Display bitmap (space to change)";
               }
               else {
                  bmp = mem_bmp;
                  text = "Memory bitmap (space to change)";
               }
            }
               
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case A5O_EVENT_TIMER:
            redraw = true;
            break;
      }
   }

   al_destroy_bitmap(bmp);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
