/*
 *    Example program for the Allegro library.
 *
 *    The native file dialog addon only supports a blocking interface.  This
 *    example makes the blocking call from another thread, using a user event
 *    source to communicate back to the main program.
 */

#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/a5_native_dialog.h>
#include <allegro5/a5_font.h>
#include <allegro5/a5_color.h>


/* To communicate from a separate thread, we need a user event. */
#define ASYNC_DIALOG_EVENT    ALLEGRO_GET_EVENT_TYPE('e', 'N', 'F', 'D')


typedef struct
{
   ALLEGRO_NATIVE_FILE_DIALOG *file_dialog;
   ALLEGRO_EVENT_SOURCE *event_source;
   ALLEGRO_THREAD *thread;
} AsyncDialog;


/* Our thread to show the native file dialog. */
static void *async_file_dialog_thread_func(ALLEGRO_THREAD *thread, void *arg)
{
   AsyncDialog *data = arg;
   ALLEGRO_EVENT event;
   (void)thread;

   /* The next line is the heart of this example - we display the
    * native file dialog.
    */
   al_show_native_file_dialog(data->file_dialog);

   /* We emit an event to let the main program now that the thread has
    * finished.
    */
   event.user.type = ASYNC_DIALOG_EVENT;
   al_emit_user_event(data->event_source, &event, NULL);

   return NULL;
}


/* Function to start the new thread. */
AsyncDialog *spawn_async_dialog(const ALLEGRO_PATH *initial_path)
{
   AsyncDialog *data = malloc(sizeof *data);

   data->file_dialog = al_create_native_file_dialog(
      initial_path, "Choose files", NULL,
      ALLEGRO_FILECHOOSER_MULTIPLE);
   data->event_source = al_create_user_event_source();
   data->thread = al_create_thread(async_file_dialog_thread_func, data);

   al_start_thread(data->thread);

   return data;
}


void stop_async_dialog(AsyncDialog *data)
{
   if (data) {
      al_destroy_thread(data->thread);
      al_destroy_user_event_source(data->event_source);
      al_destroy_native_file_dialog(data->file_dialog);
      free(data);
   }
}


/* Helper function to display the result from a file dialog. */
static void show_files_list(ALLEGRO_NATIVE_FILE_DIALOG *dialog,
   const ALLEGRO_FONT *font, ALLEGRO_COLOR info)
{
   int count = al_get_native_file_dialog_count(dialog);
   int th = al_font_text_height(font);
   float x = al_get_display_width() / 2;
   float y = al_get_display_height() / 2 - (count * th) / 2;
   int i;

   for (i = 0; i < count; i++) {
      char name[PATH_MAX];
      const ALLEGRO_PATH *path;

      path = al_get_native_file_dialog_path(dialog, i);
      al_path_to_string(path, name, sizeof name, '/');
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, info);
      al_font_textout_centre(font, x, y + i * th, name, -1);
   }
}


int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_TIMER *timer;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_FONT *font;
   ALLEGRO_COLOR background, active, inactive, info;
   AsyncDialog *old_dialog = NULL;
   AsyncDialog *cur_dialog = NULL;
   bool redraw = false;

   al_init();
   al_font_init();

   background = al_color_name("white");
   active = al_color_name("black");
   inactive = al_color_name("gray");
   info = al_color_name("red");

   al_install_mouse();
   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   font = al_font_load_font("data/fixed_font.tga", 0);
   if (!font) {
      TRACE("Error loading data/fixed_font.tga\n");
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

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE && !cur_dialog)
         break;

      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE && !cur_dialog)
            break;
      }

      /* When a mouse button is pressed, and no native dialog is
       * shown already, we show a new one.
       */
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
          if (!cur_dialog) {
             const ALLEGRO_PATH *last_path = NULL;
             /* If available, use the path from the last dialog as
              * initial path for the new one.
              */
             if (old_dialog) {
                last_path = al_get_native_file_dialog_path(
                   old_dialog->file_dialog, 0);
             }
             cur_dialog = spawn_async_dialog(last_path);
             al_register_event_source(queue, cur_dialog->event_source);
          }
      }

      /* We receive this event from the other thread when the dialog is
       * closed.
       */
      if (event.type == ASYNC_DIALOG_EVENT) {
         al_unregister_event_source(queue, cur_dialog->event_source);

         /* If files were selected, we replace the old files list.
          * Otherwise the dialog was cancelled, and we keep the old results.
          */
         if (al_get_native_file_dialog_count(cur_dialog->file_dialog) > 0) {
            if (old_dialog)
               stop_async_dialog(old_dialog);
            old_dialog = cur_dialog;
         }
         else {
            stop_async_dialog(cur_dialog);
         }
         cur_dialog = NULL;
      }

      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }

      if (redraw && al_event_queue_is_empty(queue)) {
         float x = al_get_display_width() / 2;
         float y = 0;
         redraw = false;
         al_clear(background);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                        cur_dialog ? inactive : active);
         al_font_textprintf_centre(font, x, y, "Open");
         if (old_dialog)
            show_files_list(old_dialog->file_dialog, font, info);
         al_flip_display();
      }
   }

   stop_async_dialog(old_dialog);
   stop_async_dialog(cur_dialog);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
