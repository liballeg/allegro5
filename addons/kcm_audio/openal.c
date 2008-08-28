/*
 * updated for 4.9 inclusion by Ryan Dickie
 * Originally done by KC/Milan
 */

#include <stdio.h>
#include <string.h>

#include "allegro5/allegro.h"

#include <al.h>
#include <alc.h>

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_kcm_audio.h"

/* OpenAL vars */
ALCdevice*  openal_dev;
ALCcontext* openal_context;
ALenum      openal_err;
char  openal_err_str[64];
ALCenum     alc_err;
char  alc_err_str[64];

/* TODO: make these configurable */
const size_t preferred_frag_size = 1024;
const ALuint preferred_buf_count = 4;

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
/* TODO: review */
typedef struct ALLEGRO_AL_DATA {
   ALuint *buffers;

   size_t num_buffers;
   ALuint buffer_size;

   ALuint source;
   ALuint format;

   _AL_THREAD thread;
   volatile bool stop_voice;
} ALLEGRO_AL_DATA;

/* Custom routine which runs in another thread to periodically check if OpenAL
   wants more data for a stream */
/* TODO: review */
static void _openal_update(_AL_THREAD* self, void* arg)
{
   unsigned long i, samples_per_update;
   const void *data;
   void *silence;

   ALLEGRO_VOICE* voice = (ALLEGRO_VOICE*) arg;
   ALLEGRO_AL_DATA *ex_data = (ALLEGRO_AL_DATA*)voice->extra;


   silence = calloc(1, ex_data->buffer_size);
   if(ex_data->format == AL_FORMAT_STEREO8 ||
      ex_data->format == AL_FORMAT_MONO8)
      memset(silence, 0x80, ex_data->buffer_size);

   for(i = 0;i < ex_data->num_buffers;++i)
      alBufferData(ex_data->buffers[i], ex_data->format, silence,
                   ex_data->buffer_size, voice->frequency);

   alSourceQueueBuffers(ex_data->source, ex_data->num_buffers,
                     ex_data->buffers);

   alSourcePlay(ex_data->source);

   samples_per_update = ex_data->buffer_size /
                        ((ex_data->format==AL_FORMAT_STEREO16) ? 4 :
                          ((ex_data->format==AL_FORMAT_STEREO8 ||
                            ex_data->format==AL_FORMAT_MONO16) ? 2 : 1));

   data = silence;

   while(!ex_data->stop_voice)
   {
      ALint status = 0;

      alGetSourcei(ex_data->source, AL_BUFFERS_PROCESSED, &status);
      if(status <= 0)
      {
         /* FIXME what is this for ? */
         al_rest(0.001);
         continue;
      }

      while(--status >= 0)
      {
         ALuint buffer;

         data = _al_voice_update(voice, samples_per_update);
         if(data == NULL)
            data = silence;

         alSourceUnqueueBuffers(ex_data->source, 1, &buffer);
         alBufferData(buffer, ex_data->format, data, ex_data->buffer_size,
                      voice->frequency);
         alSourceQueueBuffers(ex_data->source, 1, &buffer);
      }
      alGetSourcei(ex_data->source, AL_SOURCE_STATE, &status);
      if(status == AL_STOPPED)
         alSourcePlay(ex_data->source);
   }

   alSourceStop(ex_data->source);
}

/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached sample's looping mode should be honored, and loading
   must fail if it cannot be. */
static int _openal_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(voice->attached_stream->loop != ALLEGRO_PLAYMODE_ONCE &&
           voice->attached_stream->loop != ALLEGRO_PLAYMODE_ONEDIR)
      return 1;

   ex_data->buffer_size = voice->buffer_size;
   if(!ex_data->buffer_size)
   {
      fprintf(stderr, "Voice buffer and data buffer size mismatch\n");
      return 1;
   }
   ex_data->num_buffers = 1;

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
                data, ex_data->buffer_size, voice->frequency);

   /* sets the buffer */
   alSourcei(ex_data->source, AL_BUFFER, ex_data->buffers[0]);

   /* Loop / no loop? */
   alSourcei(ex_data->source, AL_LOOPING,
      (voice->attached_stream->loop != ALLEGRO_PLAYMODE_ONCE));

   /* make sure the volume is on */
   alSourcef(ex_data->source, AL_GAIN, 1.0f);

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
   if(!voice->is_streaming)
   {
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

   if(ex_data->stop_voice != 0)
   {
      ex_data->buffer_size = voice->buffer_size;
      if(!ex_data->buffer_size)
         ex_data->buffer_size = preferred_frag_size *
                          ((ex_data->format==AL_FORMAT_STEREO16) ? 4 :
                            ((ex_data->format==AL_FORMAT_STEREO8 ||
                              ex_data->format==AL_FORMAT_MONO16) ? 2 : 1));
      ex_data->num_buffers = voice->num_buffers;
      if(!ex_data->num_buffers)
         ex_data->num_buffers = preferred_buf_count;

      alGenSources(1, &ex_data->source);
      if(alGetError() != AL_NO_ERROR)
         return 1;

      ex_data->buffers = malloc(sizeof(ALuint) * ex_data->num_buffers);
      if(!ex_data->buffers)
      {
         alDeleteSources(1, &ex_data->source);
         return 1;
      }

      alGenBuffers(ex_data->num_buffers, ex_data->buffers);
      if(alGetError() != AL_NO_ERROR)
      {
         alDeleteSources(1, &ex_data->source);
         free(ex_data->buffers);
         ex_data->buffers = NULL;
         return 1;
      }

      alSourcef(ex_data->source, AL_GAIN, 1.0f);
      if(alGetError() != AL_NO_ERROR)
      {
         alDeleteSources(1, &ex_data->source);
         alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
         free(ex_data->buffers);
         ex_data->buffers = NULL;
         return 1;
      }

      ex_data->stop_voice = 0;
      _al_thread_create(&ex_data->thread, _openal_update, (void*) voice);

   }
      return 0;
}

/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int _openal_stop_voice(ALLEGRO_VOICE* voice)
{
   ALLEGRO_AL_DATA *ex_data = voice->extra;

   if(!ex_data->buffers)
   {
      fprintf(stderr, "Trying to stop empty voice buffer\n"); 
      return 1;
   }

   /* if playing a sample */
   if(!voice->is_streaming)
   {
      alSourceStop(ex_data->source);
      if((openal_err = alGetError()) != AL_NO_ERROR)
      {
         fprintf(stderr, "Could not stop voice\n");
         fprintf(stderr, openal_get_err_str(openal_err, sizeof(openal_err_str),openal_err_str));
         fprintf(stderr, "\n");
         return 1;
      } 
      return 0;
   }

   if(ex_data->stop_voice == 0)
   {
      ex_data->stop_voice = 1;
      _al_thread_join(&ex_data->thread);
   }

   alDeleteBuffers(ex_data->num_buffers, ex_data->buffers);
   free(ex_data->buffers);
   ex_data->buffers = NULL;
   alDeleteSources(1, &ex_data->source);
   alGetError();
   return 0;
}

/* The voice_is_playing method should only be called on non-streaming sources,
   and should return true if the voice is playing */
static bool _openal_voice_is_playing(const ALLEGRO_VOICE *voice)
{
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
   ALLEGRO_AL_DATA *ex_data;

   /* openal doesn't support very much! */
   switch (voice->depth)
   {
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         /* format supported */
         break;
      case ALLEGRO_AUDIO_DEPTH_INT8:
         fprintf(stderr, "OpenAL requires 8-bit data to be unsigned\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_UINT16:
         fprintf(stderr, "OpenAL requires 16-bit data to be signed\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_INT16:
         /* format supported */
         break;
      case ALLEGRO_AUDIO_DEPTH_UINT24:
         fprintf(stderr, "OpenAL does not support 24-bit data\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_INT24:
         fprintf(stderr, "OpenAL does not support 24-bit data\n");
         return 1;
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         fprintf(stderr, "OpenAL does not support 32-bit floating data\n");
         return 1;
      default:
         fprintf(stderr, "Cannot allocate unknown voice depth\n");
         return 1;
   }

   ex_data = calloc(1, sizeof(*ex_data));
   if(!ex_data)
   {
      fprintf(stderr, "Could not allocate voice data memory\n"); 
      return 1;
   }

   switch (voice->chan_conf)
   {
      case ALLEGRO_CHANNEL_CONF_1:
         /* format supported */
         if(voice->depth == ALLEGRO_AUDIO_DEPTH_UINT8)
            ex_data->format = AL_FORMAT_MONO8;
         else
            ex_data->format = AL_FORMAT_MONO16;
         break;
      case ALLEGRO_CHANNEL_CONF_2:
         /* format supported */
         if(voice->depth == ALLEGRO_AUDIO_DEPTH_UINT8)
            ex_data->format = AL_FORMAT_STEREO8;
         else
            ex_data->format = AL_FORMAT_STEREO16;
         break;
      case ALLEGRO_CHANNEL_CONF_3:
         fprintf(stderr, "OpenAL does not support voice with 3 channel configuration\n");
         free(ex_data);
         return 1;
      case ALLEGRO_CHANNEL_CONF_4:
         ex_data->format = alGetEnumValue("AL_FORMAT_QUAD16");
         if (ex_data->format)
         {
            fprintf(stderr, "OpenAL cannot allocate voice with 4.0 channel configuration\n");
            free(ex_data);
            return 1;
         }
         if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT16)
         {
            fprintf(stderr, "OpenAL requires 16-bit signed data for 4 channel configuration\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      case ALLEGRO_CHANNEL_CONF_5_1:
         ex_data->format = alGetEnumValue("AL_FORMAT_51CHN_16");
         if (!ex_data->format)
         {
            fprintf(stderr, "Cannot allocate voice with 5.1 channel configuration\n");
            free(ex_data);
            return 1;
         }
         if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
         {
            fprintf(stderr, "5.1 channel requires 16-bit signed data\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      case ALLEGRO_CHANNEL_CONF_6_1:
         ex_data->format = alGetEnumValue("AL_FORMAT_61CHN_16");
         if (!ex_data->format)
         {
            fprintf(stderr, "Cannot allocate voice with 6.1 channel configuration\n");
            free(ex_data);
            return 1;
         }
         if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
         {
            fprintf(stderr, "6.1 channel requires 16-bit signed data\n");
            free(ex_data);
            return 1;
         }
         /* else it is supported */
         break;
      case ALLEGRO_CHANNEL_CONF_7_1:
         ex_data->format = alGetEnumValue("AL_FORMAT_71CHN_16");
         if (!ex_data->format)
         {
            fprintf(stderr, "Cannot allocate voice with 7.1 channel configuration\n");
            free(ex_data);
            return 1;
         }
         if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
         {
            fprintf(stderr, "7.1 channel requires 16-bit signed data\n");
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

   /* stop_voice to true */
   ex_data->stop_voice = 1;
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

ALLEGRO_AUDIO_DRIVER _openal_driver = {
   "OpenAL",

   _openal_open,
   _openal_close,

   _openal_allocate_voice,
   _openal_deallocate_voice,

   _openal_load_voice,
   _openal_unload_voice,

   _openal_start_voice,
   _openal_stop_voice,

   _openal_voice_is_playing,

   _openal_get_voice_position,
   _openal_set_voice_position,
};
