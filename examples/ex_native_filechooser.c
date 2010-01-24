/*
 *    Example program for the Allegro library.
 *
 *    The native file dialog addon only supports a blocking interface.  This
 *    example makes the blocking call from another thread, using a user event
 *    source to communicate back to the main program.
 */

#include <stdio.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>

#include "common.c"

/* To communicate from a separate thread, we need a user event. */
#define ASYNC_DIALOG_EVENT1   ALLEGRO_GET_EVENT_TYPE('e', 'N', 'F', '1')
#define ASYNC_DIALOG_EVENT2   ALLEGRO_GET_EVENT_TYPE('e', 'N', 'F', '2')


typedef struct
{
   ALLEGRO_NATIVE_DIALOG *file_dialog;
   ALLEGRO_EVENT_SOURCE event_source;
   ALLEGRO_THREAD *thread;
#ifdef ALLEGRO_WINDOWS
   /* XXX This is only used for a workaround for Windows. In general it
    * is NOT supported to have a single display be current for multiple
    * threads simultaneously.
    */
   ALLEGRO_DISPLAY *display;
#endif
} AsyncDialog;


/* Our thread to show the native file dialog. */
static void *async_file_dialog_thread_func(ALLEGRO_THREAD *thread, void *arg)
{
   AsyncDialog *data = arg;
   ALLEGRO_EVENT event;
   (void)thread;

#ifdef ALLEGRO_WINDOWS
   al_set_current_display(data->display);
#endif

   /* The next line is the heart of this example - we display the
    * native file dialog.
    */
   al_show_native_file_dialog(data->file_dialog);

   /* We emit an event to let the main program know that the thread has
    * finished.
    */
   event.user.type = ASYNC_DIALOG_EVENT1;
   al_emit_user_event(&data->event_source, &event, NULL);

   return NULL;
}


/* A thread to show the message boxes. */
static void *message_box_thread(ALLEGRO_THREAD *thread, void *arg)
{
   AsyncDialog *data = arg;
   ALLEGRO_EVENT event;
   int button;

   (void)thread;

#ifdef ALLEGRO_WINDOWS
   /* We need to set the current display for this thread because
    * al_show_native_file_dialog() shows the dialog on the current window.
    */
   al_set_current_display(data->display);
#endif

   button = al_show_native_message_box("Warning",
      "Warning! Click Detected!",
      "This is the last warning. There is nothing to click here.",
      "Oh no!|Don't press|Ok", ALLEGRO_MESSAGEBOX_WARN);
   if (button == 2) {
      button = al_show_native_message_box("Error", "Error!",
         "Invalid button press detected.",
         NULL, ALLEGRO_MESSAGEBOX_ERROR);
   }

   event.user.type = ASYNC_DIALOG_EVENT2;
   al_emit_user_event(&data->event_source, &event, NULL);

   return NULL;
}


/* Function to start the new thread. */
static AsyncDialog *spawn_async_file_dialog(const ALLEGRO_PATH *initial_path)
{
   AsyncDialog *data = malloc(sizeof *data);

   data->file_dialog = al_create_native_file_dialog(
      initial_path, "Choose files", NULL,
      ALLEGRO_FILECHOOSER_MULTIPLE);
   al_init_user_event_source(&data->event_source);
#ifdef ALLEGRO_WINDOWS
   /* Keep in mind that the current display is thread-local. */
   data->display = al_get_current_display();
#endif
   data->thread = al_create_thread(async_file_dialog_thread_func, data);

   al_start_thread(data->thread);

   return data;
}

static AsyncDialog *spawn_async_message_dialog(void)
{
   AsyncDialog *data = calloc(1, sizeof *data);

   al_init_user_event_source(&data->event_source);
#ifdef ALLEGRO_WINDOWS
   data->display = al_get_current_display();
#endif
   data->thread = al_create_thread(message_box_thread, data);

   al_start_thread(data->thread);

   return data;
}


static void stop_async_dialog(AsyncDialog *data)
{
   if (data) {
      al_destroy_thread(data->thread);
      al_destroy_user_event_source(&data->event_source);
      if (data->file_dialog)
         al_destroy_native_dialog(data->file_dialog);
      free(data);
   }
}


/* Helper function to display the result from a file dialog. */
static void show_files_list(ALLEGRO_NATIVE_DIALOG *dialog,
   const ALLEGRO_FONT *font, ALLEGRO_COLOR info)
{
   int count = al_get_native_file_dialog_count(dialog);
   int th = al_get_font_line_height(font);
   float x = al_get_display_width() / 2;
   float y = al_get_display_height() / 2 - (count * th) / 2;
   int i;

   for (i = 0; i < count; i++) {
      const ALLEGRO_PATH *path;
      const char *name;

      path = al_get_native_file_dialog_path(dialog, i);
      name = al_path_cstr(path, '/');
      al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, info);
      al_draw_textf(font, x, y + i * th, ALLEGRO_ALIGN_CENTRE, name, 0, 0);
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
   AsyncDialog *message_box = NULL;
   bool redraw = false;
   int button;

   al_init();
   al_init_font_addon();

   background = al_color_name("white");
   active = al_color_name("black");
   inactive = al_color_name("gray");
   info = al_color_name("red");

   al_install_mouse();
   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Error creating display\n");
      return 1;
   }

   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      abort_example("Error loading data/fixed_font.tga\n");
      return 1;
   }

   timer = al_install_timer(1.0 / 30);
restart:
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
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
          if (event.mouse.y > 30) {
             if (!message_box) {
                message_box = spawn_async_message_dialog();
                al_register_event_source(queue, &message_box->event_source);
             }
          }
          else if (!cur_dialog) {
             const ALLEGRO_PATH *last_path = NULL;
             /* If available, use the path from the last dialog as
              * initial path for the new one.
              */
             if (old_dialog) {
                last_path = al_get_native_file_dialog_path(
                   old_dialog->file_dialog, 0);
             }
             cur_dialog = spawn_async_file_dialog(last_path);
             al_register_event_source(queue, &cur_dialog->event_source);
          }
      }
      /* We receive this event from the other thread when the dialog is
       * closed.
       */
      if (event.type == ASYNC_DIALOG_EVENT1) {
         al_unregister_event_source(queue, &cur_dialog->event_source);

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
      if (event.type == ASYNC_DIALOG_EVENT2) {
         al_unregister_event_source(queue, &message_box->event_source);
         stop_async_dialog(message_box);
         message_box = NULL;
      }

      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }

      if (redraw && al_event_queue_is_empty(queue)) {
         float x = al_get_display_width() / 2;
         float y = 0;
         redraw = false;
         al_clear_to_color(background);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
                        cur_dialog ? inactive : active);
         al_draw_textf(font, x, y, ALLEGRO_ALIGN_CENTRE, "Open");
         if (old_dialog)
            show_files_list(old_dialog->file_dialog, font, info);
         al_flip_display();
      }
   }

   al_destroy_event_queue(queue);

   button = al_show_native_message_box(
      "Warning",
      "Are you sure?",
      "If you click yes then this example will inevitably close."
      " This is your last chance to rethink your decision."
      " Do you really want to quit?",
      NULL,
      ALLEGRO_MESSAGEBOX_YES_NO);
   if (button != 1)
      goto restart;

   stop_async_dialog(old_dialog);
   stop_async_dialog(cur_dialog);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
