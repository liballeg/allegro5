/*
 *    Example program for the Allegro library.
 *
 *    Demonstrate 'simple' audio interface.
 */

#define A5O_UNSTABLE
#include <math.h>
#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

#define RESERVED_SAMPLES   16
#define MAX_SAMPLE_DATA    10

const char *default_files[] = {NULL, "data/welcome.wav",
         "data/haiku/fire_0.ogg", "data/haiku/fire_1.ogg",
         "data/haiku/fire_2.ogg", "data/haiku/fire_3.ogg",
         "data/haiku/fire_4.ogg", "data/haiku/fire_5.ogg",
         "data/haiku/fire_6.ogg", "data/haiku/fire_7.ogg",
         };

int main(int argc, const char *argv[])
{
   A5O_SAMPLE *sample_data[MAX_SAMPLE_DATA] = {NULL};
   A5O_DISPLAY *display = NULL;
   A5O_EVENT_QUEUE *event_queue;
   A5O_EVENT event;
   A5O_SAMPLE_ID sample_id;
   bool sample_id_valid = false;
   A5O_TIMER* timer;
   A5O_FONT* font;
   A5O_AUDIO_STREAM *music = NULL;
   int i;
   bool panning = false;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   if (!al_init_font_addon()) {
      abort_example("Could not init the font addon.\n");
   }

   open_log();

   if (argc < 2) {
      log_printf("This example can be run from the command line.\nUsage: %s {audio_files}\n", argv[0]);
      argv = default_files;
      argc = 10;
   }
   argc--;
   argv++;

   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display\n");
   }
   font = al_create_builtin_font();

   timer = al_create_timer(1/60.0);
   al_start_timer(timer);
   event_queue = al_create_event_queue();
   al_register_event_source(event_queue, al_get_keyboard_event_source());
   al_register_event_source(event_queue, al_get_display_event_source(display));
   al_register_event_source(event_queue, al_get_timer_event_source(timer));

   al_init_acodec_addon();

Restart:

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }

   if (!al_reserve_samples(RESERVED_SAMPLES)) {
      abort_example("Could not set up voice and mixer.\n");
   }

   memset(sample_data, 0, sizeof(sample_data));
   for (i = 0; i < argc && i < MAX_SAMPLE_DATA; i++) {
      const char *filename = argv[i];

      /* Load the entire sound file from disk. */
      sample_data[i] = al_load_sample(argv[i]);
      if (!sample_data[i]) {
         log_printf("Could not load sample from '%s'!\n", filename);
         continue;
      }
   }

   log_printf(
      "Press digits to play sounds.\n"
      "Space to stop sounds.\n"
      "Add Alt to play sounds repeatedly.\n"
      "'p' to pan the last played sound.\n"
      "Escape to quit.\n");

   while (true) {
      al_wait_for_event(event_queue, &event);
      if (event.type == A5O_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == A5O_KEY_ESCAPE) {
            break;
         }
         if (event.keyboard.unichar == ' ') {
            log_printf("Stopping all sounds\n");
            al_stop_samples();
            if (music) {
               al_set_audio_stream_playing(music, false);
            }
         }

         if (event.keyboard.keycode >= A5O_KEY_0 && event.keyboard.keycode <= A5O_KEY_9) {
            bool loop = event.keyboard.modifiers & A5O_KEYMOD_ALT;
            bool bidir = event.keyboard.modifiers & A5O_KEYMOD_CTRL;
            bool reversed = event.keyboard.modifiers & A5O_KEYMOD_SHIFT;
            i = (event.keyboard.keycode - A5O_KEY_0 + 9) % 10;
            if (sample_data[i]) {
               A5O_SAMPLE_ID new_sample_id;
               A5O_PLAYMODE playmode;
               const char* playmode_str;
               if (loop) {
                  playmode = A5O_PLAYMODE_LOOP;
                  playmode_str = "on a loop";
               }
               else if (bidir) {
                  playmode = A5O_PLAYMODE_BIDIR;
                  playmode_str = "on a bidirectional loop";
               }
               else {
                  playmode = A5O_PLAYMODE_ONCE;
                  playmode_str = "once";
               }
               bool ret = al_play_sample(sample_data[i], 1.0, 0.0, 1.0,
                  playmode, &new_sample_id);
               if (reversed) {
                  A5O_SAMPLE_INSTANCE* inst = al_lock_sample_id(&new_sample_id);
                  al_set_sample_instance_position(inst, al_get_sample_instance_length(inst) - 1);
                  al_set_sample_instance_speed(inst, -1);
                  al_unlock_sample_id(&new_sample_id);
               }
               if (ret) {
                  log_printf("Playing %d %s\n", i, playmode_str);
               }
               else {
                  log_printf(
                     "al_play_sample_data failed, perhaps too many sounds\n");
               }
               if (!panning) {
                  if (ret) {
                     sample_id = new_sample_id;
                     sample_id_valid = true;
                  }
               }
            }
         }

         if (event.keyboard.unichar == 'p') {
            if (sample_id_valid) {
               panning = !panning;
               if (panning) {
                  log_printf("Panning\n");
               }
               else {
                  log_printf("Not panning\n");
               }
            }
         }

         if (event.keyboard.unichar == 'm') {
            music = al_play_audio_stream("../demos/cosmic_protector/data/sfx/title_music.ogg");
         }

         /* Hidden feature: restart audio subsystem.
          * For debugging race conditions on shutting down the audio.
          */
         if (event.keyboard.unichar == 'r') {
            for (i = 0; i < argc && i < MAX_SAMPLE_DATA; i++) {
               if (sample_data[i])
                  al_destroy_sample(sample_data[i]);
            }
            al_uninstall_audio();
            goto Restart;
         }
      }
      else if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == A5O_EVENT_TIMER) {
         int y = 12;
         int dy = 12;
         if (panning && sample_id_valid) {
            A5O_SAMPLE_INSTANCE* instance = al_lock_sample_id(&sample_id);
            if (instance) {
               al_set_sample_instance_pan(instance, sin(al_get_time()));
            }
            al_unlock_sample_id(&sample_id);
         }
         al_clear_to_color(al_map_rgb_f(0., 0., 0.));
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "CONTROLS");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "1-9 - play the sounds");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "SPACE - stop all sounds");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "Ctrl 1-9 - play sounds with bidirectional looping");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "Alt 1-9 - play sounds with regular looping");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "Shift 1-9 - play sounds reversed");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "p - pan the last played sound");
         y += dy;
         al_draw_text(font, al_map_rgb_f(1., 0.5, 0.5), 12, y,
            A5O_ALIGN_LEFT, "m - play music");
         y += 2 * dy;
         al_draw_text(font, al_map_rgb_f(0.5, 1., 0.5), 12, y,
            A5O_ALIGN_LEFT, "SOUNDS");
         y += dy;
         for (i = 0; i < argc && i < MAX_SAMPLE_DATA; i++) {
            al_draw_textf(font, al_map_rgb_f(0.5, 1., 0.5), 12, y,
               A5O_ALIGN_LEFT, "%d - %s", i + 1, argv[i]);
            y += dy;
         }
         al_flip_display();
      }
   }

   for (i = 0; i < argc && i < MAX_SAMPLE_DATA; i++) {
      if (sample_data[i])
         al_destroy_sample(sample_data[i]);
   }

   /* Sample data and other objects will be automatically freed. */
   al_uninstall_audio();

   al_destroy_display(display);
   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
