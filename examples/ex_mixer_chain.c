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

int main(int argc, char **argv)
{
   ALLEGRO_VOICE *voice;
   ALLEGRO_MIXER *mixer;
   ALLEGRO_MIXER *submixer[2];
   ALLEGRO_SAMPLE_INSTANCE *sample[2];
   ALLEGRO_SAMPLE *sample_data[2];
   float sample_time;
   float max_sample_time;
   int i;

   if (argc < 3) {
      printf("Usage: %s file1 file2\n", argv[0]);
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

   voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
      ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create ALLEGRO_VOICE.\n");
      return 1;
   }

   mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   submixer[0] = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   submixer[1] = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   if (!mixer || !submixer[0] || !submixer[1]) {
      fprintf(stderr, "al_create_mixer failed.\n");
      return 1;
   }

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      fprintf(stderr, "al_attach_mixer_to_voice failed.\n");
      return 1;
   }

   for (i = 0; i < 2; i++) {
      const char *filename = argv[i + 1];
      sample_data[i] = al_load_sample(filename);
      if (!sample_data[i]) {
         fprintf(stderr, "Could not load sample from '%s'!\n", filename);
         return 1;
      }
      sample[i] = al_create_sample_instance(NULL);
      if (!sample[i]) {
         fprintf(stderr, "al_create_sample failed.\n");
         return 1;
      }
      if (!al_set_sample(sample[i], sample_data[i])) {
         fprintf(stderr, "al_set_sample_ptr failed.\n");
         return 1;
      }
      if (!al_attach_sample_instance_to_mixer(sample[i], submixer[i])) {
         fprintf(stderr, "al_attach_sample_instance_to_mixer failed.\n");
         return 1;
      }
      if (!al_attach_mixer_to_mixer(submixer[i], mixer)) {
         fprintf(stderr, "al_attach_mixer_to_mixer failed.\n");
         return 1;
      }
   }

   /* Play sample in looping mode. */
   for (i = 0; i < 2; i++) {
      al_set_sample_instance_playmode(sample[i], ALLEGRO_PLAYMODE_LOOP);
      al_play_sample_instance(sample[i]);
   }

   max_sample_time = al_get_sample_instance_time(sample[0]);
   sample_time = al_get_sample_instance_time(sample[1]);
   if (sample_time > max_sample_time)
      max_sample_time = sample_time;

   printf("Playing...");
   fflush(stdout);

   al_rest(max_sample_time);

   al_set_sample_instance_gain(sample[0], 0.5);
   al_rest(max_sample_time);

   al_set_sample_instance_gain(sample[1], 0.25);
   al_rest(max_sample_time);

   al_stop_sample_instance(sample[0]);
   al_stop_sample_instance(sample[1]);
   printf("\n");

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

   return 0;
}

/* vim: set sts=3 sw=3 et: */
