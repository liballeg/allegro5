/*
 * Ryan Dickie
 * Audio example that loads a series of files and puts them through the mixer.
 * Originally derived from the audio example on the wiki.
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
      ALLEGRO_STREAM* stream = NULL;
      ALLEGRO_VOICE*  voice = NULL;
      const char*     filename = argv[i];
      float           sample_time = 0;

      /* loads the entire sound file from disk into sample
       * using al_load_stream for large files or streaming 
       * data */
      stream = al_load_stream(filename);
      if (!stream) {
         fprintf(stderr, "Could not load ALLEGRO_STREAM from '%s'!\n", filename);
         continue;
      }
      fprintf(stderr, "loaded stream\n");

      /* A voice is used for playback. */
      voice = al_voice_create_stream(stream);
      if (!voice) {
         fprintf(stderr, "Could not create ALLEGRO_VOICE from sample\n");
         continue;
      }
      fprintf(stderr,"created voice\n");

      /*play sample in looping mode*/
      al_voice_start(voice);

      fprintf(stderr, "Playing %s .. Waiting for stream to finish ", filename);
      while (!al_stream_is_dry(stream))
      {
         /* yield the CPU to conserve power + allow smooth playback */
         al_rest(0.100);
      }
      fprintf(stderr, "\n");

      /* free the memory allocated when creating the sample + voice */
      al_voice_stop(voice);
      fprintf(stderr, "Stopped voice\n");
      al_voice_destroy(voice);
      fprintf(stderr, "destroyed voice\n");
      al_stream_destroy(stream);
      fprintf(stderr, "destroyed stream\n");
   }

   al_audio_deinit();
   fprintf(stderr,"Deinitiated audio");

   return 0;
}
END_OF_MAIN()
