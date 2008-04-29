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
ALLEGRO_STREAM* al_stream_create(unsigned long frequency, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf, bool (*stream_update)(ALLEGRO_STREAM* stream, void* data, unsigned long samples))
{
   ALLEGRO_STREAM *stream;
   size_t i;

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

   stream->frequency = frequency;
   stream->depth = depth;
   stream->chan_conf = chan_conf;
   stream->stream_update = stream_update;

   return stream;
}

/* TODO: make sure i shut it down if needed */
void al_stream_destroy(ALLEGRO_STREAM *stream)
{
   ASSERT(stream);
   /* TODO: free buffers */
   free(stream);
}

/* TODO: make stream getters */
