/*
 * Allegro5 Ogg Vorbis reader.
 * author: Ryan Dickie (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"

#ifdef ALLEGRO_CFG_ACODEC_VORBIS

#include <vorbis/vorbisfile.h>

/* note: decoding library returns floats..
 * so i always return 16-bit (most commonly supported)
 * TODO: figure out a way for 32-bit float or 24-bit
 * support in the al framework while keeping API simple to use
 */
ALLEGRO_SAMPLE* al_load_sample_oggvorbis(const char *filename)
{
   const int endian = 0; /* 0 for Little-Endian, 1 for Big-Endian */
   int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   int signedness = 1; /* 0  for unsigned, 1 for signed */
   const int packet_size = 4096; /* suggestion for size to read at a time */
   OggVorbis_File vf;
   vorbis_info* vi;
   FILE* file;

   file = fopen(filename, "rb");
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

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);
 
   /* al_sample_destroy will clean it! */
   char* buffer = (char*) malloc(total_size);
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
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels),
            TRUE
         );

   return sample;
}

/* TODO: make these not globals and put a spot for them in stream */
OggVorbis_File vf;
vorbis_info* vi;
FILE* file;
int bitStream = -1;

/* this also needs to be called when stream is destroyed */
void __ogg_stream_close(ALLEGRO_STREAM* stream)
{
   ov_clear(&vf);
   fclose(file);
}

bool _ogg_stream_update(ALLEGRO_STREAM* stream, void* data, unsigned long samples)
{
   const int endian = 0; /* 0 for Little-Endian, 1 for Big-Endian */
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   const int signedness = 1; /* 0  for unsigned, 1 for signed */
   int channels = vi->channels;

   unsigned long pos = 0;
   while (pos < samples)
   {
      unsigned long read = ov_read(&vf, data + pos, samples, endian, word_size, signedness, &bitStream);
      pos += read;
      /* now to silence from here to the end. */
   }
   return true;
}



ALLEGRO_STREAM* al_load_stream_oggvorbis(const char *filename)
{
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   int signedness = 1; /* 0  for unsigned, 1 for signed */


   file = fopen(filename, "rb");
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

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);
 
   ALLEGRO_STREAM* stream = al_stream_create(
            rate,
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels),
            _ogg_stream_update
         );

   return stream;
}

#endif /* ALLEGRO_CFG_ACODEC_VORBIS */
