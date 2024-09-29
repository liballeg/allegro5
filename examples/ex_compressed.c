#include <stdio.h>
#include <stdlib.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_font.h"

#include "common.c"

typedef struct BITMAP_TYPE {
   A5O_BITMAP* bmp;
   A5O_BITMAP* clone;
   A5O_BITMAP* decomp;
   A5O_BITMAP* lock_clone;
   A5O_PIXEL_FORMAT format;
   const char* name;
} BITMAP_TYPE;

int main(int argc, char **argv)
{
   const char *filename;
   A5O_DISPLAY *display;
   A5O_TIMER *timer;
   A5O_EVENT_QUEUE *queue;
   A5O_FONT *font;
   A5O_BITMAP *bkg;
   bool redraw = true;
   int ii;
   int cur_bitmap = 0;
   bool compare = false;
   #define NUM_BITMAPS 4
   BITMAP_TYPE bitmaps[NUM_BITMAPS] = {
      {NULL, NULL, NULL, NULL, A5O_PIXEL_FORMAT_ANY,       "Uncompressed"},
      {NULL, NULL, NULL, NULL, A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT1, "DXT1"},
      {NULL, NULL, NULL, NULL, A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT3, "DXT3"},
      {NULL, NULL, NULL, NULL, A5O_PIXEL_FORMAT_COMPRESSED_RGBA_DXT5, "DXT5"},
   };

   if (argc > 1) {
      filename = argv[1];
   }
   else {
      filename = "data/mysha.pcx";
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (argc > 2) {
      al_set_new_display_adapter(atoi(argv[2]));
   }

   al_init_image_addon();
   al_init_font_addon();
   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   for (ii = 0; ii < NUM_BITMAPS; ii++) {
      double t0, t1;
      al_set_new_bitmap_format(bitmaps[ii].format);

      /* Load */
      t0 = al_get_time();
      bitmaps[ii].bmp = al_load_bitmap(filename);
      t1 = al_get_time();

      if (!bitmaps[ii].bmp) {
         abort_example("%s not found or failed to load\n", filename);
      }
      log_printf("%s load time: %f sec\n", bitmaps[ii].name, t1 - t0);

      /* Clone */
      t0 = al_get_time();
      bitmaps[ii].clone = al_clone_bitmap(bitmaps[ii].bmp);
      t1 = al_get_time();

      if (!bitmaps[ii].clone) {
         abort_example("Couldn't clone %s\n", bitmaps[ii].name);
      }
      log_printf("%s clone time: %f sec\n", bitmaps[ii].name, t1 - t0);

      /* Decompress */
      al_set_new_bitmap_format(A5O_PIXEL_FORMAT_ANY);
      t0 = al_get_time();
      bitmaps[ii].decomp = al_clone_bitmap(bitmaps[ii].bmp);
      t1 = al_get_time();

      if (!bitmaps[ii].decomp) {
         abort_example("Couldn't decompress %s\n", bitmaps[ii].name);
      }
      log_printf("%s decompress time: %f sec\n", bitmaps[ii].name, t1 - t0);

      /* RW lock */
      al_set_new_bitmap_format(bitmaps[ii].format);
      bitmaps[ii].lock_clone = al_clone_bitmap(bitmaps[ii].bmp);

      if (!bitmaps[ii].lock_clone) {
         abort_example("Couldn't clone %s\n", bitmaps[ii].name);
      }

      if (al_get_bitmap_width(bitmaps[ii].bmp) > 128
            && al_get_bitmap_height(bitmaps[ii].bmp) > 128) {
         int bitmap_format = al_get_bitmap_format(bitmaps[ii].bmp);
         int block_width = al_get_pixel_block_width(bitmap_format);
         int block_height = al_get_pixel_block_height(bitmap_format);

         /* Lock and unlock it, hopefully causing a no-op operation */
         al_lock_bitmap_region_blocked(bitmaps[ii].lock_clone,
            16 / block_width, 16 / block_height, 64 / block_width,
            64 / block_height, A5O_LOCK_READWRITE);

         al_unlock_bitmap(bitmaps[ii].lock_clone);
      }
   }

   bkg = al_load_bitmap("data/bkg.png");
   if (!bkg) {
      abort_example("data/bkg.png not found or failed to load\n");
   }

   al_set_new_bitmap_format(A5O_PIXEL_FORMAT_ANY);
   font = al_create_builtin_font();
   timer = al_create_timer(1.0 / 30);
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   while (1) {
      A5O_EVENT event;
      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            goto EXIT;
         case A5O_EVENT_TIMER:
            redraw = true;
            break;
         case A5O_EVENT_KEY_DOWN:
            switch (event.keyboard.keycode) {
               case A5O_KEY_LEFT:
                  cur_bitmap = (cur_bitmap - 1 + NUM_BITMAPS) % NUM_BITMAPS;
                  break;
               case A5O_KEY_RIGHT:
                  cur_bitmap = (cur_bitmap + 1) % NUM_BITMAPS;
                  break;
               case A5O_KEY_SPACE:
                  compare = true;
                  break;
               case A5O_KEY_ESCAPE:
                  goto EXIT;
            }
            break;
         case A5O_EVENT_KEY_UP:
            if (event.keyboard.keycode == A5O_KEY_SPACE) {
               compare = false;
            }
            break;
      }
      if (redraw && al_is_event_queue_empty(queue)) {
         int w = al_get_bitmap_width(bitmaps[cur_bitmap].bmp);
         int h = al_get_bitmap_height(bitmaps[cur_bitmap].bmp);
         int idx = compare ? 0 : cur_bitmap;
         redraw = false;
         al_clear_to_color(al_map_rgb_f(0, 0, 0));
         al_draw_bitmap(bkg, 0, 0, 0);
         al_draw_textf(font, al_map_rgb_f(1, 1, 1), 5, 5, A5O_ALIGN_LEFT,
            "SPACE to compare. Arrows to switch. Format: %s", bitmaps[idx].name);
         al_draw_bitmap(bitmaps[idx].bmp, 0, 20, 0);
         al_draw_bitmap(bitmaps[idx].clone, w, 20, 0);
         al_draw_bitmap(bitmaps[idx].decomp, 0, 20 + h, 0);
         al_draw_bitmap(bitmaps[idx].lock_clone, w, 20 + h, 0);
         al_flip_display();
      }
   }
EXIT:

   al_destroy_bitmap(bkg);
   for (ii = 0; ii < NUM_BITMAPS; ii++) {
      al_destroy_bitmap(bitmaps[ii].bmp);
   }

   close_log(false);

   return 0;
}

/* vim: set sts=4 sw=4 et: */
