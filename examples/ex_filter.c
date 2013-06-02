#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <stdlib.h>
#include <math.h>

#include "common.c"

#define FPS 60

static struct Example {
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;
   ALLEGRO_BITMAP *bitmaps[2][9];    
   ALLEGRO_COLOR bg, fg, info;
   int bitmap;
   int ticks;
} example;

static int const filter_flags[6] = {
   0,
   ALLEGRO_MIN_LINEAR,
   ALLEGRO_MIPMAP,
   ALLEGRO_MIN_LINEAR | ALLEGRO_MIPMAP,
   0,
   ALLEGRO_MAG_LINEAR
};

static char const *filter_text[4] = {
   "nearest", "linear",
   "nearest mipmap", "linear mipmap"
};

static void update(void)
{
   example.ticks++;
}

static void redraw(void)
{
   int w = al_get_display_width(example.display);
   int h = al_get_display_height(example.display);
   int i;
   
   al_clear_to_color(example.bg);
   
   for (i = 0; i < 6; i++) {
      float x = (i / 2) * w / 3 + w / 6;
      float y = (i % 2) * h / 2 + h / 4;
      ALLEGRO_BITMAP *bmp = example.bitmaps[example.bitmap][i];
      float bw = al_get_bitmap_width(bmp);
      float bh = al_get_bitmap_height(bmp);
      float t = 1 - 2 * fabsf((example.ticks % (FPS * 16)) / 16.0 / FPS - 0.5);
      float scale;
      float angle = example.ticks * ALLEGRO_PI * 2 / FPS / 8;
      
      if (i < 4)
         scale = 1 - t * 0.9;
      else
         scale = 1 + t * 9;

      al_draw_textf(example.font, example.fg, x, y - 64 - 14,
         ALLEGRO_ALIGN_CENTRE, "%s", filter_text[i % 4]);
         
      al_set_clipping_rectangle(x - 64, y - 64, 128, 128);
      al_draw_scaled_rotated_bitmap(bmp, bw / 2, bh / 2,
         x, y, scale, scale, angle, 0);
      al_set_clipping_rectangle(0, 0, w, h);
   }
   al_draw_textf(example.font, example.info, w / 2, h - 14,
         ALLEGRO_ALIGN_CENTRE, "press space to change");
}

int main(void)
{
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   int w = 640, h = 480;
   bool done = false;
   bool need_redraw = true;
   int i;
   ALLEGRO_BITMAP *mysha;

   if (!al_init()) {
      abort_example("Failed to init Allegro.\n");
   }

   if (!al_init_image_addon()) {
      abort_example("Failed to init IIO addon.\n");
   }

   al_init_font_addon();

   example.display = al_create_display(w, h);

   if (!example.display) {
      abort_example("Error creating display.\n");
   }

   if (!al_install_keyboard()) {
      abort_example("Error installing keyboard.\n");
   }
    
   if (!al_install_mouse()) {
      abort_example("Error installing mouse.\n");
   }

   example.font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!example.font) {
      abort_example("Error loading data/fixed_font.tga\n");
   }
   
   mysha = al_load_bitmap("data/mysha256x256.png");
   if (!mysha) {
      abort_example("Error loading data/mysha256x256.png\n");
   }

   for (i = 0; i < 6; i++) {
      ALLEGRO_LOCKED_REGION *lock;
      int x, y;
      /* Only power-of-two bitmaps can have mipmaps. */
      al_set_new_bitmap_flags(filter_flags[i]);
      example.bitmaps[0][i] = al_create_bitmap(1024, 1024);
      example.bitmaps[1][i] = al_clone_bitmap(mysha);
      lock = al_lock_bitmap(example.bitmaps[0][i],
         ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);
      for (y = 0; y < 1024; y++) {
         unsigned char *row = (unsigned char *)lock->data + lock->pitch * y;
         unsigned char *ptr = row;
         for (x = 0; x < 1024; x++) {
            int c = 0;
            if (((x >> 2) & 1) ^ ((y >> 2) & 1)) c = 255;
            *(ptr++) = c;
            *(ptr++) = c;
            *(ptr++) = c;
            *(ptr++) = 255;
         }
      }
      al_unlock_bitmap(example.bitmaps[0][i]);
      
   }

   example.bg = al_map_rgb_f(0, 0, 0);
   example.fg = al_map_rgb_f(1, 1, 1);
   example.info = al_map_rgb_f(0.5, 0.5, 1);

   timer = al_create_timer(1.0 / FPS);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_display_event_source(example.display));

   al_start_timer(timer);

   while (!done) {
      ALLEGRO_EVENT event;

      if (need_redraw && al_is_event_queue_empty(queue)) {
         redraw();
         al_flip_display();
         need_redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               done = true;
            if (event.keyboard.keycode == ALLEGRO_KEY_SPACE)
               example.bitmap = (example.bitmap + 1) % 2;
            break;

         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            done = true;
            break;

         case ALLEGRO_EVENT_TIMER:
            update();
            need_redraw = true;
            break;
        
         case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            example.bitmap = (example.bitmap + 1) % 2;
            break;
      }
   }

   for (i = 0; i < 6; i++) {
      al_destroy_bitmap(example.bitmaps[0][i]);
      al_destroy_bitmap(example.bitmaps[1][i]);
   }

   return 0;
}

/* vim: set sts=3 sw=3 et: */
