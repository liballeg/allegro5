/*
 * Ryan Dickie
 * Audio example that loads a series of files and puts them through the mixer.
 * Originlly derived from the audio example on the wiki.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/acodec.h"

int main(int argc, char **argv)
{
   ALLEGRO_VOICE *voice;
   ALLEGRO_MIXER *mixer;
   int i;

   if (argc < 2) {
      fprintf(stderr, "Usage: %s {audio_files}\n", argv[0]);
      return 1;
   }

   if (!al_init()) {
      fprintf(stderr, "Could not init allegro\n");
      return 1;
   }

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT)) {
      fprintf(stderr, "Could not init sound!\n");
      return 1;
   }

   voice = al_voice_create(44100, ALLEGRO_AUDIO_DEPTH_INT16,
      ALLEGRO_CHANNEL_CONF_2);
   if (!voice) {
      fprintf(stderr, "Could not create ALLEGRO_VOICE.\n");
      return 1;
   }

   mixer = al_mixer_create(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   if (!mixer) {
      fprintf(stderr, "al_mixer_create failed.\n");
      return 1;
   }

   if (al_voice_attach_mixer(voice, mixer) != 0) {
      TRACE("al_voice_attach_mixer failed.\n");
      return 1;
   }

   for (i = 1; i < argc; ++i) {
      ALLEGRO_SAMPLE_DATA *sample_data = NULL;
      ALLEGRO_SAMPLE *sample = NULL;
      const char *filename = argv[i];
      float sample_time = 0;

      /* Load the entire sound file from disk. */
      sample_data = al_load_sample(filename);
      if (!sample_data) {
         fprintf(stderr, "Could not load sample from '%s'!\n",
            filename);
         continue;
      }

      sample = al_sample_create(sample_data);
      if (!sample) {
         fprintf(stderr, "al_sample_create failed.\n");
         continue;
      }

      if (al_mixer_attach_sample(mixer, sample) != 0) {
         fprintf(stderr, "al_mixer_attach_sample failed.\n");
         continue;
      }

      /* Play sample in looping mode. */
      al_sample_set_enum(sample, ALLEGRO_AUDIOPROP_LOOPMODE,
         ALLEGRO_PLAYMODE_ONEDIR);
      al_sample_play(sample);

      al_sample_get_float(sample, ALLEGRO_AUDIOPROP_TIME, &sample_time);
      fprintf(stderr, "Playing '%s' (%.3f seconds) 3 times", filename,
         sample_time);
      al_rest(sample_time * 3.0f);
      al_sample_stop(sample);
      fprintf(stderr, "\n");

      /* Free the memory allocated. */
      al_sample_destroy(sample);
      al_sample_data_destroy(sample_data);
   }

   al_mixer_destroy(mixer);
   al_voice_destroy(voice);

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
