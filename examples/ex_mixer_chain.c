/*
 *    Example program for the Allegro library.
 *
 *    Test chaining mixers to mixers.
 */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

char *default_files[] = {NULL, "data/haiku/water_0.ogg",
   "data/haiku/water_7.ogg"};

int main(int argc, char **argv)
{
   A5O_VOICE *voice;
   A5O_MIXER *mixer;
   A5O_MIXER *submixer[2];
   A5O_SAMPLE_INSTANCE *sample[2];
   A5O_SAMPLE *sample_data[2];
   float sample_time;
   float max_sample_time;
   int i;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   if (argc < 3) {
      log_printf("This example can be run from the command line.\nUsage: %s file1 file2\n", argv[0]);
      argv = default_files;
      argc = 3;
   }

   al_init_acodec_addon();

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }

   voice = al_create_voice(44100, A5O_AUDIO_DEPTH_INT16,
      A5O_CHANNEL_CONF_2);
   if (!voice) {
      abort_example("Could not create A5O_VOICE.\n");
   }

   mixer = al_create_mixer(44100, A5O_AUDIO_DEPTH_FLOAT32,
      A5O_CHANNEL_CONF_2);
   submixer[0] = al_create_mixer(44100, A5O_AUDIO_DEPTH_FLOAT32,
      A5O_CHANNEL_CONF_2);
   submixer[1] = al_create_mixer(44100, A5O_AUDIO_DEPTH_FLOAT32,
      A5O_CHANNEL_CONF_2);
   if (!mixer || !submixer[0] || !submixer[1]) {
      abort_example("al_create_mixer failed.\n");
   }

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      abort_example("al_attach_mixer_to_voice failed.\n");
   }

   for (i = 0; i < 2; i++) {
      const char *filename = argv[i + 1];
      sample_data[i] = al_load_sample(filename);
      if (!sample_data[i]) {
         abort_example("Could not load sample from '%s'!\n", filename);
      }
      sample[i] = al_create_sample_instance(NULL);
      if (!sample[i]) {
         abort_example("al_create_sample failed.\n");
      }
      if (!al_set_sample(sample[i], sample_data[i])) {
         abort_example("al_set_sample_ptr failed.\n");
      }
      if (!al_attach_sample_instance_to_mixer(sample[i], submixer[i])) {
         abort_example("al_attach_sample_instance_to_mixer failed.\n");
      }
      if (!al_attach_mixer_to_mixer(submixer[i], mixer)) {
         abort_example("al_attach_mixer_to_mixer failed.\n");
      }
   }

   /* Play sample in looping mode. */
   for (i = 0; i < 2; i++) {
      al_set_sample_instance_playmode(sample[i], A5O_PLAYMODE_LOOP);
      al_play_sample_instance(sample[i]);
   }

   max_sample_time = al_get_sample_instance_time(sample[0]);
   sample_time = al_get_sample_instance_time(sample[1]);
   if (sample_time > max_sample_time)
      max_sample_time = sample_time;

   log_printf("Playing...");

   al_rest(max_sample_time);

   al_set_sample_instance_gain(sample[0], 0.5);
   al_rest(max_sample_time);

   al_set_sample_instance_gain(sample[1], 0.25);
   al_rest(max_sample_time);

   al_stop_sample_instance(sample[0]);
   al_stop_sample_instance(sample[1]);
   log_printf("Done\n");

   /* Free the memory allocated. */
   for (i = 0; i < 2; i++) {
      al_set_sample(sample[i], NULL);
      al_destroy_sample(sample_data[i]);
      al_destroy_sample_instance(sample[i]);
      al_destroy_mixer(submixer[i]);
   }
   al_destroy_mixer(mixer);
   al_destroy_voice(voice);

   al_uninstall_audio();

   close_log(true);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
