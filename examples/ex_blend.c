/* An example demonstrating different blending modes.
 */

#define ALLEGRO_UNSTABLE
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

#include "common.c"

/* A structure holding all variables of our example program. */
struct Example
{
   ALLEGRO_BITMAP *example; /* Our example bitmap. */
   ALLEGRO_BITMAP *offscreen; /* An offscreen buffer, for testing. */
   ALLEGRO_BITMAP *memory; /* A memory buffer, for testing. */
   ALLEGRO_FONT *myfont; /* Our font. */
   ALLEGRO_EVENT_QUEUE *queue; /* Our events queue. */
   int image; /* Which test image to use. */
   int mode; /* How to draw it. */
   int BUTTONS_X; /* Where to draw buttons. */

   int FPS;
   double last_second;
   int frames_accum;
   double fps;
} ex;

/* Print some text with a shadow. */
static void print(int x, int y, bool vertical, char const *format, ...)
{
   va_list list;
   char message[1024];
   ALLEGRO_COLOR color;
   int h;
   int j;

   va_start(list, format);
   vsnprintf(message, sizeof message, format, list);
   va_end(list);

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   h = al_get_font_line_height(ex.myfont);

   for (j = 0; j < 2; j++) {
      if (j == 0)
         color = al_map_rgb(0, 0, 0);
      else
         color = al_map_rgb(255, 255, 255);

      if (vertical) {
         int i;
         ALLEGRO_USTR_INFO ui;
         const ALLEGRO_USTR *us = al_ref_cstr(&ui, message);
         for (i = 0; i < (int)al_ustr_length(us); i++) {
            ALLEGRO_USTR_INFO letter;
            al_draw_ustr(ex.myfont, color, x + 1 - j, y + 1 - j + h * i, 0,
               al_ref_ustr(&letter, us, al_ustr_offset(us, i),
               al_ustr_offset(us, i + 1)));
         }
      }
      else {
         al_draw_text(ex.myfont, color, x + 1 - j, y + 1 - j, 0, message);
      }
   }
}

/* Create an example bitmap. */
static ALLEGRO_BITMAP *create_example_bitmap(void)
{
   ALLEGRO_BITMAP *bitmap;
   int i, j;
   ALLEGRO_LOCKED_REGION *locked;
   unsigned char *data;

   bitmap = al_create_bitmap(100, 100);
   locked = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
   data = locked->data;

   for (j = 0; j < 100; j++) {
      for (i = 0; i < 100; i++) {
         int x = i - 50, y = j - 50;
         int r = sqrt(x * x + y * y);
         float rc = 1 - r / 50.0;
         if (rc < 0)
            rc = 0;
         data[i * 4 + 0] = i * 255 / 100;
         data[i * 4 + 1] = j * 255 / 100;
         data[i * 4 + 2] = rc * 255;
         data[i * 4 + 3] = rc * 255;
      }
      data += locked->pitch;
   }
   al_unlock_bitmap(bitmap);

   return bitmap;
}

/* Draw our example scene. */
static void draw(void)
{
   ALLEGRO_COLOR test[5];
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   char const *blend_names[] = {"ZERO", "ONE", "ALPHA", "INVERSE"};
   char const *blend_vnames[] = {"ZERO", "ONE", "ALPHA", "INVER"};
   int blend_modes[] = {ALLEGRO_ZERO, ALLEGRO_ONE, ALLEGRO_ALPHA,
      ALLEGRO_INVERSE_ALPHA};
   float x = 40, y = 40;
   int i, j;

   al_clear_to_color(al_map_rgb_f(0.5, 0.5, 0.5));

   test[0] = al_map_rgba_f(1, 1, 1, 1);
   test[1] = al_map_rgba_f(1, 1, 1, 0.5);
   test[2] = al_map_rgba_f(1, 1, 1, 0.25);
   test[3] = al_map_rgba_f(1, 0, 0, 0.75);
   test[4] = al_map_rgba_f(0, 0, 0, 0);

   print(x, 0, false, "D  E  S  T  I  N  A  T  I  O  N  (%0.2f fps)", ex.fps);
   print(0, y, true, "S O U R C E");
   for (i = 0; i < 4; i++) {
      print(x + i * 110, 20, false, blend_names[i]);
      print(20, y + i * 110, true, blend_vnames[i]);
   }

   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   if (ex.mode >= 1 && ex.mode <= 5) {
      al_set_target_bitmap(ex.offscreen);
      al_clear_to_color(test[ex.mode - 1]);
   }
   if (ex.mode >= 6 && ex.mode <= 10) {
      al_set_target_bitmap(ex.memory);
      al_clear_to_color(test[ex.mode - 6]);
   }

   for (j = 0; j < 4; j++) {
      for (i = 0; i < 4; i++) {
         al_set_blender(ALLEGRO_ADD, blend_modes[j], blend_modes[i]);
         if (ex.image == 0)
            al_draw_bitmap(ex.example, x + i * 110, y + j * 110, 0);
         else if (ex.image >= 1 && ex.image <= 6) {
            al_draw_filled_rectangle(x + i * 110, y + j * 110,
               x + i * 110 + 100, y + j * 110 + 100,
                  test[ex.image - 1]);
         }
      }
   }

   if (ex.mode >= 1 && ex.mode <= 5) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
      al_set_target_bitmap(target);
      al_draw_bitmap_region(ex.offscreen, x, y, 430, 430, x, y, 0);
   }
   if (ex.mode >= 6 && ex.mode <= 10) {
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
      al_set_target_bitmap(target);
      al_draw_bitmap_region(ex.memory, x, y, 430, 430, x, y, 0);
   }

   #define IS(x)  ((ex.image == x) ? "*" : " ")
   print(ex.BUTTONS_X, 20 * 1, false, "What to draw");
   print(ex.BUTTONS_X, 20 * 2, false, "%s Picture", IS(0));
   print(ex.BUTTONS_X, 20 * 3, false, "%s Rec1 (1/1/1/1)", IS(1));
   print(ex.BUTTONS_X, 20 * 4, false, "%s Rec2 (1/1/1/.5)", IS(2));
   print(ex.BUTTONS_X, 20 * 5, false, "%s Rec3 (1/1/1/.25)", IS(3));
   print(ex.BUTTONS_X, 20 * 6, false, "%s Rec4 (1/0/0/.75)", IS(4));
   print(ex.BUTTONS_X, 20 * 7, false, "%s Rec5 (0/0/0/0)", IS(5));
   #undef IS

   #define IS(x)  ((ex.mode == x) ? "*" : " ")
   print(ex.BUTTONS_X, 20 * 9, false, "Where to draw");
   print(ex.BUTTONS_X, 20 * 10, false, "%s screen", IS(0));

   print(ex.BUTTONS_X, 20 * 11, false, "%s offscreen1", IS(1));
   print(ex.BUTTONS_X, 20 * 12, false, "%s offscreen2", IS(2));
   print(ex.BUTTONS_X, 20 * 13, false, "%s offscreen3", IS(3));
   print(ex.BUTTONS_X, 20 * 14, false, "%s offscreen4", IS(4));
   print(ex.BUTTONS_X, 20 * 15, false, "%s offscreen5", IS(5));

   print(ex.BUTTONS_X, 20 * 16, false, "%s memory1", IS(6));
   print(ex.BUTTONS_X, 20 * 17, false, "%s memory2", IS(7));
   print(ex.BUTTONS_X, 20 * 18, false, "%s memory3", IS(8));
   print(ex.BUTTONS_X, 20 * 19, false, "%s memory4", IS(9));
   print(ex.BUTTONS_X, 20 * 20, false, "%s memory5", IS(10));
   #undef IS
}

/* Called a fixed amount of times per second. */
static void tick(void)
{
   /* Count frames during the last second or so. */
   double t = al_get_time();
   if (t >= ex.last_second + 1) {
      ex.fps = ex.frames_accum / (t - ex.last_second);
      ex.frames_accum = 0;
      ex.last_second = t;
   }

   draw();
   al_flip_display();
   ex.frames_accum++;
}

/* Run our test. */
static void run(void)
{
   ALLEGRO_EVENT event;
   float x, y;
   bool need_draw = true;

   while (1) {
      /* Perform frame skipping so we don't fall behind the timer events. */
      if (need_draw && al_is_event_queue_empty(ex.queue)) {
         tick();
         need_draw = false;
      }

      al_wait_for_event(ex.queue, &event);

      switch (event.type) {
         /* Was the X button on the window pressed? */
         case ALLEGRO_EVENT_DISPLAY_CLOSE:
            return;

         /* Was a key pressed? */
         case ALLEGRO_EVENT_KEY_DOWN:
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
               return;
            break;

         /* Is it time for the next timer tick? */
         case ALLEGRO_EVENT_TIMER:
            need_draw = true;
            break;

         /* Mouse click? */
         case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            x = event.mouse.x;
            y = event.mouse.y;
            if (x >= ex.BUTTONS_X) {
               int button = y / 20;
               if (button == 2) ex.image = 0;
               if (button == 3) ex.image = 1;
               if (button == 4) ex.image = 2;
               if (button == 5) ex.image = 3;
               if (button == 6) ex.image = 4;
               if (button == 7) ex.image = 5;

               if (button == 10) ex.mode = 0;

               if (button == 11) ex.mode = 1;
               if (button == 12) ex.mode = 2;
               if (button == 13) ex.mode = 3;
               if (button == 14) ex.mode = 4;
               if (button == 15) ex.mode = 5;

               if (button == 16) ex.mode = 6;
               if (button == 17) ex.mode = 7;
               if (button == 18) ex.mode = 8;
               if (button == 19) ex.mode = 9;
               if (button == 20) ex.mode = 10;
            }
            break;
      }
   }
}

/* Initialize the example. */
static void init(void)
{
   ex.BUTTONS_X = 40 + 110 * 4;
   ex.FPS = 60;

   ex.myfont = al_load_font("data/font.tga", 0, 0);
   if (!ex.myfont) {
      abort_example("data/font.tga not found\n");
   }
   ex.example = create_example_bitmap();

   ex.offscreen = al_create_bitmap(640, 480);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   ex.memory = al_create_bitmap(640, 480);
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_install_keyboard();
   al_install_mouse();
   al_install_touch_input();
   al_init_image_addon();
   al_init_font_addon();
   init_platform_specific();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
   }

   init();

   timer = al_create_timer(1.0 / ex.FPS);

   ex.queue = al_create_event_queue();
   al_register_event_source(ex.queue, al_get_keyboard_event_source());
   al_register_event_source(ex.queue, al_get_mouse_event_source());
   al_register_event_source(ex.queue, al_get_display_event_source(display));
   al_register_event_source(ex.queue, al_get_timer_event_source(timer));
   if (al_is_touch_input_installed()) {
      al_register_event_source(ex.queue,
         al_get_touch_input_mouse_emulation_event_source());
   }

   al_start_timer(timer);
   run();

   al_destroy_event_queue(ex.queue);

   return 0;
}
