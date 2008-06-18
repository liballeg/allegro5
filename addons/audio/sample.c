/**
 * Sample.c
 * Contains methods for the ALLEGRO_SAMPLE structure
 * In the style of python/c++:
 *    al_sample_create is the constructor
 *    al_sample_destroy is the destructor
 *    all other methods take the sample structure as the first argument
 *
 * Based on the code from KC/Milan
 * Rewritten by Ryan Dickie
 */

#include "allegro5/internal/aintern_audio.h"
#include "allegro5/audio.h"

/* al_sample_create:
 *   Creates a sample using a the supplied buffer and parameters.
 *   if (free_buffer == TRUE) then al_sample_destroy will call free(free_buffer)
 *      do not mix calls with "new" and "free". Use (malloc and free) or (new and delete).
 *   This call must eventually be followed by one to al_sample_destroy
 *   This must be attached to a voice or mixer before it can be played.
 */
ALLEGRO_SAMPLE *al_sample_create(void *buf, unsigned long samples, unsigned long frequency, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf, bool free_buffer)
{
   ALLEGRO_SAMPLE *sample;

   ASSERT(buf);

   sample = malloc(sizeof(*sample));
   if(!sample)
   {
      TRACE("Out of memory allocating sample object\n");
      return NULL;
   }

   sample->depth       = depth;
   sample->chan_conf   = chan_conf;
   sample->frequency   = frequency;
   sample->length      = samples;
   sample->free_buffer = free_buffer;
   sample->buffer  = buf;

   return sample;
}

void al_sample_destroy(ALLEGRO_SAMPLE *sample)
{
   ASSERT(sample);
   if (sample->free_buffer)
      free(sample->buffer);
   free(sample);
}

ALLEGRO_AUDIO_ENUM al_sample_get_depth(const ALLEGRO_SAMPLE *sample)
{
   ASSERT(sample);
   return sample->depth;
}

ALLEGRO_AUDIO_ENUM al_sample_get_channels(const ALLEGRO_SAMPLE *sample)
{
   ASSERT(sample);
   return sample->chan_conf;
}

unsigned long al_sample_get_frequency(const ALLEGRO_SAMPLE* sample)
{
   ASSERT(sample);
   return sample->frequency;
}

unsigned long al_sample_get_length(const ALLEGRO_SAMPLE* sample)
{
   ASSERT(sample);
   return sample->length;
}

/*returns sample length in seconds */
float al_sample_get_time(const ALLEGRO_SAMPLE* sample)
{
   ASSERT(sample);
   return (float)(sample->length) / (float)(sample->frequency);
}

void* al_sample_get_buffer(const ALLEGRO_SAMPLE* sample)
{
   ASSERT(sample);
   return sample->buffer;
}

/*use with caution*/
void al_sample_set_buffer(ALLEGRO_SAMPLE* sample, void* buf)
{
   ASSERT(sample);
   sample->buffer = buf;
}
