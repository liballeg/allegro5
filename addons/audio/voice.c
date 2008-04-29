/**
 * voice.c
 * Contains methods for the ALLEGRO_VOICE structure
 * In the style of python/c++:
 *    al_voice_create is the constructor
 *    al_voice_destroy is the destructor
 *    all other methods take the voice structure as the first argument
 *
 * Based on the code from KC/Milan
 * Rewritten by Ryan Dickie
 */

#include "allegro5/internal/aintern_audio.h"
#include "allegro5/audio.h"

extern ALLEGRO_AUDIO_DRIVER* driver;

/* al_voice_create: give it a sample*/
ALLEGRO_VOICE* al_voice_create(ALLEGRO_SAMPLE* sample)
{
   ALLEGRO_VOICE *voice = NULL;

   ASSERT(sample);

   if(!driver)
   {
      TRACE("Error. Cannot create voice when no audio driver is initiated");
      return NULL;
   }

   voice = malloc(sizeof(*voice));
   if(!voice)
   {
      TRACE("Error allocation voice structure memory");
      return NULL;
   }
   voice->sample = sample;
   voice->stream = NULL;
   voice->streaming = FALSE;

   if(driver->allocate_voice(voice) != 0)
   {
      free(voice);
      TRACE("Error creating hardware voice");
      return NULL;
   }

   return voice;
}

/* al_voice_create: give it a stream */
ALLEGRO_VOICE* al_voice_create_stream(ALLEGRO_STREAM* stream)
{
   ALLEGRO_VOICE *voice = NULL;

   ASSERT(stream);

   if(!driver)
   {
      TRACE("Error. Cannot create voice when no audio driver is initiated");
      return NULL;
   }

   voice = malloc(sizeof(*voice));
   if(!voice)
   {
      TRACE("Error allocation voice structure memory");
      return NULL;
   }

   voice->sample = NULL;
   voice->stream = stream;
   voice->streaming = TRUE;

   if(driver->allocate_voice(voice) != 0)
   {
      free(voice);
      TRACE("Error creating hardware voice");
      return NULL;
   }

   return voice;  
}

void al_voice_destroy(ALLEGRO_VOICE *voice)
{
   ASSERT(voice);
   driver->deallocate_voice(voice);
   free(voice);
}

unsigned long al_voice_get_position(const ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   return driver->get_voice_position(voice);
}

bool al_voice_is_playing(const ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   return driver->voice_is_playing(voice);
}

int al_voice_set_position(ALLEGRO_VOICE* voice, unsigned long position)
{
   ASSERT(voice);
   return driver->set_voice_position(voice, position);
}

int al_voice_set_loop_mode(ALLEGRO_VOICE* voice, ALLEGRO_AUDIO_ENUM loop_mode)
{
   ASSERT(voice);
   return driver->set_loop_mode(voice, loop_mode);
}

int al_voice_get_loop_mode(ALLEGRO_VOICE* voice, ALLEGRO_AUDIO_ENUM loop_mode)
{
   ASSERT(voice);
   return driver->get_loop_mode(voice);
}

int al_voice_pause(ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   return driver->stop_voice(voice);
}

/* pauses and sets position to 0 */
int al_voice_stop(ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   return (driver->stop_voice(voice) | driver->set_voice_position(voice,0));
}

int al_voice_start(ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   return driver->start_voice(voice);
}
