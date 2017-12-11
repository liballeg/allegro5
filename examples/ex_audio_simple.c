/*
 *    Example program for the Allegro library.
 *
 *    Demonstrate 'simple' audio interface.
 */

#define ALLEGRO_UNSTABLE
#include <math.h>
#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

#define RESERVED_SAMPLES   16
#define MAX_SAMPLE_DATA    10

int main(int argc, const char *argv[])
{
   ALLEGRO_SAMPLE *sample_data[MAX_SAMPLE_DATA] = {NULL};
   ALLEGRO_DISPLAY *display = NULL;
   ALLEGRO_EVENT_QUEUE *event_queue;
   ALLEGRO_EVENT event;
   ALLEGRO_SAMPLE_ID sample_id;
   bool sample_id_valid = false;
   ALLEGRO_TIMER* timer;
   int i;
   bool panning = false;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (argc < 2) {
      log_printf("This example needs to be run from the command line.\nUsage: %s {audio_files}\n", argv[0]);
      goto done;
   }
   argc--;
   argv++;

   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display\n");
   }

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
      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
         }
         if (event.keyboard.unichar == ' ') {
            log_printf("Stopping all sounds\n");
            al_stop_samples();
         }

         if (event.keyboard.unichar >= '0' && event.keyboard.unichar <= '9') {
            bool loop = event.keyboard.modifiers & ALLEGRO_KEYMOD_ALT;
            i = (event.keyboard.unichar - '0' + 9) % 10;
            if (sample_data[i]) {
               ALLEGRO_SAMPLE_ID new_sample_id;
               bool ret = al_play_sample(sample_data[i], 1.0, 0.0, 1.0,
                  loop ? ALLEGRO_PLAYMODE_LOOP : ALLEGRO_PLAYMODE_ONCE, &new_sample_id);
               if (ret) {
                  log_printf("Playing %d %s\n", i, loop ? "repeatedly" : "once");
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
      else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (event.type == ALLEGRO_EVENT_TIMER) {
         if (panning && sample_id_valid) {
            ALLEGRO_SAMPLE_INSTANCE* instance = al_lock_sample_id(&sample_id);
            if (instance) {
               al_set_sample_instance_pan(instance, sin(al_get_time()));
            }
            al_unlock_sample_id(&sample_id);
         }
      }
   }

   for (i = 0; i < argc && i < MAX_SAMPLE_DATA; i++) {
      if (sample_data[i])
         al_destroy_sample(sample_data[i]);
   }

   /* Sample data and other objects will be automatically freed. */
   al_uninstall_audio();

done:
   al_destroy_display(display);
   close_log(false);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
