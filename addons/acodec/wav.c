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
   SNDFILE* sndfile;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   void* buffer;
   ALLEGRO_SAMPLE* sample;

   sfinfo.format = 0;
   sndfile = sf_open(filename, SFM_READ, &sfinfo); 

   if (sndfile == NULL)
      return NULL;

   word_size = 0;
   depth = _get_depth_enum(sfinfo.format,&word_size);  
   channels = sfinfo.channels;
   rate = sfinfo.samplerate;
   total_samples = sfinfo.frames;
   total_size = total_samples * channels * word_size; 

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);

   buffer = malloc(total_size);
   if (buffer == NULL)
   {
      sf_close(sndfile);
      return NULL;
   }

   if (depth == ALLEGRO_AUDIO_16_BIT_INT) {
      sf_readf_short(sndfile, buffer, total_samples);
   }
   else if (depth == ALLEGRO_AUDIO_32_BIT_FLOAT) {
      sf_readf_float(sndfile, buffer, total_samples);
   }
   else {
      sf_read_raw(sndfile, buffer, total_samples);
   }

   sf_close(sndfile);

   sample = al_sample_create(buffer, total_samples, rate,
         depth,
         _al_count_to_channel_conf(channels), TRUE);

   return sample;
}

bool _sndfile_stream_update(ALLEGRO_STREAM* stream, void* data, unsigned long buf_size)
{
   int bytes_per_sample, samples, num_read, bytes_read, silence;

   SNDFILE* sndfile = (SNDFILE*) stream->ex_data;
   bytes_per_sample = al_audio_channel_count(stream->chan_conf) * al_audio_depth_size(stream->depth);
   samples = buf_size / bytes_per_sample;

   if (stream->depth == ALLEGRO_AUDIO_16_BIT_INT) {
      num_read = sf_readf_short(sndfile, data, samples);
   }
   else if (stream->depth == ALLEGRO_AUDIO_32_BIT_FLOAT) {
      num_read = sf_readf_float(sndfile, data, samples);
   }
   else {
      num_read = sf_read_raw(sndfile, data, samples);
   }

   if (num_read == samples)
      return true;

   /* out of data */
   bytes_read = num_read*bytes_per_sample;
   silence = _al_audio_get_silence(stream->depth);
   memset((char*)data + bytes_read, silence, buf_size - bytes_read);
   return false;
}

void _sndfile_stream_close(ALLEGRO_STREAM* stream)
{
   SNDFILE* sndfile = (SNDFILE*) stream->ex_data;
   sf_close(sndfile);
   stream->ex_data = NULL;
   return;
}

ALLEGRO_STREAM* al_load_stream_sndfile(const char *filename)
{
   ALLEGRO_AUDIO_ENUM depth; 
   SF_INFO sfinfo;
   SNDFILE* sndfile;
   int word_size;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   short* buffer;
   ALLEGRO_STREAM* stream;
   
   sfinfo.format = 0;
   sndfile = sf_open(filename, SFM_READ, &sfinfo); 

   if (sndfile == NULL)
      return NULL;

   /* supports 16-bit, 32-bit (and float) */
   word_size = 0;
   depth = _get_depth_enum(sfinfo.format,&word_size);  
   channels = sfinfo.channels;
   rate = sfinfo.samplerate;
   total_samples = sfinfo.frames;
   total_size = total_samples * channels * word_size; 

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",channels);
   fprintf(stderr, "    word_size %d\n", word_size);
   fprintf(stderr, "    rate %ld\n",rate);
   fprintf(stderr, "    total_samples %ld\n",total_samples);
   fprintf(stderr, "    total_size %ld\n",total_size);
   
   buffer = (short*) malloc(total_size);
   if (buffer == NULL)
   {
      sf_close(sndfile);
      return NULL;
   }

   stream = al_stream_create(rate,
         depth,
         _al_count_to_channel_conf(channels),
         _sndfile_stream_update, _sndfile_stream_close);

   stream->ex_data = sndfile;
   return stream;
}


#endif /* ALLEGRO_CFG_ACODEC_SNDFILE */
