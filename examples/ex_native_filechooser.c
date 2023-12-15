/*
 *    Example program for the Allegro library.
 *
 *    The native file dialog addon only supports a blocking interface.  This
 *    example makes the blocking call from another thread, using a user event
 *    source to communicate back to the main program.
 */

#define ALLEGRO_UNSTABLE
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_color.h>

#ifdef ALLEGRO_ANDROID
#include <allegro5/allegro_android.h>
#endif

#include "common.c"

/* To communicate from a separate thread, we need a user event. */
#define ASYNC_DIALOG_EVENT1   ALLEGRO_GET_EVENT_TYPE('e', 'N', 'F', '1')
#define ASYNC_DIALOG_EVENT2   ALLEGRO_GET_EVENT_TYPE('e', 'N', 'F', '2')


typedef struct
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FILECHOOSER *file_dialog;
   ALLEGRO_EVENT_SOURCE event_source;
   ALLEGRO_THREAD *thread;
} AsyncDialog;

ALLEGRO_TEXTLOG *textlog;

static void message(char const *format, ...)
{
   char str[1024];
   va_list args;

   va_start(args, format);
   vsnprintf(str, sizeof str, format, args);
   va_end(args);
   al_append_native_text_log(textlog, "%s", str);
}

/* Our thread to show the native file dialog. */
static void *async_file_dialog_thread_func(ALLEGRO_THREAD *thread, void *arg)
{
   AsyncDialog *data = arg;
   ALLEGRO_EVENT event;
   (void)thread;

   /* The next line is the heart of this example - we display the
    * native file dialog.
    */
   al_show_native_file_dialog(data->display, data->file_dialog);

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

   button = al_show_native_message_box(data->display, "Warning",
      "Click Detected",
      "That does nothing. Stop clicking there.",
      "Oh no!|Don't press|Ok", ALLEGRO_MESSAGEBOX_WARN);
   if (button == 2) {
      button = al_show_native_message_box(data->display, "Error", "Hey!",
         "Stop it! I told you not to click there.",
         NULL, ALLEGRO_MESSAGEBOX_ERROR);
   }

   event.user.type = ASYNC_DIALOG_EVENT2;
   al_emit_user_event(&data->event_source, &event, NULL);

   return NULL;
}


/* Function to start the new thread. */
static AsyncDialog *spawn_async_file_dialog(ALLEGRO_DISPLAY *display,
                                            const char *initial_path,
                                            bool save)
{
   AsyncDialog *data = malloc(sizeof *data);
   int flags = save ? ALLEGRO_FILECHOOSER_SAVE : ALLEGRO_FILECHOOSER_MULTIPLE;
   const char* title = save ? "Save (no files will be changed)" : "Choose files";
   data->file_dialog = al_create_native_file_dialog(
      initial_path, title, NULL,
      flags);
   al_init_user_event_source(&data->event_source);
   data->display = display;
   data->thread = al_create_thread(async_file_dialog_thread_func, data);

   al_start_thread(data->thread);

   return data;
}

static AsyncDialog *spawn_async_message_dialog(ALLEGRO_DISPLAY *display)
{
   AsyncDialog *data = calloc(1, sizeof *data);

   al_init_user_event_source(&data->event_source);
   data->display = display;
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
         al_destroy_native_file_dialog(data->file_dialog);
      free(data);
   }
}


/* Helper function to display the result from a file dialog. */
static void show_files_list(ALLEGRO_FILECHOOSER *dialog,
   const ALLEGRO_FONT *font, ALLEGRO_COLOR info)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   int count = al_get_native_file_dialog_count(dialog);
   int th = al_get_font_line_height(font);
   float x = al_get_bitmap_width(target) / 2;
   float y = al_get_bitmap_height(target) / 2 - (count * th) / 2;
   int i;

   for (i = 0; i < count; i++) {
      const char *name = al_get_native_file_dialog_path(dialog, i);
      al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
      al_draw_textf(font, info, x, y + i * th, ALLEGRO_ALIGN_CENTRE, name, 0, 0);
   }
}


int main(int argc, char **argv)
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
   bool halt_drawing = false;
   bool close_log = false;
   int button;
   bool message_log = true;
   bool touch;

   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   if (!al_init_native_dialog_addon()) {
      abort_example("Could not init native dialog addon.\n");
   }

   textlog = al_open_native_text_log("Log", 0);
   message("Starting up log window.\n");

   al_init_image_addon();
   al_init_font_addon();

   background = al_color_name("white");
   active = al_color_name("black");
   inactive = al_color_name("gray");
   info = al_color_name("red");

   al_install_mouse();
   touch = al_install_touch_input();
   al_install_keyboard();
    
   if (touch) {
      al_set_mouse_emulation_mode(ALLEGRO_MOUSE_EMULATION_5_0_x);
   }

   message("Creating window...");
    
#ifdef ALLEGRO_IPHONE
   al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
#endif

   display = al_create_display(640, 480);
   if (!display) {
      message("failure.\n");
      abort_example("Error creating display\n");
   }
   message("success.\n");

#ifdef ALLEGRO_ANDROID
   al_android_set_apk_file_interface();
#endif

   message("Loading font '%s'...", "data/fixed_font.tga");
   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      message("failure.\n");
      abort_example("Error loading data/fixed_font.tga\n");
   }
   message("success.\n");

   timer = al_create_timer(1.0 / 30);
restart:
   message("Starting main loop.\n");
   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   if (touch) {
      al_register_event_source(queue, al_get_touch_input_event_source());
      al_register_event_source(queue, al_get_touch_input_mouse_emulation_event_source());
   }
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   if (textlog) {
      al_register_event_source(queue, al_get_native_text_log_event_source(
         textlog));
   }
   al_start_timer(timer);

   while (1) {
      float h = al_get_display_height(display);
      
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE && !cur_dialog)
         break;

      if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
         if (!cur_dialog) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE ||
                event.keyboard.keycode == ALLEGRO_KEY_BACK)
               break;
         }
      }
       
      /* When a mouse button is pressed, and no native dialog is
       * shown already, we show a new one.
       */
      if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
          message("Mouse clicked at %d,%d.\n", event.mouse.x, event.mouse.y);
          if (event.mouse.y > 30) {
             if (event.mouse.y > h - 30) {
                message_log = !message_log;
                if (message_log) {
                   textlog = al_open_native_text_log("Log", 0);
                   if (textlog) {
                      al_register_event_source(queue,
                         al_get_native_text_log_event_source(textlog));
                   }
                }
                else {
                   close_log = true;
                }
             }
             else if (!message_box) {
                message_box = spawn_async_message_dialog(display);
                al_register_event_source(queue, &message_box->event_source);
             }
          }
          else if (!cur_dialog) {
             const char *last_path = NULL;
             bool save = event.mouse.x > al_get_display_width(display) / 2;
             /* If available, use the path from the last dialog as
              * initial path for the new one.
              */
             if (old_dialog) {
                last_path = al_get_native_file_dialog_path(
                   old_dialog->file_dialog, 0);
             }
             cur_dialog = spawn_async_file_dialog(display, last_path, save);
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

      if (event.type == ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE) {
         close_log = true;
      }

      if (event.type == ALLEGRO_EVENT_TIMER) {
         redraw = true;
      }

#ifdef ALLEGRO_ANDROID
      if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING) {
         message("Drawing halt");
         halt_drawing = true;
         al_stop_timer(timer);
         al_acknowledge_drawing_halt(display);
      }

      if (event.type == ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING) {
         message("Drawing resume");
         al_acknowledge_drawing_resume(display);
         al_resume_timer(timer);
         halt_drawing = false;
      }

      if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
         message("Display resize");
         al_acknowledge_resize(display);
      }
#else
      (void)halt_drawing;
#endif

      if (redraw && !halt_drawing && al_is_event_queue_empty(queue)) {
         float x = al_get_display_width(display) / 2;
         float y = 0;
         redraw = false;
         al_clear_to_color(background);
         al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
         al_draw_textf(font, cur_dialog ? inactive : active, x/2, y, ALLEGRO_ALIGN_CENTRE, "Open");
         al_draw_textf(font, cur_dialog ? inactive : active, x/2*3, y, ALLEGRO_ALIGN_CENTRE, "Save");
         al_draw_textf(font, cur_dialog ? inactive : active, x, h - 30,
            ALLEGRO_ALIGN_CENTRE, message_log ? "Close Message Log" : "Open Message Log");
         if (old_dialog)
            show_files_list(old_dialog->file_dialog, font, info);
         al_flip_display();
      }

      if (close_log && textlog) {
         close_log = false;
         message_log = false;
         al_unregister_event_source(queue,
            al_get_native_text_log_event_source(textlog));
         al_close_native_text_log(textlog);
         textlog = NULL;
      }
   }

   message("Exiting.\n");

   al_destroy_event_queue(queue);

   button = al_show_native_message_box(display,
      "Warning",
      "Are you sure?",
      "If you click yes then this example will inevitably close."
      " This is your last chance to rethink your decision."
      " Do you really want to quit?",
      NULL,
      ALLEGRO_MESSAGEBOX_YES_NO | ALLEGRO_MESSAGEBOX_QUESTION);
   if (button != 1)
      goto restart;

   stop_async_dialog(old_dialog);
   stop_async_dialog(cur_dialog);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
