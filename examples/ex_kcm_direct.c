/* Shows the ability to play a sample without a mixer. */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include "common.c"

int main(int argc, char **argv)
{
   ALLEGRO_VOICE *voice;
   ALLEGRO_SAMPLE_INSTANCE *sample;
   int i;

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

   for (i = 1; i < argc; ++i) {
      ALLEGRO_SAMPLE *sample_data = NULL;
      const char *filename = argv[i];
      ALLEGRO_CHANNEL_CONF chan;
      ALLEGRO_AUDIO_DEPTH depth;
      unsigned long freq;
      float sample_time = 0;

      /* Load the entire sound file from disk. */
      sample_data = al_load_sample(filename);
      if (!sample_data) {
         fprintf(stderr, "Could not load sample from '%s'!\n",
            filename);
         continue;
      }

      sample = al_create_sample_instance(NULL);
      if (!sample) {
         fprintf(stderr, "al_create_sample failed.\n");
        return 1;
      }

      if (!al_set_sample(sample, sample_data)) {
         fprintf(stderr, "al_set_sample failed.\n");
         continue;
      }

      depth = al_get_sample_instance_depth(sample);
      chan = al_get_sample_instance_channels(sample);
      freq = al_get_sample_instance_frequency(sample);
      fprintf(stderr, "Loaded sample: %i-bit depth, %i channels, %li Hz\n",
         (depth < 8) ? (8+depth*8) : 0, (chan>>4)+(chan%0xF), freq);
      fprintf(stderr, "Trying to create a voice with the same specs... ");
      voice = al_create_voice(freq, depth, chan);
      if (!voice) {
         fprintf(stderr, "Could not create ALLEGRO_VOICE.\n");
         return 1;
      }
      fprintf(stderr, "done.\n");

      if (!al_attach_sample_instance_to_voice(sample, voice)) {
         fprintf(stderr, "al_attach_sample_instance_to_voice failed.\n");
         return 1;
      }

      /* Play sample in looping mode. */
      al_set_sample_instance_playmode(sample, ALLEGRO_PLAYMODE_LOOP);
      al_play_sample_instance(sample);

      sample_time = al_get_sample_instance_time(sample);
      fprintf(stderr, "Playing '%s' (%.3f seconds) 3 times", filename,
         sample_time);

      al_rest(sample_time * 3);

      al_stop_sample_instance(sample);
      fprintf(stderr, "\n");

      /* Free the memory allocated. */
      al_set_sample(sample, NULL);
      al_destroy_sample(sample_data);
      al_destroy_sample_instance(sample);
      al_destroy_voice(voice);
   }

   al_uninstall_audio();

   return 0;
}

/* vim: set sts=3 sw=3 et: */
