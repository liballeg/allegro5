/*
 * Allegro FLAC reader
 * author: Ryan Dickie, (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"

#ifdef ALLEGRO_CFG_ACODEC_FLAC

#include <FLAC/stream_decoder.h>
#include <stdio.h>



typedef struct FLACFILE {
   char* buffer;
   long total_samples;
   int sample_rate;
   int word_size;
   int channels;
   long pos;
   long total_size; //number of bytes!
} FLACFILE;

void metadata_callback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
   FLACFILE* out = (FLACFILE*) client_data;
   if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
   {
      out->total_samples = metadata->data.stream_info.total_samples;
      out->sample_rate = metadata->data.stream_info.sample_rate;
      out->channels = metadata->data.stream_info.channels;
      out->word_size = metadata->data.stream_info.bits_per_sample / 8;
      out->total_size = out->total_samples * out->channels * out->word_size;
      out->buffer = (char*) malloc(out->total_size);
      out->pos = 0;

#ifndef NDEBUG      
      printf("channels %d\n",out->channels);
      printf("rate %d\n",out->sample_rate);
      printf("total_samples %ld\n",out->total_samples);
      printf("total_size %ld\n",out->total_size); 
      printf("Metadata callback\n");
#endif

   }
}


void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
   fprintf(stderr, "Got error callback: %s\n", FLAC__StreamDecoderErrorStatusString[status]);
}


FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
   FLACFILE* out = (FLACFILE*) client_data;
   long len = frame->header.blocksize;
   long chunk = len * out->channels * out->word_size;
   char* buf = out->buffer + out->pos;
   int i;

   if (out->channels == 1)
   {
      if (out->word_size == 1)
      {
         for(i = 0; i < len; i++)
            buf[i] = (FLAC__int8)buffer[0][i];    /* left channel */
      }
      else if (out->word_size == 2)
      {
         short* sbuf = (short*)buf;
         for(i = 0; i < len; i++)
            sbuf[i] = (FLAC__int16)buffer[0][i];    /* left channel */
      }
   }
   else if (out->channels == 2) 
   {
      if (out->word_size == 1)
      {
         for(i = 0; i < len; i++)
         {
            buf[i*2] = (FLAC__int8)buffer[0][i];    /* left channel */
            buf[i*2+1] = (FLAC__int8)buffer[1][i];    /* right channel */
         }
      }
      else if (out->word_size == 2)
      {
         short* sbuf = (short*)buf;
         for(i = 0; i < len; i++)
         {
            sbuf[i*2] = (FLAC__int16)buffer[0][i];    /* left channel */
            sbuf[i*2+1] = (FLAC__int16)buffer[1][i];    /* right channel */
         }
      }
   }

   out->pos += chunk;
   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


//for ff
ALLEGRO_SAMPLE* al_load_sample_flac(const char *filename)
{
   ALLEGRO_SAMPLE *sample;
   FLAC__StreamDecoder *decoder = 0;
   FLAC__StreamDecoderInitStatus init_status;
   FLACFILE ff;

   decoder = FLAC__stream_decoder_new();
   if(decoder == NULL)
   {
      fprintf(stderr, "ERROR: allocating decoder\n");
      return NULL;
   }

   init_status = FLAC__stream_decoder_init_file(decoder, filename, write_callback, metadata_callback, error_callback, &ff);
   if(init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
   {
      fprintf(stderr, "ERROR: initializing decoder: %s\n", FLAC__StreamDecoderInitStatusString[init_status]);
      return NULL;
   }

   FLAC__stream_decoder_process_until_end_of_stream(decoder);
   FLAC__stream_decoder_delete(decoder);

   //TODO: handle more formats
   sample = al_sample_create(ff.buffer, ff.total_samples, ff.sample_rate,
                     (ff.word_size==1)?ALLEGRO_AUDIO_8_BIT_UINT:ALLEGRO_AUDIO_16_BIT_INT,
                     (ff.channels==1)?ALLEGRO_AUDIO_1_CH:ALLEGRO_AUDIO_2_CH);

   return sample;
}



#endif /* ALLEGRO_CFG_ACODEC_FLAC */
