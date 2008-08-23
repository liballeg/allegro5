/*
 * Originally done by KC/Milan
 * Rewritten by Ryan Dickie
 */

#include <stdio.h>
#include <string.h>

#include <AL/al.h>
#include <AL/alc.h>

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"

/* OpenAL vars */
ALCdevice*  openal_dev;
ALCcontext* openal_context;
ALenum      openal_err;
char  openal_err_str[64];
ALCenum     alc_err;
char  alc_err_str[64];

const size_t preferred_frag_size = 1024;
const ALuint preferred_buf_count = 1; /* FIXME: not good for streaming */

char* openal_get_err_str(ALenum err, int len_str, char* str)
{
   switch (err)
   {
      case AL_NO_ERROR:
         snprintf(str, len_str, "There is no OpenAL error"); 
         break;
      case AL_INVALID_NAME: 
         snprintf(str, len_str, "A bad name (ID) was passed to OpenAL");
         break;
      case AL_INVALID_ENUM:
         snprintf(str, len_str, "An invalid enum was passed to OpenAL");
         break;
      case AL_INVALID_VALUE:
         snprintf(str, len_str, "An Invalid enum was passed to OpenAL");
         break;
      case AL_INVALID_OPERATION:
         snprintf(str, len_str, "The requestion operation is invalid");
         break;
      case AL_OUT_OF_MEMORY:
         snprintf(str, len_str, "OpenAL ran out of memory");
         break;
      default:
         snprintf(str, len_str, "Unknown error");
         break;
   }

   return str;
}

char* alc_get_err_str(ALCenum err, int len_str, char* str)
{
   switch (err)
   {
      case ALC_NO_ERROR:
         snprintf(str, len_str, "There is no OpenAL error"); 
         break;
      case ALC_INVALID_DEVICE: 
         snprintf(str, len_str, "A bad device was passed to OpenAL");
         break;
      case ALC_INVALID_CONTEXT:
         snprintf(str, len_str, "An bad context was passed to OpenAL");
         break;
      case ALC_INVALID_ENUM:
         snprintf(str, len_str, "An Invalid enum was passed to OpenAL");
         break;
      case ALC_INVALID_VALUE:
         snprintf(str, len_str, "The requestion operation is invalid");
         break;
      case ALC_OUT_OF_MEMORY:
         snprintf(str, len_str, "OpenAL ran out of memory");
         break;
      default:
         snprintf(str, len_str, "Unknown error");
         break;
   }

   return str;
}

/* The open method starts up the driver and should lock the device, using the
   previously set paramters, or defaults. It shouldn't need to start sending
   audio data to the device yet, however. */
static int _openal_open()
{ 
   fprintf(stderr, "Starting OpenAL...\n");

   /* clear the error state */
   openal_err = alGetError();

   /* pick default device. always a good choice */
   #if defined(ALLEGRO_WINDOWS)
      openal_dev = alcOpenDevice("DirectSound3D");
      if (!openal_dev)
      {
         openal_dev = alcOpenDevice("DirectSound");
         if (!openal_dev)
         {
            openal_dev = alcOpenDevice(NULL);
         }
      }
   #else
      openal_dev = alcOpenDevice(NULL);
   #endif

   if(!openal_dev || (alc_err = alcGetError(openal_dev)) != ALC_NO_ERROR)
   {
      fprintf(stderr, "Could not open audio device\n");
      fprintf(stderr, alc_get_err_str(alc_err, sizeof(alc_err_str),alc_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   openal_context = alcCreateContext(openal_dev, NULL);
   if (!openal_context || (alc_err = alcGetError(openal_dev)) != ALC_NO_ERROR)
   {
      fprintf(stderr, "Could not create current device context\n");
      fprintf(stderr, alc_get_err_str(alc_err, sizeof(alc_err_str),alc_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   alcMakeContextCurrent(openal_context);
   if ((alc_err = alcGetError(openal_dev)) != ALC_NO_ERROR)
   {
      fprintf(stderr, "Could not make context current\n");
      fprintf(stderr, alc_get_err_str(alc_err, sizeof(alc_err_str),alc_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   alDistanceModel(AL_NONE);
   if ((openal_err = alGetError()) != AL_NO_ERROR)
   {
      fprintf(stderr, "Could not set distance model\n");
      fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   fprintf(stderr, "\n");
   fprintf(stderr, "      Vendor: %s\n", alGetString(AL_VENDOR));
   fprintf(stderr, "     Version: %s\n", alGetString(AL_VERSION));
   fprintf(stderr, "    Renderer: %s\n", alGetString(AL_RENDERER));
   fprintf(stderr, "  Extensions: %s\n", alGetString(AL_EXTENSIONS));

   fprintf(stderr, "\n...done\n\n");

   return 0;
}

/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void _openal_close()
{
   /* clear error states */
   alGetError();
   alcGetError(openal_dev);

   /* remove traces from openal */
   alcMakeContextCurrent(NULL);
   alcDestroyContext(openal_context);
   alcCloseDevice(openal_dev);

   /* reset the pointers to NULL */
   openal_context = NULL;
   openal_dev = NULL;
}

/* Custom struct to hold voice information OpenAL needs */
typedef struct ALLEGRO_AL_DATA {
   ALuint *buffers;

   size_t num_buffers;
   ALuint buffer_size;

   ALuint source;
   ALuint format;

} ALLEGRO_AL_DATA;

/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached sample's looping mode should be honored, and loading
   must fail if it cannot be. */
static int _openal_load_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(!ex_data->buffer_size)
   {
      fprintf(stderr, "Voice buffer and data buffer size mismatch\n");
      return 1;
   }

   alGenSources(1, &ex_data->source);
   if ((openal_err = alGetError()) != AL_NO_ERROR)
   {
      fprintf(stderr, "Could not Generate (voice) source\n");
      fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   ex_data->buffers = malloc(sizeof(ALuint) * ex_data->num_buffers);
   if(!ex_data->buffers)
   {
      alDeleteSources(1, &ex_data->source);
      fprintf(stderr, "Could not allocate voice buffer memory\n"); 
      return 1;
   }

   alGenBuffers(ex_data->num_buffers, ex_data->buffers);
   if ((openal_err = alGetError()) != AL_NO_ERROR)
   {
      alDeleteSources(1, &ex_data->source);
      free(ex_data->buffers);
      ex_data->buffers = NULL;
      fprintf(stderr, "Could not Generate (voice) buffer\n");
      fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   /* copies data into a buffer */
   alBufferData(ex_data->buffers[0], ex_data->format,
                voice->sample->buffer, ex_data->buffer_size, voice->sample->frequency);

   /* sets the buffer */
   alSourcei(ex_data->source, AL_BUFFER, ex_data->buffers[0]);



   /* make sure the volume is on */
   alSourcef(ex_data->source, AL_GAIN, 1.0f);

   /* No loop  by default */
   alSourcei(ex_data->source, AL_LOOPING, 0);

   if((openal_err = alGetError()) != AL_NO_ERROR)
   {
      alDeleteSources(1, &ex_data->source);
      alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
      free(ex_data->buffers);
      ex_data->buffers = NULL;
      fprintf(stderr, "Could not attach voice source\n");
      fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
      fprintf(stderr, "\n");
      return 1;
   }

   return 0;
}

/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void _openal_unload_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   alDeleteSources(1, &ex_data->source);
   alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
   free(ex_data->buffers);
   ex_data->buffers = NULL;
   alGetError();
}


/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int _openal_start_voice(ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   /* playing a sample instead of a stream */
   if(!voice->streaming)
   {
      _openal_load_voice(voice);
      alSourcePlay(ex_data->source);

      if((openal_err = alGetError()) != AL_NO_ERROR)
      {
         fprintf(stderr, "Could not start voice\n");
         fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
         fprintf(stderr, "\n");
         return 1;
      }

      fprintf(stderr, "Starting voice\n");
      return 0;
   }
   /* FIXME add streaming */
   return 1;
}

/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int _openal_stop_voice(ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(!ex_data->buffers)
   {
      fprintf(stderr, "Trying to stop empty voice buffer\n"); 
      return 1;
   }

   /* if playing a sample */
   if(!voice->streaming)
   {
      alSourceStop(ex_data->source);
      if((openal_err = alGetError()) != AL_NO_ERROR)
      {
         fprintf(stderr, "Could not stop voice\n");
         fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
         fprintf(stderr, "\n");
         return 1;
      }
      _openal_unload_voice(voice);
      return 0;
   }

   /* FIXME: streaming */
   return 1;
}

/* The voice_is_playing method should only be called on non-streaming sources,
   and should return true if the voice is playing */
static bool _openal_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   ASSERT(voice);
   ALLEGRO_AL_DATA *ex_data = voice->extra;
   ALint status;

   if(!ex_data)
      return false;

   alGetSourcei(ex_data->source, AL_SOURCE_STATE, &status);
   return (status == AL_PLAYING);
}

/* The allocate_voice method should grab a voice from the system, and allocate
   any data common to streaming and non-streaming sources. */
static int _openal_allocate_voice(ALLEGRO_VOICE *voice)
{
   ASSERT(voice);
   ALLEGRO_AL_DATA *ex_data;

   switch (voice->sample->depth)
   {
      case ALLEGRO_AUDIO_8_BIT_UINT:
         /* format supported */
         break;
      case ALLEGRO_AUDIO_16_BIT_INT:
         /* format supported */
         break;
      case ALLEGRO_AUDIO_24_BIT_INT:  
         fprintf(stderr, "OpenAL does not support 24-bit data\n");
         return 1;
      case ALLEGRO_AUDIO_32_BIT_FLOAT:
         fprintf(stderr, "OpenAL does not support 32-bit floating data\n");
         return 1;
      default:
         fprintf(stderr, "Cannot allocate unknown voice depth\n");
         return 1;
   }

   ex_data = malloc(sizeof(*ex_data));
   if(!ex_data)
   {
      fprintf(stderr, "Could not allocate voice data memory\n"); 
      return 1;
   }

   ex_data->num_buffers = preferred_buf_count;
   ex_data->buffer_size = al_audio_buffer_size(voice->sample);

   switch (voice->sample->chan_conf)
   {
      case ALLEGRO_AUDIO_1_CH:
         /* format supported */
         if(voice->sample->depth == ALLEGRO_AUDIO_8_BIT_UINT)
            ex_data->format = AL_FORMAT_MONO8;
         else
            ex_data->format = AL_FORMAT_MONO16;
         break;
      case ALLEGRO_AUDIO_2_CH:
         /* format supported */
         if(voice->sample->depth == ALLEGRO_AUDIO_8_BIT_UINT)
            ex_data->format = AL_FORMAT_STEREO8;
         else
            ex_data->format = AL_FORMAT_STEREO16;
         break;
      case ALLEGRO_AUDIO_3_CH:
         fprintf(stderr, "OpenAL does not support voice with 3 channel configuration\n");
         free(ex_data);
         return 1;
      case ALLEGRO_AUDIO_4_CH:
         ex_data->format = alGetEnumValue("AL_FORMAT_QUAD16");
         if (ex_data->format)
         {
            fprintf(stderr, "OpenAL cannot allocate voice with 4.0 channel configuration\n");
            free(ex_data);
            return 1;
         }
         if (voice->sample->depth == ALLEGRO_AUDIO_16_BIT_INT)
         {
            fprintf(stderr, "OpenAL requires 16-bit signed data for 4 channel configuration\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      case ALLEGRO_AUDIO_5_1_CH:
         ex_data->format = alGetEnumValue("AL_FORMAT_51CHN_16");
         if (!ex_data->format)
         {
            fprintf(stderr, "Cannot allocate voice with 5.1 channel configuration\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      case ALLEGRO_AUDIO_6_1_CH:
         ex_data->format = alGetEnumValue("AL_FORMAT_61CHN_16");
         if (!ex_data->format)
         {
            fprintf(stderr, "Cannot allocate voice with 6.1 channel configuration\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      case ALLEGRO_AUDIO_7_1_CH:
         ex_data->format = alGetEnumValue("AL_FORMAT_71CHN_16");
         if (!ex_data->format)
         {
            fprintf(stderr, "Cannot allocate voice with 7.1 channel configuration\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      default:
         fprintf(stderr, "Cannot allocate voice with unknown channel configuration\n");
         free(ex_data);
         return 1;
   }

   voice->extra = ex_data;

   return 0;
}

/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void _openal_deallocate_voice(ALLEGRO_VOICE *voice)
{
   free(voice->extra);
   voice->extra = NULL;
}

/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned long _openal_get_voice_position(const ALLEGRO_VOICE *voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;
   ALint pos;

   alGetSourcei(ex_data->source, AL_SAMPLE_OFFSET, &pos);
   if(alGetError() != AL_NO_ERROR)
      return 0;
   return pos;
}

/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int _openal_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   alSourcei(ex_data->source, AL_SAMPLE_OFFSET, val);
   if(alGetError() != AL_NO_ERROR)
      return 1;
   return 0;
}

static int _openal_set_loop(ALLEGRO_VOICE* voice, bool loop)
{
   ASSERT(voice);
   ALLEGRO_AL_DATA *ex_data = voice->extra;
   alSourcei(ex_data->source, AL_LOOPING, loop);
   return 0;
}

static bool _openal_get_loop(ALLEGRO_VOICE* voice)
{
   ASSERT(voice);
   ALLEGRO_AL_DATA *ex_data = voice->extra;
   ALint value = 0;
   alGetSourcei(ex_data->source, AL_LOOPING, &value);
   if (value != 0)
      return TRUE;
   else
      return FALSE;
}

ALLEGRO_AUDIO_DRIVER _openal_driver = {
   _openal_open,
   _openal_close,

   _openal_allocate_voice,
   _openal_deallocate_voice,

   _openal_start_voice,
   _openal_stop_voice,

   _openal_voice_is_playing,

   _openal_get_voice_position,
   _openal_set_voice_position,

   _openal_set_loop,
   _openal_get_loop
};
