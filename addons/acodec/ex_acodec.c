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

   allegro_init();

   /* sets audio drivers to prefered defaults */

   al_audio_set_enum(ALLEGRO_AUDIO_DEPTH, ALLEGRO_AUDIO_16_BIT_INT);
   al_audio_set_enum(ALLEGRO_AUDIO_CHANNELS, ALLEGRO_AUDIO_2_CH);
   al_audio_set_long(ALLEGRO_AUDIO_FREQUENCY, FREQUENCY);  

   if (al_audio_init(ALLEGRO_AUDIO_DRIVER_AUTODETECT))
   {
       fprintf(stderr, "Could not init sound!\n");
       return 1;
   }

   /* Find out what i actually got after initting */
   al_audio_get_enum(ALLEGRO_AUDIO_DEPTH, &depth);
   al_audio_get_enum(ALLEGRO_AUDIO_CHANNELS, &chan);
   al_audio_get_long(ALLEGRO_AUDIO_FREQUENCY, &freq);

   display_driver_info();

   for (x = 1; x < argc; ++x) {
      ALLEGRO_SAMPLE       *sample = NULL;
      ALLEGRO_VOICE        *voice = NULL;
      ALLEGRO_MIXER        *mixer = NULL;
      ALLEGRO_AUDIO_ENUM   chan_conf = 0;
      unsigned long        voice_freq = 0;
      const char*          filename = argv[x];
      float                sample_time = 0;

      sample = al_load_sample(filename);
      if (!sample) {
         fprintf(stderr, "Could not create ALLEGRO_SAMPLE from '%s'!\n", filename);
         continue;
      }

      /*TODO: find out why voice params don't actually match up with driver params */
      //voice = al_voice_create(freq, depth, chan, 0);
      voice = al_voice_create(FREQUENCY, ALLEGRO_AUDIO_16_BIT_INT, ALLEGRO_AUDIO_2_CH, 0);
      if(!voice) {
         fprintf(stderr, "Error allocating voice!\n");
         goto getout;
      }
      display_voice_info(voice);
      al_voice_get_long(voice, ALLEGRO_AUDIO_FREQUENCY, &voice_freq);
      al_voice_get_enum(voice, ALLEGRO_AUDIO_CHANNELS, &chan_conf);

      /* TODO: why is this 32-bit_float, why won't it work with others??? */
      mixer = al_mixer_create(voice_freq, ALLEGRO_AUDIO_32_BIT_FLOAT, chan_conf);
      if(!mixer) {
         fprintf(stderr, "Error allocating mixer!\n");
         goto getout;
      }

      if(al_voice_attach_mixer(voice, mixer) != 0)
      {
         fprintf(stderr, "Error attaching mixer to voice!\n");
         goto getout;
      }

      if(al_mixer_attach_sample(mixer, sample) != 0)
      {
         fprintf(stderr, "Error attaching sample to mixer!\n");
         goto getout;
      }

      printf("mixer info:\n");
      display_mixer_info(mixer);
/*
      if(al_voice_attach_sample(voice, sample) != 0)
      {
         fprintf(stderr, "Error attaching sample to voice!\n");
         goto getout;
      }
*/
      fprintf(stderr, "Playing '%s'", filename);

      /* TODO: create sample info printer */
      al_sample_play(sample);
      al_sample_get_float(sample, ALLEGRO_AUDIO_TIME, &sample_time);
      fprintf(stderr, " (%.3f seconds) ", sample_time);
      al_rest(sample_time);
      fprintf(stderr, "\n");

      al_sample_set_bool(sample, ALLEGRO_AUDIO_ATTACHED, 0);
      al_mixer_set_bool(mixer, ALLEGRO_AUDIO_ATTACHED, 0);

getout:
      al_mixer_destroy(mixer);
      al_voice_destroy(voice);
      al_sample_destroy(sample);
   }

   al_audio_deinit();

   return 0;
}
END_OF_MAIN()


void display_driver_info()
{
   const void *devname;
   unsigned long freq;
   ALLEGRO_AUDIO_ENUM val;

   if(al_audio_get_ptr(ALLEGRO_AUDIO_DEVICE, &devname) == 0)
      fprintf(stderr, "Output driver: %s\n", (const char*)devname);
   else
      fprintf(stderr, "Could not get driver name!\n");

   if(al_audio_get_long(ALLEGRO_AUDIO_FREQUENCY, &freq) == 0)
      fprintf(stderr, "Output frequency: %lu\n", freq);
   else
      fprintf(stderr, "Could not get output frequency!\n");

   if(al_audio_get_enum(ALLEGRO_AUDIO_CHANNELS, &val) == 0)
      fprintf(stderr, "Output on %d.%d channels.\n", val>>4, val&0xF);
   else
      fprintf(stderr, "Could not get channel config!\n");

   fprintf(stderr, "\n");
}


void display_voice_info(ALLEGRO_VOICE *voice)
{
   unsigned long voice_freq;
   ALLEGRO_AUDIO_ENUM chan_conf;
   ALLEGRO_AUDIO_ENUM depth;

   al_voice_get_long(voice, ALLEGRO_AUDIO_FREQUENCY, &voice_freq);
   al_voice_get_enum(voice, ALLEGRO_AUDIO_CHANNELS, &chan_conf);
   al_voice_get_enum(voice, ALLEGRO_AUDIO_DEPTH, &depth);

   fprintf(stderr, "Got voice: %s, %lu hz, %d channels\n",
         ((depth == ALLEGRO_AUDIO_8_BIT_INT) ? "8-bit signed" :
          ((depth == ALLEGRO_AUDIO_8_BIT_UINT) ? "8-bit unsigned" :
           ((depth == ALLEGRO_AUDIO_16_BIT_INT) ? "16-bit signed" :
            ((depth == ALLEGRO_AUDIO_16_BIT_UINT) ? "16-bit unsigned" :
            ((depth == ALLEGRO_AUDIO_24_BIT_INT) ? "24-bit signed" :
             ((depth == ALLEGRO_AUDIO_24_BIT_UINT) ? "24-bit unsigned" :
              "32-bit float")))))),
         voice_freq, (chan_conf>>4)+(chan_conf&0xF));
}


void display_mixer_info(ALLEGRO_MIXER *mixer)
{
   ALLEGRO_AUDIO_ENUM val;

   if(al_mixer_get_enum(mixer, ALLEGRO_AUDIO_QUALITY, &val) != 0)
      fprintf(stderr, "al_mixer_get_enum failed!\n");
   else
      fprintf(stderr, "%s resampling\n",
            ((val==ALLEGRO_AUDIO_POINT) ? "Point" :
            ((val==ALLEGRO_AUDIO_LINEAR) ? "Linear" : "Unknown")));
}
