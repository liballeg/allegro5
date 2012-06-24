/*
 * Ryan Dickie
 * Audio example that loads a series of files and puts them through the mixer.
 * Originlly derived from the audio example on the wiki.
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
   ALLEGRO_SAMPLE_INSTANCE *sample;
   int i;
   char const **filenames;
   int n;

   if (argc < 2) {
      n = 1;
      filenames = malloc(sizeof *filenames);
      filenames[0] = "data/testing.ogg";
   }
   else {
      n = argc - 1;
      filenames = malloc(sizeof *filenames * n);
      for (i = 1; i < argc; ++i) {
         filenames[i - 1] = argv[i];
      }
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
   if (!mixer) {
      fprintf(stderr, "al_create_mixer failed.\n");
      return 1;
   }

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      fprintf(stderr, "al_attach_mixer_to_voice failed.\n");
      return 1;
   }

   sample = al_create_sample_instance(NULL);
   if (!sample) {
      fprintf(stderr, "al_create_sample failed.\n");
      return 1;
   }

   for (i = 0; i < n; ++i) {
      ALLEGRO_SAMPLE *sample_data = NULL;
      const char *filename = filenames[i];
      float sample_time = 0;

      /* Load the entire sound file from disk. */
      sample_data = al_load_sample(filename);
      if (!sample_data) {
         fprintf(stderr, "Could not load sample from '%s'!\n",
            filename);
         continue;
      }

      if (!al_set_sample(sample, sample_data)) {
         fprintf(stderr, "al_set_sample_instance_ptr failed.\n");
         continue;
      }

      if (!al_attach_sample_instance_to_mixer(sample, mixer)) {
         fprintf(stderr, "al_attach_sample_instance_to_mixer failed.\n");
         return 1;
      }

      /* Play sample in looping mode. */
      al_set_sample_instance_playmode(sample, ALLEGRO_PLAYMODE_LOOP);
      al_play_sample_instance(sample);

      sample_time = al_get_sample_instance_time(sample);
      fprintf(stderr, "Playing '%s' (%.3f seconds) 3 times", filename,
         sample_time);

      al_rest(sample_time);

      if (!al_set_sample_instance_gain(sample, 0.5)) {
         fprintf(stderr, "Failed to set gain.\n");
      }
      al_rest(sample_time);

      if (!al_set_sample_instance_gain(sample, 0.25)) {
         fprintf(stderr, "Failed to set gain.\n");
      }
      al_rest(sample_time);

      al_stop_sample_instance(sample);
      fprintf(stderr, "\n");

      /* Free the memory allocated. */
      al_set_sample(sample, NULL);
      al_destroy_sample(sample_data);
   }

   al_destroy_sample_instance(sample);
   al_destroy_mixer(mixer);
   al_destroy_voice(voice);

   al_uninstall_audio();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
