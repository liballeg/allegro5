/*
 * Allegro5 WAV/PCM/AIFF reader
 * author: Ryan Dickie (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"


#ifdef ALLEGRO_CFG_ACODEC_SNDFILE

#include <sndfile.h>

ALLEGRO_AUDIO_ENUM _get_depth_enum( int format , int* word_size)
{
   switch (format&0xFFFF)
   {
      case SF_FORMAT_PCM_U8:
         *word_size = 1;
         return ALLEGRO_AUDIO_8_BIT_UINT; 
   
      case SF_FORMAT_PCM_S8:
         *word_size = 1;
         return ALLEGRO_AUDIO_8_BIT_INT;
         
      case SF_FORMAT_PCM_16:
         *word_size = 2;
         return ALLEGRO_AUDIO_16_BIT_INT;
   
      case SF_FORMAT_PCM_24:
         *word_size = 3;
         return ALLEGRO_AUDIO_24_BIT_INT;
   
      case SF_FORMAT_FLOAT:
         *word_size = 4;
         return ALLEGRO_AUDIO_32_BIT_FLOAT;
    
      default:
         fprintf(stderr, "Unsupported sndfile depth format (%X)\n",format);
         *word_size = 0;
         return 0;
   }
}

ALLEGRO_SAMPLE* al_load_sample_sndfile(const char *filename)
{
   ALLEGRO_AUDIO_ENUM depth; 
   SF_INFO sfinfo;
   sfinfo.format = 0;
   SNDFILE* sndfile = sf_open(filename, SFM_READ, &sfinfo); 

   if (sndfile == NULL)
      return NULL;

   /* supports 16-bit, 32-bit (and float) */
   int word_size = 0;
   depth = _get_depth_enum(sfinfo.format,&word_size);  
   int channels = sfinfo.channels;
   long rate = sfinfo.samplerate;
   long total_samples = sfinfo.frames;
   long total_size = total_samples * channels * word_size; 

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);
   
   short* buffer = (short*) malloc(total_size);
   if (buffer == NULL)
   {
      sf_close(sndfile);
      return NULL;
   }

   sf_readf_short(sndfile, buffer, total_samples);
   sf_close(sndfile);

   ALLEGRO_SAMPLE* sample = al_sample_create(buffer, total_samples, rate,
         depth,
         _al_count_to_channel_conf(channels));

   return sample;
}



#endif /* ALLEGRO_CFG_ACODEC_SNDFILE */
