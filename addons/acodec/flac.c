/*
 * Allegro FLAC reader
 * author: Ryan Dickie, (c) 2008
 */


#include "allegro5/allegro5.h"
#include "allegro5/a5_flac.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_memory.h"

#include <FLAC/stream_decoder.h>
#include <stdio.h>


typedef struct FLACFILE {
   char *buffer;
   long total_samples;
   int sample_rate;
   int word_size;
   int channels;
   long pos;
   long total_size; /* number of bytes */
   ALLEGRO_FILE *fh;
} FLACFILE;


static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder,
   FLAC__byte buffer[], size_t *bytes, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FILE *fh = ff->fh;
   (void)decoder;
   
   if(*bytes > 0) {
      int ret = al_fread(fh, buffer, *bytes);
      if(al_ferror(fh))
         return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
      else if(ret == 0)
         return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
      else
         return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
   }
   else
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}


static FLAC__StreamDecoderSeekStatus seek_callback(
   const FLAC__StreamDecoder *decoder,
   FLAC__uint64 absolute_byte_offset, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FILE *fh = ff->fh;
   (void)decoder;
   
   if(!al_fseek(fh, ALLEGRO_SEEK_SET, absolute_byte_offset))
      return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
   else
      return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}


static FLAC__StreamDecoderTellStatus tell_callback(
   const FLAC__StreamDecoder *decoder,
   FLAC__uint64 *absolute_byte_offset, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FILE *fh = ff->fh;
   int64_t pos = 0;
   (void)decoder;
   
   pos = al_ftell(fh);
   if(pos == -1)
      return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

   *absolute_byte_offset = (FLAC__uint64)pos;
   return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}


static FLAC__StreamDecoderLengthStatus length_callback(
   const FLAC__StreamDecoder *decoder,
   FLAC__uint64 *stream_length, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FILE *fh = ff->fh;
   (void)decoder;
   
   /* XXX check error */
   *stream_length = (FLAC__uint64)al_fsize(fh);
   return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}


static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FILE *fh = ff->fh;
   (void)decoder;
   
   if(al_feof(fh))
      return true;

   return false;
}


static void metadata_callback(const FLAC__StreamDecoder *decoder,
    const FLAC__StreamMetadata *metadata, void *client_data)
{
   FLACFILE *out = (FLACFILE *) client_data;

   (void)decoder;

   if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
      out->total_samples = metadata->data.stream_info.total_samples;
      out->sample_rate = metadata->data.stream_info.sample_rate;
      out->channels = metadata->data.stream_info.channels;
      out->word_size = metadata->data.stream_info.bits_per_sample / 8;
      out->total_size = out->total_samples * out->channels * out->word_size;
      out->buffer = _AL_MALLOC_ATOMIC(out->total_size);
      out->pos = 0;
   }
}


static void error_callback(const FLAC__StreamDecoder *decoder,
    FLAC__StreamDecoderErrorStatus status, void *client_data)
{
   (void)decoder;
   (void)client_data;

   TRACE("Got FLAC error callback: %s\n",
      FLAC__StreamDecoderErrorStatusString[status]);
}


static FLAC__StreamDecoderWriteStatus write_callback(
   const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
   const FLAC__int32 * const buffer[], void *client_data)
{
   FLACFILE *out = (FLACFILE *) client_data;
   long len = frame->header.blocksize;
   long chunk = len * out->channels * out->word_size;
   int sample_index;
   int channel_index;
   int out_index = 0;
   int word_size = out->word_size;

   /* FLAC returns FLAC__int32 and I need to convert it to my own format. */
   FLAC__uint8 *buf8 = (FLAC__uint8 *) (out->buffer + out->pos);
   FLAC__int16 *buf16 = (FLAC__int16 *) buf8;
   float       *buf32 = (float *) buf8;

   (void)decoder;
   (void)client_data;

   /* Flatten the array */
   /* TODO: test this array flattening process on 5.1 and higher flac files */
   switch (word_size) {
      case 1:
         for (sample_index = 0; sample_index < len; sample_index++) {
             for (channel_index = 0;
                  channel_index < out->channels;
                  channel_index++) {
                buf8[out_index++] =
                   (FLAC__uint8) buffer[channel_index][sample_index];
             }
         }
         break;

      case 2:
         for (sample_index = 0; sample_index < len; sample_index++) {
             for (channel_index = 0; channel_index < out->channels;
                   channel_index++) {
                buf16[out_index++] =
                   (FLAC__int16) buffer[channel_index][sample_index];
             }
         }
         break;

      case 3:
         for (sample_index = 0; sample_index < len; sample_index++) {
             for (channel_index = 0; channel_index < out->channels;
                channel_index++)
             {
                /* Little endian */
                /* FIXME: does this work? I only have 16-bit sound card mixer
                 * garbages for other 24-bit codecs too.
                 */
                buf8[out_index++] = (FLAC__uint8) (buffer[channel_index][sample_index] & 0xFF);
                buf8[out_index++] = (FLAC__uint8) ((buffer[channel_index][sample_index] & 0xFF00) >> 8);
                buf8[out_index++] = (FLAC__uint8) ((buffer[channel_index][sample_index] & 0xFF0000) >> 16);
             }
         }
         break;

      case 4:
         for (sample_index = 0; sample_index < len; sample_index++) {
             for (channel_index = 0; channel_index < out->channels;
                   channel_index++) {
                buf32[out_index++] =
                   (float) buffer[channel_index][sample_index];
             }
         }
         break;

      default:
         /* Word_size not supported. */
         return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
   }

   out->pos += chunk;
   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


/* Function: al_init_flac_addon
 */
bool al_init_flac_addon(void)
{
   return al_register_sample_loader(".flac", al_load_sample_flac);
}


/* Function: al_load_sample_flac
 */
ALLEGRO_SAMPLE *al_load_sample_flac(const char *filename)
{
   ALLEGRO_SAMPLE *sample;
   FLAC__StreamDecoder *decoder = 0;
   FLAC__StreamDecoderInitStatus init_status;
   FLACFILE ff;

   decoder = FLAC__stream_decoder_new();
   if (!decoder) {
      TRACE("Error allocating FLAC decoder\n");
      return NULL;
   }

   ff.fh = al_fopen(filename, "rb");
   if(!ff.fh) {
      TRACE("Error opening FLAC file (%s)\n", filename);
      return NULL;
   }
   
   init_status = FLAC__stream_decoder_init_stream(decoder, read_callback,
      seek_callback, tell_callback, length_callback, eof_callback,
      write_callback, metadata_callback, error_callback, &ff);
   if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
      TRACE("Error initializing FLAC decoder: %s\n",
         FLAC__StreamDecoderInitStatusString[init_status]);
      FLAC__stream_decoder_delete(decoder);
      al_fclose(ff.fh);
      return NULL;
   }
 
   FLAC__stream_decoder_process_until_end_of_stream(decoder);

   TRACE("Loaded sample %s with properties:\n", filename);
   TRACE("    channels %d\n", ff.channels);
   TRACE("    word_size %d\n", ff.word_size);
   TRACE("    rate %d\n", ff.sample_rate);
   TRACE("    total_samples %ld\n", ff.total_samples);
   TRACE("    total_size %ld\n", ff.total_size);

   FLAC__stream_decoder_delete(decoder);
   al_fclose(ff.fh);
   
   if (ff.word_size == 0) {
      TRACE("Error: don't support sub 8-bit sizes\n");
      return NULL;
   }

   sample = al_create_sample(ff.buffer, ff.total_samples, ff.sample_rate,
      _al_word_size_to_depth_conf(ff.word_size),
      _al_count_to_channel_conf(ff.channels), true);

   if (!sample) {
      _AL_FREE(ff.buffer);
   }

   return sample;
}


/* vim: set sts=3 sw=3 et: */
