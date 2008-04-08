/*
 * This is the allegro audio example from the wiki.
 *
 * Massively stripped down to use as a test case
 * for the new audio codecs (Ryan Dickie)
 */


#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/audio.h"
#include "allegro5/acodec.h"


#define FREQUENCY 44000


void display_driver_info();
void display_voice_info(ALLEGRO_VOICE *voice);
void display_mixer_info(ALLEGRO_MIXER *mixer);
void stream_file(const char *filename);

/* Function that allegro usually provides. */
ALLEGRO_SAMPLE* allegro_load_sample(const char *filename);


ALLEGRO_AUDIO_ENUM chan, depth;
unsigned long freq;

int main(int argc, char **argv)
{
   int x;
   bool stream_it = false;

   if(argc < 2) {
      fprintf(stderr, "Usage: %s [-stream] <sample.wav/ogg/flac/aiff>...\n", argv[0]);
      return 1;
   }

   if(strcasecmp(argv[1], "-stream") == 0)
   {
      ++argv;
      --argc;
      stream_it = true;
   }

   if (allegro_init())
   {
       fprintf(stderr, "Could not init allegro\n");
   }

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT))
   {
       fprintf(stderr, "Could not init sound!\n");
       return 1;
   }

   for (x = 1; x < argc; ++x)
   {
      ALLEGRO_SAMPLE       *sample = NULL;
      ALLEGRO_VOICE        *voice = NULL;
      ALLEGRO_AUDIO_ENUM   chan_conf = 0;
      ALLEGRO_AUDIO_ENUM   depth_conf = 0;

      unsigned long        freq = 0;
      unsigned long        voice_freq = 0;
      const char*          filename = argv[x];
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
   
      al_sample_get_long(sample, ALLEGRO_AUDIO_FREQUENCY, &freq);
      al_sample_get_enum(sample, ALLEGRO_AUDIO_CHANNELS, &chan_conf);
      al_sample_get_enum(sample, ALLEGRO_AUDIO_DEPTH, &depth_conf);
   
      /* am not using the mixer so settings are required to match */
      voice = al_voice_create(freq, depth_conf, chan_conf, ALLEGRO_AUDIO_REQUIRE);
      if(!voice)
      {
         al_sample_destroy(sample);
         fprintf(stderr, "Error creating voice!\n");
         continue;
      }
   
      if(al_voice_attach_sample(voice, sample))
      {
         al_sample_destroy(sample);
         al_voice_destroy(voice);
         fprintf(stderr, "Error attaching sample to voice!\n");
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
      al_sample_set_bool(sample, ALLEGRO_AUDIO_ATTACHED, 0);

      /* free the memory allocated when creating the sample */
      al_sample_destroy(sample);
   }

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()
