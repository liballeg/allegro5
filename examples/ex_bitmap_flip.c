/* An example showing bitmap flipping flags, by Steven Wallace. */

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "common.c"

#define INTERVAL 0.01


float bmp_x = 200;
float bmp_y = 200;
float bmp_dx = 96;
float bmp_dy = 96;
int bmp_flag = 0;


static void update(ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
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
      bmp_flag &= ~ALLEGRO_FLIP_VERTICAL;
   }

   if (bmp_x < 0) {
      bmp_x = 0;
      bmp_dx *= -1;
      bmp_flag &= ~ALLEGRO_FLIP_HORIZONTAL;
   }

   if (bmp_y > display_h - bitmap_h) {
      bmp_y = display_h - bitmap_h;
      bmp_dy *= -1;
      bmp_flag |= ALLEGRO_FLIP_VERTICAL;
   }

   if (bmp_x > display_w - bitmap_w) {
      bmp_x = display_w - bitmap_w;
      bmp_dx *= -1;
      bmp_flag |= ALLEGRO_FLIP_HORIZONTAL;
   }
}


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_BITMAP *mem_bmp;
   ALLEGRO_BITMAP *disp_bmp;
   ALLEGRO_FONT *font;
   char *text;
   bool done = false;
   bool redraw = true;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   al_init_font_addon();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }

   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      abort_example("Error loading data/fixed_font.tga\n");
   }

   bmp = disp_bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Error loading data/mysha.pcx\n");
   }
   text = "Display bitmap (space to change)";

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
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

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);

   while (!done) {
      ALLEGRO_EVENT event;

      if (redraw && al_is_event_queue_empty(queue)) {
         update(bmp);
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         al_draw_tinted_bitmap(bmp, al_map_rgba_f(1, 1, 1, 0.5),
            bmp_x, bmp_y, bmp_flag);
         al_draw_text(font, al_map_rgba_f(1, 1, 1, 0.5), 0, 0, 0, text);
         al_flip_display();
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            else if (event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
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

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            redraw = true;
            break;
      }
   }

   al_destroy_bitmap(bmp);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
