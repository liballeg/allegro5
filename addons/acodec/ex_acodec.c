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
   int i;
   bool stream_it = false;

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

   for (i = 1; i < argc; ++i)
   {
      ALLEGRO_SAMPLE* sample = NULL;
      ALLEGRO_VOICE*  voice = NULL;
      ALLEGRO_MIXER*  mixer = NULL;
      const char*     filename = argv[i];
      float           sample_time = 0;

      /* loads the entire sound file from disk into sample
       * using al_load_stream for large files or streaming 
       * data */
      sample = al_load_sample(filename);
      if (!sample) {
         fprintf(stderr, "Could not load ALLEGRO_SAMPLE from '%s'!\n", filename);
         continue;
      }

      /* A voice is used for playback. You give it a sample to play
       * you can give a sample to many voices if you want */
      voice = al_voice_create(44100, ALLEGRO_AUDIO_16_BIT_INT, ALLEGRO_AUDIO_2_CH);
      if (!voice) {
         fprintf(stderr, "Could not create ALLEGRO_VOICE from sample\n");
         continue;
      }

      mixer = al_mixer_create(44100, ALLEGRO_AUDIO_32_BIT_FLOAT, ALLEGRO_AUDIO_2_CH);
      if (!mixer) {
         fprintf(stderr, "al_mixer_create failed.\n");
         continue;
      }

      if (al_voice_attach_mixer(voice, mixer) != 0) {
         TRACE("al_voice_attach_mixer failed.\n");
         continue;
      }

      if (al_mixer_attach_sample(mixer, sample) != 0) {
         fprintf(stderr, "al_mixer_attach_sample failed.\n");
         continue;
      }

      /* play sample in looping mode */
      al_sample_set_enum(sample, ALLEGRO_AUDIO_LOOPMODE, ALLEGRO_AUDIO_ONE_DIR);
      al_sample_play(sample);

      al_sample_get_float(sample, ALLEGRO_AUDIO_TIME, &sample_time);
      fprintf(stderr, "Playing '%s' (%.3f seconds) 3 times", filename, sample_time);
      al_rest(sample_time*3.0f);
      al_sample_stop(sample);
      fprintf(stderr, "\n");

      /* free the memory allocated when creating the sample + voice */
      al_sample_destroy(sample);
      al_mixer_destroy(mixer);
      al_voice_destroy(voice);
   }

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()
