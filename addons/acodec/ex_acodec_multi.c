/*
 * Milan Mimica
 * Audio example that plays multiple files at the same time
 * Originlly derived from the ex_acodec example.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/acodec.h"

int main(int argc, char **argv)
{
   int i;
   ALLEGRO_SAMPLE **sample;
   ALLEGRO_MIXER *mixer;
   ALLEGRO_VOICE *voice;
   float longest_sample;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s {audio_files}\n", argv[0]);
      return 1;
   }

   if (!al_init())
   {
       fprintf(stderr, "Could not init allegro\n");
       return 1;
   }

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT))
   {
       fprintf(stderr, "Could not init sound!\n");
       return 1;
   }

   sample = malloc(argc * sizeof(*sample));
   if (!sample)
      return 1;

   /* a voice is used for playback */
   voice = al_voice_create(44100, ALLEGRO_AUDIO_16_BIT_INT, ALLEGRO_AUDIO_2_CH);
   if (!voice) {
      fprintf(stderr, "Could not create ALLEGRO_VOICE from sample\n");
      return 1;
   }

   mixer = al_mixer_create(44100, ALLEGRO_AUDIO_32_BIT_FLOAT, ALLEGRO_AUDIO_2_CH);
   if (!mixer) {
      fprintf(stderr, "al_mixer_create failed.\n");
      return 1;
   }

   if (al_voice_attach_mixer(voice, mixer) != 0) {
      TRACE("al_voice_attach_mixer failed.\n");
      return 1;
   }

   for (i = 1; i < argc; ++i)
   {
      const char*     filename = argv[i];
      float           sample_time = 0;

      /* loads the entire sound file from disk into sample data */
      sample[i] = al_load_sample(filename);
      if (!sample[i]) {
         fprintf(stderr, "Could not load ALLEGRO_SAMPLE from '%s'!\n", filename);
         continue;
      }

      if (al_mixer_attach_sample(mixer, sample[i]) != 0) {
         fprintf(stderr, "al_mixer_attach_sample failed.\n");
         continue;
      }
   }

   longest_sample = 0;

   for (i = 1; i < argc; ++i)
   {
      const char* filename = argv[i];
      float sample_time;

      /* play each sample once */
      al_sample_play(sample[i]);

      al_sample_get_float(sample[i], ALLEGRO_AUDIO_TIME, &sample_time);
      fprintf(stderr, "Playing '%s' (%.3f seconds)\n", filename, sample_time);

      if (sample_time > longest_sample)
         longest_sample = sample_time;
   }

   al_rest(longest_sample);

   for (i = 1; i < argc; ++i)
   {
      /* free the memory allocated when creating the sample + voice */
      if (sample[i]) {
         al_sample_stop(sample[i]);
         al_sample_destroy(sample[i]);
      }
   }
   al_mixer_destroy(mixer);
   al_voice_destroy(voice);

   free(sample);

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()
