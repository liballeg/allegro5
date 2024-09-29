#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#ifdef A5O_ANDROID
#include <allegro5/allegro_android.h> /* al_android_set_apk_file_interface */
#endif

A5O_DEBUG_CHANNEL("main")

#define MAX_TOUCH 10

struct touch {
   bool down;
   double x, y;
} touch[MAX_TOUCH];

/* debugging */
#define print_standard_path(std)                           \
   do {                                                    \
      A5O_PATH *path = al_get_standard_path(std);      \
      A5O_DEBUG(#std ": %s", al_path_cstr(path, '/')); \
   } while (0)

static void print_standard_paths(void)
{
   print_standard_path(A5O_RESOURCES_PATH);
   print_standard_path(A5O_TEMP_PATH);
   print_standard_path(A5O_USER_DATA_PATH);
   print_standard_path(A5O_USER_HOME_PATH);
   print_standard_path(A5O_USER_SETTINGS_PATH);
   print_standard_path(A5O_USER_DOCUMENTS_PATH);
   print_standard_path(A5O_EXENAME_PATH);
}

static void draw_touches(void)
{
   int i;

   for (i = 0; i < MAX_TOUCH; i++) {
      if (touch[i].down) {
         al_draw_filled_rectangle(
            touch[i].x-40, touch[i].y-40,
            touch[i].x+40, touch[i].y+40,
            al_map_rgb(100+i*20, 40+i*20, 40+i*20));
      }
   }
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *dpy;
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   A5O_TIMER *timer;
   A5O_BITMAP *image;
   A5O_BITMAP *image2;

   (void) argc;
   (void) argv;

   A5O_DEBUG("init allegro!");
   if (!al_init()) {
      return 1;
   }

   A5O_DEBUG("init primitives");
   al_init_primitives_addon();

   A5O_DEBUG("init image addon");
   al_init_image_addon();

   A5O_DEBUG("init touch input");
   al_install_touch_input();

   A5O_DEBUG("init keyboard");
   al_install_keyboard();

   A5O_DEBUG("creating display");
   dpy = al_create_display(800, 480);
   if (!dpy) {
      A5O_ERROR("failed to create display!");
      return 1;
   }

   print_standard_paths();

   /* This is loaded from assets in the apk. */
   #ifdef A5O_ANDROID
   al_android_set_apk_file_interface();
   #endif
   image = al_load_bitmap("data/alexlogo.png");
   if (!image) {
      A5O_DEBUG("failed to load alexlogo.png");
      return 1;
   }

   #ifdef A5O_ANDROID
   /* Copy the .png from the .apk into the user data area. */
   A5O_FILE *fin = al_fopen("data/alexlogo.png", "rb");
   al_set_standard_file_interface();
   A5O_PATH *path = al_get_standard_path(A5O_USER_DATA_PATH);
   al_set_path_filename(path, "alexlogo.png");
   A5O_FILE *fout = al_fopen(al_path_cstr(path, '/'), "wb");
   while (!al_feof(fin)) {
      char buf[1024];
      int n = al_fread(fin, buf, 1024);
      al_fwrite(fout, buf, n);
   }
   al_fclose(fin);
   al_fclose(fout);

   /* This is now loaded with the normal stdio file interface and not
    * from the APK.
    */
   image2 = al_load_bitmap(al_path_cstr(path, '/'));

   al_destroy_path(path);
   #else
   image2 = image;
   #endif


   al_convert_mask_to_alpha(image, al_map_rgb(255,0,255));
   if (image2)
      al_convert_mask_to_alpha(image2, al_map_rgb(255,0,255));

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_display_event_source(dpy));
   al_register_event_source(queue, al_get_touch_input_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());

   timer = al_create_timer(1/60.0);
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_start_timer(timer);

   bool draw = true;
   bool running = true;
   int count = 0;

   while (running) {
      al_wait_for_event(queue, &event);

      switch (event.type) {
         case A5O_EVENT_TOUCH_BEGIN:
            //A5O_DEBUG("touch %i begin", event.touch.id);
            touch[event.touch.id].down = true;
            touch[event.touch.id].x = event.touch.x;
            touch[event.touch.id].y = event.touch.y;
            break;

         case A5O_EVENT_TOUCH_END:
            //A5O_DEBUG("touch %i end", event.touch.id);
            touch[event.touch.id].down = false;
            touch[event.touch.id].x = 0.0;
            touch[event.touch.id].y = 0.0;
            break;

         case A5O_EVENT_TOUCH_MOVE:
            //A5O_DEBUG("touch %i move: %fx%f", event.touch.id, event.touch.x, event.touch.y);
            touch[event.touch.id].x = event.touch.x;
            touch[event.touch.id].y = event.touch.y;
            break;

         case A5O_EVENT_TOUCH_CANCEL:
            //A5O_DEBUG("touch %i canceled", event.touch.id);
            break;

         case A5O_EVENT_KEY_UP:
            if (event.keyboard.keycode == A5O_KEY_BACK) {
               A5O_DEBUG("back key pressed, exit!");
               running = false;
            }
            else {
               A5O_DEBUG("%i key pressed", event.keyboard.keycode);
            }
            break;

         case A5O_EVENT_TIMER:
            draw = true;
            if (count == 60) {
               A5O_DEBUG("tick");
               count = 0;
            }
            count++;
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            A5O_DEBUG("display close");
            running = false;
            break;

         case A5O_EVENT_DISPLAY_HALT_DRAWING:
            A5O_DEBUG("halt drawing");
            // Stop the timer so we don't run at all while our display isn't
            // active.
            al_stop_timer(timer);
            A5O_DEBUG("after set target");
            draw = false;
            al_acknowledge_drawing_halt(dpy);
            break;

         case A5O_EVENT_DISPLAY_RESUME_DRAWING:
            A5O_DEBUG("resume drawing");

            al_acknowledge_drawing_resume(dpy);
            A5O_DEBUG("done waiting for surface recreated");

            al_start_timer(timer);
            break;

         case A5O_EVENT_DISPLAY_RESIZE:
            A5O_DEBUG("display resize");
            al_acknowledge_resize(dpy);
            A5O_DEBUG("done resize");
            break;
      }

      if (draw && al_event_queue_is_empty(queue)) {
         draw = false;
         al_clear_to_color(al_map_rgb(255, 255, 255));
         if (image) {
            al_draw_bitmap(image,
               al_get_display_width(dpy)/2 - al_get_bitmap_width(image)/2,
               al_get_display_height(dpy)/2 - al_get_bitmap_height(image)/2,
               0);
         }
         if (image2) {
            al_draw_bitmap(image2,
               al_get_display_width(dpy)/2 - al_get_bitmap_width(image)/2,
               al_get_display_height(dpy)/2 + al_get_bitmap_height(image)/2,
               0);
         }
         draw_touches();
         al_flip_display();
      }
   }

   A5O_DEBUG("done");
   return 0;
}

/* vim: set sts=3 sw=3 et: */
