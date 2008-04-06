/**
 * Allegro5 WAV/PCM/AIFF reader
 * -lsndfile
 * author: Ryan Dickie (c) 2008
 */
#include "allegro5/internal/aintern_acodec.h"
#include <sndfile.h>

// returns NULL on error
ALLEGRO_SAMPLE* al_load_sample_sndfile(const char *filename)
{
   SF_INFO sfinfo;
   sfinfo.format = 0;
   SNDFILE* sndfile = sf_open(filename, SFM_READ, &sfinfo); 

   if (sndfile == NULL)
      return NULL;

   const int word_size = 2; //supports 16-bit, 32-bit (and float) 
   int channels = sfinfo.channels;
   long rate = sfinfo.samplerate;
   long total_samples = sfinfo.frames;
   long total_size = total_samples * channels * word_size; 

   short* buffer = (short*) malloc(total_size);
   if (buffer == NULL)
   {
      sf_close(sndfile);
      return NULL;
   }

   sf_readf_short(sndfile, buffer, total_samples);
   sf_close(sndfile);

   ALLEGRO_SAMPLE* sample = al_sample_create(buffer, total_samples, rate,
         (word_size==1)?ALLEGRO_AUDIO_8_BIT_UINT:ALLEGRO_AUDIO_16_BIT_INT,
         (channels==1)?ALLEGRO_AUDIO_1_CH:ALLEGRO_AUDIO_2_CH);

   return sample;
}
