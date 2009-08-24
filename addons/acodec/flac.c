/*
 * Allegro FLAC reader
 * author: Ryan Dickie, (c) 2008
 * streaming support by Elias Pschernig
 */


#include "allegro5/allegro5.h"
#include "allegro5/allegro_flac.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_memory.h"

#include <FLAC/stream_decoder.h>
#include <stdio.h>

ALLEGRO_DEBUG_CHANNEL("sound")

typedef struct FLACFILE {
   FLAC__StreamDecoder *decoder;
   double sample_rate;
   int sample_size;
   int channels;

   /* The file buffer. */
   uint64_t buffer_pos, buffer_size;
   char *buffer;

   /* Number of samples in the complete FLAC. */
   uint64_t total_samples;

   /* Sample position one past last decoded sample. */
   uint64_t decoded_samples;

   /* Sample position one past last streamed sample. */
   uint64_t streamed_samples;

   ALLEGRO_FILE *fh;
   uint64_t loop_start, loop_end; /* in samples */
} FLACFILE;


static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder,
   FLAC__byte buffer[], size_t *bytes, void *dptr)
{
   FLACFILE *ff = (FLACFILE *)dptr;
   ALLEGRO_FILE *fh = ff->fh;
   (void)decoder;
   if(*bytes > 0) {
      *bytes = al_fread(fh, buffer, *bytes);
      if(al_ferror(fh))
         return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
      else if (*bytes == 0)
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

   if(!al_fseek(fh, absolute_byte_offset, ALLEGRO_SEEK_SET))
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
   FLACFILE *out = (FLACFILE *)client_data;

   (void)decoder;

   if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
      out->total_samples = metadata->data.stream_info.total_samples;
      out->sample_rate = metadata->data.stream_info.sample_rate;
      out->channels = metadata->data.stream_info.channels;
      out->sample_size = metadata->data.stream_info.bits_per_sample / 8;
   }
}


static void error_callback(const FLAC__StreamDecoder *decoder,
    FLAC__StreamDecoderErrorStatus status, void *client_data)
{
   (void)decoder;
   (void)client_data;

   ALLEGRO_ERROR("Got FLAC error callback: %s\n",
      FLAC__StreamDecoderErrorStatusString[status]);
}


static FLAC__StreamDecoderWriteStatus write_callback(
   const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame,
   const FLAC__int32 * const buffer[], void *client_data)
{
   FLACFILE *ff = (FLACFILE *) client_data;
   long len = frame->header.blocksize;
   long bytes = len * ff->channels * ff->sample_size;
   int sample_index;
   int channel_index;
   int out_index = 0;
   int word_size = ff->sample_size;

   if (ff->buffer_pos + bytes > ff->buffer_size) {
      ff->buffer = _AL_REALLOC(ff->buffer, ff->buffer_pos + bytes);
      ff->buffer_size = ff->buffer_pos + bytes;
   }

   /* FLAC returns FLAC__int32 and I need to convert it to my own format. */
   FLAC__uint8 *buf8 = (FLAC__uint8 *) (ff->buffer + ff->buffer_pos);
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
                  channel_index < ff->channels;
                  channel_index++) {
                buf8[out_index++] =
                   (FLAC__uint8) buffer[channel_index][sample_index];
             }
         }
         break;

      case 2:
         for (sample_index = 0; sample_index < len; sample_index++) {
             for (channel_index = 0; channel_index < ff->channels;
                   channel_index++) {
                buf16[out_index++] =
                   (FLAC__int16) buffer[channel_index][sample_index];
             }
         }
         break;

      case 3:
         for (sample_index = 0; sample_index < len; sample_index++) {
             for (channel_index = 0; channel_index < ff->channels;
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
             for (channel_index = 0; channel_index < ff->channels;
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

   ff->decoded_samples += len;
   ff->buffer_pos += bytes;
   return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_close(FLACFILE *ff)
{
   FLAC__stream_decoder_finish(ff->decoder);
   FLAC__stream_decoder_delete(ff->decoder);
   al_fclose(ff->fh);
   _AL_FREE(ff);
}

/* In seconds. */
static double flac_stream_get_position(ALLEGRO_STREAM *stream)
{
   FLACFILE *ff = (FLACFILE *)stream->extra;
   return ff->streamed_samples / ff->sample_rate;
}


/*
 *  Updates 'stream' with the next chunk of data.
 *  Returns the actual number of bytes written.
 */
static size_t flac_stream_update(ALLEGRO_STREAM *stream, void *data,
   size_t buf_size)
{
   int bytes_per_sample;
   uint64_t wanted_samples;
   uint64_t read_samples;
   size_t written_bytes = 0;
   size_t read_bytes;
   FLACFILE *ff = (FLACFILE *)stream->extra;

   bytes_per_sample = ff->sample_size * ff->channels;
   wanted_samples = buf_size / bytes_per_sample;

   if (ff->streamed_samples + wanted_samples > ff->loop_end) {
      if (ff->loop_end > ff->streamed_samples)
         wanted_samples = ff->loop_end - ff->streamed_samples;
      else
         return 0;
   }

   while (wanted_samples > 0) {
      read_samples = ff->decoded_samples - ff->streamed_samples;

      /* If the buffer size is small, we shouldn't read a new frame or our
       * buffer keeps growing - so only refill when needed.
       */
      if (!read_samples) {
         if (!FLAC__stream_decoder_process_single(ff->decoder))
            break;
         read_samples = ff->decoded_samples - ff->streamed_samples;
         if (!read_samples) {
            break;
         }
      }

      if (read_samples > wanted_samples)
         read_samples = wanted_samples;
      ff->streamed_samples += read_samples;
      wanted_samples -= read_samples;
      read_bytes = read_samples * bytes_per_sample;
      /* Copy data from the FLAC file buffer to the stream buffer. */
      memcpy((uint8_t *)data + written_bytes, ff->buffer, read_bytes);
      /* Make room in the FLACFILE buffer. */
      memmove(ff->buffer, ff->buffer + read_bytes,
         ff->buffer_pos - read_bytes);
      ff->buffer_pos -= read_bytes;
      written_bytes += read_bytes;
   }

   return written_bytes;
}

/* Called from al_destroy_stream. */
static void flac_stream_close(ALLEGRO_STREAM *stream)
{
   FLACFILE *ff = stream->extra;
   flac_close(ff);
}

static bool real_seek(ALLEGRO_STREAM *stream, uint64_t sample)
{
   FLACFILE *ff = stream->extra;

   /* We use ff->streamed_samples as the exact sample position for looping and
    * returning the position. Therefore we also use it as reference position
    * when seeking - that is, we call flush below to make the FLAC decoder
    * discard any additional samples it may have buffered already.
    * */
   FLAC__stream_decoder_flush(ff->decoder);
   FLAC__stream_decoder_seek_absolute(ff->decoder, sample);

   ff->buffer_pos = 0;
   ff->streamed_samples = sample;
   ff->decoded_samples = sample;
   return true;
}

static bool flac_stream_seek(ALLEGRO_STREAM *stream, double time)
{
   FLACFILE *ff = stream->extra;
   uint64_t sample = time * ff->sample_rate;
   return real_seek(stream, sample);
}

static bool flac_stream_rewind(ALLEGRO_STREAM *stream)
{
   FLACFILE *ff = stream->extra;
   return real_seek(stream, ff->loop_start);
}

static double flac_stream_get_length(ALLEGRO_STREAM *stream)
{
   FLACFILE *ff = stream->extra;
   return ff->total_samples / ff->sample_rate;
}

static bool flac_stream_set_loop(ALLEGRO_STREAM *stream, double start, double end)
{
   FLACFILE *ff = stream->extra;
   ff->loop_start = start * ff->sample_rate;
   ff->loop_end = end * ff->sample_rate;
   return true;
}


/* Function: al_init_flac_addon
 */
bool al_init_flac_addon(void)
{
   bool rc1 = al_register_sample_loader(".flac", al_load_sample_flac);
   bool rc2 = al_register_stream_loader(".flac", al_load_stream_flac);
   return rc1 && rc2;
}

static FLACFILE *flac_open(char const *filename)
{
   FLACFILE *ff;
   FLAC__StreamDecoderInitStatus init_status;

   ff = _AL_MALLOC(sizeof *ff);
   memset(ff, 0, sizeof *ff);

   ff->decoder = FLAC__stream_decoder_new();
   if (!ff->decoder) {
      ALLEGRO_ERROR("Error allocating FLAC decoder\n");
      goto error;
   }

   ff->fh = al_fopen(filename, "rb");
   if (!ff->fh) {
      ALLEGRO_ERROR("Error opening FLAC file (%s)\n", filename);
      goto error;
   }
   
   init_status = FLAC__stream_decoder_init_stream(ff->decoder, read_callback,
      seek_callback, tell_callback, length_callback, eof_callback,
      write_callback, metadata_callback, error_callback, ff);
   if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
      ALLEGRO_ERROR("Error initializing FLAC decoder: %s\n",
         FLAC__StreamDecoderInitStatusString[init_status]);
      goto error;
   }

   FLAC__stream_decoder_process_until_end_of_metadata(ff->decoder);

   if (ff->sample_size == 0) {
      ALLEGRO_ERROR("Error: don't support sub 8-bit sizes\n");
      goto error;
   }

   ALLEGRO_INFO("Loaded sample %s with properties:\n", filename);
   ALLEGRO_INFO("    channels %d\n", ff->channels);
   ALLEGRO_INFO("    sample_size %d\n", ff->sample_size);
   ALLEGRO_INFO("    rate %.f\n", ff->sample_rate);
   ALLEGRO_INFO("    total_samples %ld\n", (long) ff->total_samples);

   return ff;
error:
   if (ff && ff->decoder) FLAC__stream_decoder_delete(ff->decoder);
   if (ff && ff->fh) al_fclose(ff->fh);
   if (ff) _AL_FREE(ff);
   return NULL;
}

/* Function: al_load_sample_flac
 */
ALLEGRO_SAMPLE *al_load_sample_flac(const char *filename)
{
   ALLEGRO_SAMPLE *sample;
   FLACFILE *ff = flac_open(filename);

   ff->buffer_size = ff->total_samples * ff->channels * ff->sample_size;
   ff->buffer = _AL_MALLOC_ATOMIC(ff->buffer_size);

   FLAC__stream_decoder_process_until_end_of_stream(ff->decoder);

   sample = al_create_sample(ff->buffer, ff->total_samples, ff->sample_rate,
      _al_word_size_to_depth_conf(ff->sample_size),
      _al_count_to_channel_conf(ff->channels), true);

   if (!sample) {
      _AL_FREE(ff->buffer);
   }

   flac_close(ff);

   return sample;
}

/* Function: al_get_allegro_flac_version
 */
uint32_t al_get_allegro_flac_version(void)
{
   return ALLEGRO_VERSION_INT;
}

/* Function: al_load_stream_flac
*/
ALLEGRO_STREAM *al_load_stream_flac(const char *filename, size_t buffer_count,
   unsigned int samples)
{
   ALLEGRO_STREAM *stream;
   FLACFILE *ff = flac_open(filename);

   stream = al_create_stream(buffer_count, samples, ff->sample_rate,
      _al_word_size_to_depth_conf(ff->sample_size),
      _al_count_to_channel_conf(ff->channels));

   if (stream) {
      stream->extra = ff;
      ff->loop_start = 0;
      ff->loop_end = ff->total_samples;
      stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
      stream->feeder = flac_stream_update;
      stream->unload_feeder = flac_stream_close;
      stream->rewind_feeder = flac_stream_rewind;
      stream->seek_feeder = flac_stream_seek;
      stream->get_feeder_position = flac_stream_get_position;
      stream->get_feeder_length = flac_stream_get_length;
      stream->set_feeder_loop = flac_stream_set_loop;
      al_start_thread(stream->feed_thread);
   }

   return stream;
}


/* vim: set sts=3 sw=3 et: */
