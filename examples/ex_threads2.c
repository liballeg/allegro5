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
#include <allegro5/allegro.h>

#include "common.c"

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
   int random_seed;
   double target_x, target_y;
} ThreadInfo;

typedef struct Viewport {
   double centre_x;
   double centre_y;
   double x_extent;
   double y_extent;
   double zoom;
} Viewport;

static ThreadInfo thread_info[NUM_THREADS];

unsigned char sin_lut[256];


static double cabs2(double re, double im)
{
   return re*re + im*im;
}


static int mandel(double cre, double cim, int MAX_ITER)
{
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


/* local_rand:
 *  Simple rand() replacement with guaranteed randomness in the lower 16 bits.
 *  We just need a RNG with a thread-safe interface.
 */
static int local_rand(int *seed)
{
   const int LOCAL_RAND_MAX = 0xFFFF;

   *seed = (*seed + 1) * 1103515245 + 12345;
   return ((*seed >> 16) & LOCAL_RAND_MAX);
}


static void random_palette(unsigned char palette[256][3], int *seed)
{
   unsigned char rmax = 128 + local_rand(seed) % 128;
   unsigned char gmax = 128 + local_rand(seed) % 128;
   unsigned char bmax = 128 + local_rand(seed) % 128;
   int i;

   for (i = 0; i < 256; i++) {
      palette[i][0] = rmax * i / 256;
      palette[i][1] = gmax * i / 256;
      palette[i][2] = bmax * i / 256;
   }
}


static void draw_mandel_line(ALLEGRO_BITMAP *bitmap, const Viewport *viewport,
   unsigned char palette[256][3], const int y)
{
   ALLEGRO_LOCKED_REGION *lr;
   unsigned char *rgb;
   double xlower, ylower;
   double xscale, yscale;
   double im;
   double re;
   int w, h;
   int x;
   int n = 512 / pow(2, viewport->zoom);

   w = al_get_bitmap_width(bitmap);
   h = al_get_bitmap_height(bitmap);

   if (!(lr = al_lock_bitmap_region(bitmap, 0, y, w, 1, ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
         ALLEGRO_LOCK_WRITEONLY))) {
      abort_example("draw_mandel_line: al_lock_bitmap_region failed\n");
   }

   xlower = viewport->centre_x - viewport->x_extent / 2.0 * viewport->zoom;
   ylower = viewport->centre_y - viewport->y_extent / 2.0 * viewport->zoom;
   xscale = viewport->x_extent / w * viewport->zoom;
   yscale = viewport->y_extent / h * viewport->zoom;

   re = xlower;
   im = ylower + y * yscale;
   rgb = lr->data;

   for (x = 0; x < w; x++) {
      int i = mandel(re, im, n);
      int v = sin_lut[(int)(i * 64 / n)];

      rgb[0] = palette[v][0];
      rgb[1] = palette[v][1];
      rgb[2] = palette[v][2];
      rgb += 3;

      re += xscale;
   }

   al_unlock_bitmap(bitmap);
}


static void *thread_func(ALLEGRO_THREAD *thr, void *arg)
{
   ThreadInfo *info = (ThreadInfo *) arg;
   Viewport viewport;
   unsigned char palette[256][3];
   int y, h;

   y = 0;
   h = al_get_bitmap_height(info->bitmap);

   viewport.centre_x = info->target_x;
   viewport.centre_y = info->target_y;
   viewport.x_extent = 3.0;
   viewport.y_extent = 3.0;
   viewport.zoom = 1.0;
   info->target_x = 0;
   info->target_y = 0;

   while (!al_get_thread_should_stop(thr)) {
      al_lock_mutex(info->mutex);

      while (info->is_paused) {
         al_wait_cond(info->cond, info->mutex);

         /* We might be awoken because the program is terminating. */
         if (al_get_thread_should_stop(thr)) {
            break;
         }
      }

      if (!info->is_paused) {
         if (y == 0) {
            random_palette(palette, &info->random_seed);
         }

         draw_mandel_line(info->bitmap, &viewport, palette, y);

         y++;
         if (y >= h) {
            double z = viewport.zoom;
            y = 0;
            viewport.centre_x += z * viewport.x_extent * info->target_x;
            viewport.centre_y += z * viewport.y_extent * info->target_y;
            info->target_x = 0;
            info->target_y = 0;
            viewport.zoom *= 0.99;
         }
      }

      al_unlock_mutex(info->mutex);
      al_rest(0);
   }

   return NULL;
}


static void show_images(void)
{
   int x = 0;
   int y = 0;
   int i;

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
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


static void set_target(int n, double x, double y)
{
   thread_info[n].target_x = x;
   thread_info[n].target_y = y;
}


static void toggle_pausedness(int n)
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

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_install_keyboard();
   al_install_mouse();
   display = al_create_display(W * IMAGES_PER_ROW,
      H * NUM_THREADS / IMAGES_PER_ROW);
   if (!display) {
      abort_example("Error creating display\n");
   }
   timer = al_create_timer(1.0/3);
   if (!timer) {
      abort_example("Error creating timer\n");
   }
   queue = al_create_event_queue();
   if (!queue) {
      abort_example("Error creating event queue\n");
   }
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));

   /* Note:
    * Right now, A5 video displays can only be accessed from the thread which
    * created them (at least for OpenGL). To lift this restriction, we could
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
      thread_info[i].random_seed = i;
      thread[i] = al_create_thread(thread_func, &thread_info[i]);
      if (!thread[i]) {
         goto Error;
      }
   }
   set_target(0, -0.56062033041600878303, -0.56064322926933807256);
   set_target(1, -0.57798076669230014080, -0.63449861991138123418);
   set_target(2,  0.36676836392830602929, -0.59081385302214906030);
   set_target(3, -1.48319283039401317303, -0.00000000200514696273);
   set_target(4, -0.74052910500707636032,  0.18340899525730713915);
   set_target(5,  0.25437906525768350097, -0.00046678223345789554);
   set_target(6, -0.56062033041600878303,  0.56064322926933807256);
   set_target(7, -0.57798076669230014080,  0.63449861991138123418);
   set_target(8,  0.36676836392830602929,  0.59081385302214906030);

   for (i = 0; i < NUM_THREADS; i++) {
      al_start_thread(thread[i]);
   }
   al_start_timer(timer);

   need_draw = true;
   while (true) {
      if (need_draw && al_is_event_queue_empty(queue)) {
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
            double x = event.mouse.x - (event.mouse.x / W) * W;
            double y = event.mouse.y - (event.mouse.y / H) * H;
            /* Center to the mouse click position. */
            if (thread_info[n].is_paused) {
               thread_info[n].target_x = x / W - 0.5;
               thread_info[n].target_y = y / H - 0.5;
            }
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
      /* Set the flag to stop the thread.  The thread might be waiting on a
       * condition variable, so signal the condition to force it to wake up.
       */
      al_set_thread_should_stop(thread[i]);
      al_lock_mutex(thread_info[i].mutex);
      al_broadcast_cond(thread_info[i].cond);
      al_unlock_mutex(thread_info[i].mutex);

      /* al_destroy_thread() implicitly joins the thread, so this call is not
       * strictly necessary.
       */
      al_join_thread(thread[i], NULL);
      al_destroy_thread(thread[i]);
   }

   al_destroy_event_queue(queue);
   al_destroy_timer(timer);
   al_destroy_display(display);

   return 0;

Error:

   return 1;
}

/* vim: set sts=3 sw=3 et: */
