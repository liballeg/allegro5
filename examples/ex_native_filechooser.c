#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/a5_native_dialog.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_color.h>

/* To communicate from a separate thread, we need a user event. */
#define MY_EVENT 1024
typedef struct
{
    ALLEGRO_NATIVE_FILE_DIALOG *fd;
    ALLEGRO_EVENT_SOURCE *event_source;
} MyEventData;

/* Our thread to show the native file dialog. */
static void *dialog_thread(ALLEGRO_THREAD *thread, void *arg)
{
   ALLEGRO_EVENT event;
   MyEventData *data = arg;
   
   /* The next line is the heart of this example - we display the
    * native file dialog.
    */
   al_show_native_file_dialog(data->fd);

   /* We emit an event to let the main program now that the thread has
    * finished.
    */
   event.user.type = MY_EVENT;
   event.user.data1 = (intptr_t)data->fd;
   al_emit_user_event(data->event_source, &event, NULL);

   // FIXME: Can we do this? If not, where else should we do it?
   //al_destroy_thread(thread);
   return NULL;
}

/* Function to start the new thread. */
ALLEGRO_EVENT_SOURCE *spawn_dialog_thread(ALLEGRO_PATH *initial_path)
{
   ALLEGRO_THREAD *thread;
   MyEventData *data = malloc(sizeof *data);

   data->fd = al_create_native_file_dialog(
      initial_path, "Choose files", NULL,
      ALLEGRO_FILECHOOSER_MULTIPLE);
   
   data->event_source = al_create_user_event_source();
   thread = al_create_thread(dialog_thread, data);
   al_start_thread(thread);
   return data->event_source;
}

/* Helper function to display the result from a file dialog. */
static void show_files_list(ALLEGRO_NATIVE_FILE_DIALOG *dialog,
   ALLEGRO_FONT *font,
   ALLEGRO_COLOR info)
{
   int count = al_get_native_file_dialog_count(dialog);
   int th = al_font_text_height(font);
   float x = al_get_display_width() / 2;
   float y = al_get_display_height() / 2 - (count * th) / 2;
   int i;
   for (i = 0; i < count; i++) {
      char name[PATH_MAX];
      ALLEGRO_PATH *path;
      path = al_get_native_file_dialog_path(dialog, i);
      al_path_to_string(path, name, sizeof name, '/');
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
         info);
      al_font_textprintf_centre(font, x, y + i * th, name);
   }
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   bool redraw = true;
   ALLEGRO_COLOR background, active, inactive, info;
   ALLEGRO_NATIVE_FILE_DIALOG *dialog = NULL;
   bool already_shown = false;
   ALLEGRO_EVENT_SOURCE *my_event;

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
      /* When a mouse button is pressed, and no native dialog is
       * shown already, we show a new one.
       */
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
          if (!already_shown) {
             ALLEGRO_PATH *last_path = NULL;
             /* If available, use the path from the last dialog as
              * initial path for the new one.
              */
             if (dialog)
                last_path = al_get_native_file_dialog_path(dialog, 0);
             my_event = spawn_dialog_thread(last_path);
             al_register_event_source(queue, my_event);
             already_shown = true;
          }
      }
      /* We receive this event from the other thread when the dialog is
       * closed.
       */
      if (event.type == MY_EVENT) {
         ALLEGRO_NATIVE_FILE_DIALOG *new_dialog;
         new_dialog = (void *)event.user.data1;
         /* If files were selected, we replace the old files list. */
         if (al_get_native_file_dialog_count(new_dialog)) {
            if (dialog)
                al_destroy_native_file_dialog(dialog);
            dialog = new_dialog;
         }
         /* Otherwise the dialog was cancelled, and we keep the old
          * results.
          */
         else {
             al_destroy_native_file_dialog(new_dialog);
         }
         al_unregister_event_source(queue, my_event);
         al_destroy_user_event_source(my_event);
         already_shown = false;
      }

      if (event.type == ALLEGRO_EVENT_TIMER)
         redraw = true;

      if (redraw && al_event_queue_is_empty(queue)) {
         float x = al_get_display_width() / 2;
         float y = 0;
         redraw = false;
         al_clear(background);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                        already_shown ? inactive : active);
         al_font_textprintf_centre(font, x, y, "Open");
         if (dialog)
            show_files_list(dialog, font, info);
         al_flip_display();
      }
   }

   return 0;
}
END_OF_MAIN()
