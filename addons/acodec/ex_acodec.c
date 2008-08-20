/*
 * Ryan Dickie
 * Audio example that loads a series of files and puts them through the mixer.
 * Originlly derived from the audio example on the wiki.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/audio.h"
#include "allegro5/acodec.h"

int main(int argc, char **argv)
{
   int i;
   bool stream_it = false;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s {audio_files}\n", argv[0]);
      return 1;
   }

   if (al_init())
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
      voice = al_voice_create(sample);
      if (!voice) {
         fprintf(stderr, "Could not create ALLEGRO_VOICE from sample\n");
         continue;
      }

      /*play sample in looping mode*/
      al_voice_start(voice);
      al_voice_set_loop_mode(voice,TRUE);

      sample_time = al_sample_get_time(sample);
      fprintf(stderr, "Playing '%s' (%.3f seconds) 3 times", filename, sample_time);
      al_rest(sample_time*3.0f);
      fprintf(stderr, "\n");

      /* free the memory allocated when creating the sample + voice */
      al_voice_stop(voice);
      al_voice_destroy(voice);
      al_sample_destroy(sample);
   }

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()
