#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/a5_native_dialog.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_color.h>

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   bool redraw = true;
   ALLEGRO_COLOR background, active, inactive, info;
   ALLEGRO_EVENT_SOURCE *dialog = NULL;
   ALLEGRO_PATH *last_path = NULL;
   ALLEGRO_PATH *result[25];
   int count = 0;

   al_init();
   al_font_init();

   background = al_color_name("white");
   active = al_color_name("black");
   inactive = al_color_name("gray");
   info = al_color_name("red");

   font = al_font_load_font("data/fixed_font.tga", 0);

   al_install_mouse();
   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   timer = al_install_timer(1.0 / 30);
   queue = al_create_event_queue();
   al_register_event_source(queue, (void *)al_get_keyboard());
   al_register_event_source(queue, (void *)al_get_mouse());
   al_register_event_source(queue, (void *)display);
   al_register_event_source(queue, (void *)timer);
   al_start_timer(timer);

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);
      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
         break;
      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
            break;
      }
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
          if (!dialog) {
             dialog = al_spawn_native_file_dialog(last_path, NULL,
                ALLEGRO_FILECHOOSER_MULTIPLE);
             al_register_event_source(queue, dialog);
          }
      }
      if (event.type == ALLEGRO_EVENT_NATIVE_FILE_CHOOSER) {
         count = al_finalize_native_file_dialog(&event, result, 25);
         al_unregister_event_source(queue, dialog);
         dialog = NULL;
         if (last_path)
            al_path_free(last_path);
         last_path = NULL;
         if (count)
            last_path = result[0];
      }
      if (event.type == ALLEGRO_EVENT_TIMER)
         redraw = true;

      if (redraw && al_event_queue_is_empty(queue)) {
         float x = al_get_display_width() / 2;
         float y = 0;
         int i;
         int th = al_font_text_height(font);
         redraw = false;
         al_clear(background);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                        dialog ? inactive : active);
         al_font_textprintf_centre(font, x, y, "Open");
         y = al_get_display_height() / 2 - (count * th) / 2;
         for (i = 0; i < count; i++) {
            char name[PATH_MAX];
            al_path_to_string(result[i], name, sizeof name, '/');
            al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, info);
            al_font_textprintf_centre(font, x, y + i * th, name);
         }
         al_flip_display();
      }
   }

   return 0;
}
END_OF_MAIN()
