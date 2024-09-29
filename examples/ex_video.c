#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_video.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>

#include "common.c"

static A5O_DISPLAY *screen;
static A5O_FONT *font;
static char const *filename;
static float zoom = 0;

static void video_display(A5O_VIDEO *video)
{
   /* Videos often do not use square pixels - these return the scaled dimensions
    * of the video frame.
    */
   float scaled_w = al_get_video_scaled_width(video);
   float scaled_h = al_get_video_scaled_height(video);
   /* Get the currently visible frame of the video, based on clock
    * time.
    */
   A5O_BITMAP *frame = al_get_video_frame(video);
   int w, h, x, y;
   A5O_COLOR tc = al_map_rgba_f(0, 0, 0, 0.5);
   A5O_COLOR bc = al_map_rgba_f(0.5, 0.5, 0.5, 0.5);
   double p;

   if (!frame)
      return;

   if (zoom == 0) {
      /* Always make the video fit into the window. */
      h = al_get_display_height(screen);
      w = (int)(h * scaled_w / scaled_h);
      if (w > al_get_display_width(screen)) {
         w = al_get_display_width(screen);
         h = (int)(w * scaled_h / scaled_w);
      }
   }
   else {
      w = (int)scaled_w;
      h = (int)scaled_h;
   }
   x = (al_get_display_width(screen) - w) / 2;
   y = (al_get_display_height(screen) - h) / 2;

   /* Display the frame. */
   al_draw_scaled_bitmap(frame, 0, 0,
                         al_get_bitmap_width(frame),
                         al_get_bitmap_height(frame), x, y, w, h, 0);

   /* Show some video information. */
   al_draw_filled_rounded_rectangle(4, 4,
      al_get_display_width(screen) - 4, 4 + 14 * 4, 8, 8, bc);
   p = al_get_video_position(video, A5O_VIDEO_POSITION_ACTUAL);
   al_draw_textf(font, tc, 8, 8 , 0, "%s", filename);
   al_draw_textf(font, tc, 8, 8 + 13, 0, "%3d:%02d (V: %+5.2f A: %+5.2f)",
      (int)(p / 60),
      ((int)p) % 60,
      al_get_video_position(video, A5O_VIDEO_POSITION_VIDEO_DECODE) - p,
      al_get_video_position(video, A5O_VIDEO_POSITION_AUDIO_DECODE) - p);
   al_draw_textf(font, tc, 8, 8 + 13 * 2, 0,
      "video rate %.02f (%dx%d, aspect %.1f) audio rate %.0f",
         al_get_video_fps(video),
         al_get_bitmap_width(frame),
         al_get_bitmap_height(frame),
         scaled_w / scaled_h,
         al_get_video_audio_rate(video));
   al_draw_textf(font, tc, 8, 8 + 13 * 3, 0,
      "playing: %s", al_is_video_playing(video) ? "true" : "false");
   al_flip_display();
   al_clear_to_color(al_map_rgb(0, 0, 0));
}


int main(int argc, char *argv[])
{
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   A5O_TIMER *timer;
   A5O_VIDEO *video;
   bool fullscreen = false;
   bool redraw = true;
   bool use_frame_events = false;
   int filename_arg_idx = 1;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (argc < 2) {
      log_printf("This example needs to be run from the command line.\n"
                 "Usage: %s [--use-frame-events] <file>\n", argv[0]);
      goto done;
   }

   /* If use_frame_events is false, we use a fixed FPS timer. If the video is
    * displayed in a game this probably makes most sense. In a
    * dedicated video player you probably want to listen to
    * A5O_EVENT_VIDEO_FRAME_SHOW events and only redraw whenever one
    * arrives - to reduce possible jitter and save CPU.
    */
   if (argc == 3 && strcmp(argv[1], "--use-frame-events") == 0) {
      use_frame_events = true;
      filename_arg_idx++;
   }

   if (!al_init_video_addon()) {
      abort_example("Could not initialize the video addon.\n");
   }
   al_init_font_addon();
   al_install_keyboard();

   al_install_audio();
   al_reserve_samples(1);
   al_init_primitives_addon();

   timer = al_create_timer(1.0 / 60);

   al_set_new_display_flags(A5O_RESIZABLE);
   al_set_new_display_option(A5O_VSYNC, 1, A5O_SUGGEST);
   screen = al_create_display(640, 480);
   if (!screen) {
      abort_example("Could not set video mode - exiting\n");
   }
   
   font = al_create_builtin_font();
   if (!font) {
      abort_example("No font.\n");
   }

   al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR);

   filename = argv[filename_arg_idx];
   video = al_open_video(filename);
   if (!video) {
      abort_example("Cannot read %s.\n", filename);
   }
   log_printf("video FPS: %f\n", al_get_video_fps(video));
   log_printf("video audio rate: %f\n", al_get_video_audio_rate(video));
   log_printf(
      "keys:\n"
      "Space: Play/Pause\n"
      "cursor right/left: seek 10 seconds\n"
      "cursor up/down: seek one minute\n"
      "F: toggle fullscreen\n"
      "1: disable scaling\n"
      "S: scale to window\n");

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_video_event_source(video));
   al_register_event_source(queue, al_get_display_event_source(screen));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_keyboard_event_source());

   al_start_video(video, al_get_default_mixer());
   al_start_timer(timer);
   for (;;) {
      double incr;

      if (redraw && al_event_queue_is_empty(queue)) {
         video_display(video);
         redraw = false;
      }

      al_wait_for_event(queue, &event);
      switch (event.type) {
         case A5O_EVENT_KEY_DOWN:
            switch (event.keyboard.keycode) {
               case A5O_KEY_SPACE:
                  al_set_video_playing(video, !al_is_video_playing(video));
                  break;
               case A5O_KEY_ESCAPE:
                  al_close_video(video);
                  goto done;
                  break;
               case A5O_KEY_LEFT:
                  incr = -10.0;
                  goto do_seek;
               case A5O_KEY_RIGHT:
                  incr = 10.0;
                  goto do_seek;
               case A5O_KEY_UP:
                  incr = 60.0;
                  goto do_seek;
               case A5O_KEY_DOWN:
                  incr = -60.0;
                  goto do_seek;

               do_seek:
                  al_seek_video(video, al_get_video_position(video, A5O_VIDEO_POSITION_ACTUAL) + incr);
                  break;

               case A5O_KEY_F:
                  fullscreen = !fullscreen;
                  al_set_display_flag(screen, A5O_FULLSCREEN_WINDOW,
                     fullscreen);
                  break;
               
               case A5O_KEY_1:
                  zoom = 1;
                  break;

               case A5O_KEY_S:
                  zoom = 0;
                  break;
               default:
                  break;
            }
            break;
         
         case A5O_EVENT_DISPLAY_RESIZE:
            al_acknowledge_resize(screen);
            al_clear_to_color(al_map_rgb(0, 0, 0));
            break;

         case A5O_EVENT_TIMER:
            /*
            display_time += 1.0 / 60;
            if (display_time >= video_time) {
               video_time = display_time + video_refresh_timer(is);
            }*/

            if (!use_frame_events) {
               redraw = true;
            }
            break;

         case A5O_EVENT_DISPLAY_CLOSE:
            al_close_video(video);
            goto done;
            break;

         case A5O_EVENT_VIDEO_FRAME_SHOW:
            if (use_frame_events) {
               redraw = true;
            }
            break;

         case A5O_EVENT_VIDEO_FINISHED:
            log_printf("video finished\n");
            break;
         default:
            break;
      }
   }
done:
   al_destroy_display(screen);
   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
