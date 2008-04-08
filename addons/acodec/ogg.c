/*
 * Allegro5 Ogg Vorbis reader.
 * author: Ryan Dickie (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"

#ifdef ALLEGRO_CFG_ACODEC_VORBIS

#include <vorbis/vorbisfile.h>



ALLEGRO_SAMPLE* al_load_sample_oggvorbis(const char *fileName)
{
   const int endian = 0; // 0 for Little-Endian, 1 for Big-Endian
   const int word_size = 2; //1 = 8bit, 2 = 16-bit. nothing else
   const int signedness = 1; // 0  for unsigned, 1 for signed
   const int packet_size = 4096; //suggestion for size to read at a time
   OggVorbis_File vf;
   vorbis_info* vi;
   FILE* file;

   file = fopen(fileName, "rb");
   if (file == NULL)
      return NULL;


   if(ov_open_callbacks(file, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
   {
      fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
      fclose(file);
      return NULL;
   }

   vi = ov_info(&vf, -1);
   int channels = vi->channels;
   long rate = vi->rate;
   long total_samples = ov_pcm_total(&vf,-1);
   long total_size = total_samples * channels * word_size;
   int bitStream = -1;

#ifndef NDEBUG   
   printf("channels %d\n",channels);
   printf("rate %ld\n",rate);
   printf("total_samples %ld\n",total_samples);
   printf("total_size %ld\n",total_size);
#endif

   char* buffer = (char*) malloc(total_size); //al_sample_destroy should clean it!
   long pos = 0;
   while (true)
   {
      long read = ov_read(&vf, buffer + pos, packet_size, endian, word_size, signedness, &bitStream);
      pos += read;
      if (read == 0)
         break;
   }
   ov_clear(&vf);
   fclose(file);

   ALLEGRO_SAMPLE* sample = al_sample_create(
         buffer,
         total_samples,
         rate,
         ALLEGRO_AUDIO_16_BIT_INT,
         (channels==1)?ALLEGRO_AUDIO_1_CH:ALLEGRO_AUDIO_2_CH
         );

   return sample;
}



#endif /* ALLEGRO_CFG_ACODEC_VORBIS */
