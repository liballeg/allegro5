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
   long total_size; /* number of bytes! */
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
   int sample_index;
   int channel_index;
   int out_index = 0;
   int word_size = out->word_size;

   /* Flac returns FLAC__int32 and i need to convert it to my own format */
   FLAC__uint8* buf8 = (FLAC__uint8*) (out->buffer + out->pos);
   FLAC__int16* buf16 = (FLAC__int16*) buf8;
   float*       buf32 = (float*) buf8;

   /* this process flattens the array */
   /* TODO: test this array flattening process on 5.1 and higher flac files */
   switch (word_size)
   {
      case 1:
         for(sample_index = 0; sample_index < len; sample_index++)
         {
             for (channel_index = 0; channel_index < out->channels; channel_index++)
             {
                buf8[out_index++] = (FLAC__uint8) buffer[channel_index][sample_index];
             }
         }
         break;
      case 2:
         for(sample_index = 0; sample_index < len; sample_index++)
         {
             for (channel_index = 0; channel_index < out->channels; channel_index++)
             {
                buf16[out_index++] = (FLAC__int16) buffer[channel_index][sample_index];
             }
         }
         break;
      case 3:
         for(sample_index = 0; sample_index < len; sample_index++)
         {
             for (channel_index = 0; channel_index < out->channels; channel_index++)
             {
                /* little endian */
                /* FIXME: does this work? I only have 16-bit sound card mixer garbages for other 24-bit codecs too*/
                buf8[out_index++] = (FLAC__uint8) ((buffer[channel_index][sample_index]&0xFF));
                buf8[out_index++] = (FLAC__uint8) ((buffer[channel_index][sample_index]&0xFF00)>>8);
                buf8[out_index++] = (FLAC__uint8) ((buffer[channel_index][sample_index]&0xFF0000)>>16);
             }
         }
         break;
      case 4:
         for(sample_index = 0; sample_index < len; sample_index++)
         {
             for (channel_index = 0; channel_index < out->channels; channel_index++)
             {
                buf32[out_index++] = (float) buffer[channel_index][sample_index];
             }
         }
         break;
      default:
         /* word_size not supported */
         return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
   }

   out->pos += chunk;
   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

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
      FLAC__stream_decoder_delete(decoder);
      return NULL;
   }
 
   FLAC__stream_decoder_process_until_end_of_stream(decoder);

   fprintf(stderr, "loaded sample %s with properties:\n",filename);
   fprintf(stderr, "    channels %d\n",ff.channels);
   fprintf(stderr, "    word_size %d\n", ff.word_size);
   fprintf(stderr, "    rate %d\n",ff.sample_rate);
   fprintf(stderr, "    total_samples %ld\n",ff.total_samples);
   fprintf(stderr, "    total_size %ld\n",ff.total_size);

   FLAC__stream_decoder_delete(decoder);
 
   if (ff.word_size == 0)
   {
      fprintf(stderr, "ERROR: I do not support sub 8-bit sizes\n");
      return NULL;     
   }

   sample = al_sample_create(ff.buffer, ff.total_samples, ff.sample_rate,
                     _al_word_size_to_depth_conf(ff.word_size),
                     _al_count_to_channel_conf(ff.channels),
                     TRUE);

   return sample;
}

/* TODO implement */
bool _flac_stream_update(ALLEGRO_STREAM* stream, void* data, unsigned long buf_size)
{
   return false;
}

/* TODO implement */
ALLEGRO_STREAM* al_load_stream_flac(const char *filename)
{
   return NULL;
/*
   stream = al_stream_create(ff.sample_rate,
                     _al_word_size_to_depth_conf(ff.word_size),
                     _al_count_to_channel_conf(ff.channels),
                     _flac_stream_update);
*/
}

#endif /* ALLEGRO_CFG_ACODEC_FLAC */
