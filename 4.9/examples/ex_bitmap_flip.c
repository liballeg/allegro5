/* An example showing bitmap flipping flags, by Steven Wallace. */

#include <allegro5/allegro5.h>
#include <allegro5/a5_iio.h>

#define INTERVAL 0.01


float bmp_x = 0;
float bmp_y = 0;
float bmp_dx = 96;
float bmp_dy = 96;
int bmp_flag = 0;


void update(ALLEGRO_BITMAP *bmp)
{
   int display_w = al_get_display_width();
   int display_h = al_get_display_height();
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
   bool done = false;

   if (!al_init()) {
      TRACE("Failed to init Allegro.\n");
      return 1;
   }

   if (!al_iio_init()) {
      TRACE("Failed to init IIO addon.\n");
      return 1;
   }

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display.\n");
      return 1;
   }

   if (!al_install_keyboard()) {
      TRACE("Error installing keyboard.\n");
      return 1;
   }

   bmp = al_iio_load("data/mysha.pcx");
   if (!bmp) {
      TRACE("Error loading data/mysha.pcx\n");
      return 1;
   }

   timer = al_install_timer(INTERVAL);

   queue = al_create_event_queue();
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) timer);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *) display);

   al_start_timer(timer);

   while (!done) {
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            update(bmp);
            al_clear(al_map_rgb(0, 0, 0));
            al_draw_bitmap(bmp, bmp_x, bmp_y, bmp_flag);
            al_flip_display();
            break;
      }
   }

   al_destroy_bitmap(bmp);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
