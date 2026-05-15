/*
 *    Example program for the Allegro library.
 *
 *    Demonstrate sample instance finished events.
 */

#define ALLEGRO_UNSTABLE
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

int main(int argc, char **argv)
{
   ALLEGRO_EVENT_QUEUE *event_queue;
   ALLEGRO_EVENT event;
   ALLEGRO_SAMPLE *sample;
   ALLEGRO_SAMPLE_INSTANCE *instance;
   const char *filename;
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   open_log();

   filename = (argc > 1) ? argv[1] : "data/welcome.wav";

   al_init_acodec_addon();
   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }
   al_reserve_samples(4);

   sample = al_load_sample(filename);
   if (!sample) {
      log_printf("Could not load sample from '%s'!\n", filename);
      abort_example("Sample load failed\n");
   }

   instance = al_create_sample_instance(sample);
   if (!instance) {
      abort_example("Could not create sample instance\n");
   }

   event_queue = al_create_event_queue();
   if (!event_queue) {
      abort_example("Could not create event queue\n");
   }

   al_register_event_source(event_queue, al_get_sample_instance_event_source(instance));

   if (!al_attach_sample_instance_to_mixer(instance, al_get_default_mixer())) {
      abort_example("Could not attach sample instance to mixer\n");
   }

   log_printf("Playing '%s' (%.2f seconds) 3 times\n", filename, al_get_sample_instance_time(instance));

   for (i = 0; i < 3; i++) {
      log_printf("Playing sample instance...\n");
      if (!al_play_sample_instance(instance)) {
         abort_example("Could not start sample instance\n");
      }
      
      if (!al_wait_for_event_timed(event_queue, &event, al_get_sample_instance_time(instance) + 2.0)) {
         abort_example("Event never fired!\n");
      } else {
         log_printf("finished!\n");
      }
   }

   al_destroy_sample_instance(instance);
   al_destroy_sample(sample);
   al_destroy_event_queue(event_queue);

   al_uninstall_audio();

   close_log(true);

   return 0;
}
