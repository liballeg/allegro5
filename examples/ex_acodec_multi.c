/*
 * Milan Mimica
 * Audio example that plays multiple files at the same time
 * Originlly derived from the ex_acodec example.
 */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

int main(int argc, char **argv)
{
   int i;
   ALLEGRO_SAMPLE **sample_data;
   ALLEGRO_SAMPLE_INSTANCE **sample;
   ALLEGRO_MIXER *mixer;
   ALLEGRO_VOICE *voice;
   float longest_sample;

   if (argc < 2) {
      fprintf(stderr, "Usage: %s {audio_files}\n", argv[0]);
      return 1;
   }

   if (!al_init()) {
       fprintf(stderr, "Could not init Allegro.\n");
       return 1;
   }

   al_init_acodec_addon();

   if (!al_install_audio()) {
       fprintf(stderr, "Could not init sound!\n");
       return 1;
   }

   sample = malloc(argc * sizeof(*sample));
   if (!sample)
      return 1;

   sample_data = malloc(argc * sizeof(*sample_data));
   if (!sample_data)
      return 1;

   /* a voice is used for playback */
   voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
      ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create ALLEGRO_VOICE from sample\n");
      return 1;
   }

   mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      fprintf(stderr, "al_create_mixer failed.\n");
      return 1;
   }

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      abort_example("al_attach_mixer_to_voice failed.\n");
      return 1;
   }

   for (i = 1; i < argc; ++i) {
      const char *filename = argv[i];

      sample[i] = NULL;

      /* loads the entire sound file from disk into sample data */
      sample_data[i] = al_load_sample(filename);
      if (!sample_data[i]) {
         fprintf(stderr, "Could not load sample from '%s'!\n", filename);
         continue;
      }

      sample[i] = al_create_sample_instance(sample_data[i]);
      if (!sample[i]) {
         fprintf(stderr, "Could not create sample!\n");
	 al_destroy_sample(sample_data[i]);
	 sample_data[i] = NULL;
	 continue;
      }

      if (!al_attach_sample_instance_to_mixer(sample[i], mixer)) {
         fprintf(stderr, "al_attach_sample_instance_to_mixer failed.\n");
         continue;
      }
   }

   longest_sample = 0;

   for (i = 1; i < argc; ++i) {
      const char *filename = argv[i];
      float sample_time;

      if (!sample[i])
	 continue;

      /* play each sample once */
      al_play_sample_instance(sample[i]);

      sample_time = al_get_sample_instance_time(sample[i]);
      fprintf(stderr, "Playing '%s' (%.3f seconds)\n", filename, sample_time);

      if (sample_time > longest_sample)
         longest_sample = sample_time;
   }

   al_rest(longest_sample);

   for (i = 1; i < argc; ++i) {
      /* free the memory allocated when creating the sample + voice */
      if (sample[i]) {
         al_stop_sample_instance(sample[i]);
         al_destroy_sample_instance(sample[i]);
	 al_destroy_sample(sample_data[i]);
      }
   }
   al_destroy_mixer(mixer);
   al_destroy_voice(voice);

   free(sample);
   free(sample_data);

   al_uninstall_audio();

   return 0;
}
