/*
 * Milan Mimica
 * Audio example that plays multiple files at the same time
 * Originlly derived from the ex_acodec example.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/audio.h"
#include "allegro5/acodec.h"

int main(int argc, char **argv)
{
   int i;
   ALLEGRO_SAMPLE** sample;
   ALLEGRO_VOICE**  voice;
   float longest_sample;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s {audio_files}\n", argv[0]);
      return 1;
   }

   if (allegro_init())
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

   voice  = malloc(argc * sizeof(*voice));
   if (!voice)
      return 1;

   for (i = 1; i < argc; ++i)
   {
      const char*     filename = argv[i];
      float           sample_time = 0;

      /* loads the entire sound file from disk into sample data */
      sample[i] = al_load_sample(filename);
      if (!sample[i]) {
         voice[i] = NULL;
         fprintf(stderr, "Could not load ALLEGRO_SAMPLE from '%s'!\n", filename);
         continue;
      }

      /* a voice is used for playback */
      voice[i] = al_voice_create(sample[i]);
      if (!voice[i]) {
         fprintf(stderr, "Could not create ALLEGRO_VOICE from sample\n");
         continue;
      }
   }

   longest_sample = 0;

   for (i = 1; i < argc; ++i)
   {
      const char* filename = argv[i];
      float sample_time;

      if (!voice[i])
         continue;

      /* play each sample once */
      al_voice_start(voice[i]);

      sample_time = al_sample_get_time(sample[i]);
      fprintf(stderr, "Playing '%s' (%.3f seconds)\n", filename, sample_time);

      if (sample_time > longest_sample)
         longest_sample = sample_time;
   }

   al_rest(longest_sample);

   for (i = 1; i < argc; ++i)
   {
      /* free the memory allocated when creating the sample + voice */
      if (voice[i]) {
         al_voice_stop(voice[i]);
         al_voice_destroy(voice[i]);
      }

      if (sample[i])
         al_sample_destroy(sample[i]);
   }
   free(sample);
   free(voice);

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()
