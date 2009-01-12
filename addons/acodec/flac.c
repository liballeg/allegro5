/*
 * Allegro FLAC reader
 * author: Ryan Dickie, (c) 2008
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"
#include "allegro5/internal/aintern_memory.h"

#ifdef ALLEGRO_CFG_ACODEC_FLAC

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
   ALLEGRO_FS_ENTRY *fh;
} FLACFILE;

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder,
   FLAC__byte buffer[], size_t *bytes, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FS_ENTRY *fh = ff->fh;
   (void)decoder;
   
   if(*bytes > 0) {
      int ret = al_fs_entry_read(fh, *bytes, buffer);
      if(al_fs_entry_error(fh))
         return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
      else if(ret == 0)
         return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
      else
         return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
   }
   else
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

FLAC__StreamDecoderSeekStatus seek_callback(
   const FLAC__StreamDecoder *decoder,
   FLAC__uint64 absolute_byte_offset, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FS_ENTRY *fh = ff->fh;
   (void)decoder;
   
   if(!al_fs_entry_seek(fh, ALLEGRO_SEEK_SET, absolute_byte_offset))
      return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
   else
      return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

FLAC__StreamDecoderTellStatus tell_callback(
   const FLAC__StreamDecoder *decoder,
   FLAC__uint64 *absolute_byte_offset, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FS_ENTRY *fh = ff->fh;
   int64_t pos = 0;
   (void)decoder;
   
   pos = al_fs_entry_tell(fh);
   if(pos == -1)
      return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;

   *absolute_byte_offset = (FLAC__uint64)pos;
   return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus length_callback(
   const FLAC__StreamDecoder *decoder,
   FLAC__uint64 *stream_length, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FS_ENTRY *fh = ff->fh;
   (void)decoder;
   
   *stream_length = (FLAC__uint64)al_fs_entry_size(fh);
   return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FS_ENTRY *fh = ff->fh;
   (void)decoder;
   
   if(al_fs_entry_eof(fh))
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


ALLEGRO_SAMPLE_DATA *al_load_sample_data_flac(const char *filename)
{
   ALLEGRO_SAMPLE_DATA *sample;
   FLAC__StreamDecoder *decoder = 0;
   FLAC__StreamDecoderInitStatus init_status;
   FLACFILE ff;

   decoder = FLAC__stream_decoder_new();
   if (!decoder) {
      TRACE("Error allocating FLAC decoder\n");
      return NULL;
   }

   ff.fh = al_fs_entry_open(filename, "rb");
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
      al_fs_entry_close(ff.fh);
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
   al_fs_entry_close(ff.fh);
   
   if (ff.word_size == 0) {
      TRACE("Error: don't support sub 8-bit sizes\n");
      return NULL;
   }

   sample = al_create_sample_data(ff.buffer, ff.total_samples, ff.sample_rate,
      _al_word_size_to_depth_conf(ff.word_size),
      _al_count_to_channel_conf(ff.channels), true);

   if (!sample) {
      _AL_FREE(ff.buffer);
   }

   return sample;
}


/* TODO implement */
bool _flac_stream_update(ALLEGRO_STREAM *stream, void *data,
   unsigned long buf_size)
{
   (void)stream;
   (void)data;
   (void)buf_size;
   return false;
}


/* TODO implement */
ALLEGRO_STREAM *al_load_stream_flac(const char *filename)
{
   (void)filename;
   return NULL;
/*
   stream = al_create_stream(ff.sample_rate,
                     _al_word_size_to_depth_conf(ff.word_size),
                     _al_count_to_channel_conf(ff.channels),
                     _flac_stream_update);
*/
}


#endif /* ALLEGRO_CFG_ACODEC_FLAC */

/* vim: set sts=3 sw=3 et: */
