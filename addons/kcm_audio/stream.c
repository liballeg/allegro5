/**
 * stream.c
 * Contains methods for the ALLEGRO_STREAM structure
 * In the style of python/c++:
 *    al_stream_create is the constructor
 *    al_stream_destroy is the destructor
 *    all other methods take the voice structure as the first argument
 *
 * Based on the code from KC/Milan
 * Completely rewritten by Ryan Dickie
 */
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/audio.h"

/* al_stream_create:
 *   Creates an audio stream, using the supplied values. The stream will be set
 *   to play by default.
 */
ALLEGRO_STREAM* al_stream_create(size_t buffer_count, unsigned long samples, unsigned long frequency, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf)
{
   ALLEGRO_STREAM *stream;
   unsigned long bytes_per_buffer;
   size_t i;

   if (!buffer_count)
   {
      TRACE("Attempted to create stream with no buffer");
      return NULL;
   }

   if (!samples)
   {
      TRACE("Attempted to create stream with no buffer size");
      return NULL;
   }

   if (!frequency)
   {
      TRACE("Attempted to create stream with no frequency");
      return NULL;
   }

   stream = malloc(sizeof(*stream));
   if(!stream)
   {
      TRACE("Out of memory allocating stream object");
      return NULL;
   }


/* TODO: allocate memory */
   void *main_buffer;
   void **pending_bufs;
   void **used_bufs;

   stream->sample = *al_sample_create(stream->main_buffer, samples, frequency, depth, chan_conf, FALSE);
   stream->buf_count = buffer_count;

   return stream;
}

void al_stream_destroy(ALLEGRO_STREAM *stream)
{
   ASSERT(stream);
   /* TODO: free buffers */
   free(stream);
}

/* get the sample the stream is currently playing. Call streams methods to get/set its properties */
ALLEGRO_SAMPLE* al_stream_get_sample(ALLEGRO_STREAM* stream)
{
   ASSERT(stream);
   return &(stream->sample);
}
