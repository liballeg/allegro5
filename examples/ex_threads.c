/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    In this example, each thread handles its own window and event queue.
 */

#include <math.h>
#include <allegro5/allegro5.h>


#define MAX_THREADS     100
#define MAX_BACKGROUNDS 10

typedef struct Background {
   double rmax;
   double gmax;
   double bmax;
} Background;


static void *thread_func(ALLEGRO_THREAD *thr, void *arg)
{
   const Background *background = (Background *) arg;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_EVENT_QUEUE *queue = NULL;
   ALLEGRO_TIMER *timer = NULL;
   ALLEGRO_EVENT event;
   double theta = 0.0;

   (void)thr;

   al_set_new_display_flags(ALLEGRO_RESIZABLE);

   display = al_create_display(300, 300);
   if (!display) {
      goto Quit;
   }
   queue = al_create_event_queue();
   if (!queue) {
      goto Quit;
   }
   timer = al_install_timer(0.1);
   if (!timer) {
      goto Quit;
   }

   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   al_start_timer(timer);

   while (true) {
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_TIMER) {
         double r = (sin(theta) + 1.0) / 2.0;
         ALLEGRO_COLOR c = al_map_rgb_f(
            background->rmax * r,
            background->gmax * r,
            background->bmax * r
         );
         al_clear_to_color(c);
         al_flip_display();
         theta += 0.1;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN
            && event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         al_acknowledge_resize(event.display.source);
      }
   }

Quit:

   if (timer) {
      al_uninstall_timer(timer);
   }
   if (queue) {
      al_destroy_event_queue(queue);
   }
   if (display) {
      al_destroy_display(display);
   }

   return NULL;
}


int main(int argc, const char *argv[])
{
   ALLEGRO_THREAD *thread[MAX_THREADS];
   Background background[MAX_BACKGROUNDS] = {
      { 1.0, 0.5, 0.5 },
      { 0.5, 1.0, 0.5 },
      { 0.5, 0.5, 1.0 },
      { 1.0, 1.0, 0.5 },
      { 0.5, 1.0, 1.0 },
      { 1.0, 0.7, 0.5 },
      { 0.5, 1.0, 0.7 },
      { 0.7, 0.5, 1.0 },
      { 1.0, 0.7, 0.5 },
      { 0.5, 0.7, 1.0 }
   };
   int num_threads;
   int i;

   if (argc > 1) {
      num_threads = strtol(argv[1], NULL, 10);
      if (num_threads > MAX_THREADS)
         num_threads = MAX_THREADS;
      else if (num_threads < 1)
         num_threads = 1;
   }
   else {
      num_threads = 3;
   }

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_keyboard();
   al_install_mouse();

   for (i = 0; i < num_threads; i++) {
      thread[i] = al_create_thread(thread_func,
         &background[i % MAX_BACKGROUNDS]);
   }
   for (i = 0; i < num_threads; i++) {
      al_start_thread(thread[i]);
      al_rest(0.5);
   }
   for (i = 0; i < num_threads; i++) {
      al_join_thread(thread[i], NULL);
      al_destroy_thread(thread[i]);
   }

   return 0;
}
END_OF_MAIN()


/* vim: set sts=3 sw=3 et: */
