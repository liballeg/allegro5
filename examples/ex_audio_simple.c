/*
 *    Example program for the Allegro library.
 *
 *    Demonstrate 'simple' audio interface.
 */

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
   int i;

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

   event_queue = al_create_event_queue();
   al_register_event_source(event_queue, al_get_keyboard_event_source());

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

   log_printf("Press digits to play sounds, space to stop sounds, "
      "Escape to quit.\n");

   while (true) {
      al_wait_for_event(event_queue, &event);
      if (event.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
         }
         if (event.keyboard.unichar == ' ') {
            al_stop_samples();
         }

         if (event.keyboard.unichar >= '0' && event.keyboard.unichar <= '9') {
            i = (event.keyboard.unichar - '0' + 9) % 10;
            if (sample_data[i]) {
               log_printf("Playing %d\n",i);
               if (!al_play_sample(sample_data[i], 1.0, 0.5, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL)) {
                  log_printf(
                     "al_play_sample_data failed, perhaps too many sounds\n");
               }
            }
         }

         /* Hidden feature: restart audio subsystem.
          * For debugging race conditions on shutting down the audio.
          */
         if (event.keyboard.unichar == 'r') {
            al_uninstall_audio();
            goto Restart;
         }
      }
   }

   /* Sample data and other objects will be automatically freed. */
   al_uninstall_audio();

done:
   al_destroy_display(display);
   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
