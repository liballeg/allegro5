/*
 * Ryan Dickie
 * Audio example that loads a series of files and puts them through the mixer.
 * Originlly derived from the audio example on the wiki.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/audio.h"
#include "allegro5/acodec.h"

#define FREQUENCY 44000

int main(int argc, char **argv)
{
   int i;
   bool stream_it = false;
   ALLEGRO_MIXER* mixer;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s [--stream] {audio_files}\n", argv[0]);
      return 1;
   }

   if(strcasecmp(argv[1], "--stream") == 0)
   {
      ++argv;
      --argc;
      stream_it = true;
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

   for (i = 1; i < argc; ++i)
   {
      ALLEGRO_SAMPLE       *sample = NULL;
      ALLEGRO_VOICE        *voice = NULL;
      ALLEGRO_AUDIO_ENUM   chan_conf = 0;
      ALLEGRO_AUDIO_ENUM   depth_conf = 0;

      unsigned long        freq = 0;
      unsigned long        voice_freq = 0;
      const char*          filename = argv[i];
      float                sample_time = 0;

      /* 
       * loads the entire sound file from disk into sample
       * using al_load_stream for large files or streaming 
       * data
       */
      sample = al_load_sample(filename);
      if (!sample) {
         fprintf(stderr, "Could not load ALLEGRO_SAMPLE from '%s'!\n", filename);
         continue;
      }

      voice = al_voice_create(FREQUENCY, ALLEGRO_AUDIO_16_BIT_INT, ALLEGRO_AUDIO_2_CH);
      if(!voice) {
         fprintf(stderr, "Error creating voice!\n");
         al_sample_destroy(sample);
         continue;
      }

      mixer = al_mixer_create(FREQUENCY, ALLEGRO_AUDIO_32_BIT_FLOAT, ALLEGRO_AUDIO_2_CH);
      if(!mixer) {
         fprintf(stderr, "Error creating mixer!\n");
         al_audio_deinit();
         return 1;
      }

      if(al_voice_attach_mixer(voice, mixer))
      {
         fprintf(stderr, "Error attaching mixer to voice!\n");
         al_sample_destroy(sample);
         al_mixer_destroy(mixer);
         al_voice_destroy(voice);
         continue;
      }

      if(al_mixer_attach_sample(mixer, sample))
      {
         fprintf(stderr, "Error attaching sample to mixer!\n");
         al_sample_destroy(sample);
         al_mixer_destroy(mixer);
         al_voice_destroy(voice);
         continue;
      }
   
      al_sample_play(sample);

      /* play sample and wait for it to finish
       * al_sample_play returns immediately
       * once data is sent to sound driver
       */
      fprintf(stderr, "Playing '%s'", filename);
      al_sample_get_float(sample, ALLEGRO_AUDIO_TIME, &sample_time);
      fprintf(stderr, " (%.3f seconds) ", sample_time);
      al_rest(sample_time);
      
      fprintf(stderr, "\n");

      /* do this to deattach sample from voice */
      /* FIXME: mismatch in the API, attach/detach are diff */
      al_sample_set_bool(sample, ALLEGRO_AUDIO_ATTACHED, FALSE);

      /* free the memory allocated when creating the sample */
      al_sample_destroy(sample);
   }

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()
