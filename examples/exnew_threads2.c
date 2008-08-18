/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    In this example, threads render to their own memory buffers, while the
 *    main thread handles events and drawing (copying from the memory buffers
 *    to the display).
 *
 *    Click on an image to pause its thread.
 */

#include <math.h>
#include <allegro5/allegro5.h>


/* feel free to bump these up */
#define NUM_THREADS     9
#define IMAGES_PER_ROW  3

/* size of each fractal image */
#define W               120
#define H               120


typedef struct ThreadInfo {
   ALLEGRO_BITMAP *bitmap;
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
   bool is_paused;
} ThreadInfo;

typedef struct Viewport {
   double centre_x;
   double centre_y;
   double x_extent;
   double y_extent;
} Viewport;

ThreadInfo thread_info[NUM_THREADS];

unsigned char sin_lut[256];


double cabs2(double re, double im)
{
   return re*re + im*im;
}


int mandel(double cre, double cim)
{
   const int MAX_ITER = 512;
   const float Z_MAX2 = 4.0;
   double zre = cre, zim = cim;
   int iter;

   for (iter = 0; iter < MAX_ITER; iter++) {
      double z1re, z1im;
      z1re = zre * zre - zim * zim;
      z1im = 2 * zre * zim;
      z1re += cre;
      z1im += cim;
      if (cabs2(z1re, z1im) > Z_MAX2) {
         return iter + 1;  /* outside set */
      }
      zre = z1re;
      zim = z1im;
   }

   return 0;               /* inside set */
}


void random_palette(unsigned char palette[256][3])
{
   unsigned char rmax = 128 + rand() % 128;
   unsigned char gmax = 128 + rand() % 128;
   unsigned char bmax = 128 + rand() % 128;
   int i;

   for (i = 0; i < 256; i++) {
      palette[i][0] = rmax * i / 256;
      palette[i][1] = gmax * i / 256;
      palette[i][2] = bmax * i / 256;
   }
}


void draw_mandel_line(ALLEGRO_BITMAP *bitmap, const Viewport *viewport,
   unsigned char palette[256][3], const int y)
{
   ALLEGRO_LOCKED_REGION lr;
   unsigned char *rgb;
   double xlower, ylower;
   double xscale, yscale;
   double im;
   double re;
   int w, h;
   int x;

   w = al_get_bitmap_width(bitmap);
   h = al_get_bitmap_height(bitmap);

   if (!al_lock_bitmap_region(bitmap, 0, y, w, 1, &lr,
         0)) {
      TRACE("draw_mandel_line: al_lock_bitmap_region failed\n");
      return;
   }

   xlower = viewport->centre_x - viewport->x_extent / 2.0;
   ylower = viewport->centre_y - viewport->y_extent / 2.0;
   xscale = viewport->x_extent / w;
   yscale = viewport->y_extent / h;

   re = xlower;
   im = ylower + y * yscale;
   rgb = lr.data;

   for (x = 0; x < w; x++) {
      int i = mandel(re, im);
      int v = sin_lut[i/8];

      rgb[0] = palette[v][0];
      rgb[1] = palette[v][1];
      rgb[2] = palette[v][2];
      rgb += 3;

      re += xscale;
   }

   al_unlock_bitmap(bitmap);
}


void *thread_func(ALLEGRO_THREAD *thr, void *arg)
{
   ThreadInfo *info = (ThreadInfo *) arg;
   Viewport viewport;
   unsigned char palette[256][3];
   ALLEGRO_TIMEOUT timeout;
   int x, y, w, h;

   y = 0;
   w = al_get_bitmap_width(info->bitmap);
   h = al_get_bitmap_height(info->bitmap);

   viewport.centre_x = -0.5;
   viewport.centre_y = 0.0;
   viewport.x_extent = 3.0;
   viewport.y_extent = 3.0;

   while (!al_thread_should_stop(thr)) {
      if (y == 0) {
         random_palette(palette);
      }

      al_lock_mutex(info->mutex);

      /* When we wait for a condition we won't wake up if someone calls
       * al_join_thread() on us, so we won't know to quit.  In a proper
       * program you would signal the condition variable in that cause.  Here,
       * we just make sure to wait up every so often so we can make the check.
       */
      al_init_timeout(&timeout, 0.5);
      while (info->is_paused) {
         if (-1 == al_wait_cond_timed(info->cond, info->mutex, &timeout)) {
            /* timeout */
            break;
         }
      }

      if (!info->is_paused) {
         draw_mandel_line(info->bitmap, &viewport, palette, y);
         al_rest(0);
         y++;
         if (y >= h) {
            y = 0;
         }
      }

      al_unlock_mutex(info->mutex);
   }

   return NULL;
}


void show_images(void)
{
   int x = 0;
   int y = 0;
   int i;

   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, al_map_rgb_f(1, 1, 1));
   for (i = 0; i < NUM_THREADS; i++) {
      /* for lots of threads, this is not good enough */
      al_lock_mutex(thread_info[i].mutex);
      al_draw_bitmap(thread_info[i].bitmap, x * W, y * H, 0);
      al_unlock_mutex(thread_info[i].mutex);

      x++;
      if (x == IMAGES_PER_ROW) {
         x = 0;
         y++;
      }
   }
   al_flip_display();
}


void toggle_pausedness(int n)
{
   ThreadInfo *info = &thread_info[n];

   al_lock_mutex(info->mutex);
   info->is_paused = !info->is_paused;
   al_broadcast_cond(info->cond);
   al_unlock_mutex(info->mutex);
}


int main(void)
{
   ALLEGRO_THREAD *thread[NUM_THREADS];
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   bool need_draw;
   int i;

   for (i = 0; i < 256; i++) {
      sin_lut[i] = 128 + (int) (127.0 * sin(i / 8.0));
   }

   al_init();
   al_install_keyboard();
   al_install_mouse();
   display = al_create_display(W * IMAGES_PER_ROW,
      H * NUM_THREADS / IMAGES_PER_ROW);
   if (!display) {
      allegro_message("Error creating display");
      return 1;
   }
   timer = al_install_timer(1.0/3);
   if (!timer) {
      allegro_message("Error creating timer");
      return 1;
   }
   queue = al_create_event_queue();
   if (!queue) {
      allegro_message("Error creating event queue");
      return 1;
   }
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);

   /* Note:
    * Right now, A5 video displays can only be accessed from the thread which
    * created them (at lesat for OpenGL). To lift this restriction, we could
    * keep track of the current OpenGL context for each thread and make all
    * functions accessing the display check for it.. not sure it's worth the
    * additional complexity though.
    */
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_RGB_888);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   for (i = 0; i < NUM_THREADS; i++) {
      thread_info[i].bitmap = al_create_bitmap(W, H);
      if (!thread_info[i].bitmap) {
         goto Error;
      }
      thread_info[i].mutex = al_create_mutex();
      if (!thread_info[i].mutex) {
         goto Error;
      }
      thread_info[i].cond = al_create_cond();
      if (!thread_info[i].cond) {
         goto Error;
      }
      thread_info[i].is_paused = false;
      thread[i] = al_create_thread(thread_func, &thread_info[i]);
      if (!thread[i]) {
         goto Error;
      }
   }

   for (i = 0; i < NUM_THREADS; i++) {
      al_start_thread(thread[i]);
   }
   al_start_timer(timer);
   al_show_mouse_cursor();

   need_draw = true;
   while (true) {
      if (need_draw && al_event_queue_is_empty(queue)) {
         show_images();
         need_draw = false;
      }

      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_TIMER) {
         need_draw = true;
      }
      else if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
         int n = (event.mouse.y / H) * IMAGES_PER_ROW + (event.mouse.x / W);
         if (n < NUM_THREADS) {
            toggle_pausedness(n);
         }
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE) {
         need_draw = true;
      }
      else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
         }
         need_draw = true;
      }
   }

   for (i = 0; i < NUM_THREADS; i++) {
      al_join_thread(thread[i], NULL);
      al_destroy_thread(thread[i]);
   }

   al_destroy_event_queue(queue);
   al_uninstall_timer(timer);
   al_destroy_display(display);

   return 0;

Error:

   return 1;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
