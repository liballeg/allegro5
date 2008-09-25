/* An example showing bitmap flipping flags, by Steven Wallace. */

#include <allegro5/allegro5.h>
#include <allegro5/a5_iio.h>

#define INTERVAL 0.01


void update(float *bmp_x, float *bmp_y, int *bmp_flag, ALLEGRO_BITMAP *bmp) {
   static float bmp_dx = 96, bmp_dy = 96;
	*bmp_x += bmp_dx * INTERVAL;
	*bmp_y += bmp_dy * INTERVAL;
	
	// Make sure bitmap is still on the screen
	if(*bmp_y < 0) {
		*bmp_y = 0;
		bmp_dy *= -1;
		*bmp_flag &= ~ALLEGRO_FLIP_VERTICAL;
	}
	
	if(*bmp_x < 0) {
		*bmp_x = 0;
		bmp_dx *= -1;
		*bmp_flag &= ~ALLEGRO_FLIP_HORIZONTAL;
	}
	
	if(*bmp_y > al_get_display_height() - al_get_bitmap_height(bmp)) {
		*bmp_y = al_get_display_height() - al_get_bitmap_height(bmp);
		bmp_dy *= -1;
		*bmp_flag |= ALLEGRO_FLIP_VERTICAL;
	}
	
	if(*bmp_x > al_get_display_width() - al_get_bitmap_width(bmp)) {
		*bmp_x = al_get_display_width() - al_get_bitmap_width(bmp);
		bmp_dx *= -1;
		*bmp_flag |= ALLEGRO_FLIP_HORIZONTAL;
	}	
}


int main(int argc, char *argv[]) {
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_BITMAP *bmp;
   float bmp_x = 0, bmp_y = 0;
   int bmp_flag = 0;
   volatile bool done = false;
	
   if (!al_init()) {
      TRACE("Failed to init Allegro.\n");
      return 1;
   }
	
   if (!al_iio_init()) {
      TRACE("Failed to init IIO addon.\n");
      return 1;
   }
	
	display = al_create_display(640, 480);
	if(!display) {
		TRACE("Error creating display.\n");
		return 1;
	}
	
	if(!al_install_keyboard()) {
		TRACE("Error installing keyboard.\n");
		return 1;
	}

	timer = al_install_timer(INTERVAL);
	al_start_timer(timer);

	queue = al_create_event_queue();
	al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
	al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);
	
	bmp = al_iio_load("data/mysha.pcx");
	if(!bmp) {
		TRACE("Error loading data/mysha.pcx\n");
		return 1;
	}
	
	while (!done) {
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);
      switch(event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
            if(event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            update(&bmp_x, &bmp_y, &bmp_flag, bmp);
            al_clear(al_map_rgb(0, 0, 0));
            al_draw_bitmap(bmp, bmp_x, bmp_y, bmp_flag);
            al_flip_display();
            break;
      }
   }

	return 0;
}
END_OF_MAIN()
